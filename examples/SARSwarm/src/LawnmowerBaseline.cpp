#include "LawnmowerBaseline.h"
#include "SeaEnvironment.h"
#include "Drone.h"
#include "Castaway.h"
#include <algorithm>
#include <vector>

namespace
{
    struct Waypoint { float x, y; };

    // tracks of a drone strip: 4 vertical passes alternating direction
    std::vector<Waypoint> buildLanes(int droneIdx)
    {
        const float stripW = WORLD_W / NUM_DRONES;   // 1000 m per drone
        const int   lanes = 4;
        const float spacing = stripW / lanes;        // 250 m between tracks
        const float margin = 150.0f;                 // keep away from the world border
        std::vector<Waypoint> wp;
        bool goingUp = true;                         // the ship is at mid height: start going up
        for (int k = 0; k < lanes; k++)
        {
            float x = droneIdx * stripW + spacing * (k + 0.5f);
            float yStart = goingUp ? WORLD_H - margin : margin;
            float yEnd = goingUp ? margin : WORLD_H - margin;
            if (k == 0) yStart = WORLD_H * 0.5f;     // first track: from the ship height
            wp.push_back({ x, yStart });
            wp.push_back({ x, yEnd });
            goingUp = !goingUp;
        }
        return wp;
    }
}

LawnmowerStats LawnmowerBaseline::compute(int episodes, unsigned seed, const std::function<void(int, int)>& progress)
{
    LawnmowerStats st;

    // own instances: they do not touch the training environment or network
    const char* names[NUM_DRONES] = { "LAW_1", "LAW_2", "LAW_3", "LAW_4" };
    Drone* drones[NUM_DRONES];
    for (int i = 0; i < NUM_DRONES; i++)
        drones[i] = new Drone(i, names[i]);
    Castaway* castaway = new Castaway();
    SeaEnvironment* env = new SeaEnvironment(); // on the heap: the maps take ~250 KB
    env->bindAgents(drones, castaway);
    env->seed(seed);

    std::vector<std::vector<Waypoint>> lanes(NUM_DRONES);
    std::vector<size_t> next(NUM_DRONES);
    std::vector<float> contacts;
    std::vector<float> redundancies;              // redundancy % of every episode
    int successes = 0;
    double newCells = 0.0, redundant = 0.0;

    for (int e = 0; e < episodes; e++)
    {
        env->reset();
        for (int i = 0; i < NUM_DRONES; i++)
        {
            lanes[i] = buildLanes(i);
            next[i] = 0;
        }

        int done = 0;
        while (done == 0)
        {
            Metis::State state;
            env->getState(&state);

            // autopilot: direct heading to the next waypoint (without the 8 discrete actions)
            for (int i = 0; i < NUM_DRONES; i++)
            {
                Drone* d = drones[i];
                Waypoint w = lanes[i][next[i]];
                float dx = w.x - d->x, dy = w.y - d->y;
                if (dx * dx + dy * dy < 30.0f * 30.0f)
                {
                    next[i] = (next[i] + 1) % lanes[i].size(); // when finished, repeat the pattern
                    w = lanes[i][next[i]];
                    dx = w.x - d->x;
                    dy = w.y - d->y;
                }
                d->targetHeading = atan2f(dy, dx);
            }
            env->updatePhysics(env->getDeltaTime());

            Metis::State nextState;
            env->getState(&nextState);
            env->calculateRewards(nextState, &done);
            for (int i = 0; i < NUM_DRONES; i++)
                drones[i]->endStep();
        }

        if (done > 0) successes++;
        contacts.push_back(env->contactTimeHistory.back());
        redundancies.push_back(env->redundancyPct());
        newCells += env->episodeNewCells;
        redundant += env->episodeRedundantCells;

        if (progress) progress(e + 1, episodes);
    }

    st.valid = episodes > 0;
    st.episodes = episodes;
    st.successRate = 100.0f * successes / std::max(1, episodes);
    double sum = 0.0;
    for (float c : contacts) sum += c;
    st.meanContact = (float)(sum / std::max<size_t>(1, contacts.size()));
    // percentiles by sorting (value at position p*(n-1))
    auto pct = [](std::vector<float>& v, float p) { return v.empty() ? 0.0f : v[(size_t)(p * (v.size() - 1) + 0.5f)]; };
    std::sort(contacts.begin(), contacts.end());
    std::sort(redundancies.begin(), redundancies.end());
    st.medianContact = pct(contacts, 0.5f);
    st.contactP25 = pct(contacts, 0.25f);
    st.contactP75 = pct(contacts, 0.75f);
    st.redundancyP25 = pct(redundancies, 0.25f);
    st.redundancyP75 = pct(redundancies, 0.75f);
    st.redundancyPct = newCells > 0.0 ? (float)(100.0 * redundant / newCells) : 0.0f;

    delete env;
    delete castaway;
    for (int i = 0; i < NUM_DRONES; i++) delete drones[i];
    return st;
}
