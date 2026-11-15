#pragma once
// "Drawable" grid state: plain data, NO dependencies (neither raylib nor METIS-Core).
// The visualizer only reads this. GridEnvironment::fillRenderState() fills it.
#include <string>
#include <vector>

namespace smartgrid {

struct BusView {
    float x = 0, y = 0;            // screen position (px)
    std::string label;            // "A", "B", ...
    float demandaMW     = 0.0f;   // served/requested demand
    float deslastradoMW = 0.0f;
    bool  cuarentena    = false;
    bool  esCentral     = false;  // node with generation
    // deception state (only affects what Blue "sees")
    bool  contadorCongelado = false;
    bool  falsaAlarma       = false;
    std::string ultimaAccion;     // name of the last action of its head
};

struct LineView {
    int   from = 0, to = 0;       // indices into buses[]
    float flujoMW      = 0.0f;
    float capacidadMW  = 1.0f;
    bool  cerrada      = true;    // false = open/tripped
    float ratio() const { return capacidadMW > 0 ? (flujoMW < 0 ? -flujoMW : flujoMW) / capacidadMW : 0.0f; }
};

struct HackerView {
    float x = 0, y = 0;
    std::string label;            // "StrikerHacker" / "SpooferHacker"
    std::string ultimaAccion;
    bool  esStriker = true;
};

enum class Fase { Normal, Ataque, Caso1_Blackout, Caso2_Defendido };

struct GridRenderState {
    std::vector<BusView>    buses;
    std::vector<LineView>   lineas;
    std::vector<HackerView> hackers;

    int   currentEpisode = 0;
    int   step          = 0;
    int   maxSteps      = 288;
    float generacionMW  = 0.0f;
    float genMaxMW      = 150.0f;
    int   presupuestoRed = 6;
    int   currentPlayer  = -1;    // whose turn it is (0..3 Blue subst, 4 striker, 5 spoofer; -1 none)
    float demandaRealTotalMW = 0.0f;   // total real demand (natural + botnet - shed), for the daily graph
    float realDemandMW = 0.0f;         // real customer demand (post-physics truth), for the BALANCE panel
    float realServedMW = 0.0f;         // real customers actually served (post-physics truth)

    // Daily history for the demand graph (indexed by step; -1 = not reached yet). Owned by the environment.
    std::vector<float> demandaRealDia;   // demand actually wanted each step
    std::vector<float> generacionDia;    // electricity actually generated/served each step
    std::vector<float> eventoStrikerDia; // Striker attack intensity per step (0 = none)
    std::vector<float> eventoSpooferDia; // Spoofer deception intensity per step
    std::vector<float> eventoSyncDia;    // bitmask of buses with a synchronized attack per step (0 = none)
    int   winner        = -1;      // -1 draw/ongoing, 0 Blue, 1 Red
    Fase  fase          = Fase::Normal;
    std::string titulo  = "METIS-Core | AEGIS - Smart Grid Self-Healing";
};

// Builds the "Option A" scene for a given phase, with precomputed numbers.
GridRenderState buildRenderOpcionA(Fase fase);

} // namespace smartgrid
