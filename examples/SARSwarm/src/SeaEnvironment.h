#pragma once
#include "MetisInclude.h"
#include "SimConfig.h"
#include <random>
#include <deque>
#include <vector>

class Drone;
class Castaway;

/// <summary>
/// User state stored in the Metis-Core Replay Buffer (max 1000 bytes).
/// It stores the ALREADY computed observation, so serizalizeState() only copies from the struct
/// and never reads the "live" world (avoids mixing past states with current positions).
/// </summary>
typedef struct _sarState
{
    float obs[NUM_DRONES][OBS_PER_DRONE]; // observation of each drone (input block of each head)
    float timeNorm;                       // step / MAX_STEPS (float division!)
    float currentX, currentY;             // episode current normalized by CURRENT_MAX_SPEED (known by the drones)
    int   step;
    int   detectedBy;                     // -1 if nobody has detected the castaway
    float castawayX, castawayY;           // ground truth: debugging only, NOT a network input
} TSARSTATE;

static_assert(sizeof(TSARSTATE) < MAX_SIZE_STATE, "TSARSTATE does not fit in Metis::State (1000 bytes)");

// indices inside the observation block of each drone
constexpr int OBS_POS = 0;          // normalized x, y
constexpr int OBS_HEADING = 2;      // cos, sin
constexpr int OBS_SECTORS = 4;      // 8 sectors ring 1 + 8 sectors ring 2
constexpr int OBS_OTHERS = OBS_SECTORS + NUM_SECTORS * 2;  // 3 x (dx, dy, cos heading, sin heading, connected)
constexpr int OBS_ISOLATED = OBS_OTHERS + (NUM_DRONES - 1) * OBS_PER_MATE;
constexpr int OBS_DISC_TIME = OBS_ISOLATED + 1;   // normalized disconnected time (the penalty depends on it: Markov state)
static_assert(OBS_DISC_TIME + 1 == OBS_PER_DRONE, "inconsistent observation block size");

/// information computed in every world tick, used by the rewards
struct TickInfo
{
    float exploreGain[NUM_DRONES];   // "coldness" (1-heat) of the scanned cells, measured on the GLOBAL team map
    float shareFactor[NUM_DRONES];   // (island_size-1)/(N-1): fraction of the team that receives what it scans
    float rescanHeat[NUM_DRONES];    // sum of the heat (team map) of the cells that ENTER the sensor
    float overlapPen[NUM_DRONES];
    float commPen[NUM_DRONES];
    int   detectedBy;
};

class SeaEnvironment : public Metis::Enviroment
{
public:
    SeaEnvironment();

    void bindAgents(Drone* drones[NUM_DRONES], Castaway* castaway);

    // ---- Metis-Core overrides ----
    void  reset() override;
    void  getState(Metis::State* pState) override;
    void  serizalizeState(void* state, std::vector<float>* stateVector) override;
    void  applyAction(Metis::IAgent* pAgent, int actionId) override;
    float calculateReward(Metis::State& state, int* iDone) override { return 0.0f; } // single-head DQN only
    std::vector<Metis::TMULTIHEAD> calculateRewards(Metis::State& state, int* iDone) override;
    float getDeltaTime() override { return SIM_DT; }

    // ---- utilities ----
    void markDirty() { _dirty = true; }
    int  currentStep() const { return _step; }
    float commRiskOfHeading(int droneIdx, int action) const; // for the heuristic
    float cellHeat(int viewDrone, int cx, int cy) const;     // viewDrone<0: union of all drones

    Drone* drone(int i) { return _drones[i]; }
    Castaway* castaway() { return _castaway; }

    // ---- public data for rendering / telemetry ----
    float belief[NUM_DRONES][GRID_H][GRID_W]; // heat map of each drone (what each one "knows")
    float linkDist[NUM_DRONES][NUM_DRONES];
    bool  directLink[NUM_DRONES][NUM_DRONES];
    int   component[NUM_DRONES];              // communication island each drone belongs to
    int   componentSize[NUM_DRONES];
    float lastKnownX[NUM_DRONES][NUM_DRONES]; // last known position of j as seen by i
    float lastKnownY[NUM_DRONES][NUM_DRONES];
    float lastReward[NUM_DRONES];
    int   disconnectedSteps[NUM_DRONES];      // consecutive seconds outside the full network (island < N)
    int   detectedBy = -1;
    int   episode = 0;
    float currentVx = 0.0f, currentVy = 0.0f;  // sea current of this episode (m/s)
    float currentSpeed() const { return sqrtf(currentVx * currentVx + currentVy * currentVy); }
    float currentBearingDeg() const;           // nautical bearing it flows towards (0 = N, 90 = E)
    std::deque<int> outcomes;                 // last episodes: 1 found, -1 dead
    int   successesLastN() const;

    // statistics of the current episode
    float episodeReward[NUM_DRONES];

    // ---- redundancy: cells that enter a drone footprint but the team already scanned < 5 min ago ----
    long  episodeNewCells = 0;                // cells new to some drone in this episode
    long  episodeRedundantCells = 0;          // of those, how many were already "hot" on the team map
    float redundancyPct() const { return episodeNewCells > 0 ? 100.0f * episodeRedundantCells / episodeNewCells : 0.0f; }
    // per-episode history (for the charts)
    std::deque<float> redundancyHistory;      // redundancy % of every finished episode
    std::deque<float> contactTimeHistory;     // seconds until contact (MAX_STEPS if the castaway dies)
    void seed(unsigned s) { _rng.seed(s); }   // reproducibility (lawnmower baseline)

private:
    void worldTick();
    void decayMaps();
    void scanAndGain();
    void updateComms();
    void mergeMaps();
    void detectCastaway();
    void computePenalties();
    void buildObservation(TSARSTATE& s);

    Drone* _drones[NUM_DRONES];
    Castaway* _castaway = nullptr;
    bool _dirty = false;
    int  _step = 0;
    bool _episodeClosed = false;
    TickInfo _tick;
    TSARSTATE _obs;
    float _teamMap[GRID_H][GRID_W];           // global map (union) of this tick, only used to measure the reward
    int   _footStamp[NUM_DRONES][GRID_H][GRID_W]; // last step in which the cell was inside the drone footprint
    std::mt19937 _rng;
};
