#pragma once
// Action spaces for METIS-Core asymmetric AlphaGoZero multi-head.
//
// This METIS-Core build sizes the output PER HEAD: MultiHeadBrainBuilder::addHead(name, agent,
// maxActionsHead) takes each head's action count, and setNumberInputsOutputs(num_inputs) only
// takes the inputs. So every head has its OWN action space with head-local indices 0..COUNT-1,
// and the index returned by getPossibleActions maps 1:1 to that head's policy logits.
//
// Network data (capacities, loads, line ownership) lives in GridTopology; the functions that
// depend on it take a const GridTopology&.
#include "GridTopology.h"
#include <array>
#include <cstdint>

namespace smartgrid {

    // ---- Players (sequential IDs 0..N-1, as METIS-Core requires) ----
    enum PlayerID : int {
        PID_A = 0, PID_B = 1, PID_C = 2, PID_D = 3,   // Blue (substations)
        PID_STRIKER = 4, PID_SPOOFER = 5,             // Red (hackers)
        MAX_PLAYERS = 6
    };
    enum TeamID : int { TEAM_BLUE_ID = 0, TEAM_RED_ID = 1, TEAM_DRAW_ID = -1 };

    constexpr int teamDe(int pid) { return pid <= PID_D ? TEAM_BLUE_ID : TEAM_RED_ID; }
    constexpr Bus busDeSubestacion(int pid) { return Bus(pid); }   // PID 0..3 == Bus A..D

    // ============================================================================
    // BLUE: one action space PER head (head-local indices 0..COUNT-1).
    // Each substation's NN head is sized exactly to its own COUNT. The "static" legality
    // (which actions a substation has at all) is encoded by the enum itself; only the
    // state-dependent legality is checked at runtime.
    // ============================================================================
    enum class ACT_A : uint8_t { NoOp = 0, ToggleL1, ToggleL2, Verify, COUNT };                          // 4
    enum class ACT_B : uint8_t { NoOp = 0, Shed10, Shed25, Restore, Verify, Quarantine, COUNT };          // 6 (B and C)
    enum class ACT_D : uint8_t { NoOp = 0, Shed10, Shed25, Restore, ToggleL1, ToggleL2, Verify, Quarantine, COUNT }; // 8

#define ACTIONS_SUB_A   int(ACT_A::COUNT)   // 4
#define ACTIONS_SUB_B   int(ACT_B::COUNT)   // 6
#define ACTIONS_SUB_C   int(ACT_B::COUNT)   // 6  (C shares B's repertoire today)
#define ACTIONS_SUB_D   int(ACT_D::COUNT)   // 8

    // Head width (number of logits) for a given substation.
    constexpr int blueNumActions(Bus b)
    {
        switch (b) {
        case Bus::A: return int(ACT_A::COUNT);
        case Bus::D: return int(ACT_D::COUNT);
        default:     return int(ACT_B::COUNT);   // B and C
        }
    }

    // ============================================================================
    // RED: two heads with their own sizes (Striker = 12, Spoofer = 11).
    // ============================================================================
    enum class STRIKER_ACTIONS : uint8_t {
        NoOp = 0,
        Botnet10_B = 1, Botnet10_C = 2, Botnet10_D = 3,
        Botnet25_B = 4, Botnet25_C = 5, Botnet25_D = 6,
        Release = 7,
        Trip_AB = 8, Trip_AC = 9, Trip_BD = 10, Trip_CD = 11,
        MAX_ACTIONS = 12
    };
    enum class SPOOFER_ACTIONS : uint8_t {
        NoOp = 0,
        FreezeMeter_B = 1, FreezeMeter_C = 2, FreezeMeter_D = 3,
        FreezeFlow_AB = 4, FreezeFlow_AC = 5, FreezeFlow_BD = 6, FreezeFlow_CD = 7,
        FalseAlarm_B = 8, FalseAlarm_C = 9, FalseAlarm_D = 10,
        MAX_ACTIONS = 11
    };

#define ACTIONS_STRIKER  int(STRIKER_ACTIONS::MAX_ACTIONS)   // 12
#define ACTIONS_SPOOFER  int(SPOOFER_ACTIONS::MAX_ACTIONS)   // 11

    // Widest head (used only as a buffer size for the legal-index arrays).
    constexpr int kRedMaxActions = int(STRIKER_ACTIONS::MAX_ACTIONS);   // 12

