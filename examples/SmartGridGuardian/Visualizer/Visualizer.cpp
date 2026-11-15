#include "Visualizer.h"
#include "raylib.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace smartgrid {

    // Paleta
    static const Color FONDO = { 13,  17,  28, 255 };
    static const Color AZUL = { 60, 150, 255, 255 };
    static const Color AZUL_TENUE = { 60, 150, 255,  60 };
    static const Color ROJO = { 235,  70,  70, 255 };
    static const Color MORADO = { 180,  90, 240, 255 };
    static const Color VERDE = { 70, 210, 140, 255 };
    static const Color AMBAR = { 240, 190,  70, 255 };
    static const Color GRIS = { 150, 160, 180, 255 };
    static const Color BLANCO = { 235, 240, 250, 255 };

    static Color colorPorCarga(float ratio) {
        if (ratio >= 1.0f) return ROJO;
        if (ratio >= 0.85f) return AMBAR;
        return VERDE;
    }

    void Visualizer::init(int ancho, int alto, const char* titulo) {
        SetConfigFlags(FLAG_MSAA_4X_HINT);
        InitWindow(ancho, alto, titulo);
        SetTargetFPS(60);
    }
    bool Visualizer::deberiaCerrar() const { return WindowShouldClose(); }
    void Visualizer::cerrar() {
        if (_logoOk) UnloadTexture(_logo);   // release the logo before closing the window
        CloseWindow();
    }

    void Visualizer::cargarLogo(const char* ruta) {
        // NOTE: must be called AFTER init() (needs the OpenGL context already created)
        Image img = LoadImage(ruta);
        if (img.data == nullptr) { _logoOk = false; return; }   // file could not be loaded

        // The PNG uses green as the chroma key. It is removed with TOLERANCE (not exact),
        // to also cover the anti-aliased halo at the edges.
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        Color* px = (Color*)img.data;
        const int n = img.width * img.height;
        for (int i = 0; i < n; ++i) {
            // "background green" pixel: dominant green and little red/blue -> transparent
            if (px[i].g > 110 && px[i].r < 120 && px[i].b < 120)
                px[i].a = 0;
        }

        _logo = LoadTextureFromImage(img);
        UnloadImage(img);
        _logoOk = (_logo.id != 0);
    }

    void Visualizer::setMenuAcciones(const std::vector<std::string>& ops, int sel, const char* titulo) {
        _menuAcc = ops;
        _menuSel = sel;
        _menuTitulo = titulo ? titulo : "";
        _menuOn = true;
    }
    void Visualizer::limpiarMenu() { _menuOn = false; _menuAcc.clear(); }

    int Visualizer::menuInicio() {
        while (!WindowShouldClose()) {
            const int cx = GetScreenWidth() / 2;
            Rectangle bTrain = { (float)cx - 160, 330, 320, 64 };
            Rectangle bPlay  = { (float)cx - 160, 420, 320, 64 };
            Vector2 m = GetMousePosition();
            const bool overTrain = CheckCollisionPointRec(m, bTrain);
            const bool overPlay  = CheckCollisionPointRec(m, bPlay);

            BeginDrawing();
            ClearBackground(FONDO);

            // logo centered at the top (green)
            if (_logoOk) {
                const float destW = 220.0f;
                const float sc = destW / (float)_logo.width;
                DrawTextureEx(_logo, { (float)cx - destW / 2, 70.0f }, 0.0f, sc, Color{ 255,255,255,255 });
            }
            const char* t1 = "SmartGridGuardian";
            const char* t2 = "MARL Self-Healing Smart Grid";
            DrawText(t1, cx - MeasureText(t1, 40) / 2, 215, 40, BLANCO);
            DrawText(t2, cx - MeasureText(t2, 18) / 2, 262, 18, GRIS);

            // TRAIN button
            DrawRectangleRounded(bTrain, 0.25f, 8, overTrain ? Color{ 40,80,140,255 } : Color{ 28,40,66,255 });
            DrawRectangleRoundedLines(bTrain, 0.25f, 8, AZUL);
            DrawText("1  -  TRAIN", (int)bTrain.x + 70, (int)bTrain.y + 20, 24, BLANCO);

            // PLAY button
            DrawRectangleRounded(bPlay, 0.25f, 8, overPlay ? Color{ 120,40,60,255 } : Color{ 50,26,34,255 });
            DrawRectangleRoundedLines(bPlay, 0.25f, 8, ROJO);
            DrawText("2  -  PLAY (you: Hacker)", (int)bPlay.x + 36, (int)bPlay.y + 20, 24, BLANCO);

            DrawText("[ESC] quit", GetScreenWidth() - 100, 6, 16, GRIS);
            EndDrawing();

            if ((overTrain && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) return 0;
            if ((overPlay  && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) return 1;
        }
        return -1;   // window closed
    }


    // Draws energy dots moving along a line, in the direction of the flow.
    // Speed and density grow with MW: this way you "see" the energy flowing.
    static void dibujarFlujoAnimado(Vector2 a, Vector2 b, float flujoMW, Color c, double t) {
        const float mag = std::fabs(flujoMW);
        if (mag < 0.5f) return;
        // direction: + goes a->b; - reverses
        Vector2 ini = flujoMW >= 0 ? a : b;
        Vector2 fin = flujoMW >= 0 ? b : a;
        Vector2 d = { fin.x - ini.x, fin.y - ini.y };
        const float len = std::sqrt(d.x * d.x + d.y * d.y);
        if (len < 1.0f) return;
        d.x /= len; d.y /= len;

        const int   nPuntos = 3 + int(mag / 15.0f);          // more MW -> more dots
        const float velocidad = 40.0f + mag * 1.6f;           // px/s, more MW -> faster
        const float sep = len / nPuntos;
        const float desfase = std::fmod(float(t) * velocidad, sep);
        for (int i = 0; i < nPuntos; ++i) {
            const float s = desfase + i * sep;
            if (s >= len) continue;
            Vector2 p = { ini.x + d.x * s, ini.y + d.y * s };
            DrawCircleV(p, 3.5f, c);
        }
    }

    void Visualizer::dibujar(const GridRenderState& s) {
        const double t = GetTime();
        BeginDrawing();
        ClearBackground(FONDO);


        DrawText(s.titulo.c_str(), 20, 16, 22, BLANCO);

        // --- power plant (hanging off A) ---
        if (!s.buses.empty()) {
            const BusView& a = s.buses[0];
            Vector2 cen = { a.x - 190, a.y };
            DrawLineEx(cen, { a.x, a.y }, 4, GRIS);
            // energy generated by the plant flows into A
            dibujarFlujoAnimado(cen, { a.x, a.y }, s.generacionMW, AMBAR, t);
            DrawRectangleRounded({ cen.x - 55, cen.y - 30, 90, 60 }, 0.2f, 6, Color{ 40,48,66,255 });
            DrawText("PLANT", (int)cen.x - 46, (int)cen.y - 22, 16, BLANCO);
            char gen[32]; std::snprintf(gen, sizeof(gen), "GEN %.0f MW", s.generacionMW);
            DrawText(gen, (int)cen.x - 46, (int)cen.y + 2, 16, s.generacionMW > s.genMaxMW ? ROJO : AMBAR);
        }

        // --- lines + animated flow ---
        for (const LineView& l : s.lineas) {
            const BusView& a = s.buses[l.from];
            const BusView& b = s.buses[l.to];
            Vector2 pa = { a.x, a.y }, pb = { b.x, b.y };
            if (!l.cerrada) {
                DrawLineEx(pa, pb, 2, Color{ 80,80,90,255 });
                Vector2 mid = { (pa.x + pb.x) / 2, (pa.y + pb.y) / 2 };
                DrawText("OPEN", (int)mid.x - 18, (int)mid.y - 8, 16, ROJO);
                continue;
            }
            Color c = colorPorCarga(l.ratio());
            float grosor = 3.0f + 7.0f * (l.ratio() > 1.2f ? 1.2f : l.ratio());
            DrawLineEx(pa, pb, grosor, Color{ c.r, c.g, c.b, 90 });   // faint "pipe"
            dibujarFlujoAnimado(pa, pb, l.flujoMW, c, t);             // energy flowing
            Vector2 mid = { (pa.x + pb.x) / 2, (pa.y + pb.y) / 2 };
            char txt[48]; std::snprintf(txt, sizeof(txt), "%.1f / %.0f", l.flujoMW, l.capacidadMW);
            DrawRectangle((int)mid.x - 34, (int)mid.y - 11, 68, 22, Color{ 20,26,40,220 });
            DrawText(txt, (int)mid.x - 30, (int)mid.y - 8, 16, c);
        }

        // --- buses / substations ---
        // pulse to highlight the agent whose turn it is
        const float pulso = 0.5f + 0.5f * std::sin(float(t) * 5.0f);   // 0..1
        float demandaServidaTotal = 0.0f;
        for (int bi = 0; bi < (int)s.buses.size(); ++bi) {
            const BusView& b = s.buses[bi];
            demandaServidaTotal += b.demandaMW;
            Vector2 p = { b.x, b.y };
            const bool esTurno = (s.currentPlayer == bi);   // its PID matches the bus index (0..3)
            // outer ring: normal, or highlighted and pulsing on its turn
            if (esTurno) {
                unsigned char al = (unsigned char)(120 + 135 * pulso);
                DrawCircleV(p, 56, Color{ 255, 230, 120, 50 });
                DrawCircleLines((int)p.x, (int)p.y, 56, Color{ 255, 220, 90, al });
                DrawCircleLines((int)p.x, (int)p.y, 57, Color{ 255, 220, 90, al });
            }
            DrawCircleV(p, 44, AZUL_TENUE);
            DrawCircleLines((int)p.x, (int)p.y, 44, AZUL);
            if (b.cuarentena) DrawCircleLines((int)p.x, (int)p.y, 50, ROJO);
            const float rInner = esTurno ? 38.0f : 30.0f;   // bigger circle on its turn
            DrawCircleV(p, rInner, Color{ 28,36,54,255 });
            DrawCircleLines((int)p.x, (int)p.y, (int)rInner, esTurno ? Color{255,220,90,255} : (b.esCentral ? AMBAR : AZUL));
            DrawText(b.label.c_str(), (int)p.x - 7, (int)p.y - 12, 26, BLANCO);
            if (esTurno) DrawText("TURN", (int)p.x - 22, (int)p.y - 78, 16, Color{255,220,90,255});
            if (esTurno && !b.ultimaAccion.empty()) {
                int wTxt = MeasureText(b.ultimaAccion.c_str(), 18);
                DrawRectangle((int)p.x + 44, (int)p.y - 12, wTxt + 12, 24, Color{ 40,40,20,230 });
                DrawText(b.ultimaAccion.c_str(), (int)p.x + 50, (int)p.y - 8, 18, Color{255,220,90,255});
            }

            // --- consumers: outward arrow + consumed MW ---
            if (b.demandaMW > 0.0f) {
                Vector2 c0 = { p.x, p.y + 30 };
                Vector2 c1 = { p.x, p.y + 44 };
                dibujarFlujoAnimado(c0, c1, b.demandaMW, VERDE, t);   // energy going out to consumers
                DrawTriangle({ p.x - 6, p.y + 44 }, { p.x + 6, p.y + 44 }, { p.x, p.y + 54 }, VERDE);
                char d[48]; std::snprintf(d, sizeof(d), "%.0f MW", b.demandaMW);
                DrawText(d, (int)p.x - 22, (int)p.y + 58, 16, VERDE);
            }
            if (b.deslastradoMW > 0.0f) {
                char sh[48]; std::snprintf(sh, sizeof(sh), "-%.1f shed", b.deslastradoMW);
                DrawText(sh, (int)p.x - 22, (int)p.y + 76, 15, AMBAR);
            }
            if (!b.ultimaAccion.empty() && b.ultimaAccion != "NoOp")
                DrawText(b.ultimaAccion.c_str(), (int)p.x - 24, (int)p.y - 62, 15, AZUL);
            if (b.contadorCongelado) DrawText("[meter spoofed]", (int)p.x + 36, (int)p.y, 14, MORADO);
            if (b.falsaAlarma)       DrawText("[false alarm]", (int)p.x + 36, (int)p.y, 14, MORADO);
        }

        // --- hackers ---  (PID 4 = striker, PID 5 = spoofer)
        for (const HackerView& h : s.hackers) {
            Vector2 p = { h.x, h.y };
            Color c = h.esStriker ? ROJO : MORADO;
            const bool esTurno = (s.currentPlayer == (h.esStriker ? 4 : 5));
            if (esTurno) {
                unsigned char al = (unsigned char)(120 + 135 * pulso);
                DrawCircleV(p, 34, Color{ 255, 230, 120, 50 });
                DrawCircleLines((int)p.x, (int)p.y, 34, Color{ 255, 220, 90, al });
                DrawText("TURN", (int)p.x - 22, (int)p.y - 44, 16, Color{255,220,90,255});
                if (!h.ultimaAccion.empty()) {
                    int wTxt = MeasureText(h.ultimaAccion.c_str(), 18);
                    DrawRectangle((int)p.x - wTxt - 46, (int)p.y - 12, wTxt + 12, 24, Color{ 40,40,20,230 });
                    DrawText(h.ultimaAccion.c_str(), (int)p.x - wTxt - 40, (int)p.y - 8, 18, Color{255,220,90,255});
                }
            }
            const float r = esTurno ? 28.0f : 22.0f;
            DrawCircleV(p, r, Color{ 30,20,30,255 });
            DrawCircleLines((int)p.x, (int)p.y, (int)r, esTurno ? Color{255,220,90,255} : c);
            DrawText(h.esStriker ? "S" : "H", (int)p.x - 6, (int)p.y - 10, 22, c);
            DrawText(h.label.c_str(), (int)p.x - 46, (int)p.y + 30, 15, c);
            if (!h.ultimaAccion.empty() && h.ultimaAccion != "NoOp")
                DrawText(h.ultimaAccion.c_str(), (int)p.x - 46, (int)p.y + 48, 15, c);
        }

        // --- top HUD ---
        int y = 54;
        char hud[96];
        std::snprintf(hud, sizeof(hud), "Episode:%d Step %d/%d   Red budget: %d",s.currentEpisode, s.step, s.maxSteps, s.presupuestoRed);
        DrawText(hud, 20, y, 18, GRIS);
        const char* faseTxt =
            s.fase == Fase::Normal ? "PHASE: normal operation" :
            s.fase == Fase::Ataque ? "PHASE: attack (A-B overloaded)" :
            s.fase == Fase::Caso1_Blackout ? "PHASE: Case 1 - masked hit -> BLACKOUT" :
            "PHASE: Case 2 - B+D cooperation -> grid saved";
        DrawText(faseTxt, 20, y + 24, 18, BLANCO);
        if (s.winner == 0) DrawText("BLUE WINS (self-healing)", 20, y + 50, 20, VERDE);
        if (s.winner == 1) DrawText("RED WINS (blackout)", 20, y + 50, 20, ROJO);

        // --- BALANCE PANEL (generation == consumption) ---
        {
            const int px = 20, py = 560, bw = 300, bh = 14;
            // Post-physics truth (matches the daily graph): served vs real customer demand.
            const float gen = s.realServedMW, dem = s.realDemandMW;
            DrawText("BALANCE  real supply / real demand", px, py - 22, 16, GRIS);
            // generation bar (cap = genMax)
            float fg = s.genMaxMW > 0 ? (gen / s.genMaxMW) : 0.0f; if (fg > 1) fg = 1;
            DrawRectangle(px, py, bw, bh, Color{ 30,36,52,255 });
            DrawRectangle(px, py, int(bw * fg), bh, AMBAR);
            char tg[48]; std::snprintf(tg, sizeof(tg), "SERVED  %.0f / %.0f MW", gen, s.genMaxMW);
            DrawText(tg, px, py + bh + 4, 15, AMBAR);
            // demand bar
            float fd = s.genMaxMW > 0 ? (dem / s.genMaxMW) : 0.0f; if (fd > 1) fd = 1;
            DrawRectangle(px, py + 40, bw, bh, Color{ 30,36,52,255 });
            DrawRectangle(px, py + 40, int(bw * fd), bh, VERDE);
            char td[48]; std::snprintf(td, sizeof(td), "REAL DEMAND  %.0f MW", dem);
            DrawText(td, px, py + 40 + bh + 4, 15, VERDE);
            // balance verdict
            // verdict BELOW the bars (avoids overlapping the bus labels on the right)
            const float deficit = dem - gen;
            if (deficit > 0.5f) {
                char tdef[72]; std::snprintf(tdef, sizeof(tdef), "UNSERVED %.0f MW (customers without power)", deficit);
                DrawText(tdef, px, py + 76, 16, ROJO);
            }
            else {
                DrawText("FULL SUPPLY", px, py + 76, 16, VERDE);
            }
        }

        // ================== BOTTOM GRAPHS (two, side by side) ==================
        const int gy = 705, gh = 90;
        const int totalW = GetScreenWidth() - 40;
        const int halfW = (totalW - 20) / 2;
        const int gxL = 20;                 // left: win rate
        const int gxR = 20 + halfW + 20;    // right: daily demand

        // --- LEFT: WIN RATE per episode ---
        {
            DrawText("WIN RATE (last 30 episodes)", gxL, gy - 20, 16, GRIS);
            DrawRectangle(gxL, gy, halfW, gh, Color{ 18,22,34,255 });
            DrawRectangleLines(gxL, gy, halfW, gh, Color{ 50,58,80,255 });
            for (int k = 0; k <= 2; ++k) { int yy = gy + gh - (int)((k*0.5f)*gh); DrawLine(gxL, yy, gxL + halfW, yy, Color{ 40,46,66,255 }); }
            auto serie = [&](const std::vector<float>& h, Color col){
                if (h.size() < 2) return;
                float step = (float)halfW / (float)(h.size() - 1);
                for (size_t i = 1; i < h.size(); ++i) {
                    float v1 = h[i-1] < 0 ? 0 : (h[i-1] > 100 ? 100 : h[i-1]);
                    float v2 = h[i]   < 0 ? 0 : (h[i]   > 100 ? 100 : h[i]);
                    DrawLineEx({ gxL + (i-1)*step, gy + gh - (v1/100.0f)*gh }, { gxL + i*step, gy + gh - (v2/100.0f)*gh }, 2.0f, col);
                }
            };
            serie(_histBlue, AZUL);
            serie(_histRed, ROJO);
            if (!_histBlue.empty()) { char t[48]; std::snprintf(t,sizeof(t),"Blue %.0f%%",_histBlue.back()); DrawText(t, gxL+8, gy+6, 15, AZUL); }
            if (!_histRed.empty())  { char t[48]; std::snprintf(t,sizeof(t),"Red %.0f%%", _histRed.back());  DrawText(t, gxL+96, gy+6, 15, ROJO); }
        }

        // --- RIGHT: DAILY DEMAND (forecast / real / generation) ---
        {
            DrawText("DAILY DEMAND  (forecast / real / gen)", gxR, gy - 20, 16, GRIS);
            DrawRectangle(gxR, gy, halfW, gh, Color{ 18,22,34,255 });
            DrawRectangleLines(gxR, gy, halfW, gh, Color{ 50,58,80,255 });
            const float yMax = (s.genMaxMW > 0 ? s.genMaxMW : 150.0f) * 1.3f;   // headroom to see overshoot
            // generation cap line (genMax)
            int yCap = gy + gh - (int)((s.genMaxMW / yMax) * gh);
            DrawLine(gxR, yCap, gxR + halfW, yCap, Color{ 90,70,40,255 });
            DrawText("genMax", gxR + halfW - 54, yCap - 14, 12, Color{ 150,120,70,255 });
            int N = (int)_forecast.size();
            if ((int)s.demandaRealDia.size() > N) N = (int)s.demandaRealDia.size();

            // --- demand PEAK band (18:30-21:30) and hourly grid every 3h ---
            if (N > 1) {
                const int xP0 = gxR + (int)((18.5f / 24.0f) * halfW);   // 18:30
                const int xP1 = gxR + (int)((21.5f / 24.0f) * halfW);   // 21:30
                DrawRectangle(xP0, gy, xP1 - xP0, gh, Color{ 240, 150, 40, 28 });
                DrawText("PEAK", xP0 + 4, gy + gh - 14, 11, Color{ 240, 170, 80, 200 });
                for (int h = 0; h <= 24; h += 3) {
                    const int xh = gxR + (int)(((float)h / 24.0f) * halfW);
                    if (h > 0 && h < 24) DrawLine(xh, gy + 1, xh, gy + gh - 1, Color{ 40, 46, 66, 255 });   // rejilla
                    DrawLine(xh, gy + gh, xh, gy + gh + 4, GRIS);                                         // marca
                    char th[8]; std::snprintf(th, sizeof(th), "%02dh", h);
                    DrawText(th, xh - 9, gy + gh + 5, 11, GRIS);
                }
            }

            // --- attack events: vertical lines (red=Striker, purple=Spoofer), intensity=alpha ("heat") ---
            if (N > 1) {
                for (int i = 0; i < N && i < (int)s.eventoStrikerDia.size(); ++i) {
                    const float mag = s.eventoStrikerDia[i];
                    if (mag > 0.01f) {
                        const int xx = gxR + (int)((float)i / (float)(N - 1) * halfW);
                        unsigned char a = (unsigned char)std::min(210.0f, 50.0f + mag * 6.0f);
                        DrawLine(xx, gy, xx, gy + gh, Color{ 235, 70, 70, a });     // Striker
                    }
                }
                for (int i = 0; i < N && i < (int)s.eventoSpooferDia.size(); ++i) {
                    const float mag = s.eventoSpooferDia[i];
                    if (mag > 0.01f) {
                        const int xx = gxR + (int)((float)i / (float)(N - 1) * halfW);
                        unsigned char a = (unsigned char)std::min(190.0f, 60.0f + mag * 55.0f);
                        DrawLine(xx, gy, xx, gy + gh, Color{ 180, 90, 240, a });    // Spoofer
                    }
                }
            }
            // --- SYNCHRONIZED ATTACKS (Striker + Spoofer on the SAME bus within <= kSync steps): gold band ---
            int nSync = 0;
            if (N > 1) {
                int ultimoLabelX = -1000;
                for (int i = 0; i < N && i < (int)s.eventoSyncDia.size(); ++i) {
                    const int mask = (int)s.eventoSyncDia[i];
                    if (mask == 0) continue;
                    ++nSync;
                    const int xx = gxR + (int)((float)i / (float)(N - 1) * halfW);
                    DrawRectangle(xx - 2, gy, 5, gh, Color{ 255, 210, 60, 110 });                 // gold band
                    DrawPoly(Vector2{ (float)xx, (float)gy + 9.0f }, 4, 6.0f, 0.0f, Color{ 255, 215, 70, 255 });   // rombo
                    if (xx - ultimoLabelX > 16) {                                              // label of the attacked bus
                        char lb[8]; int k = 0;
                        if (mask & (1 << 1)) lb[k++] = 'B';
                        if (mask & (1 << 2)) lb[k++] = 'C';
                        if (mask & (1 << 3)) lb[k++] = 'D';
                        lb[k] = 0;
                        DrawText(lb, xx - 4, gy + 17, 12, Color{ 255, 215, 70, 255 });
                        ultimoLabelX = xx;
                    }
                }
            }
            auto plot = [&](const std::vector<float>& v, Color col, bool soloHastaAhora){
                if (N < 2) return;
                float step = (float)halfW / (float)(N - 1);
                for (int i = 1; i < N && i < (int)v.size(); ++i) {
                    if (soloHastaAhora && (v[i] < 0 || v[i-1] < 0)) continue;   // not reached yet
                    float v1 = v[i-1], v2 = v[i];
                    DrawLineEx({ gxR + (i-1)*step, gy + gh - (v1/yMax)*gh }, { gxR + i*step, gy + gh - (v2/yMax)*gh }, 2.0f, col);
                }
            };
            plot(_forecast, Color{ 130,140,160,180 }, false);   // forecast (grey)
            plot(s.generacionDia, AMBAR, true);                 // generated/served (amber)
            plot(s.demandaRealDia, VERDE, true);                // real demand (green)
            // current step marker
            if (N > 1) { int xNow = gxR + (int)((float)s.step / (float)(N-1) * halfW); DrawLine(xNow, gy, xNow, gy + gh, Color{ 255,255,255,60 }); }
            DrawText("forecast", gxR+8,  gy+6, 13, Color{ 150,160,180,255 });
            DrawText("real",     gxR+78, gy+6, 13, VERDE);
            DrawText("gen",      gxR+128, gy+6, 13, AMBAR);
            DrawText("| atk", gxR+158, gy+6, 13, GRIS);
            DrawText("S", gxR+198, gy+6, 13, ROJO);
            DrawText("H", gxR+214, gy+6, 13, MORADO);
            DrawText("| SYNC", gxR+232, gy+6, 13, Color{ 255, 215, 70, 255 });
            { char ts[32]; std::snprintf(ts, sizeof(ts), "synchronized today: %d", nSync);
              DrawText(ts, gxR + halfW - MeasureText(ts, 13) - 8, gy + 22, 13, Color{ 255, 215, 70, 255 }); }
        }

        // --- METIS watermark (subtle, green, top-right corner) ---
        if (_logoOk) {
            const float destW = 140.0f;
            const float sc = destW / (float)_logo.width;
            DrawTextureEx(_logo, { GetScreenWidth() - destW - 16.0f, 28.0f }, 0.0f, sc, Color{ 255,255,255,180 });
        }

        // --- human player action overlay (play mode) ---
        if (_menuOn && !_menuAcc.empty()) {
            const int w = 320, x = GetScreenWidth() - w - 16, y = 150, rowH = 26;
            const int h = 60 + rowH * (int)_menuAcc.size();
            DrawRectangle(x, y, w, h, Color{ 10,14,24,235 });
            DrawRectangleLines(x, y, w, h, Color{ 90,80,40,255 });
            DrawText(_menuTitulo.c_str(), x + 12, y + 10, 18, Color{ 255,220,90,255 });
            DrawText("UP/DOWN select   ENTER confirm", x + 12, y + 34, 13, GRIS);
            for (int i = 0; i < (int)_menuAcc.size(); ++i) {
                const int ry = y + 60 + i * rowH;
                if (i == _menuSel) {
                    DrawRectangle(x + 8, ry - 3, w - 16, rowH - 2, Color{ 40,40,20,255 });
                    DrawText(">", x + 12, ry, 18, Color{ 255,220,90,255 });
                }
                DrawText(_menuAcc[i].c_str(), x + 32, ry, 18, i == _menuSel ? Color{ 255,220,90,255 } : BLANCO);
            }
        }

        DrawText("[ESC] quit", GetScreenWidth() - 100, 6, 16, GRIS);
        EndDrawing();
    }

    void Visualizer::pushWinRates(float bluePct, float redPct) {
        _histBlue.push_back(bluePct);
        _histRed.push_back(redPct);
        const size_t MAXN = 600;   // sliding window
        if (_histBlue.size() > MAXN) _histBlue.erase(_histBlue.begin());
        if (_histRed.size()  > MAXN) _histRed.erase(_histRed.begin());
    }

    void Visualizer::setDailyForecast(const std::vector<float>& mwPerStep) {
        _forecast = mwPerStep;
    }

} // namespace smartgrid