// ============================================================================
//  SARSwarm - Search and Rescue with a swarm of 4 drones (multi-head DQN)
//  raylib (rendering) + Metis-Core (multi-agent reinforcement learning)
//
//  One network head = one drone. The drones take off from a mothership and search
//  for a drifting castaway. Over the radio (mesh network) they share the areas already
//  scanned (T mark), which slowly "cool down" until they become areas to search again.
// ============================================================================
#include "raylib.h"
#include "SeaEnvironment.h"
#include "Drone.h"
#include "Castaway.h"
#include "Renderer.h"
#include "LawnmowerBaseline.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

static const char* MODEL_FILE = "sarSwarm.ai";          // best model during training
static const char* MODEL_FILE_LAST = "sarSwarm_last.ai"; // last model when training is stopped
static const char* LOG_FILE = "trainingLogSAR.txt";
static const size_t HISTORY_LEN = 240;

enum class AppMode { Menu, Training, Play };

struct App
{
    // simulation
    Drone* drones[NUM_DRONES] = { nullptr };
    Castaway* castaway = nullptr;
    SeaEnvironment* env = nullptr;
    Metis::MultiHeadAgent* brain = nullptr;
    bool gpu = false;

    // user interface
    Renderer renderer;
    RenderOptions view;
    AppMode mode = AppMode::Menu;
    std::vector<float> rewardHistory[NUM_DRONES];
    std::vector<float> teamHistory;            // team reward (sum of the 4 heads)
    std::string status;
    bool modelLoaded = false;
    bool quit = false;
    LawnmowerStats lawn;          // classic sweep baseline

    // training
    bool stopRequested = false;
    int  renderEvery = 25;        // while training, draw every N steps
    int  trainEpisode = 0;
    int  bestSuccesses = -1;
    float bestMeanReward = -1e9f;
    int  lastLoggedEpisode = -1;
    double rewardSum[NUM_DRONES] = { 0 };
    long rewardCount = 0;
    FILE* log = nullptr;

    // play
    int  ticksPerFrame = 2;
    bool paused = false;
    bool finished = false;
    bool useHeuristic = false;
    std::string resultText;
    Color resultColor = WHITE;
};

static App* g_app = nullptr;

// ---------------------------------------------------------------------------
static void pushHistory(int i, float r)
{
    std::vector<float>& h = g_app->rewardHistory[i];
    h.push_back(r);
    if (h.size() > HISTORY_LEN)
        h.erase(h.begin());
}

static void pushTeamHistory(float r)
{
    std::vector<float>& h = g_app->teamHistory;
    h.push_back(r);
    if (h.size() > HISTORY_LEN)
        h.erase(h.begin());
}

static void clearHistory()
{
    for (int i = 0; i < NUM_DRONES; i++)
        g_app->rewardHistory[i].clear();
    g_app->teamHistory.clear();
}

static PanelInfo makePanelInfo()
{
    App& a = *g_app;
    PanelInfo p;
    p.gpu = a.gpu;
    p.rewardHistory = a.rewardHistory;
    p.teamHistory = &a.teamHistory;
    p.lawn = &a.lawn;
    p.status = a.status;
    p.successes = a.env->successesLastN();
    p.outcomesCount = (int)a.env->outcomes.size();
    switch (a.mode)
    {
    case AppMode::Training:
        p.mode = "TRAINING";
        p.policy = "DQN multi-head (epsilon-greedy)";
        p.episode = a.trainEpisode;
        p.ticksPerFrame = 0;
        p.renderEvery = a.renderEvery;
        break;
    case AppMode::Play:
        p.mode = "PLAY";
        p.policy = a.useHeuristic ? "Greedy heuristic (baseline)"
                                  : (a.modelLoaded ? "AI (model loaded)" : "UNTRAINED AI");
        p.episode = a.env->episode;
        p.ticksPerFrame = a.ticksPerFrame;
        p.paused = a.paused;
        p.useHeuristic = a.useHeuristic;
        break;
    default:
        p.mode = "MENU";
        p.policy = a.modelLoaded ? "AI (model loaded)" : "-";
        p.episode = a.env->episode;
        p.ticksPerFrame = 0;
        break;
    }
    return p;
}

