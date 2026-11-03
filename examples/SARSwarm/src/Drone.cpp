#include "Drone.h"
#include "SeaEnvironment.h"
#include <algorithm>

// wraps an angle to (-PI, PI]
static float wrapAngle(float a)
{
    while (a > SIM_PI) a -= 2.0f * SIM_PI;
    while (a <= -SIM_PI) a += 2.0f * SIM_PI;
    return a;
}

Drone::Drone(int id, const char* pszName)
{
    setID(id);
    name = pszName;
}

void Drone::resetAt(float px, float py, float headingRad, int launchAtStep)
{
    x = px;
    y = py;
    heading = headingRad;
    targetHeading = headingRad;
    launchStep = launchAtStep;
    launched = (launchAtStep <= 0);
    hitBorder = false;
    lastAction = -1;
}

void Drone::setAction(int action)
{
    if (action < 0 || action >= NUM_DRONE_ACTIONS)
        return; // out-of-range action: ignored
    lastAction = action;
    targetHeading = wrapAngle(action * 45.0f * SIM_DEG2RAD); // action k -> heading k*45 degrees
}

int Drone::update(double delta_time)
{
    float dt = (float)delta_time;
    hitBorder = false;

    if (!launched)
    {
        // on deck: waits for its take-off turn
        if (env != nullptr && env->currentStep() >= launchStep)
            launched = true;
        else
            return 0;
    }

    // limited turn towards the target heading
    float diff = wrapAngle(targetHeading - heading);
    float maxTurn = DRONE_TURN_RATE_DEG * SIM_DEG2RAD * dt;
    diff = std::clamp(diff, -maxTurn, maxTurn);
    heading = wrapAngle(heading + diff);

    // move forward at constant speed
    x += cosf(heading) * DRONE_SPEED * dt;
    y += sinf(heading) * DRONE_SPEED * dt;

    // world limits (the drone is not destroyed, but it is penalized)
    if (x < 0.0f)     { x = 0.0f;     hitBorder = true; }
    if (x > WORLD_W)  { x = WORLD_W;  hitBorder = true; }
    if (y < 0.0f)     { y = 0.0f;     hitBorder = true; }
    if (y > WORLD_H)  { y = WORLD_H;  hitBorder = true; }

    if (env != nullptr)
        env->markDirty(); // the environment will process the world "tick" (scan, comms, detection)
    return 0;
}

bool Drone::isValidAction(int iAction)
{
    if (iAction < 0 || iAction >= NUM_DRONE_ACTIONS)
        return false;
    if (!launched)
        return true; // on deck it does not matter

    // project the heading BORDER_LOOKAHEAD meters ahead: if it leaves the world, the action is not valid
    float ang = iAction * 45.0f * SIM_DEG2RAD;
    float px = x + cosf(ang) * BORDER_LOOKAHEAD;
    float py = y + sinf(ang) * BORDER_LOOKAHEAD;
    return (px >= 0.0f && px <= WORLD_W && py >= 0.0f && py <= WORLD_H);
}

/// Greedy heuristic (baseline to compare with the AI in Play):
/// fly towards the sector with the most "cold" (black) area according to ITS OWN belief map.
int Drone::getActionProcedural(Metis::State& state)
{
    TSARSTATE* s = (TSARSTATE*)state.getUserState();
    const float* obs = s->obs[getID()];
    const int sectorBase = 4; // pos(2) + heading(2)

    int best = GO_E;
    float bestScore = -1e9f;
    for (int k = 0; k < NUM_SECTORS; k++)
    {
        if (!isValidAction(k))
            continue;
        float score = obs[sectorBase + k] + 0.5f * obs[sectorBase + NUM_SECTORS + k];
        // penalize moving away when at the edge of the communication range
        if (env != nullptr)
            score -= env->commRiskOfHeading(getID(), k);
        if (score > bestScore)
        {
            bestScore = score;
            best = k;
        }
    }
    return best;
}

void Drone::endStep()
{
    hitBorder = false; // per-step flag
}
