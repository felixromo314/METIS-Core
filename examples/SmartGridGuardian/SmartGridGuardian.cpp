// Visualization + training + play app (human = Red).
#include "Visualizer\Visualizer.h"
#include "Kernel/RenderModel.h"
#include "raylib.h"

#include "Kernel/GridEnvironment.h"
#include "Kernel/Actions.h"
#include "METIS-Core.h"

#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <cstdio>

using namespace smartgrid;

// --- global state used by the callbacks (free functions) ---
static Visualizer g_viz;
static int   g_blueWins = 0, g_redWins = 0, g_draws = 0;
static int   g_frameSkip = 0;          // draw 1 out of N steps so training is not slowed down

// Render frequency during training (cycle with the 'V' key).
static const int   g_renderFreqs[]   = { 1, 10, 50 };                                  // every how many steps
static const char* g_renderNombres[] = { "every step", "every 10 steps", "every 50 steps" };
static int    g_renderModo     = 1;        // starts at "every 10 steps"
static long   g_stepCounter    = 0;        // steps vistos
static double g_lastRenderTime = 0.0;      // last time something was drawn

static int g_blue = 0, g_red = 0, g_draw = 0;
static int g_episodeCount = 0;
static Metis::MultiHeadAgent* g_teamBlue = nullptr;   // reference to the Blue team (to save the best model)
static int g_bestBlueWins50 = -1;                     // best number of Blue wins in a 50-episode window
static std::vector<int> g_ventanaBlue;                // latest Blue results (1=win, 0=no)

std::ofstream f("training_stats.txt", std::ios::out | std::ios::trunc);


// Called on EVERY agent decision during self-play.
static void onStepTraining(void* /*pSender*/, Metis::TSEARCHACTIONASYM* m)
{
    // --- hacker action LOG (measures Red coordination) -> attack.txt ---
    // IMPORTANT: it goes here (callback of the REAL move), never in getMaterialHeuristicValue/applyAction,
    // which run on MCTS clones (hypothetical states) and would pollute the measurement.
    {
        GridEnvironment* envLog = static_cast<GridEnvironment*>(m->pState);
        const int pid = m->playerID;
        const int acc = (pid >= 0 && pid < MAX_PLAYERS) ? envLog->_ultimaAccion[pid] : 0;
        if ((pid == PID_STRIKER || pid == PID_SPOOFER) && acc != 0)   // 0 = NoOp -> not logged
        {
            // coordination state: masked attacks (botnet+frozen meter, open line+frozen flow)
            int maskedBus = 0;
            for (int i = 0; i < kNumBuses; ++i)
                if (envLog->_botnetMW[i] > 1.0f && envLog->_contadorCongelado[i]) ++maskedBus;
            int maskedLine = 0;
            for (int l = 0; l < (int)envLog->_lineaCerrada.size(); ++l)
                if (!envLog->_lineaCerrada[l] && envLog->_flujoCongelado[l]) ++maskedLine;

            const char* quien = (pid == PID_STRIKER) ? "STRIKER" : "SPOOFER";
            const char* nomb  = (pid == PID_STRIKER) ? nombreStriker(acc) : nombreSpoofer(acc);

            std::ofstream fa("attack.txt", std::ios::app);
            if (fa.is_open())
            {
                char line[220];
                std::snprintf(line, sizeof(line),
                    "ep=%d step=%d %-7s %-14s budget=%d maskedBus=%d maskedLine=%d\n",
                    g_episodeCount, envLog->_step, quien, nomb,
                    (pid == PID_STRIKER ? envLog->_presupuestoStriker : envLog->_presupuestoSpoofer),
                    maskedBus, maskedLine);
                fa << line;
            }

        }
    }

    // 'V' cycles the frequency: every step -> every 10 -> every 50 -> ...
    if (IsKeyPressed(KEY_V))
        g_renderModo = (g_renderModo + 1) % 3;

    ++g_stepCounter;
    const int    freq         = g_renderFreqs[g_renderModo];
    const double now          = GetTime();
    const bool   porFrecuencia = (g_stepCounter % freq) == 0;
    const bool   porTiempo     = (now - g_lastRenderTime) > 2.0;   // never more than 2s without drawing
    if (!porFrecuencia && !porTiempo)
        return;                                                     // skip this step (not rendered)

    g_lastRenderTime = now;

    GridEnvironment* env = static_cast<GridEnvironment*>(m->pState);
    GridRenderState s;
    env->fillRenderState(s, /*mostrarVerdad=*/true);
    s.currentEpisode = g_episodeCount;

    if (m->playerID >= 0)
        s.currentPlayer = m->playerID;   // highlight the agent that just acted

    // small menu: current frequency + how to change it (drawn in the title, top left)
    s.titulo = std::string("SmartGridGuardian - training...   [V] render: ") + g_renderNombres[g_renderModo];
    g_viz.dibujar(s);
}