static void handleViewKeys()
{
    App& a = *g_app;
    if (IsKeyPressed(KEY_ZERO)) a.view.viewDrone = -1;
    for (int i = 0; i < NUM_DRONES; i++)
        if (IsKeyPressed((KeyboardKey)(KEY_ONE + i))) a.view.viewDrone = i;
    if (IsKeyPressed(KEY_C)) a.view.showCommRange = !a.view.showCommRange;
    if (IsKeyPressed(KEY_N)) a.view.showCastaway = !a.view.showCastaway;
}

static void drawFrame(const char* subtitle)
{
    App& a = *g_app;
    BeginDrawing();
    ClearBackground(Color{ 8, 12, 22, 255 });
    a.renderer.drawHeader(subtitle);
    a.renderer.drawWorld(*a.env, a.view);
    a.renderer.drawLogo();   // "Powered by METIS-Core" watermark (training and play only)
    a.renderer.drawPanel(*a.env, makePanelInfo(), a.view);
    if (a.mode == AppMode::Play && a.finished)
        a.renderer.drawBanner(a.resultText.c_str(), a.resultColor);
    EndDrawing();
}

// ============================================================================
//  TRAINING (Metis-Core callbacks)
// ============================================================================
static void saveModel(const char* file)
{
    g_app->brain->saveIAModel((char*)file);
}

static void onStepTraining(void* pSender, Metis::TMULTIHEADAGENTMETRICS* pMetrics)
{
    App& a = *g_app;
    a.trainEpisode = pMetrics->episode;

    float teamR = 0.0f;
    for (int i = 0; i < NUM_DRONES; i++)
    {
        float r = pMetrics->_headMetric[a.drones[i]->name].reward;
        a.rewardSum[i] += r;
        teamR += r;
        if ((pMetrics->step % 5) == 0)
            pushHistory(i, r);
    }
    if ((pMetrics->step % 5) == 0)
        pushTeamHistory(teamR);
    a.rewardCount++;

    // draw every N steps, but never less than ~4 fps: in turbo mode 500 steps can take
    // several seconds and the window would stop processing keys (V, ESC) and look frozen
    static double lastDrawTime = 0.0;
    const double now = GetTime();
    if ((pMetrics->step % a.renderEvery) == 0 || (now - lastDrawTime) > 1.0)
    {
        lastDrawTime = now;
        drawFrame("Training: Metis-Core MultiHeadsAgentTrainerDQN  |  4 heads = 4 drones");
        handleViewKeys();
        if (IsKeyPressed(KEY_V))
        {
            // cycle: every 25 -> turbo 500 -> every step -> every 10 -> every 25 ...
            switch (a.renderEvery)
            {
            case 25:  a.renderEvery = 500; break;
            case 500: a.renderEvery = 1;   break;
            case 1:   a.renderEvery = 10;  break;
            default:  a.renderEvery = 25;  break;
            }
        }
        if (IsKeyPressed(KEY_ESCAPE))
            a.stopRequested = true;
        if (WindowShouldClose())
        {
            a.stopRequested = true;
            a.quit = true;
        }
    }
    pMetrics->bForceStopTraining = a.stopRequested;
}

