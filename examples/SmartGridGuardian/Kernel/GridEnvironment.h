#pragma once
// Game environment (derives from Metis::IAlphaGoZeroState), analogous to SpaceEnviroment.
// Orchestrates 6 players: 4 substations (Blue) + 2 hackers (Red).

 #include "METIS-Core.h"

#include "GridAgent.h"
//#include "GridModel.h"
#include "Actions.h"
#include <vector>

#define MAX_SUBSTATIONS 4



namespace smartgrid {

// Observation sizes (must match getState(player).size() EXACTLY).
// Grid block (28): per bus [demand,shed,quarantine]=3*4=12
//                      per line [flow,ratio,closed]=3*4=12
//                      global [gen,demandaTot,sinFase,cosFase]=4
#define GRID_BLOCK_INPUTS 28
#define TEAM_BLUE_INPUTS  (GRID_BLOCK_INPUTS + 4)   // + substation one-hot = 32 (MEASURED grid, spoofable)
#define TEAM_RED_INPUTS   (GRID_BLOCK_INPUTS + 3 + kNumBuses)   // + [budget, hacker one-hot] + botnet freshness per bus = 35 (REAL grid)
#define MAX_STEPS         288                       // 1 episode = 1 day at 5 min/step

// --- LEGAL actions per agent (real subset after masking) ---
#define ACTIONS_SUB_A       4   // NoOp, ToggleLinea1, ToggleLinea2, VerifySensor
#define ACTIONS_SUB_B       6   // NoOp, Shed10, Shed25, Restore, VerifySensor, Quarantine
#define ACTIONS_SUB_C       6   // same as B
#define ACTIONS_SUB_D       8   // B's actions + ToggleLinea1/2 (ring closing)

#define ACTIONS_STRIKER     12  // NoOp, Botnet10/25 x3, Release, Trip x4
#define ACTIONS_SPOOFER     11  // NoOp, FreezeMeter x3, FreezeFlow x4, FalseAlarm x3

// "Joint" state (the real one lives in the members; this satisfies the template)
struct TGridState { int dummy = 0; };

// Blackout threshold: below this fraction of customers served, Red wins.
// 0.65 > 0.60 so that "sacrificing D" (40% of the load) is NO longer a guaranteed Blue win.
// 0.775 -> 0.82 -> 0.78: at 0.775 Blue won 95% (341/357 ep.); at 0.82 Red always won at the peak
constexpr int kSyncVentana = 3;   // max steps between Striker hit and Spoofer deception to count as SYNCHRONIZED
constexpr float kUmbralBlackout = 0.78f;    // < 0.79 (worst case for a Blue that SEES): a defense always exists; Red only wins with deception + peak (18:50-21h)

// Service margin in [-1, +1] relative to the threshold (piecewise so that 1.0 -> +1 and 0.0 -> -1).
//   f = 1.0 -> +1 ; f = umbral -> 0 ; f = 0.0 -> -1
inline float margenServicio(float f)
{
    float v = (f >= kUmbralBlackout) ? (f - kUmbralBlackout) / (1.0f - kUmbralBlackout)
                                     : (f - kUmbralBlackout) / kUmbralBlackout;
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    return v;
}

// ---------------------------------------------------------------------------
// Observation building blocks. Each struct mirrors what the network sees;
// toVector() just serializes it in a fixed order. getState() decides normalization.
// ---------------------------------------------------------------------------
struct BusObs                                   // 3 inputs
{
    float demanda;      // normalized demand (MW / genMax)
    float deslastrado;  // normalized shed   (MW / genMax)
    float cuarentena;   // 1.0 if quarantined, else 0.0
    BusObs() { memset(this, 0, sizeof(BusObs)); }
    void toVector(std::vector<float>& in)
    {
        in.push_back(demanda);
        in.push_back(deslastrado);
        in.push_back(cuarentena);
    }
};

struct LineObs                                  // 3 inputs
{
    float flujo;    // signed flow normalized (MW / genMax)
    float ratio;    // |flow| / capacity (0..~1.2)
    float cerrada;  // 1.0 closed, 0.0 open/tripped
    LineObs() { memset(this, 0, sizeof(LineObs)); }
    void toVector(std::vector<float>& in)
    {
        in.push_back(flujo);
        in.push_back(ratio);
        in.push_back(cerrada);
    }
};

struct GridBlockObs                             // 4*3 + 4*3 + 4 = 28 == GRID_BLOCK_INPUTS
{
    BusObs  buses[kNumBuses];    // 12
    LineObs lineas[kNumLines];   // 12
    float   generacion;          // gen / genMax
    float   demandaTotal;        // total served / genMax
    float   faseSin;             // sin(2*pi * step/day)  -> time of day
    float   faseCos;             // cos(2*pi * step/day)
    GridBlockObs() : generacion(0), demandaTotal(0), faseSin(0), faseCos(0) {}
    void toVector(std::vector<float>& in)
    {
        for (int i = 0; i < kNumBuses; ++i) buses[i].toVector(in);
        for (int l = 0; l < kNumLines; ++l) lineas[l].toVector(in);
        in.push_back(generacion);
        in.push_back(demandaTotal);
        in.push_back(faseSin);
        in.push_back(faseCos);
    }
};

struct BlueObs                                  // 28 + 4 = 32 == TEAM_BLUE_INPUTS
{
    GridBlockObs grid;                 // MEASURED values (may be spoofed)
    float        oneHotSub[kNumBuses]; // which substation am I (A..D)
    BlueObs() { for (int i = 0; i < kNumBuses; ++i) oneHotSub[i] = 0.0f; }
    void toVector(std::vector<float>& in)
    {
        grid.toVector(in);
        for (int i = 0; i < kNumBuses; ++i) in.push_back(oneHotSub[i]);
    }
};

struct RedObs                                   // 28 + 3 + 4 = 35 == TEAM_RED_INPUTS
{
    GridBlockObs grid;                      // REAL values (attackers see the truth)
    float        presupuesto;               // budget / maxBudget
    float        oneHotHacker[2];           // striker / spoofer
    float        botnetReciente[kNumBuses]; // 1.0 = botnet just injected on that bus, decays over time, 0 = no botnet
    RedObs() : presupuesto(0)
    {
        oneHotHacker[0] = oneHotHacker[1] = 0.0f;
        for (int i = 0; i < kNumBuses; ++i) botnetReciente[i] = 0.0f;
    }
    void toVector(std::vector<float>& in)
    {
        grid.toVector(in);
        in.push_back(presupuesto);
        in.push_back(oneHotHacker[0]);
        in.push_back(oneHotHacker[1]);
        for (int i = 0; i < kNumBuses; ++i) in.push_back(botnetReciente[i]);
    }
};

struct TGridAction 
{ 
        int id = 0; 
        int toIndex(int currentPlayer) const 
        { 
            return id; 
        } 
};

class GridEnvironment : public Metis::IAlphaGoZeroState<TGridState, TGridAction> {
public:
    GridEnvironment();
    GridEnvironment(GridEnvironment& o);                 // deep copy (MCTS)

