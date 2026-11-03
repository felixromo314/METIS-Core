#pragma once
#include "MetisInclude.h"
#include "SimConfig.h"
#include <random>
#include <vector>

class SeaEnvironment;

/// <summary>
/// Castaway. It is the "sparring" (procedural) agent given to the Metis-Core trainer.
/// It moves with a random drift (Ornstein-Uhlenbeck) so it is never static.
/// In Play mode the user can push it with the arrow keys.
/// </summary>
class Castaway : public Metis::IAgent
{
public:
    Castaway();

    float x = 0.0f, y = 0.0f;     // meters
    float vx = 0.0f, vy = 0.0f;   // m/s
    float userPushX = 0.0f, userPushY = 0.0f; // user push in this frame
    std::vector<float> trail;     // trail: x0,y0,x1,y1,...

    std::mt19937* rng = nullptr;
    SeaEnvironment* env = nullptr;

    void resetAt(float px, float py);
    void push(float ax, float ay);  // accumulates a push (Play)

    int  update(double delta_time) override;
    int  getActionProcedural(Metis::State& state) override;
    bool isValidAction(int iAction) override { return true; }
    void endStep() override {}

private:
    int _stepsSinceTrail = 0;
};