static void onEndEpisode(void* pSender, Metis::TMULTIHEADAGENTMETRICS* pMetrics)
{
    App& a = *g_app;
    a.trainEpisode = pMetrics->episode;
    if ((pMetrics->episode % METRICS_X_EPISODES) != 0 || pMetrics->episode == a.lastLoggedEpisode)
        return;
    a.lastLoggedEpisode = pMetrics->episode;

    int succ = a.env->successesLastN();
    float meanR = 0.0f;
    float meanHead[NUM_DRONES];
    for (int i = 0; i < NUM_DRONES; i++)
    {
        meanHead[i] = a.rewardCount > 0 ? (float)(a.rewardSum[i] / a.rewardCount) : 0.0f;
        meanR += meanHead[i] / NUM_DRONES;
        a.rewardSum[i] = 0.0;
    }
    a.rewardCount = 0;

    if (a.log == nullptr)
        a.log = fopen(LOG_FILE, "w+t");
    if (a.log != nullptr)
    {
        fprintf(a.log, "\n==================================================\n");
        fprintf(a.log, "Episode:%d  successes(last %d):%d  countSuccesX50(Metis):%d  meanReward:%.3f  totalLoss:%.4f\n",
                pMetrics->episode, (int)a.env->outcomes.size(), succ, pMetrics->countSuccesX50episodes, meanR, pMetrics->_meanTotalLoss);
        for (int i = 0; i < NUM_DRONES; i++)
        {
            Metis::THEADMETRIC& hm = pMetrics->_headMetric[a.drones[i]->name];
            fprintf(a.log, "   %s  meanReward:%.3f  loss:%.4f  grad:%.4f\n", a.drones[i]->name.c_str(), meanHead[i], hm._meanLossHead, hm._meanGradient);
        }
    }

    // save if the number of successes improves, or with equal successes if the mean reward improves
    bool better = (succ > a.bestSuccesses) || (succ == a.bestSuccesses && meanR > a.bestMeanReward);
    if (better)
    {
        a.bestSuccesses = succ;
        a.bestMeanReward = meanR;
        saveModel(MODEL_FILE);
        a.modelLoaded = true;
        a.status = TextFormat("Model saved (ep %d, %d successes/50)", pMetrics->episode, succ);
        if (a.log != nullptr)
            fprintf(a.log, "   >> Saved %s\n", MODEL_FILE);
    }
    if (a.log != nullptr)
        fflush(a.log);
}


static void startTraining(bool bContinue)
{

    App& a = *g_app;

    // continue: load previous weights (last saved; if it does not exist, the best one)
    if (bContinue)
    {
        const char* file = FileExists(MODEL_FILE_LAST) ? MODEL_FILE_LAST
            : (FileExists(MODEL_FILE) ? MODEL_FILE : nullptr);
        if (file == nullptr)
        {
            a.status = "No saved model to continue from";
            return;
        }
        int res = a.brain->loadIAModel((char*)file, a.gpu);
        a.modelLoaded = true;
        a.status = TextFormat("Continuing from %s (res=%d)", file, res);
    }
    else
    {
        a.status = "Training from scratch...";
    }

    a.mode = AppMode::Training;
    a.stopRequested = false;
    clearHistory();

    Metis::MultiHeadsAgentTrainerDQN trainer;
    trainer.setCallbackPerStep(onStepTraining);
    trainer.setCallbackEndEpisode(onEndEpisode);
    trainer.training(a.env, a.brain, a.castaway);

    saveModel(MODEL_FILE_LAST);
    a.status = TextFormat("Training stopped. Last model: %s", MODEL_FILE_LAST);
    a.mode = AppMode::Menu;
    a.env->reset();
}

// ============================================================================
//  PLAY: inference with the network (or the heuristic) while the user moves the castaway
// ============================================================================
static bool loadModel()
{
    App& a = *g_app;
    if (!FileExists(MODEL_FILE))
    {
        a.status = TextFormat("%s not found: train first", MODEL_FILE);
        return false;
    }
    int res = a.brain->loadIAModel((char*)MODEL_FILE, a.gpu);
    a.modelLoaded = true;
    a.status = TextFormat("Model %s loaded (res=%d)", MODEL_FILE, res);
    return true;
}

