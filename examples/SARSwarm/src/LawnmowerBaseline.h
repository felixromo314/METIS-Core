#pragma once
#include "SimConfig.h"
#include <functional>

/// result of the classic search (lawnmower) in this same scenario
struct LawnmowerStats
{
    bool  valid = false;
    int   episodes = 0;
    float successRate = 0.0f;     // % of episodes with contact before MAX_STEPS
    float meanContact = 0.0f;     // s until contact (a failure counts as MAX_STEPS)
    float medianContact = 0.0f;   // s, median
    float redundancyPct = 0.0f;   // % of re-scanned cells (same definition as the swarm)
    // spread between episodes (25-75 percentile band in the charts)
    float contactP25 = 0.0f, contactP75 = 0.0f;
    float redundancyP25 = 0.0f, redundancyP75 = 0.0f;
};

/// <summary>
/// Baseline: classic "lawnmower" sweep (parallel track search).
/// Each drone owns a vertical strip of WORLD_W/4 and covers it in 4 parallel tracks
/// 250 m apart (sensor sweep width = 2R = 300 m -> ~17 % overlap, a typical SAR plan).
/// It is simulated with the SAME environment (current, drift, detection, staggered launch),
/// in its own instances, without a neural network.
/// </summary>
class LawnmowerBaseline
{
public:
    /// progress(done, total) is called after every episode (to draw a loading bar)
    static LawnmowerStats compute(int episodes, unsigned seed,
                                  const std::function<void(int, int)>& progress = nullptr);
};
