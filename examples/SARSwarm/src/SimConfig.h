#pragma once
// ============================================================================
//  SARSwarm - Proof of concept: swarm of 4 SAR drones trained with Metis-Core
//  Simulation parameters. All distances in METERS, all times in SECONDS.
// ============================================================================

// ---- World ----
constexpr float WORLD_W = 4000.0f;          // width of the search area (4 km)
constexpr float WORLD_H = 3000.0f;          // height of the search area (3 km)
constexpr float CELL_SIZE = 50.0f;          // heat map resolution (50 m cells)
constexpr int   GRID_W = (int)(WORLD_W / CELL_SIZE);   // 80 cells
constexpr int   GRID_H = (int)(WORLD_H / CELL_SIZE);   // 60 cells

// ---- Time ----
constexpr float SIM_DT = 1.0f;              // 1 step = 1 simulated second
constexpr int   MAX_STEPS = 1800;           // 30 min: castaway "survival time"; when it runs out the castaway dies (failure)

// ---- Agents ----
constexpr int   NUM_DRONES = 4;             // one network head per drone
constexpr int   CASTAWAY_ID = NUM_DRONES;   // ID of the castaway (procedural / sparring agent)

// ---- Drone ----
constexpr float DRONE_SPEED = 20.0f;        // 20 m/s (72 km/h), cruise speed of a light fixed-wing/VTOL UAV
constexpr float DRONE_TURN_RATE_DEG = 30.0f;// degrees per second
constexpr float SENSOR_RADIUS = 150.0f;     // EO/IR sensor footprint to detect a person in the water
constexpr float P_DETECT = 0.85f;           // detection probability per step when inside the footprint
constexpr float BORDER_LOOKAHEAD = 150.0f;  // isValidAction: headings that would take the drone out of the world are not allowed
constexpr int   LAUNCH_GAP_STEPS = 8;       // drones take off from the ship staggered every 8 s

// ---- Communications (multi-hop mesh network between drones) ----
constexpr float COMM_RANGE = 1200.0f;       // drone-to-drone radio range (1.2 km)
constexpr float COMM_WARN_RATIO = 0.80f;    // >80% of the range: link about to be lost (yellow line)

// ---- Heat map ("T mark") ----
constexpr float HEAT_DECAY_STEPS = 600.0f;  // a scanned area (heat=1) goes back to black (0) in 10 min

// ---- Mothership ----
constexpr float SHIP_X = 300.0f;
constexpr float SHIP_Y = WORLD_H * 0.5f;

// ---- Castaway ----
constexpr float CASTAWAY_MIN_DIST_SHIP = 1200.0f; // spawns far from the ship
constexpr float CASTAWAY_MAX_SPEED = 2.0f;        // random drift (simplified waves), m/s
constexpr float CASTAWAY_USER_MAX_SPEED = 4.0f;   // in Play the user can push it a bit faster
constexpr float CASTAWAY_OU_THETA = 0.01f;        // Ornstein-Uhlenbeck process: return to zero velocity
constexpr float CASTAWAY_OU_SIGMA = 0.25f;        // drift noise

// ---- Sea current (different and random in every episode) ----
// the castaway swims/moves relative to the water (OU) and the water drags it with the current
constexpr float CURRENT_MIN_SPEED = 0.2f;   // m/s (weak surface current)
constexpr float CURRENT_MAX_SPEED = 0.6f;   // m/s (moderate current, ~1.2 knots)

// ---- Observation (network input) ----
constexpr int   NUM_SECTORS = 8;            // 8 sectors of 45 degrees, aligned with the 8 actions
constexpr float RING1_RADIUS = 600.0f;      // near ring
constexpr float RING2_RADIUS = 1800.0f;     // far ring
// data of each teammate in the block: dx, dy, cos(heading), sin(heading), connected
constexpr int   OBS_PER_MATE = 5;
// per drone: pos(2) + heading(2) + sectors(16) + teammates(3x5=15) + isolated(1) + disconnected time(1) = 37
constexpr int   OBS_PER_DRONE = 2 + 2 + NUM_SECTORS * 2 + (NUM_DRONES - 1) * OBS_PER_MATE + 1 + 1;
// +1 normalized time +2 normalized current (vx, vy) = 4*37+3 = 151
constexpr int   NET_INPUTS = NUM_DRONES * OBS_PER_DRONE + 3;

// ---- Rewards ----
constexpr float EXPLORE_NORM = 3.0f;        // expected new "cold" cells per step (2R*v*dt/cell^2 = 2.4)
constexpr float W_TEAM_EXPLORE = 0.25f;     // share of the teammates' exploration that each head receives
constexpr float R_TIME = -0.01f;            // cost per second (urgency)
// ---- Communications: penalty GROWING with the time outside the full network ----
// pen = severity * (R_DISC_START + (R_DISC_MAX - R_DISC_START) * min(1, t_disc / DISC_RAMP_STEPS))
// severity = (N - island_size) / (N - 1): alone = 1, pair = 2/3, trio = 1/3, all together = 0
constexpr float R_DISC_START = -0.2f;       // penalty when the full network is lost (t = 0)
constexpr float R_DISC_MAX = -1.0f;         // maximum penalty, reached after DISC_RAMP_STEPS
constexpr float DISC_RAMP_STEPS = 120.0f;   // 2 min disconnected -> maximum penalty
constexpr float R_WARN_MAX = -0.2f;         // strongest link in the yellow zone
// gradual re-scan penalty: for every cell that ENTERS the sensor, -K x heat of the team map
// (white just scanned = maximum penalty, red = medium, almost black = very light). Same normalizer as exploration
constexpr float R_RESCAN_K = 0.5f;
constexpr float R_OVERLAP_MAX = -0.3f;      // two overlapping sensor footprints
constexpr float R_BORDER = -0.3f;           // hits the world border
constexpr float R_FOUND_FINDER = 5.0f;      // terminal: the drone that finds the castaway
constexpr float R_FOUND_TEAM = 3.0f;        // terminal: the rest of the team
constexpr float R_DEAD = -3.0f;             // terminal: the castaway dies

// ---- Mission metrics ----
constexpr float REDUNDANCY_HEAT = 0.5f;     // a cell is "repeated" if the team scanned it less than 5 min ago (heat > 0.5)
constexpr int   METRIC_HISTORY = 400;       // episodes kept for the history charts

// ---- Drone actions: desired heading (screen coordinates: y grows downwards) ----
enum DRONE_ACTIONS
{
    GO_E = 0, GO_SE, GO_S, GO_SW, GO_W, GO_NW, GO_N, GO_NE,
    NUM_DRONE_ACTIONS                       // 8 actions per head
};

// action of the procedural agent (castaway); outside the range of the heads
constexpr int CASTAWAY_DRIFT = 100;

constexpr float SIM_PI = 3.14159265358979f;
constexpr float SIM_DEG2RAD = SIM_PI / 180.0f;
