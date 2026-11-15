#pragma once
// raylib window that draws a GridRenderState. Does not depend on METIS-Core.
#include "RenderModel.h"
#include "raylib.h"           // needed for Texture2D
#include <vector>
#include <string>

namespace smartgrid {

class Visualizer {
public:
    void init(int ancho = 1000, int alto = 680, const char* titulo = "AEGIS - Smart Grid");
    bool deberiaCerrar() const;          // WindowShouldClose()
    void dibujar(const GridRenderState& s);
    void cerrar();

    // --- start menu: returns 0 = train, 1 = play, -1 = quit ---
    int  menuInicio();

    // --- logo / watermark (load AFTER init, needs a GL context) ---
    void cargarLogo(const char* ruta);

    // --- action overlay for the human player ---
    void setMenuAcciones(const std::vector<std::string>& ops, int sel, const char* titulo);
    void limpiarMenu();

    // Push one episode outcome (percentages 0..100). Feeds the bottom win-rate graph.
    void pushWinRates(float bluePct, float redPct);

    // Predicted demand curve for the whole day (MW per step). Set once (it is fixed).
    void setDailyForecast(const std::vector<float>& mwPerStep);

private:
    std::vector<float> _histBlue;   // win% history for the bottom graph
    std::vector<float> _histRed;
    std::vector<float> _forecast;   // predicted daily demand (fixed, MAX_STEPS)

    // logo
    Texture2D _logo{};
    bool      _logoOk = false;

    // human action overlay
    std::vector<std::string> _menuAcc;
    int         _menuSel = 0;
    std::string _menuTitulo;
    bool        _menuOn = false;
};

} // namespace smartgrid
