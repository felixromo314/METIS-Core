#pragma once
// Grid topology: SINGLE source of truth (runtime data).
//
//   [CENTRAL] -- A -------- B
//               |           |
//               C -------- D
//
// Designed so that loading the IEEE 14-bus later means changing data, not code:
// just another function that fills a GridTopology.
#include <vector>
#include <array>
#include <cstdint>

namespace smartgrid {

// Structural names for Option A (readable labels only).
enum class Bus  : uint8_t { A = 0, B = 1, C = 2, D = 3 };
enum class Line : uint8_t { AB = 0, AC = 1, BD = 2, CD = 3 };
constexpr int kNumBuses = 4;   // Option A size (for fixed-size state arrays)
constexpr int kNumLines = 4;

struct LineaModelo {
    int   from;
    int   to;
    float susceptancia;   // b (p.u.)
    float capacidadMW;    // thermal limit: if exceeded, protection trips
};

// Full description of a grid. Everything here is DATA (not compile-time constants),
// so another grid (IEEE 14-bus) can be loaded without touching the code.
struct GridTopology {
    int                            nBuses   = 0;
    std::vector<LineaModelo>       lineas;
    int                            busSlack = 0;      // power plant (theta = 0, balances demand)
    float                          genMaxMW = 0.0f;   // power plant max capacity
    std::vector<float>             cargaBaseMW;       // consumer base load per bus
    std::vector<std::array<int,2>> lineaPropia;       // up to 2 lines per bus (-1 = none)

    int  nLineas() const { return int(lineas.size()); }
    bool tieneCarga(int bus) const { return cargaBaseMW[bus] > 0.0f; }
    int  lineaPropiaDe(int bus, int cual) const { return lineaPropia[bus][cual]; }
};

// "Option A" validation grid (4-bus ring, power plant at A)
GridTopology crearRedOpcionA();

} // namespace smartgrid
