#pragma once
#include "raylib.h"
#include "SimConfig.h"
#include "LawnmowerBaseline.h"
#include <deque>
#include <vector>
#include <string>

class SeaEnvironment;

// ---- window layout ----
constexpr int   SCREEN_W = 1420;
constexpr int   SCREEN_H = 830;
constexpr float MAP_SCALE = 0.25f;                 // 1 px = 4 m
constexpr int   MAP_X = 12;
constexpr int   MAP_Y = 64;
constexpr int   MAP_W = (int)(WORLD_W * MAP_SCALE); // 1000 px
constexpr int   MAP_H = (int)(WORLD_H * MAP_SCALE); // 750 px
constexpr int   PANEL_X = MAP_X + MAP_W + 14;

struct RenderOptions
{
    int  viewDrone = -1;        // -1: union map of all drones; 0..3: what that drone "knows"
    bool showCommRange = false; // radio range circles
    bool showCastaway = true;   // castaway ground truth
};

/// data for the side panel
struct PanelInfo
{
    std::string mode;           // "TRAINING", "PLAY", "MENU"
    std::string policy;         // "AI", "Heuristic", ...
    std::string status;
    int  episode = 0;
    int  successes = 0;         // successes in the last 50 episodes
    int  outcomesCount = 0;
    int  ticksPerFrame = 1;
    bool paused = false;
    bool gpu = false;
    const std::vector<float>* rewardHistory = nullptr; // array of NUM_DRONES histories
    const std::vector<float>* teamHistory = nullptr;   // team reward (sum of the heads)
    // state of the controls to highlight the active option
    int  renderEvery = 0;       // >0 only while training (1, 10, 25 or 500)
    bool useHeuristic = false;  // Play: active policy
    const LawnmowerStats* lawn = nullptr; // classic sweep baseline (for comparison)
};

extern const Color DRONE_COLORS[NUM_DRONES];

class Renderer
{
public:
    void init();
    void shutdown();
    void drawLogo();            // "Powered by METIS-Core" watermark (training and play windows)

    void drawHeader(const char* subtitle);
    void drawWorld(SeaEnvironment& env, const RenderOptions& opt);
    void drawPanel(SeaEnvironment& env, const PanelInfo& info, const RenderOptions& opt);
    void drawBanner(const char* text, Color color);

    static Vector2 toScreen(float wx, float wy) { return Vector2{ MAP_X + wx * MAP_SCALE, MAP_Y + wy * MAP_SCALE }; }

private:
    void drawHeatMap(SeaEnvironment& env, const RenderOptions& opt);
    void drawCurrentFlow(SeaEnvironment& env);   // animated flow arrows over the sea
    void drawCurrentCompass(SeaEnvironment& env);// icon: compass rose with arrow, speed and bearing
    void drawShip();
    void drawComms(SeaEnvironment& env, const RenderOptions& opt);
    void drawDrones(SeaEnvironment& env);
    void drawCastaway(SeaEnvironment& env, const RenderOptions& opt);
    void drawScaleAndLegend();
    void drawControls(const PanelInfo& info, const RenderOptions& opt, int x, int w);
    static int controlsTop(const PanelInfo& info);
    void drawMissionCharts(SeaEnvironment& env, const PanelInfo& info, int x, int yTop, int w, int yBottom);
    void drawHistoryChart(const std::deque<float>& data, int x, int y, int w, int h, float yMax,
                          bool hasRef, float refValue, float refLo, float refHi,
                          Color c, Color refC, const char* fmt, const char* refLabel);
    void drawSparkline(const std::vector<float>& h, int x, int y, int w, int hgt, Color c, float scale = 1.0f);

    // current flow particles (position in screen px)
    struct FlowParticle { float x, y, life, maxLife; };
    std::vector<FlowParticle> _flow;
    void respawnParticle(FlowParticle& p, bool randomAge);

    Texture2D _heatTex = { 0 };
    Texture2D _logoTex = { 0 };
    bool _logoLoaded = false;
    std::vector<Color> _pixels;
};