    constexpr int kRedPresupuestoEpisodio = 6;
    constexpr int kStrikerPresupuestoEpisodio = 4;   // Striker's OWN budget
    constexpr int kSpooferPresupuestoEpisodio = 4;   // Spoofer's OWN budget
    constexpr int kRedRecargaPorTramo         = 1;   // points EACH hacker gains every kRedRecargaCadaSteps (accumulative)
    constexpr int kRedRecargaCadaSteps        = 12;  // drip recharge: 1 point every 12 steps (= 4 every 48, same average rate).
                                                     // Previously 4 points at once every 48 steps: Red always attacked at the
                                                     // steps 48, 96, 144... and Blue learned to anticipate it
    constexpr int kRedPresupuestoTope         = 8;   // max points a hacker can accumulate

    constexpr int strikerCoste(STRIKER_ACTIONS a) {
        switch (a) {
        case STRIKER_ACTIONS::Botnet10_B: case STRIKER_ACTIONS::Botnet10_C: case STRIKER_ACTIONS::Botnet10_D: return 1;
        case STRIKER_ACTIONS::Botnet25_B: case STRIKER_ACTIONS::Botnet25_C: case STRIKER_ACTIONS::Botnet25_D: return 2;
        case STRIKER_ACTIONS::Trip_AB: case STRIKER_ACTIONS::Trip_AC:
        case STRIKER_ACTIONS::Trip_BD: case STRIKER_ACTIONS::Trip_CD:                                         return 4;
        default:                                                                                              return 0;
        }
    }
    constexpr int spooferCoste(SPOOFER_ACTIONS a) { return a == SPOOFER_ACTIONS::NoOp ? 0 : 1; }

    // ============================================================================
    // Action -> effect decoding
    // ============================================================================
    enum class BlueOp : uint8_t { NoOp, Shed, Restore, ToggleLinea, Verify, Quarantine };
    struct BlueEfecto { BlueOp op = BlueOp::NoOp; int bus = -1; float fraccion = 0.0f; int linea = -1; };
    // 'sub' selects the head; 'accionIdx' is that head's local index (0..blueNumActions(sub)-1).
    BlueEfecto blueDecodificar(const GridTopology& topo, Bus sub, int accionIdx);

    struct StrikerEfecto { int bus = -1; float deltaMW = 0.0f; int lineaDisparo = -1; bool release = false; };   // deltaMW = FRACTION of the bus base load (0.25 / 0.50)
    constexpr int kMaxBusesBotnet = 2;   // at most 2 buses with an active botnet at the same time
    StrikerEfecto strikerDecodificar(int accionIdx);

    enum class EnganoTipo : uint8_t { Ninguno, CongelarContador, CongelarFlujo, FalsaAlarma };
    struct SpooferEfecto { EnganoTipo tipo = EnganoTipo::Ninguno; int bus = -1; int linea = -1; };
    SpooferEfecto spooferDecodificar(int accionIdx);

    // ============================================================================
    // State context for the dynamic (state-dependent) masks
    // ============================================================================
    struct ActionContext {
        std::array<float, kNumBuses> deslastradoMW{};   // shed load per bus
        std::array<bool, kNumBuses> enCuarentena{};    // which buses are isolated
        std::array<float, kNumBuses> botnetMW{};        // active fake load per bus
        int presupuestoRed = kRedPresupuestoEpisodio;   // attack points left for the Red team
    };

    // Legal head-local indices (for getPossibleActions). Each returns the count written to out[].
    int blueAccionesLegales(const GridTopology& topo, Bus sub, const ActionContext& ctx, int* outIdx, int maxOut);
    int strikerAccionesLegales(const ActionContext& ctx, int* outIdx, int maxOut);
    int spooferAccionesLegales(const ActionContext& ctx, int presupuestoTrasStriker, int* outIdx, int maxOut);

    // getNumActions(player): width of that player's head.
    constexpr int numActionsDe(int pid)
    {
        if (teamDe(pid) == TEAM_BLUE_ID) return blueNumActions(busDeSubestacion(pid));
        return (pid == PID_STRIKER) ? int(STRIKER_ACTIONS::MAX_ACTIONS) : int(SPOOFER_ACTIONS::MAX_ACTIONS);
    }

    const char* nombreBlue(Bus sub, int accionIdx);   // name depends on the head
    const char* nombreStriker(int accionIdx);
    const char* nombreSpoofer(int accionIdx);
    const char* nombreBus(int pid);

} // namespace smartgrid