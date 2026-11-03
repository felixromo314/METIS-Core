#include "Castaway.h"
#include "SeaEnvironment.h"
#include <algorithm>

Castaway::Castaway()
{
    setID(CASTAWAY_ID);
}

void Castaway::resetAt(float px, float py)
{
    x = px;
    y = py;
    vx = vy = 0.0f;
    userPushX = userPushY = 0.0f;
    trail.clear();
    _stepsSinceTrail = 0;
}

void Castaway::push(float ax, float ay)
{
    userPushX += ax;
    userPushY += ay;
}

int Castaway::getActionProcedural(Metis::State& state)
{
    // the drift is computed in update(); the action only means "drift"
    return CASTAWAY_DRIFT;
}

int Castaway::update(double delta_time)
{
    float dt = (float)delta_time;

    // Ornstein-Uhlenbeck drift: v += -theta*v*dt + sigma*sqrt(dt)*N(0,1)
    std::normal_distribution<float> n01(0.0f, 1.0f);
    float sq = sqrtf(dt);
    if (rng != nullptr)
    {
        vx += -CASTAWAY_OU_THETA * vx * dt + CASTAWAY_OU_SIGMA * sq * n01(*rng);
        vy += -CASTAWAY_OU_THETA * vy * dt + CASTAWAY_OU_SIGMA * sq * n01(*rng);
    }

    // user push (Play only)
    float maxSpeed = CASTAWAY_MAX_SPEED;
    if (userPushX != 0.0f || userPushY != 0.0f)
    {
        vx += userPushX;
        vy += userPushY;
        userPushX = userPushY = 0.0f;
        maxSpeed = CASTAWAY_USER_MAX_SPEED;
    }

    float speed = sqrtf(vx * vx + vy * vy);
    if (speed > maxSpeed)
    {
        vx *= maxSpeed / speed;
        vy *= maxSpeed / speed;
    }

    // position = own motion (relative to the water) + drag of the current
    float curX = (env != nullptr) ? env->currentVx : 0.0f;
    float curY = (env != nullptr) ? env->currentVy : 0.0f;
    x += (vx + curX) * dt;
    y += (vy + curY) * dt;

    // bounce on the world borders
    if (x < 0.0f)    { x = 0.0f;    vx = fabsf(vx); }
    if (x > WORLD_W) { x = WORLD_W; vx = -fabsf(vx); }
    if (y < 0.0f)    { y = 0.0f;    vy = fabsf(vy); }
    if (y > WORLD_H) { y = WORLD_H; vy = -fabsf(vy); }

    // trail point every 10 s
    if (++_stepsSinceTrail >= 10)
    {
        _stepsSinceTrail = 0;
        trail.push_back(x);
        trail.push_back(y);
        if (trail.size() > 120) // at most 60 points
            trail.erase(trail.begin(), trail.begin() + 2);
    }

    if (env != nullptr)
        env->markDirty();
    return 0;
}
