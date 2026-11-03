#include "Renderer.h"
#include "SeaEnvironment.h"
#include "Drone.h"
#include "Castaway.h"
#include <algorithm>
#include <cmath>

const Color DRONE_COLORS[NUM_DRONES] = {
    Color{  80, 200, 255, 255 },   // UAV_1 sky blue
    Color{   0, 240, 150, 255 },   // UAV_2 mint green
    Color{ 255, 100, 210, 255 },   // UAV_3 pink
    Color{ 180, 150, 255, 255 },   // UAV_4 lavender
};

static const Color SEA_COLOR   = Color{ 4, 8, 18, 255 };
static const Color PANEL_BG    = Color{ 14, 20, 32, 255 };
static const Color TEXT_DIM    = Color{ 140, 155, 175, 255 };
static const Color TEXT_MAIN   = Color{ 225, 232, 240, 255 };
static const Color LINK_OK     = Color{ 40, 230, 120, 230 };
static const Color LINK_WARN   = Color{ 255, 215, 0, 255 };
static const Color LINK_LOST   = Color{ 255, 60, 60, 255 };
static const Color CASTAWAY_C  = Color{ 255, 140, 0, 255 };

static Color lerpColor(Color a, Color b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return Color{ (unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                  (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t) };
}

/// heat map: 1 = just scanned (white-yellow) ... 0 = cold (black, needs searching)
static Color heatColor(float h)
{
    static const float stops[] = { 0.0f, 0.12f, 0.35f, 0.6f, 0.85f, 1.0f };
    static const Color cols[] = {
        SEA_COLOR,
        Color{ 55, 0, 45, 255 },
        Color{ 150, 15, 25, 255 },
        Color{ 230, 85, 0, 255 },
        Color{ 255, 195, 30, 255 },
        Color{ 255, 250, 205, 255 } };
    if (h <= 0.0f) return cols[0];
    for (int i = 1; i < 6; i++)
        if (h <= stops[i])
            return lerpColor(cols[i - 1], cols[i], (h - stops[i - 1]) / (stops[i] - stops[i - 1]));
    return cols[5];
}

static bool loadLogo(Texture2D& tex); // defined below

void Renderer::init()
{
    _pixels.assign(GRID_W * GRID_H, SEA_COLOR);
    Image img = GenImageColor(GRID_W, GRID_H, SEA_COLOR);
    _heatTex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(_heatTex, TEXTURE_FILTER_BILINEAR); // smoothing: heat map look
    _logoLoaded = loadLogo(_logoTex);
}

// file of the watermark logo; green (0,255,0) is the transparent color
static const char* LOGO_FILE = "logo_metis_watermark.png";
static const float LOGO_SCALE = 0.6f;   // 200x123 px -> 120x74 px
static const float LOGO_ALPHA = 0.75f;  // slightly translucent: visible but discreet

/// loads the logo and turns the green chroma key into transparency
static bool loadLogo(Texture2D& tex)
{
    // look next to the executable first, then in the working directory
    const char* path = TextFormat("%s%s", GetApplicationDirectory(), LOGO_FILE);
    if (!FileExists(path)) path = LOGO_FILE;
    if (!FileExists(path)) return false;

    Image img = LoadImage(path);
    if (img.data == nullptr) return false;
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    // chroma key: pure green AND the greenish anti-aliasing fringe of the edges (~190 px),
    // otherwise a green halo would appear around the logo. Keyed pixels become (0,0,0,0)
    // so that bilinear filtering does not bleed green into the edges.
    Color* px = (Color*)img.data;
    for (int k = 0; k < img.width * img.height; k++)
    {
        Color c = px[k];
        if (c.g > c.r + 80 && c.g > c.b + 80)
            px[k] = Color{ 0, 0, 0, 0 };
    }
    tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return true;
}

void Renderer::drawLogo()
{
    if (!_logoLoaded) return;
    // top-left corner of the map: free sea area (the compass is top-right, scale bar and legend at the bottom)
    float w = _logoTex.width * LOGO_SCALE, h = _logoTex.height * LOGO_SCALE;
    DrawTexturePro(_logoTex, Rectangle{ 0, 0, (float)_logoTex.width, (float)_logoTex.height },
                   Rectangle{ (float)MAP_X + 8, (float)MAP_Y + 8, w, h }, Vector2{ 0, 0 }, 0.0f, Fade(WHITE, LOGO_ALPHA));
}

void Renderer::shutdown()
{
    if (_logoLoaded) UnloadTexture(_logoTex);
    UnloadTexture(_heatTex);
}

void Renderer::drawHeader(const char* subtitle)
{
    DrawRectangle(0, 0, SCREEN_W, 52, PANEL_BG);
    DrawText("METIS-Core  |  SAR Swarm PoC", 14, 10, 24, TEXT_MAIN);
    DrawText(subtitle, 14, 36, 12, TEXT_DIM);
}

void Renderer::drawWorld(SeaEnvironment& env, const RenderOptions& opt)
{
    drawHeatMap(env, opt);
    drawCurrentFlow(env);
    drawShip();
    drawComms(env, opt);
    drawDrones(env);
    drawCastaway(env, opt);
    drawScaleAndLegend();
    drawCurrentCompass(env);
    DrawRectangleLinesEx(Rectangle{ (float)MAP_X - 1, (float)MAP_Y - 1, (float)MAP_W + 2, (float)MAP_H + 2 }, 1, Color{ 60, 80, 110, 255 });
}

void Renderer::drawHeatMap(SeaEnvironment& env, const RenderOptions& opt)
{
    for (int cy = 0; cy < GRID_H; cy++)
        for (int cx = 0; cx < GRID_W; cx++)
            _pixels[cy * GRID_W + cx] = heatColor(env.cellHeat(opt.viewDrone, cx, cy));
    UpdateTexture(_heatTex, _pixels.data());
    DrawTexturePro(_heatTex, Rectangle{ 0, 0, (float)GRID_W, (float)GRID_H },
                   Rectangle{ (float)MAP_X, (float)MAP_Y, (float)MAP_W, (float)MAP_H }, Vector2{ 0, 0 }, 0.0f, WHITE);

    // very faint 500 m grid
    for (float gx = 500.0f; gx < WORLD_W; gx += 500.0f)
    {
        Vector2 a = toScreen(gx, 0), b = toScreen(gx, WORLD_H);
        DrawLineV(a, b, Color{ 40, 60, 90, 60 });
    }
    for (float gy = 500.0f; gy < WORLD_H; gy += 500.0f)
    {
        Vector2 a = toScreen(0, gy), b = toScreen(WORLD_W, gy);
        DrawLineV(a, b, Color{ 40, 60, 90, 60 });
    }
}

// draws an arrow from 'from' to 'to' with a triangular tip
static void drawArrow(Vector2 from, Vector2 to, float thick, float head, Color c)
{
    float dx = to.x - from.x, dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;
    float ux = dx / len, uy = dy / len;
    Vector2 base = { to.x - ux * head, to.y - uy * head };
    DrawLineEx(from, base, thick, c);
    Vector2 left = { base.x - uy * head * 0.55f, base.y + ux * head * 0.55f };
    Vector2 right = { base.x + uy * head * 0.55f, base.y - ux * head * 0.55f };
    // raylib: vertices counter-clockwise on screen
    DrawTriangle(to, left, right, c);
    DrawTriangle(to, right, left, c);
}

void Renderer::respawnParticle(FlowParticle& p, bool randomAge)
{
    // random position inside the map; life between 2.5 and 4.5 real seconds
    p.x = (float)GetRandomValue(MAP_X + 6, MAP_X + MAP_W - 6);
    p.y = (float)GetRandomValue(MAP_Y + 6, MAP_Y + MAP_H - 6);
    p.maxLife = 2.5f + GetRandomValue(0, 200) / 100.0f;
    p.life = randomAge ? p.maxLife * GetRandomValue(0, 100) / 100.0f : 0.0f;
}

void Renderer::drawCurrentFlow(SeaEnvironment& env)
{
    float spd = env.currentSpeed();
    if (spd < 0.01f) return;
    float ux = env.currentVx / spd, uy = env.currentVy / spd;
    float rel = spd / CURRENT_MAX_SPEED;                 // 0..1

    // particles instead of a grid: each arrow ONLY moves with the current,
    // it fades in and out, so there are never backward jumps (aliasing)
    const int NUM_PARTICLES = 140;
    if ((int)_flow.size() != NUM_PARTICLES)
    {
        _flow.resize(NUM_PARTICLES);
        for (FlowParticle& p : _flow) respawnParticle(p, true);
    }

    // clamped real dt: while training we draw every N steps and the time between frames is irregular
    float dt = std::min(GetFrameTime(), 1.0f / 30.0f);
    float pxPerSec = 12.0f + 28.0f * rel;               // visual speed (px/s), grows with the current
    float arrowLen = 9.0f + 12.0f * rel;
    unsigned char maxAlpha = (unsigned char)(70 + 80 * rel);

    for (FlowParticle& p : _flow)
    {
        p.x += ux * pxPerSec * dt;
        p.y += uy * pxPerSec * dt;
        p.life += dt;
        bool out = p.x < MAP_X + 4 || p.x > MAP_X + MAP_W - 4 || p.y < MAP_Y + 4 || p.y > MAP_Y + MAP_H - 4;
        if (out || p.life >= p.maxLife)
        {
            respawnParticle(p, false);
            continue;
        }
        // fade in and out: alpha = sin(pi * life)
        float fade = sinf(SIM_PI * p.life / p.maxLife);
        Color c = Color{ 90, 170, 255, (unsigned char)(maxAlpha * fade) };
        Vector2 from = { p.x - ux * arrowLen * 0.5f, p.y - uy * arrowLen * 0.5f };
        Vector2 to = { p.x + ux * arrowLen * 0.5f, p.y + uy * arrowLen * 0.5f };
        drawArrow(from, to, 1.5f, 5.0f, c);
    }
}

void Renderer::drawCurrentCompass(SeaEnvironment& env)
{
    // fixed icon in the top-right corner of the map
    float R = 30.0f;
    Vector2 c = { (float)(MAP_X + MAP_W - 52), (float)(MAP_Y + 52) };
    DrawCircleV(c, R + 8.0f, Fade(BLACK, 0.6f));
    DrawCircleLinesV(c, R, Color{ 90, 170, 255, 200 });
    DrawText("N", (int)c.x - 3, (int)(c.y - R - 2), 10, TEXT_MAIN);
    DrawLineV(Vector2{ c.x, c.y - R + 9 }, Vector2{ c.x, c.y - R + 3 }, TEXT_DIM);
    DrawLineV(Vector2{ c.x + R - 3, c.y }, Vector2{ c.x + R - 9, c.y }, TEXT_DIM);
    DrawLineV(Vector2{ c.x, c.y + R - 3 }, Vector2{ c.x, c.y + R - 9 }, TEXT_DIM);
    DrawLineV(Vector2{ c.x - R + 3, c.y }, Vector2{ c.x - R + 9, c.y }, TEXT_DIM);

    float spd = env.currentSpeed();
    if (spd > 0.01f)
    {
        float ux = env.currentVx / spd, uy = env.currentVy / spd;
        Vector2 from = { c.x - ux * (R - 8), c.y - uy * (R - 8) };
        Vector2 to = { c.x + ux * (R - 4), c.y + uy * (R - 4) };
        drawArrow(from, to, 3.0f, 10.0f, Color{ 90, 190, 255, 255 });
    }
    const char* t1 = "CURRENT";
    const char* t2 = TextFormat("%.2f m/s  bearing %03.0f", spd, env.currentBearingDeg());
    DrawText(t1, (int)c.x - MeasureText(t1, 10) / 2, (int)(c.y + R + 10), 10, Color{ 90, 190, 255, 255 });
    DrawText(t2, (int)c.x - MeasureText(t2, 10) / 2, (int)(c.y + R + 22), 10, TEXT_MAIN);
}

void Renderer::drawShip()
{
    // mothership seen from above (bow to the East), ~120 m long
    Vector2 c = toScreen(SHIP_X, SHIP_Y);
    float L = 34.0f, B = 11.0f;
    Rectangle hull = { c.x - L * 0.5f, c.y - B * 0.5f, L * 0.75f, B };
    DrawRectangleRec(hull, Color{ 120, 130, 145, 255 });
    DrawTriangle(Vector2{ hull.x + hull.width, hull.y }, Vector2{ hull.x + hull.width, hull.y + B },
                 Vector2{ c.x + L * 0.5f, c.y }, Color{ 120, 130, 145, 255 });
    DrawRectangle((int)(hull.x + 3), (int)(c.y - 3), 7, 6, Color{ 70, 80, 95, 255 }); // bridge
    DrawCircleLines((int)(c.x + 4), (int)c.y, 4, Color{ 255, 255, 255, 180 });      // helipad
    DrawText("MOTHERSHIP", (int)(c.x - 22), (int)(c.y + 10), 10, TEXT_DIM);
}

void Renderer::drawComms(SeaEnvironment& env, const RenderOptions& opt)
{
    float blink = 0.5f + 0.5f * sinf((float)GetTime() * 10.0f);
    for (int i = 0; i < NUM_DRONES; i++)
    {
        Drone* di = env.drone(i);
        Vector2 a = toScreen(di->x, di->y);
        if (opt.showCommRange)
            DrawCircleLinesV(a, COMM_RANGE * MAP_SCALE, Fade(DRONE_COLORS[i], 0.25f));

        for (int j = i + 1; j < NUM_DRONES; j++)
        {
            float d = env.linkDist[i][j];
            if (d > COMM_RANGE) continue;
            Drone* dj = env.drone(j);
            Vector2 b = toScreen(dj->x, dj->y);
            float ratio = d / COMM_RANGE;
            if (ratio <= COMM_WARN_RATIO)
            {
                DrawLineEx(a, b, 2.0f, LINK_OK);
            }
            else
            {
                // link about to be lost: blinking yellow
                DrawLineEx(a, b, 2.5f, Fade(LINK_WARN, 0.45f + 0.55f * blink));
                Vector2 mid = { (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f };
                DrawText(TextFormat("%.0f%%", ratio * 100.0f), (int)mid.x + 4, (int)mid.y - 12, 10, LINK_WARN);
            }
        }
    }
    // drones without any link: red warning
    for (int i = 0; i < NUM_DRONES; i++)
    {
        bool any = false;
        for (int j = 0; j < NUM_DRONES; j++)
            if (env.directLink[i][j]) any = true;
        if (!any)
        {
            Vector2 a = toScreen(env.drone(i)->x, env.drone(i)->y);
            DrawCircleLinesV(a, 14.0f + 3.0f * blink, LINK_LOST);
            DrawText(TextFormat("NO LINK %ds", env.disconnectedSteps[i]), (int)a.x + 12, (int)a.y + 8, 10, LINK_LOST);
        }
    }
}

void Renderer::drawDrones(SeaEnvironment& env)
{
    for (int i = 0; i < NUM_DRONES; i++)
    {
        Drone* d = env.drone(i);
        Vector2 p = toScreen(d->x, d->y);
        Color c = DRONE_COLORS[i];
        if (!d->launched)
        {
            DrawCircleV(p, 3.0f, Fade(c, 0.6f)); // on deck
            continue;
        }
        // sensor footprint
        DrawCircleV(p, SENSOR_RADIUS * MAP_SCALE, Fade(c, 0.10f));
        DrawCircleLinesV(p, SENSOR_RADIUS * MAP_SCALE, Fade(c, 0.55f));

        // quadcopter: 4 arms in X + body
        float ch = cosf(d->heading), sh = sinf(d->heading);
        for (int k = 0; k < 4; k++)
        {
            float a = d->heading + SIM_PI * 0.25f + k * SIM_PI * 0.5f;
            Vector2 tip = { p.x + cosf(a) * 7.0f, p.y + sinf(a) * 7.0f };
            DrawLineEx(p, tip, 2.0f, c);
            DrawCircleLinesV(tip, 2.5f, c);
        }
        DrawCircleV(p, 3.0f, WHITE);
        // heading vector
        DrawLineEx(p, Vector2{ p.x + ch * 16.0f, p.y + sh * 16.0f }, 1.5f, Fade(WHITE, 0.8f));
        DrawText(TextFormat("%d", i + 1), (int)p.x - 14, (int)p.y - 16, 12, c);
    }
}

void Renderer::drawCastaway(SeaEnvironment& env, const RenderOptions& opt)
{
    Castaway* c = env.castaway();
    bool found = env.detectedBy >= 0;
    if (!opt.showCastaway && !found) return;

    // drift trail
    for (size_t k = 0; k + 1 < c->trail.size(); k += 2)
    {
        Vector2 t = toScreen(c->trail[k], c->trail[k + 1]);
        DrawCircleV(t, 1.2f, Fade(CASTAWAY_C, 0.35f));
    }
    Vector2 p = toScreen(c->x, c->y);
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f);
    DrawCircleV(p, 4.0f, CASTAWAY_C);
    DrawCircleLinesV(p, 7.0f + 5.0f * pulse, Fade(CASTAWAY_C, 0.7f));
    if (found)
    {
        DrawCircleLinesV(p, 22.0f, LINK_OK);
        DrawText("FOUND", (int)p.x + 14, (int)p.y - 20, 12, LINK_OK);
    }
    else
    {
        DrawText("castaway", (int)p.x + 8, (int)p.y + 6, 10, Fade(CASTAWAY_C, 0.8f));
    }
}

void Renderer::drawScaleAndLegend()
{
    // 500 m scale bar
    int x = MAP_X + 14, y = MAP_Y + MAP_H - 20;
    int len = (int)(500.0f * MAP_SCALE);
    DrawLine(x, y, x + len, y, TEXT_MAIN);
    DrawLine(x, y - 4, x, y + 4, TEXT_MAIN);
    DrawLine(x + len, y - 4, x + len, y + 4, TEXT_MAIN);
    DrawText("500 m", x + len + 6, y - 6, 10, TEXT_MAIN);

    // heat map legend
    int lx = MAP_X + MAP_W - 230, ly = MAP_Y + MAP_H - 26;
    for (int k = 0; k < 160; k++)
        DrawLine(lx + k, ly, lx + k, ly + 10, heatColor(1.0f - k / 159.0f));
    DrawRectangleLines(lx, ly, 160, 10, TEXT_DIM);
    DrawText("T just scanned", lx, ly - 13, 10, TEXT_DIM);
    DrawText("cold", lx + 168, ly, 10, TEXT_DIM);
}

void Renderer::drawSparkline(const std::vector<float>& h, int x, int y, int w, int hgt, Color c, float scale)
{
    DrawRectangleLines(x, y, w, hgt, Color{ 50, 65, 90, 255 });
    int mid = y + hgt / 2;
    DrawLine(x, mid, x + w, mid, Color{ 50, 65, 90, 160 });
    if (h.size() < 2) return;
    size_t n = h.size();
    for (size_t k = 0; k + 1 < n; k++)
    {
        // scale: the chart goes from -scale to +scale
        float v1 = std::clamp(h[k] / scale, -1.0f, 1.0f), v2 = std::clamp(h[k + 1] / scale, -1.0f, 1.0f);
        int x1 = x + (int)(k * (float)w / (n - 1)), x2 = x + (int)((k + 1) * (float)w / (n - 1));
        DrawLine(x1, mid - (int)(v1 * hgt * 0.5f), x2, mid - (int)(v2 * hgt * 0.5f), c);
    }
}

void Renderer::drawPanel(SeaEnvironment& env, const PanelInfo& info, const RenderOptions& opt)
{
    int x = PANEL_X, y = MAP_Y;
    int w = SCREEN_W - PANEL_X - 12;
    DrawRectangle(x - 6, y - 6, w + 12, MAP_H + 12, PANEL_BG);

    DrawText(info.mode.c_str(), x, y, 20, info.mode == "TRAINING" ? LINK_WARN : LINK_OK);
    y += 26;
    DrawText(TextFormat("Policy: %s", info.policy.c_str()), x, y, 12, TEXT_MAIN); y += 16;
    DrawText(TextFormat("Episode: %d   %s", info.episode, info.gpu ? "GPU" : "CPU"), x, y, 12, TEXT_MAIN); y += 16;
    if (info.outcomesCount > 0)
    {
        DrawText(TextFormat("Successes last %d: %d (%.0f%%)", info.outcomesCount, info.successes,
                            100.0f * info.successes / info.outcomesCount), x, y, 12, TEXT_MAIN);
        y += 16;
    }
    if (info.ticksPerFrame > 0)
    {
        DrawText(TextFormat("Speed: x%d %s", info.ticksPerFrame, info.paused ? "(PAUSED)" : ""), x, y, 12, TEXT_MAIN);
        y += 16;
    }

    DrawText(TextFormat("Current: %.2f m/s towards %03.0f degrees", env.currentSpeed(), env.currentBearingDeg()),
             x, y, 12, Color{ 90, 190, 255, 255 });
    y += 16;

    // time and castaway survival
    int step = env.currentStep();
    float life = 1.0f - (float)step / (float)MAX_STEPS;
    y += 6;
    DrawText(TextFormat("T+ %02d:%02d   castaway survival", step / 60, step % 60), x, y, 12, TEXT_MAIN); y += 16;
    DrawRectangle(x, y, w, 10, Color{ 40, 20, 20, 255 });
    DrawRectangle(x, y, (int)(w * std::max(0.0f, life)), 10, lerpColor(LINK_LOST, LINK_OK, life));
    y += 20;

    // drones
    for (int i = 0; i < NUM_DRONES; i++)
    {
        Drone* d = env.drone(i);
        Color c = DRONE_COLORS[i];
        bool anyLink = false;
        for (int j = 0; j < NUM_DRONES; j++)
            if (env.directLink[i][j]) anyLink = true;
        const char* st = !d->launched ? "on deck"
                       : (!anyLink ? TextFormat("NO LINK %ds", env.disconnectedSteps[i])
                       : (env.componentSize[i] < NUM_DRONES ? TextFormat("partial island %ds", env.disconnectedSteps[i]) : "searching"));
        DrawRectangle(x, y + 2, 10, 10, c);
        DrawText(TextFormat("%s  %s  island:%d  r:%+.2f", d->name.c_str(), st, env.component[i], env.lastReward[i]),
                 x + 16, y, 12, anyLink || !d->launched ? TEXT_MAIN : LINK_LOST);
        y += 15;
        if (info.rewardHistory != nullptr)
            drawSparkline(info.rewardHistory[i], x + 16, y, w - 16, 26, c);
        y += 34;
    }

    // global team reward: sum of the 4 heads, scale [-4, +4]
    if (info.teamHistory != nullptr)
    {
        float teamNow = 0.0f, teamEpisode = 0.0f;
        for (int i = 0; i < NUM_DRONES; i++)
        {
            teamNow += env.lastReward[i];
            teamEpisode += env.episodeReward[i];
        }
        const Color TEAM_C = Color{ 255, 215, 0, 255 };
        DrawRectangle(x, y + 2, 10, 10, TEAM_C);
        DrawText(TextFormat("TEAM  r:%+.2f  episode:%+.1f", teamNow, teamEpisode), x + 16, y, 12, TEAM_C);
        y += 15;
        drawSparkline(*info.teamHistory, x + 16, y, w - 16, 34, TEAM_C, (float)NUM_DRONES);
        DrawText("+4", x + w - 16, y + 1, 9, TEXT_DIM);
        DrawText("-4", x + w - 16, y + 24, 9, TEXT_DIM);
        y += 42;
    }

    y += 2;
    if (!info.status.empty())
    {
        DrawText(info.status.c_str(), x, y, 12, LINK_WARN);
        y += 18;
    }

    // mission charts: they fill the gap between the status line and the controls
    if (controlsTop(info) > 0)
        drawMissionCharts(env, info, x, y + 4, w, controlsTop(info) - 6);

    // controls with the active option highlighted
    drawControls(info, opt, x, w);
}

// chip of an option: active = filled with color; inactive = faint border only. Returns the width used
static int drawChip(int x, int y, const char* text, bool active, Color c)
{
    const int fs = 10;
    int w = MeasureText(text, fs) + 10;
    if (active)
    {
        DrawRectangle(x, y, w, 14, c);
        DrawText(text, x + 5, y + 2, fs, Color{ 10, 14, 24, 255 });
    }
    else
    {
        DrawRectangleLines(x, y, w, 14, Color{ 60, 75, 100, 255 });
        DrawText(text, x + 5, y + 2, fs, TEXT_DIM);
    }
    return w + 4;
}

// key + description; returns the x where the chips start
static int drawKey(int x, int y, const char* key, const char* label)
{
    DrawText(key, x, y + 2, 10, TEXT_MAIN);
    DrawText(label, x + 58, y + 2, 10, TEXT_DIM);
    return x + 128;
}

void Renderer::drawControls(const PanelInfo& info, const RenderOptions& opt, int x, int w)
{
    bool training = info.renderEvery > 0;
    bool play = (info.mode == "PLAY");
    if (!training && !play) return; // in the menu the controls are the buttons

    const int ROW = 16;   // must match controlsTop()
    const Color ON_C = Color{ 40, 230, 120, 255 };
    const Color OFF_C = Color{ 120, 135, 160, 255 };
    const Color SEL_C = Color{ 255, 215, 0, 255 };
    int y = controlsTop(info);

    DrawLine(x, y, x + w, y, Color{ 50, 65, 90, 255 });
    DrawText("CONTROLS", x, y + 4, 10, TEXT_DIM);
    y += 18;

    // visible map: union or one drone's map
    int cx = drawKey(x, y, "[0-4]", "Map");
    cx += drawChip(cx, y, "0 Union", opt.viewDrone < 0, SEL_C);
    for (int i = 0; i < NUM_DRONES; i++)
        cx += drawChip(cx, y, TextFormat("%d", i + 1), opt.viewDrone == i, DRONE_COLORS[i]);
    y += ROW;

    // radio range
    cx = drawKey(x, y, "[C]", "Radio");
    cx += drawChip(cx, y, "ON", opt.showCommRange, ON_C);
    cx += drawChip(cx, y, "OFF", !opt.showCommRange, OFF_C);
    y += ROW;

    // castaway visible or hidden
    cx = drawKey(x, y, "[N]", "Castaway");
    cx += drawChip(cx, y, "Visible", opt.showCastaway, CASTAWAY_C);
    cx += drawChip(cx, y, "Hidden", !opt.showCastaway, OFF_C);
    y += ROW;

    if (training)
    {
        // drawing frequency
        // short labels so the 4 options fit in the panel width
        cx = drawKey(x, y, "[V]", "Draw every");
        cx += drawChip(cx, y, "1 step", info.renderEvery == 1, SEL_C);
        cx += drawChip(cx, y, "10", info.renderEvery == 10, SEL_C);
        cx += drawChip(cx, y, "25", info.renderEvery == 25, SEL_C);
        cx += drawChip(cx, y, "turbo 500", info.renderEvery == 500, SEL_C);
        y += ROW;
        drawKey(x, y, "[ESC]", "Stop training and save");
    }
    else
    {
        cx = drawKey(x, y, "[H]", "Policy");
        cx += drawChip(cx, y, "AI", !info.useHeuristic, SEL_C);
        cx += drawChip(cx, y, "Heuristic", info.useHeuristic, SEL_C);
        y += ROW;

        cx = drawKey(x, y, "[SPACE]", "Pause");
        cx += drawChip(cx, y, "Paused", info.paused, LINK_WARN);
        cx += drawChip(cx, y, "Running", !info.paused, ON_C);
        y += ROW;

        cx = drawKey(x, y, "[+/-]", "Speed");
        cx += drawChip(cx, y, TextFormat("x%d", info.ticksPerFrame), true, SEL_C);
        y += ROW;

        // a single row to leave room for the charts
        DrawText("[Arrows] castaway   [R] restart   [ESC] menu", x, y + 2, 10, TEXT_DIM);
    }
}

void Renderer::drawBanner(const char* text, Color color)
{
    int fs = 26;
    int tw = MeasureText(text, fs);
    int bx = MAP_X + MAP_W / 2 - tw / 2 - 20, by = MAP_Y + 30;
    DrawRectangle(bx, by, tw + 40, 46, Fade(BLACK, 0.75f));
    DrawRectangleLines(bx, by, tw + 40, 46, color);
    DrawText(text, bx + 20, by + 10, fs, color);
}

// y where the controls block starts (0 if there are no controls, e.g. in the menu)
int Renderer::controlsTop(const PanelInfo& info)
{
    bool training = info.renderEvery > 0;
    bool play = (info.mode == "PLAY");
    if (!training && !play) return 0;
    int rows = training ? 5 : 7;
    return MAP_Y + MAP_H - 16 * rows - 18;
}

/// per-episode history chart: faint dots + 20-episode moving average (swarm)
/// and the lawnmower baseline as its own line, in its own color and with a label
void Renderer::drawHistoryChart(const std::deque<float>& data, int x, int y, int w, int h, float yMax,
                                bool hasRef, float refValue, float refLo, float refHi,
                                Color c, Color refC, const char* fmt, const char* refLabel)
{
    DrawRectangle(x, y, w, h, Color{ 10, 15, 26, 255 });
    DrawRectangleLines(x, y, w, h, Color{ 50, 65, 90, 255 });
    for (int k = 1; k < 4; k++)
        DrawLine(x, y + h * k / 4, x + w, y + h * k / 4, Color{ 28, 38, 58, 255 });
    DrawText(TextFormat(fmt, yMax), x + 2, y + 2, 8, TEXT_DIM);

    auto toY = [&](float v) { return y + h - (int)(std::clamp(v / yMax, 0.0f, 1.0f) * (h - 2)) - 1; };

    // lawnmower band: 25-75 percentile of its episodes (normal spread of the classic method)
    if (hasRef && refHi > refLo)
    {
        int yHi = toY(refHi), yLo = toY(refLo);
        DrawRectangle(x + 1, yHi, w - 2, std::max(1, yLo - yHi), Fade(refC, 0.12f));
        DrawLine(x + 1, yHi, x + w - 1, yHi, Fade(refC, 0.30f));
        DrawLine(x + 1, yLo, x + w - 1, yLo, Fade(refC, 0.30f));
    }

    // swarm curve
    size_t n = data.size();
    if (n >= 2)
    {
        float dx = (float)w / (float)(n - 1);
        for (size_t k = 0; k < n; k++)
            DrawPixel(x + (int)(k * dx), toY(data[k]), Fade(c, 0.45f));   // every episode
        const int WIN = 20;                                               // 20-episode moving average
        float prevY = 0.0f;
        double acc = 0.0;
        for (size_t k = 0; k < n; k++)
        {
            acc += data[k];
            if (k >= (size_t)WIN) acc -= data[k - WIN];
            float cy = (float)toY((float)(acc / std::min<size_t>(k + 1, WIN)));
            if (k > 0)
                DrawLineEx(Vector2{ x + (k - 1) * dx, prevY }, Vector2{ x + k * dx, cy }, 2.0f, c);
            prevY = cy;
        }
    }

    // lawnmower curve: it does not learn, so it is constant. Thick dashed line + label on the line
    if (hasRef)
    {
        float ry = (float)toY(refValue);
        for (int px = x; px < x + w; px += 10)
            DrawLineEx(Vector2{ (float)px, ry }, Vector2{ (float)std::min(px + 6, x + w), ry }, 2.0f, refC);
        int tw = MeasureText(refLabel, 9);
        int ly = (ry - y > 12) ? (int)ry - 11 : (int)ry + 3;   // above the line if it fits
        DrawRectangle(x + w - tw - 6, ly - 1, tw + 4, 10, Color{ 10, 15, 26, 220 });
        DrawText(refLabel, x + w - tw - 4, ly, 9, refC);
    }
}

/// mean of the last 'win' values
static float lastMean(const std::deque<float>& d, size_t win)
{
    if (d.empty()) return 0.0f;
    size_t n = std::min(win, d.size());
    double s = 0.0;
    for (size_t k = d.size() - n; k < d.size(); k++) s += d[k];
    return (float)(s / n);
}

void Renderer::drawMissionCharts(SeaEnvironment& env, const PanelInfo& info, int x, int yTop, int w, int yBottom)
{
    const Color SWARM_C = Color{ 255, 215, 0, 255 };   // swarm: yellow
    const Color LAW_C = Color{ 255, 255, 255, 255 };   // lawnmower: white (not confused with the grey of the controls)
    bool hasLaw = info.lawn != nullptr && info.lawn->valid;

    // two stacked charts using the full panel width; height depends on the available space
    const int HEADER = 20, TITLE = 13, FOOTER = 15, GAP = 6;
    int ch = (yBottom - yTop - HEADER - 2 * (TITLE + FOOTER) - GAP) / 2;
    if (ch < 24) return; // does not fit
    ch = std::min(ch, 110);
    int y = yTop;

    // ---- title and legend ----
    DrawLine(x, y, x + w, y, Color{ 50, 65, 90, 255 });
    DrawText("SWARM vs LAWNMOWER", x, y + 5, 10, TEXT_MAIN);
    int lx = x + w - 168;
    DrawRectangle(lx, y + 9, 14, 3, SWARM_C);
    DrawText("swarm AI", lx + 18, y + 5, 10, SWARM_C);
    for (int k = 0; k < 14; k += 6) DrawRectangle(lx + 96 + k, y + 9, 4, 3, LAW_C);
    DrawText("lawnmower", lx + 114, y + 5, 10, LAW_C);
    DrawText("(band: middle 50%)", lx + 66, y + 16, 8, Fade(LAW_C, 0.7f));
    y += HEADER;

    // ---- chart 1: redundancy % per episode ----
    float swarmRed = lastMean(env.redundancyHistory, 20);
    DrawText("REDUNDANCY % per episode", x, y, 10, TEXT_MAIN);
    const char* now = TextFormat("now %.0f%%", env.redundancyPct());
    DrawText(now, x + w - MeasureText(now, 10), y, 10, SWARM_C);
    drawHistoryChart(env.redundancyHistory, x, y + TITLE, w, ch, 60.0f, hasLaw, hasLaw ? info.lawn->redundancyPct : 0.0f,
                     hasLaw ? info.lawn->redundancyP25 : 0.0f, hasLaw ? info.lawn->redundancyP75 : 0.0f,
                     SWARM_C, LAW_C, "%.0f%%", hasLaw ? TextFormat("lawnmower %.0f%%", info.lawn->redundancyPct) : "");
    y += TITLE + ch + 2;
    DrawText(TextFormat("swarm (mean 20) %.0f%%", swarmRed), x, y, 10, SWARM_C);
    if (hasLaw)
    {
        const char* lt = TextFormat("lawnmower %.0f%%", info.lawn->redundancyPct);
        DrawText(lt, x + w - MeasureText(lt, 10), y, 10, LAW_C);
    }
    y += FOOTER + GAP;

    // ---- chart 2: time to contact per episode ----
    float swarmT = lastMean(env.contactTimeHistory, 20);
    DrawText("TIME TO CONTACT per episode", x, y, 10, TEXT_MAIN);
    if (hasLaw && !env.contactTimeHistory.empty())
    {
        float diff = 100.0f * (swarmT - info.lawn->meanContact) / info.lawn->meanContact; // <0: swarm is faster
        const char* dt = TextFormat("%+.0f%% vs lawnmower", diff);
        DrawText(dt, x + w - MeasureText(dt, 10), y, 10, diff <= 0.0f ? LINK_OK : LINK_LOST);
    }
    drawHistoryChart(env.contactTimeHistory, x, y + TITLE, w, ch, (float)MAX_STEPS, hasLaw, hasLaw ? info.lawn->meanContact : 0.0f,
                     hasLaw ? info.lawn->contactP25 : 0.0f, hasLaw ? info.lawn->contactP75 : 0.0f,
                     SWARM_C, LAW_C, "%.0fs", hasLaw ? TextFormat("lawnmower %.0fs", info.lawn->meanContact) : "");
    y += TITLE + ch + 2;
    DrawText(TextFormat("swarm (mean 20) %.0fs", swarmT), x, y, 10, SWARM_C);
    if (hasLaw)
    {
        const char* lt = TextFormat("lawnmower %.0fs", info.lawn->meanContact);
        DrawText(lt, x + w - MeasureText(lt, 10), y, 10, LAW_C);
    }
}