    void fillRenderState(GridRenderState& s, bool mostrarVerdad) const;

    // --- lifecycle ---
    void reset(int tipo);
    void update(float dt);
    double getDeltaTime() { return 1.0; }                // 1 step = 1 decision tick
    Metis::IAlphaGoZeroStateBase* clone() override { return new GridEnvironment(*this); }
    bool isEqualState(Metis::IAlphaGoZeroStateBase* p);

    std::vector<float> getState() { return { 1.0f }; }   // player-less version (unused)
    std::vector<float> getState(int currentPlayer);      // per-player observation
    bool isStateRelativeToPlayer() { return true; }

    // --- actions and turns ---
    void applyAction(int accionIdx);
    void getPossibleActions(int playerID, std::vector<TGridAction>& out);
    int  getNumActions(int playerID) { return numActionsDe(playerID); }
    bool is_valid_action(int /*a*/);

    int getNumActions() override { return 0; };

    // --- reward and terminal ---
    bool   isTerminal();
    int    getWinner() { return _winner; }
    double getReward(int teamID)
    {
        // GRADED reward: Blue scores by how many real customers it served on average during the day.
        // Blackout -> hard loss (-1). Otherwise, margin of mean service vs the threshold.
        double avg = (_rondasContadas > 0) ? (_sumaFraccionServida / (double)_rondasContadas) : 1.0;
        double vBlue;
        if (_winner == TEAM_RED_ID) vBlue = -1.0;              // blackout happened
        else {
            vBlue = (double)margenServicio((float)avg);
        }
        if (teamID == TEAM_BLUE_ID) return vBlue;
        if (teamID == TEAM_RED_ID)  return -vBlue;   // zero-sum
        return 0.0;
    }
    float  getMaterialHeuristicValue(int playerToEvaluate);