static void startPlay()
{
    App& a = *g_app;
    if (!a.modelLoaded)
        loadModel();
    a.useHeuristic = !a.modelLoaded; // without a model, start with the heuristic so something useful is shown
    a.mode = AppMode::Play;
    a.paused = false;
    a.finished = false;
    clearHistory();
    a.env->reset();
}

/// one simulation step in Play. Returns true if the episode ends.
static bool playTick()
{
    App& a = *g_app;
    SeaEnvironment& env = *a.env;

    Metis::State st;
    env.getState(&st);
    env.serizalizeState(&st, &st._stateVector);

    for (int i = 0; i < NUM_DRONES; i++)
    {
        Drone* d = a.drones[i];
        int action = a.useHeuristic ? d->getActionProcedural(st) : a.brain->getBestAction(d, st);
        env.applyAction(d, action);
    }
    env.updatePhysics(env.getDeltaTime());

    Metis::State next;
    env.getState(&next); // triggers the world tick (scan, comms, detection)
    int done = 0;
    std::vector<Metis::TMULTIHEAD> r = env.calculateRewards(next, &done);
    float teamR = 0.0f;
    for (int i = 0; i < NUM_DRONES; i++)
    {
        pushHistory(i, r[i].reward);
        teamR += r[i].reward;
        a.drones[i]->endStep();
    }
    pushTeamHistory(teamR);

    if (done != 0)
    {
        int t = env.currentStep();
        if (done > 0)
        {
            a.resultText = TextFormat("CASTAWAY FOUND by %s at %02d:%02d", a.drones[env.detectedBy]->name.c_str(), t / 60, t % 60);
            a.resultColor = Color{ 40, 230, 120, 255 };
        }
        else
        {
            a.resultText = "The castaway did not survive (30:00)";
            a.resultColor = Color{ 255, 60, 60, 255 };
        }
        a.finished = true;
        return true;
    }
    return false;
}

static void updatePlay()
{
    App& a = *g_app;
    handleViewKeys();
    if (IsKeyPressed(KEY_ESCAPE)) { a.mode = AppMode::Menu; a.env->reset(); return; }
    if (IsKeyPressed(KEY_SPACE))  a.paused = !a.paused;
    if (IsKeyPressed(KEY_R))      { a.finished = false; a.paused = false; clearHistory(); a.env->reset(); }
    if (IsKeyPressed(KEY_H))      a.useHeuristic = !a.useHeuristic;
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))      a.ticksPerFrame = std::min(30, a.ticksPerFrame + 1);
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) a.ticksPerFrame = std::max(1, a.ticksPerFrame - 1);

    // the user pushes the castaway (simulates waves/motion)
    const float PUSH = 0.35f; // m/s per frame
    float px = 0.0f, py = 0.0f;
    if (IsKeyDown(KEY_LEFT))  px -= PUSH;
    if (IsKeyDown(KEY_RIGHT)) px += PUSH;
    if (IsKeyDown(KEY_UP))    py -= PUSH;
    if (IsKeyDown(KEY_DOWN))  py += PUSH;
    if (px != 0.0f || py != 0.0f)
        a.castaway->push(px, py);

    if (!a.paused && !a.finished)
        for (int k = 0; k < a.ticksPerFrame; k++)
            if (playTick())
                break;

    drawFrame("PLAY: the network decides the heading of each drone; you move the castaway with the arrow keys");
}