// Writes a team statistics block (swarm-like format).
static void escribirBloqueEquipo(std::ofstream& f, const char* nombreEquipo,
                                 const Metis::TASYMMULTIHEADAGENTMETRICS& mt, const char* modelo)
{
    // team meanReward = mean of its heads' rewards
    float meanPolicyHeadLossTotal = 0.0f;
    if (!mt._headMetric.empty())
    {
        for (const std::pair<const std::string, Metis::ASYMTHEADMETRIC>& kv : mt._headMetric)
            meanPolicyHeadLossTotal += kv.second._meanPolicyHeadLoss;
        meanPolicyHeadLossTotal /= (float)mt._headMetric.size();
    }

    char line[320];
    f << "==================================================\n";
    std::snprintf(line, sizeof(line),"[%s]  meanPolicyHeadLossTotal:%.3f  _meanValueHeadLoss:%.4f\n",
                                    nombreEquipo,meanPolicyHeadLossTotal, mt._meanValueHeadLoss);

    f << line;

    // one line per head (substation or hacker)
    for (const std::pair<const std::string, Metis::ASYMTHEADMETRIC>& kv : mt._headMetric)
    {
        std::string sHeadName = kv.first;
        const Metis::ASYMTHEADMETRIC& h = kv.second;
        std::snprintf(line, sizeof(line),"   %s  _meanPolicyHeadLoss:%.4f\n",sHeadName.c_str(), h._meanPolicyHeadLoss);
        f << line;
    }
}

static void onEndEpisode(void* /*pSender*/, Metis::TSEARCHACTIONASYM* m)
{
    GridEnvironment* env = static_cast<GridEnvironment*>(m->pState);
    switch (env->getWinner())
    {
    case TEAM_BLUE_ID: ++g_blue; break;
    case TEAM_RED_ID:  ++g_red;  break;
    default:           ++g_draw; break;
    }

    std::ofstream fa("attack.txt", std::ios::app);
    if (fa.is_open())
    {
        const char* res = (env->getWinner() == TEAM_RED_ID) ? "RED (BLACKOUT)"
            : (env->getWinner() == TEAM_BLUE_ID) ? "BLUE" : "DRAW";
        char line[160];
        std::snprintf(line, sizeof(line), "=== END ep=%d step=%d winner=%s servedFrac=%.2f ===\n",
            g_episodeCount, env->_step, res, env->_fraccionServida);
        fa << line;
    }

    ++g_episodeCount;
    const float tot     = (float)(g_blue + g_red + g_draw);
    const float bluePct = 100.0f * g_blue / tot;
    const float redPct  = 100.0f * g_red  / tot;
    // MOVING-WINDOW win rate (last 30 episodes): this shows the arms-race cycles
    static std::vector<int> ventana;                       // 1 = Blue wins, 2 = Red wins, 0 = draw
    ventana.push_back(env->getWinner() == TEAM_BLUE_ID ? 1 : (env->getWinner() == TEAM_RED_ID ? 2 : 0));
    if (ventana.size() > 30) ventana.erase(ventana.begin());
    int wB = 0, wR = 0;
    for (int w : ventana) { if (w == 1) ++wB; else if (w == 2) ++wR; }
    const float nv = (float)ventana.size();
    g_viz.pushWinRates(100.0f * wB / nv, 100.0f * wR / nv);   // feeds the left graph
    (void)bluePct; (void)redPct;

    // --- save the BEST Blue model: the one winning the most of the last 50 episodes ---
    const int VENTANA = 50;
    g_ventanaBlue.push_back(env->getWinner() == TEAM_BLUE_ID ? 1 : 0);
    if ((int)g_ventanaBlue.size() > VENTANA) g_ventanaBlue.erase(g_ventanaBlue.begin());
    if (g_teamBlue != nullptr && (int)g_ventanaBlue.size() == VENTANA)
    {
        int blueWins50 = 0;
        for (int r : g_ventanaBlue) blueWins50 += r;        // Blue wins in the window
        if (blueWins50 > g_bestBlueWins50)                  // only if it beats the record
        {
            g_bestBlueWins50 = blueWins50;
            g_teamBlue->saveIAModel((char*)"blueTeam_best.ia");
            std::printf(">> NEW BEST Blue: %d/50 wins -> saved blueTeam_best.ia (ep %d)\n",
                blueWins50, g_episodeCount);
        }
    }

    // --- dump statistics to TXT (two blocks per episode: Blue and Red) ---
    // Saved next to the .exe (same folder as the .ia files).
    //std::ofstream f("training_stats.txt", std::ios::app);
    if (f.is_open())
    {
        const char* ganador = (env->getWinner() == TEAM_BLUE_ID) ? "BLUE"
                            : (env->getWinner() == TEAM_RED_ID)  ? "RED" : "DRAW";
        char cab[160];
        std::snprintf(cab, sizeof(cab),
            "\n### Episode %d  winner:%s  Blue:%.1f%%  Red:%.1f%%  winRateModel:%.3f\n",
            g_episodeCount, ganador, bluePct, redPct, m->winRateModel);
        f << cab;
        escribirBloqueEquipo(f, "BLUE", m->metricsTeamBlue, "blueTeam.ia");
        escribirBloqueEquipo(f, "RED",  m->metricsTeamRed,  "redTeam.ia");
    }

    // --- console summary every 10 episodes ---
    if (g_episodeCount % 10 == 0)
    {
        std::printf("[ep %d] Blue %.1f%%  Red %.1f%% | lossB %.4f  lossR %.4f | winRateModel %.3f\n",
            g_episodeCount, bluePct, redPct,
            m->metricsTeamBlue._meanTotalLoss, m->metricsTeamRed._meanTotalLoss, m->winRateModel);
    }

    f.flush();
}