    // --- players / teams ---
    int getCurrentPlayerID() override { return _currentPlayer; }
    int getTeamID(int playerID) override { return teamDe(playerID); }

    int getRandomActionFromPlayer(int currentPlayer) override { return -1; };
    int getNumInputs() override { return 0; };

    void fillGridBlock(GridBlockObs& g, bool measured) const;

    void forecastDemandaDia(std::vector<float>& out) const;   // predicted daily demand (fixed)

public:
    int legalActions(int playerID, int* out, int maxOut) const;
    bool isPlayerActive(int pid);
    void advanceToNextActivePlayer(bool incluirActual);
    void construirContextoAcciones(ActionContext& ctx) const;
    void resolverFisicaFinDeRonda();                     // flow + cascade + partial reward

    GridTopology  _modelo;
    GridAgent  _subestacion[MAX_SUBSTATIONS];                  // PID 0..3
    GridAgent  _striker;                                 // PID 4
    GridAgent  _spoofer;                                 // PID 5

    int   _currentPlayer = 0;
    int   _step          = 0;
    int   _winner        = TEAM_DRAW_ID;
    int _ultimaAccion[MAX_PLAYERS] = {};   // last action index per player (for the display)

    // Real physical state
    std::vector<float> _demandaConsumidoresMW;                   // consumers (daily profile)
    std::array<float, MAX_SUBSTATIONS> _deslastradoMW{};
    std::array<float, MAX_SUBSTATIONS> _botnetMW{};
    std::array<bool, MAX_SUBSTATIONS> _enCuarentena{};
    std::vector<bool>  _lineaCerrada;
    std::vector<float> _flujoMW;   // real line flows (MW), filled by the physics step
    int   _presupuestoStriker = kStrikerPresupuestoEpisodio;   // Striker's own budget
    int   _presupuestoSpoofer = kSpooferPresupuestoEpisodio;   // Spoofer's own budget
    float _fraccionServida = 1.0f;   // fraction of real demand actually served (for blackout detection)
    

    // Deception state (what distorts Blue's observation)
    std::array<bool, MAX_SUBSTATIONS> _contadorCongelado{};
    std::vector<bool>           _flujoCongelado;
    std::vector<float>          _lecturaFlujoCongeladaMW;   // frozen flow shown to Blue (spoof)
    std::array<bool, MAX_SUBSTATIONS> _falsaAlarma{};
    std::array<float, MAX_SUBSTATIONS> _lecturaCongeladaMW{};
    // step of the last hit/deception per target, to measure Red sync (-1 = never)
    std::array<int, MAX_SUBSTATIONS> _stepBotnet{};        // last botnet injection on the bus
    std::array<int, MAX_SUBSTATIONS> _stepFreezeMeter{};   // last FreezeMeter on the bus
    std::array<int, kNumLines>       _stepTrip{};          // last Trip of the line
    std::array<int, kNumLines>       _stepFreezeFlow{};    // last FreezeFlow of the line

    std::vector<float> _demandaRealDia;   // daily history: real demand wanted per step
    std::vector<float> _generacionDia;    // daily history: electricity generated/served per step
    std::vector<float> _eventoStrikerDia; // Striker attack intensity per step (0 = no attack)
    std::vector<float> _eventoSpooferDia; // Spoofer deception intensity per step
    float _pendStriker = 0.0f;            // Striker attack accumulator for the current round
    float _pendSpoofer = 0.0f;            // Spoofer deception accumulator for the current round
    std::vector<float> _eventoSyncDia;    // per step: bitmask of buses with a SYNCHRONIZED attack (bit i = bus i), 0 = none
    int   _pendSync = 0;                  // bitmask of synchronized buses in the current round
    float _realDemandMW = 0.0f;           // real customer demand (post-physics) for the BALANCE panel
    float _realServedMW = 0.0f;           // real customers served (post-physics)
    double _sumaFraccionServida = 0.0;    // sum of served fraction over rounds (for the graded reward)
    int    _rondasContadas = 0;           // number of rounds resolved in the episode
};

} // namespace smartgrid