// ============================================================================
//  MENU
// ============================================================================
static bool button(Rectangle r, const char* text, const char* hint)
{
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, r);
    DrawRectangleRec(r, hover ? Color{ 40, 70, 110, 255 } : Color{ 24, 40, 64, 255 });
    DrawRectangleLinesEx(r, 1, Color{ 80, 200, 255, 255 });
    DrawText(text, (int)r.x + 16, (int)r.y + 10, 20, Color{ 225, 232, 240, 255 });
    DrawText(hint, (int)r.x + 16, (int)r.y + 36, 11, Color{ 140, 155, 175, 255 });
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void updateMenu()
{
    App& a = *g_app;
    int action = 0; // in the menu keys 1-4 are options, not map views
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) action = 1;
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) action = 2;
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) action = 3;
    if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) action = 4;
    if (IsKeyPressed(KEY_ESCAPE)) a.quit = true;

    BeginDrawing();
    ClearBackground(Color{ 8, 12, 22, 255 });
    a.renderer.drawHeader("SAR swarm: 4 UAVs launched from a mothership  |  multi-head DQN, one head per drone");
    a.renderer.drawWorld(*a.env, a.view);
    a.renderer.drawPanel(*a.env, makePanelInfo(), a.view);
    DrawRectangle(MAP_X, MAP_Y, MAP_W, MAP_H, Fade(BLACK, 0.55f));

    int bx = MAP_X + MAP_W / 2 - 200, by = MAP_Y + 170;
    DrawText("Search & Rescue Swarm", bx, by - 90, 34, Color{ 225, 232, 240, 255 });
    DrawText(TextFormat("Area %.0fx%.0f km | sensor %.0f m | radio %.1f km | survival %d min",
                        WORLD_W / 1000.0f, WORLD_H / 1000.0f, SENSOR_RADIUS, COMM_RANGE / 1000.0f, MAX_STEPS / 60),
             bx, by - 46, 13, Color{ 140, 155, 175, 255 });
    if (button(Rectangle{ (float)bx, (float)by, 400, 56 }, "[1] TRAIN", "Metis-Core MultiHeadsAgentTrainerDQN (ESC to stop)")) action = 1;
    if (button(Rectangle{ (float)bx, (float)by + 70, 400, 56 }, "[2] PLAY", "Loads sarSwarm.ai; you move the castaway with the arrow keys")) action = 2;
    if (button(Rectangle{ (float)bx, (float)by + 140, 400, 56 }, "[3] LOAD MODEL", "Reads sarSwarm.ai from the working directory")) action = 3;
    if (button(Rectangle{ (float)bx, (float)by + 210, 400, 56 }, "[4] CONTINUE TRAINING",
        "Loads sarSwarm_last.ai and keeps training")) action = 4;
    DrawText("[ESC] quit", bx, by + 284, 12, Color{ 140, 155, 175, 255 });
    EndDrawing();

    if (action == 1) startTraining(false);
    else if (action == 4) startTraining(true);
    else if (action == 2) startPlay();
    else if (action == 3) loadModel();
}

// ============================================================================
//  LAWNMOWER BASELINE (classic search in parallel strips)
// ============================================================================
static const char* LAWN_FILE = "lawnmower_baseline.txt";
static const int   LAWN_EPISODES = 100;
static const unsigned LAWN_SEED = 1234;

/// signature of the parameters that affect the result: if they change, it is recomputed
static std::string lawnSignature()
{
    return TextFormat("v2 W%.0f H%.0f S%d V%.1f R%.0f P%.2f C%.0f D%.0f CS%.2f-%.2f CM%.2f T%.3f N%d",
                      WORLD_W, WORLD_H, MAX_STEPS, DRONE_SPEED, SENSOR_RADIUS, P_DETECT, COMM_RANGE,
                      HEAT_DECAY_STEPS, CURRENT_MIN_SPEED, CURRENT_MAX_SPEED, CASTAWAY_MAX_SPEED,
                      CASTAWAY_OU_THETA, LAWN_EPISODES);
}

