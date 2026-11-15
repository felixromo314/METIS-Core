#include "GridTopology.h"

namespace smartgrid {

// SINGLE definition of the Option A numbers.
GridTopology crearRedOpcionA() {
    GridTopology g;
    g.nBuses   = kNumBuses;
    g.busSlack = int(Bus::A);     // the power plant is at A
    g.genMaxMW = 150.0f;

    g.lineas = {
        { int(Bus::A), int(Bus::B), 1.0f, 60.0f },  // AB
        { int(Bus::A), int(Bus::C), 1.0f, 60.0f },  // AC
        { int(Bus::B), int(Bus::D), 1.0f, 40.0f },  // BD
        { int(Bus::C), int(Bus::D), 1.0f, 40.0f },  // CD
    };
    g.cargaBaseMW = { 0.0f, 30.0f, 30.0f, 40.0f };  // A: power plant gateway, no consumption
    g.lineaPropia = {
        { int(Line::AB), int(Line::AC) },   // A
        { -1, -1 },                         // B
        { -1, -1 },                         // C
        { int(Line::BD), int(Line::CD) },   // D
    };
    return g;
}

} // namespace smartgrid