// ============================================================================
// TRAIN MODE  (main opens/closes the window)
// ============================================================================
void startTraining() {
    // 1) environment (inherits Metis::IAlphaGoZeroState)
    GridEnvironment env;
    env.reset(0);

    bool gpu = Metis::isCUDAavailable();
    gpu = false;

    // 2) build the two multi-head teams
    Metis::MultiHeadBrainBuilder bBlue;
    Metis::MultiHeadAgent* teamBlue = bBlue
        .addHead("subA", &env._subestacion[0], ACTIONS_SUB_A)
        .addHead("subB", &env._subestacion[1], ACTIONS_SUB_B)
        .addHead("subC", &env._subestacion[2], ACTIONS_SUB_C)
        .addHead("subD", &env._subestacion[3], ACTIONS_SUB_D)
        .addValueHead("valueHead")
        .setNumberInputsOutputs(TEAM_BLUE_INPUTS)
        .useGPU(gpu).build();
    teamBlue->setID(TEAM_BLUE_ID);
    g_teamBlue = teamBlue;              // used by onEndEpisode to save the best model

    env._subestacion[0].setID(PID_A);
    env._subestacion[1].setID(PID_B);
    env._subestacion[2].setID(PID_C);
    env._subestacion[3].setID(PID_D);

    Metis::MultiHeadBrainBuilder bRed;
    Metis::MultiHeadAgent* teamRed = bRed
        .addHead("striker", &env._striker, ACTIONS_STRIKER)
        .addHead("spoofer", &env._spoofer, ACTIONS_SPOOFER)
        .addValueHead("valueHead")
        .setNumberInputsOutputs(TEAM_RED_INPUTS)
        .useGPU(gpu).build();
    teamRed->setID(TEAM_RED_ID);

    env._striker.setID(PID_STRIKER);
    env._spoofer.setID(PID_SPOOFER);

    // 3) resume previous training if it exists
    //teamBlue->loadIAModel((char*)"blueTeam.ia", gpu);
    //teamRed->loadIAModel((char*)"redTeam.ia", gpu);

    // 4) demand forecast for the graph (main already opened the window)
    std::vector<float> forecast;
    env.forecastDemandaDia(forecast);
    g_viz.setDailyForecast(forecast);

    // 5) BLOCKING: self-play + MCTS + training. Drawing happens inside the callback.
    Metis::AlphaGoZeroAsymTrainer trainer;
    trainer.setMaterialHeuristicWeight(0.3);   // 30% heuristic + 70% network (tune as needed; 0 = network only)
    trainer.setCallbackPerStep(nullptr, onStepTraining);
    trainer.setCallbackEndEpisode(nullptr, onEndEpisode);
    trainer.training(&env, teamBlue, teamRed, gpu);
}

// ============================================================================
// PLAY MODE  (human = Red: Striker + Spoofer; the Blue AI defends)
// ============================================================================