static LawnmowerStats loadOrComputeLawnmower()
{
    LawnmowerStats st;
    std::string sig = lawnSignature();

    // 1) try to read the cache
    FILE* f = fopen(LAWN_FILE, "rt");
    if (f != nullptr)
    {
        char line[512] = { 0 };
        if (fgets(line, sizeof(line), f) != nullptr)
        {
            std::string fileSig(line);
            while (!fileSig.empty() && (fileSig.back() == '\n' || fileSig.back() == '\r')) fileSig.pop_back();
            if (fileSig == sig &&
                fscanf(f, "%d %f %f %f %f %f %f %f %f", &st.episodes, &st.successRate, &st.meanContact, &st.medianContact, &st.redundancyPct,
                       &st.contactP25, &st.contactP75, &st.redundancyP25, &st.redundancyP75) == 9)
                st.valid = true;
        }
        fclose(f);
        if (st.valid) return st;
    }

    // 2) compute with a progress bar (may be slow in Debug; it only happens once)
    st = LawnmowerBaseline::compute(LAWN_EPISODES, LAWN_SEED, [](int done, int total)
    {
        BeginDrawing();
        ClearBackground(Color{ 8, 12, 22, 255 });
        g_app->renderer.drawHeader("Preparing the baseline: classic lawnmower search (only once)");
        int bw = 500, bx = SCREEN_W / 2 - bw / 2, by = SCREEN_H / 2;
        DrawText(TextFormat("Simulating lawnmower: episode %d / %d", done, total), bx, by - 30, 20, Color{ 225, 232, 240, 255 });
        DrawRectangle(bx, by, bw, 14, Color{ 30, 40, 60, 255 });
        DrawRectangle(bx, by, bw * done / total, 14, Color{ 255, 215, 0, 255 });
        EndDrawing();
    });

    // 3) save the cache
    f = fopen(LAWN_FILE, "wt");
    if (f != nullptr)
    {
        fprintf(f, "%s\n%d %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n", sig.c_str(), st.episodes, st.successRate, st.meanContact, st.medianContact,
                st.redundancyPct, st.contactP25, st.contactP75, st.redundancyP25, st.redundancyP75);
        fprintf(f, "# episodes success%% t_mean(s) t_median(s) redundancy%% t_p25 t_p75 red_p25 red_p75\n");
        fclose(f);
    }
    return st;
}

// ============================================================================
int main()
{
    App app;
    g_app = &app;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "METIS-Core SAR Swarm PoC (raylib)");
    SetExitKey(KEY_NULL); // we handle ESC ourselves
    SetTargetFPS(60);
    app.renderer.init();

    // --- agents: one drone per head (the ID defines the role and the input block) ---
    const char* names[NUM_DRONES] = { "UAV_1", "UAV_2", "UAV_3", "UAV_4" };
    for (int i = 0; i < NUM_DRONES; i++)
        app.drones[i] = new Drone(i, names[i]);
    app.castaway = new Castaway();

    // --- multi-head network: 4 heads x 8 actions (headings) ---
    app.gpu = Metis::isCUDAavailable();
    Metis::MultiHeadBrainBuilder builder;
    for (int i = 0; i < NUM_DRONES; i++)
    {
        // the number of actions is the maximum among all the agents; an action that is not valid for a drone
        // (because it has fewer actions) is filtered out with IAgent::isValidAction(...)
        builder.addHead(app.drones[i]->name, app.drones[i], NUM_DRONE_ACTIONS);
    }
        
    app.brain = builder.setNumberInputsOutputs(NET_INPUTS)
                       .useGPU(app.gpu)
                       .build();

    // --- environment ---
    app.env = new SeaEnvironment();
    app.env->bindAgents(app.drones, app.castaway);
    app.env->reset();

    // --- lawnmower baseline: simulated once and saved to disk ---
    app.lawn = loadOrComputeLawnmower();

    while (!app.quit && !WindowShouldClose())
    {
        switch (app.mode)
        {
        case AppMode::Menu: updateMenu(); break;
        case AppMode::Play: updatePlay(); break;
        default: app.mode = AppMode::Menu; break;
        }
    }

    if (app.log != nullptr) fclose(app.log);
    app.renderer.shutdown();
    CloseWindow();

    delete app.env;
    delete app.castaway;
    for (int i = 0; i < NUM_DRONES; i++) delete app.drones[i];
    // app.brain is managed by Metis-Core (as in SwarmDefenseTIE, it is not released explicitly)
    return 0;
}
