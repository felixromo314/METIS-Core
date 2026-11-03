#pragma once
#include "MetisInclude.h"
#include "SimConfig.h"
#include <string>

class SeaEnvironment;

/// <summary>
/// Search UAV. Each Drone is one head of the Metis-Core multi-head network.
/// The role is given by the ID (0..3), as in SwarmDefenseTIE.
/// </summary>
class Drone : public Metis::IAgent
{
public:
    Drone(int id, const char* name);

    // --- physical state ---
    float x = 0.0f, y = 0.0f;        // position in meters (y grows downwards, like the screen)
    float heading = 0.0f;            // current heading in radians (0 = East)
    float targetHeading = 0.0f;      // heading commanded by the last action
    bool  launched = false;          // false = still on the ship deck
    int   launchStep = 0;            // step at which it takes off
    bool  hitBorder = false;         // touched the world border in this step
    int   lastAction = -1;

    std::string name;
    SeaEnvironment* env = nullptr;

    void resetAt(float px, float py, float headingRad, int launchAtStep);
    void setAction(int action);

    // --- Metis-Core methods we override ---
    int  update(double delta_time) override;
    int  getActionProcedural(Metis::State& state) override;   // heuristic (baseline), not used by the trainer
    bool isValidAction(int iAction) override;                 // mask: do not leave the world
    void endStep() override;
};