// Asks the human for an action for hacker 'pid'. Returns the chosen legal head-local index.
static int pedirAccionHumanaRed(GridEnvironment& env, int pid)
{
    std::vector<TGridAction> acc;
    env.getPossibleActions(pid, acc);              // legal actions only
    if (acc.empty()) return 0;                     // safety NoOp

    std::vector<std::string> nombres;              // readable names for each option
    nombres.reserve(acc.size());
    for (const TGridAction& a : acc)
        nombres.push_back(pid == PID_STRIKER ? nombreStriker(a.id) : nombreSpoofer(a.id));

    int sel = 0;
    const char* titulo = (pid == PID_STRIKER) ? "YOUR TURN - StrikerHacker" : "YOUR TURN - SpooferHacker";

    while (!g_viz.deberiaCerrar())
    {
        if (IsKeyPressed(KEY_DOWN)) sel = (sel + 1) % (int)acc.size();
        if (IsKeyPressed(KEY_UP))   sel = (sel - 1 + (int)acc.size()) % (int)acc.size();
        if (IsKeyPressed(KEY_ENTER)) break;

        GridRenderState s;
        env.fillRenderState(s, /*mostrarVerdad=*/true);   // the human Red player sees the truth
        s.currentPlayer = pid;
        s.titulo = "SmartGridGuardian - PLAYING (you: Red)";
        g_viz.setMenuAcciones(nombres, sel, titulo);
        g_viz.dibujar(s);
    }
    g_viz.limpiarMenu();
    return acc[sel].id;
}

void playGame()
{
    GridEnvironment env;
    env.reset(0);
    bool gpu = false;

    // Build the teams as in training (same heads/sizes)
    Metis::MultiHeadBrainBuilder bBlue;
    Metis::MultiHeadAgent* teamBlue = bBlue
        .addHead("subA", &env._subestacion[0], ACTIONS_SUB_A)
        .addHead("subB", &env._subestacion[1], ACTIONS_SUB_B)
        .addHead("subC", &env._subestacion[2], ACTIONS_SUB_C)
        .addHead("subD", &env._subestacion[3], ACTIONS_SUB_D)
        .addValueHead("valueHead")
        .setNumberInputsOutputs(TEAM_BLUE_INPUTS)
        .useGPU(gpu).build();
    teamBlue->setID(TEAM_BLUE_ID);
    env._subestacion[0].setID(PID_A);
    env._subestacion[1].setID(PID_B);
    env._subestacion[2].setID(PID_C);
    env._subestacion[3].setID(PID_D);

    Metis::MultiHeadBrainBuilder bRed;
    Metis::MultiHeadAgent* teamRed = bRed
        .addHead("striker", &env._striker, ACTIONS_STRIKER)
        .addHead("spoofer", &env._spoofer, ACTIONS_SPOOFER)
        .addValueHead("valueHead")
        .setNumberInputsOutputs(TEAM_RED_INPUTS)
        .useGPU(gpu).build();
    teamRed->setID(TEAM_RED_ID);
    env._striker.setID(PID_STRIKER);
    env._spoofer.setID(PID_SPOOFER);

    // Load trained models (adjust names to what your trainer saves)
    teamBlue->loadIAModel((char*)"blueTeam.ia", gpu);
    teamRed->loadIAModel((char*)"redTeam.ia", gpu);   // Blue's MCTS uses it to simulate the opponent
    teamBlue->setTeamBlueRed(teamBlue, teamRed);      // Blue needs both models for the asymmetric search

    std::vector<float> forecast;
    env.forecastDemandaDia(forecast);
    g_viz.setDailyForecast(forecast);

    while (!g_viz.deberiaCerrar())
    {
        if (env.isTerminal())
        {
            // result screen until the user presses ESC
            while (!g_viz.deberiaCerrar())
            {
                GridRenderState s;
                env.fillRenderState(s, true);
                s.titulo = (env.getWinner() == TEAM_BLUE_ID)
                    ? "END - BLUE wins (grid safe)"
                    : "END - RED wins (BLACKOUT)";
                g_viz.dibujar(s);
            }
            break;
        }

        const int pid = env.getCurrentPlayerID();
        int accion;
        if (teamDe(pid) == TEAM_RED_ID)
            accion = pedirAccionHumanaRed(env, pid);         // human (Striker/Spoofer)
        else
            accion = teamBlue->selectActionAsym(&env, pid);  // IA Blue defiende

        env.applyAction(accion);
        env.update(1.0f);                                    // physics is resolved when the round closes

        GridRenderState s;
        env.fillRenderState(s, true);
        s.currentPlayer = pid;
        s.titulo = "SmartGridGuardian - PLAYING (you: Red)";
        g_viz.dibujar(s);
    }
}

// ============================================================================
int main()
{
    g_viz.init(1000, 830, "SmartGridGuardian");
    g_viz.cargarLogo("logo_metis_watermark.png");   // after init (needs a GL context)

    const int opcion = g_viz.menuInicio();           // 0 train, 1 play, -1 quit
    if (opcion == 0) startTraining();
    else if (opcion == 1) playGame();

    g_viz.cerrar();
    return 0;
}
