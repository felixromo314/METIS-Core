#include "SeaEnvironment.h"
#include "Drone.h"
#include "Castaway.h"
#include <algorithm>
#include <cassert>
#include <ctime>

// expected cells per sector in each ring (to normalize the "coldness" to [0,1])
static const float RING1_CELLS = SIM_PI * RING1_RADIUS * RING1_RADIUS / NUM_SECTORS / (CELL_SIZE * CELL_SIZE);
static const float RING2_CELLS = SIM_PI * (RING2_RADIUS * RING2_RADIUS - RING1_RADIUS * RING1_RADIUS) / NUM_SECTORS / (CELL_SIZE * CELL_SIZE);
static const int   OUTCOME_WINDOW = 50;

SeaEnvironment::SeaEnvironment()
    : _rng((unsigned)time(nullptr))
{
    for (int i = 0; i < NUM_DRONES; i++)
        _drones[i] = nullptr;
    memset(belief, 0, sizeof(belief));
    memset(&_obs, 0, sizeof(_obs));
    memset(&_tick, 0, sizeof(_tick));
    memset(lastReward, 0, sizeof(lastReward));
    memset(episodeReward, 0, sizeof(episodeReward));
    memset(disconnectedSteps, 0, sizeof(disconnectedSteps));
}

void SeaEnvironment::bindAgents(Drone* drones[NUM_DRONES], Castaway* castaway)
{
    for (int i = 0; i < NUM_DRONES; i++)
    {
        _drones[i] = drones[i];
        _drones[i]->env = this;
        addAgent(_drones[i]);   // IDs 0..3
    }
    _castaway = castaway;
    _castaway->env = this;
    _castaway->rng = &_rng;
    addAgent(_castaway);        // ID 4
}

// ============================================================================
//  reset: drones on the ship deck, castaway at a random point
// ============================================================================
void SeaEnvironment::reset()
{
    _step = 0;
    _dirty = false;
    _episodeClosed = false;
    detectedBy = -1;
    episode++;
    memset(belief, 0, sizeof(belief));
    memset(&_tick, 0, sizeof(_tick));
    memset(lastReward, 0, sizeof(lastReward));
    memset(episodeReward, 0, sizeof(episodeReward));
    memset(disconnectedSteps, 0, sizeof(disconnectedSteps));
    _tick.detectedBy = -1;
    episodeNewCells = 0;
    episodeRedundantCells = 0;
    std::fill(&_footStamp[0][0][0], &_footStamp[0][0][0] + NUM_DRONES * GRID_H * GRID_W, -100);

    // random sea current for this episode (uniform direction, speed between min and max)
    std::uniform_real_distribution<float> uDir(0.0f, 2.0f * SIM_PI);
    std::uniform_real_distribution<float> uSpd(CURRENT_MIN_SPEED, CURRENT_MAX_SPEED);
    float cDir = uDir(_rng), cSpd = uSpd(_rng);
    currentVx = cosf(cDir) * cSpd;
    currentVy = sinf(cDir) * cSpd;

    // fan of departure headings towards open sea (East), with some noise
    std::uniform_real_distribution<float> noise(-15.0f, 15.0f);
    const float fanDeg[NUM_DRONES] = { -60.0f, -20.0f, 20.0f, 60.0f };
    for (int i = 0; i < NUM_DRONES; i++)
    {
        float hdg = (fanDeg[i] + noise(_rng)) * SIM_DEG2RAD;
        float ox = (i % 2) * 12.0f - 6.0f;    // small offsets on the deck
        float oy = (i / 2) * 12.0f - 6.0f;
        _drones[i]->resetAt(SHIP_X + ox, SHIP_Y + oy, hdg, i * LAUNCH_GAP_STEPS);
    }

    // castaway far from the ship
    std::uniform_real_distribution<float> ux(200.0f, WORLD_W - 200.0f);
    std::uniform_real_distribution<float> uy(200.0f, WORLD_H - 200.0f);
    float cx, cy;
    do
    {
        cx = ux(_rng);
        cy = uy(_rng);
    } while (hypotf(cx - SHIP_X, cy - SHIP_Y) < CASTAWAY_MIN_DIST_SHIP);
    _castaway->resetAt(cx, cy);

    updateComms();
    for (int i = 0; i < NUM_DRONES; i++)
        _tick.shareFactor[i] = (float)(componentSize[i] - 1) / (float)(NUM_DRONES - 1);
    for (int i = 0; i < NUM_DRONES; i++)
        for (int j = 0; j < NUM_DRONES; j++)
        {
            lastKnownX[i][j] = _drones[j]->x;
            lastKnownY[i][j] = _drones[j]->y;
        }
    buildObservation(_obs);
}

// ============================================================================
//  World tick: runs ONCE per step, after the physics of all the agents
//  (triggered by getState when some agent has moved).
// ============================================================================
void SeaEnvironment::worldTick()
{
    _step++;
    _tick.detectedBy = -1;
    decayMaps();       // 1. the T mark of every area "cools down"
    scanAndGain();     // 2. each drone scans its footprint on ITS map and measures its contribution
    updateComms();     // 3. communication graph and components (islands)
    for (int i = 0; i < NUM_DRONES; i++)
    {
        // consecutive time outside the full network; reset when reconnected with everybody
        disconnectedSteps[i] = (componentSize[i] < NUM_DRONES) ? disconnectedSteps[i] + 1 : 0;
        _tick.shareFactor[i] = (float)(componentSize[i] - 1) / (float)(NUM_DRONES - 1);
    }
    mergeMaps();       // 4. connected drones (multi-hop) share their map
    detectCastaway();  // 5. castaway detection
    computePenalties();// 6. sensor overlap and communication risk
    buildObservation(_obs);
    _dirty = false;
}

void SeaEnvironment::decayMaps()
{
    const float decay = 1.0f / HEAT_DECAY_STEPS;
    float* p = &belief[0][0][0];
    const int n = NUM_DRONES * GRID_H * GRID_W;
    for (int k = 0; k < n; k++)
        p[k] = std::max(0.0f, p[k] - decay);
}

void SeaEnvironment::scanAndGain()
{
    // global team map (union of what every drone knows) BEFORE scanning in this tick.
    // The reward is measured here (centralized training), the observation uses the drone's own map
    // (decentralized execution). So an isolated drone is NOT paid for repeating areas another drone already scanned.
    for (int cy = 0; cy < GRID_H; cy++)
        for (int cx = 0; cx < GRID_W; cx++)
        {
            float m = 0.0f;
            for (int i = 0; i < NUM_DRONES; i++)
                m = std::max(m, belief[i][cy][cx]);
            _teamMap[cy][cx] = m;
        }

    const int rc = (int)ceilf(SENSOR_RADIUS / CELL_SIZE);
    const float r2 = SENSOR_RADIUS * SENSOR_RADIUS;
    for (int i = 0; i < NUM_DRONES; i++)
    {
        _tick.exploreGain[i] = 0.0f;
        _tick.rescanHeat[i] = 0.0f;
        Drone* d = _drones[i];
        if (!d->launched)
            continue;
        int gx = (int)(d->x / CELL_SIZE);
        int gy = (int)(d->y / CELL_SIZE);
        for (int cy = gy - rc; cy <= gy + rc; cy++)
        {
            if (cy < 0 || cy >= GRID_H) continue;
            for (int cx = gx - rc; cx <= gx + rc; cx++)
            {
                if (cx < 0 || cx >= GRID_W) continue;
                float px = (cx + 0.5f) * CELL_SIZE - d->x;
                float py = (cy + 0.5f) * CELL_SIZE - d->y;
                if (px * px + py * py > r2) continue;
                // redundancy: only cells that ENTER the footprint in this step (not the ones that stay inside)
                if (_footStamp[i][cy][cx] != _step - 1)
                {
                    // gradual penalty: only cells new to the sensor (the ones that stay inside are "my own footprint")
                    _tick.rescanHeat[i] += _teamMap[cy][cx];
                    episodeNewCells++;
                    if (_teamMap[cy][cx] > REDUNDANCY_HEAT)
                        episodeRedundantCells++;
                }
                _footStamp[i][cy][cx] = _step;

                // gain according to the global map; the cell is "claimed" for this tick
                // (if two drones scan the same cell at the same time, only one is paid)
                _tick.exploreGain[i] += (1.0f - _teamMap[cy][cx]);
                _teamMap[cy][cx] = 1.0f;
                belief[i][cy][cx] = 1.0f;   // T mark on ITS map
            }
        }
    }
}

void SeaEnvironment::updateComms()
{
    for (int i = 0; i < NUM_DRONES; i++)
    {
        for (int j = 0; j < NUM_DRONES; j++)
        {
            float d = hypotf(_drones[i]->x - _drones[j]->x, _drones[i]->y - _drones[j]->y);
            linkDist[i][j] = d;
            directLink[i][j] = (i != j) && (d <= COMM_RANGE);
        }
        component[i] = -1;
    }
    // connected components (BFS): it is a mesh network, messages hop from drone to drone
    int comp = 0;
    for (int i = 0; i < NUM_DRONES; i++)
    {
        if (component[i] >= 0) continue;
        int queue[NUM_DRONES];
        int head = 0, tail = 0;
        queue[tail++] = i;
        component[i] = comp;
        while (head < tail)
        {
            int a = queue[head++];
            for (int b = 0; b < NUM_DRONES; b++)
                if (directLink[a][b] && component[b] < 0)
                {
                    component[b] = comp;
                    queue[tail++] = b;
                }
        }
        comp++;
    }
    for (int i = 0; i < NUM_DRONES; i++)
    {
        int n = 0;
        for (int j = 0; j < NUM_DRONES; j++)
            if (component[j] == component[i]) n++;
        componentSize[i] = n;
    }
    // last known position: only updated if they are in the same island
    for (int i = 0; i < NUM_DRONES; i++)
        for (int j = 0; j < NUM_DRONES; j++)
            if (component[i] == component[j])
            {
                lastKnownX[i][j] = _drones[j]->x;
                lastKnownY[i][j] = _drones[j]->y;
            }
}

void SeaEnvironment::mergeMaps()
{
    // inside each island, the shared map is the maximum (the most recent T mark)
    for (int c = 0; c < NUM_DRONES; c++)
    {
        int members[NUM_DRONES];
        int n = 0;
        for (int i = 0; i < NUM_DRONES; i++)
            if (component[i] == c) members[n++] = i;
        if (n <= 1) continue;
        for (int cy = 0; cy < GRID_H; cy++)
            for (int cx = 0; cx < GRID_W; cx++)
            {
                float m = 0.0f;
                for (int k = 0; k < n; k++)
                    m = std::max(m, belief[members[k]][cy][cx]);
                for (int k = 0; k < n; k++)
                    belief[members[k]][cy][cx] = m;
            }
    }
}

void SeaEnvironment::detectCastaway()
{
    if (detectedBy >= 0) return;
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    for (int i = 0; i < NUM_DRONES; i++)
    {
        Drone* d = _drones[i];
        if (!d->launched) continue;
        float dist = hypotf(d->x - _castaway->x, d->y - _castaway->y);
        if (dist <= SENSOR_RADIUS && u01(_rng) < P_DETECT)
        {
            detectedBy = i;
            _tick.detectedBy = i;
            break;
        }
    }
}

void SeaEnvironment::computePenalties()
{
    for (int i = 0; i < NUM_DRONES; i++)
    {
        _tick.overlapPen[i] = 0.0f;
        _tick.commPen[i] = 0.0f;
        if (!_drones[i]->launched) continue;

        // footprint overlap with connected teammates (it knows where they are)
        for (int j = 0; j < NUM_DRONES; j++)
        {
            if (j == i || !_drones[j]->launched || component[j] != component[i]) continue;
            float d = linkDist[i][j];
            if (d < 2.0f * SENSOR_RADIUS)
                _tick.overlapPen[i] += R_OVERLAP_MAX * (1.0f - d / (2.0f * SENSOR_RADIUS));
        }

        // communications: penalty GROWING with the time outside the full network
        float severity = (float)(NUM_DRONES - componentSize[i]) / (float)(NUM_DRONES - 1); // 0 all together, 1 alone
        if (severity > 0.0f)
        {
            float ramp = std::min(1.0f, disconnectedSteps[i] / DISC_RAMP_STEPS);
            _tick.commPen[i] += severity * (R_DISC_START + (R_DISC_MAX - R_DISC_START) * ramp);
        }
        else
        {
            // connected to everybody: soft warning if its strongest link is in the yellow zone
            float nearest = 1e9f;
            for (int j = 0; j < NUM_DRONES; j++)
                if (j != i) nearest = std::min(nearest, linkDist[i][j]);
            float ratio = nearest / COMM_RANGE;
            if (ratio > COMM_WARN_RATIO)
                _tick.commPen[i] += R_WARN_MAX * (ratio - COMM_WARN_RATIO) / (1.0f - COMM_WARN_RATIO);
        }
    }
}

// ============================================================================
//  Observation of each drone (computed with ITS map and what it knows through the network)
// ============================================================================
void SeaEnvironment::buildObservation(TSARSTATE& s)
{
    memset(&s, 0, sizeof(s));
    const int rc = (int)ceilf(RING2_RADIUS / CELL_SIZE);
    const float sectorWidth = 2.0f * SIM_PI / NUM_SECTORS;

    for (int i = 0; i < NUM_DRONES; i++)
    {
        Drone* d = _drones[i];
        float* o = s.obs[i];
        o[OBS_POS + 0] = d->x / WORLD_W;
        o[OBS_POS + 1] = d->y / WORLD_H;
        o[OBS_HEADING + 0] = cosf(d->heading);
        o[OBS_HEADING + 1] = sinf(d->heading);

        // "coldness" (1 - heat) per sector and ring, aligned with the 8 actions
        float cold1[NUM_SECTORS] = { 0 }, cold2[NUM_SECTORS] = { 0 };
        int gx = (int)(d->x / CELL_SIZE);
        int gy = (int)(d->y / CELL_SIZE);
        for (int cy = std::max(0, gy - rc); cy <= std::min(GRID_H - 1, gy + rc); cy++)
        {
            for (int cx = std::max(0, gx - rc); cx <= std::min(GRID_W - 1, gx + rc); cx++)
            {
                float px = (cx + 0.5f) * CELL_SIZE - d->x;
                float py = (cy + 0.5f) * CELL_SIZE - d->y;
                float dist = sqrtf(px * px + py * py);
                if (dist > RING2_RADIUS || dist < 1.0f) continue;
                float ang = atan2f(py, px); // same convention as the actions (0 = East, y down)
                int k = (int)floorf(ang / sectorWidth + 0.5f);
                k = ((k % NUM_SECTORS) + NUM_SECTORS) % NUM_SECTORS;
                float cold = 1.0f - belief[i][cy][cx];
                if (dist <= RING1_RADIUS) cold1[k] += cold;
                else                      cold2[k] += cold;
            }
        }
        // there are no cells outside the world: those sectors come out low (nothing to search there)
        for (int k = 0; k < NUM_SECTORS; k++)
        {
            o[OBS_SECTORS + k] = std::min(1.0f, cold1[k] / RING1_CELLS);
            o[OBS_SECTORS + NUM_SECTORS + k] = std::min(1.0f, cold2[k] / RING2_CELLS);
        }

        // the other drones: only if they are in the same communication island
        int slot = 0;
        bool isolated = true;
        for (int j = 0; j < NUM_DRONES; j++)
        {
            if (j == i) continue;
            if (directLink[i][j]) isolated = false;
            float* oo = &o[OBS_OTHERS + slot * OBS_PER_MATE];
            if (component[j] == component[i])
            {
                oo[0] = (_drones[j]->x - d->x) / WORLD_W;   // relative position
                oo[1] = (_drones[j]->y - d->y) / WORLD_H;
                oo[2] = cosf(_drones[j]->heading);          // where it is heading (short-term intention)
                oo[3] = sinf(_drones[j]->heading);
                oo[4] = 1.0f;                               // connected
            }                                               // no link: everything 0 (nothing is known about it)
            slot++;
        }
        o[OBS_ISOLATED] = isolated ? 1.0f : 0.0f;
        o[OBS_DISC_TIME] = std::min(1.0f, disconnectedSteps[i] / DISC_RAMP_STEPS);
    }
    s.timeNorm = (float)_step / (float)MAX_STEPS;
    s.currentX = currentVx / CURRENT_MAX_SPEED;
    s.currentY = currentVy / CURRENT_MAX_SPEED;
    s.step = _step;
    s.detectedBy = detectedBy;
    s.castawayX = _castaway->x;
    s.castawayY = _castaway->y;
}

// ============================================================================
//  Overrides Metis-Core
// ============================================================================
void SeaEnvironment::getState(Metis::State* pState)
{
    if (_dirty)
        worldTick(); // physics was already applied: process scan/comms/detection
    pState->copyState(&_obs, sizeof(TSARSTATE));
}

void SeaEnvironment::serizalizeState(void* state, std::vector<float>* stateVector)
{
    Metis::State* pState = (Metis::State*)state;
    TSARSTATE* s = (TSARSTATE*)pState->getUserState(); // only read the stored struct
    stateVector->clear();
    stateVector->reserve(NET_INPUTS);
    for (int i = 0; i < NUM_DRONES; i++)
        for (int k = 0; k < OBS_PER_DRONE; k++)
            stateVector->push_back(s->obs[i][k]);
    stateVector->push_back(s->timeNorm);
    stateVector->push_back(s->currentX);
    stateVector->push_back(s->currentY);
    assert((int)stateVector->size() == NET_INPUTS);
}

void SeaEnvironment::applyAction(Metis::IAgent* pAgent, int actionId)
{
    if (pAgent->getID() == CASTAWAY_ID)
        return; // the castaway drifts on its own in its update()
    Drone* d = (Drone*)pAgent;
    d->setAction(actionId);
}

std::vector<Metis::TMULTIHEAD> SeaEnvironment::calculateRewards(Metis::State& state, int* pDone)
{
    TSARSTATE* s = (TSARSTATE*)state.getUserState();
    std::vector<Metis::TMULTIHEAD> rewards;
    *pDone = 0;

    // USEFUL exploration: measured on the global map and multiplied by the fraction of the team that receives it.
    // A drone alone (shareFactor = 0) contributes nothing to the team with what it scans: nobody learns about it.
    float explore[NUM_DRONES];
    for (int i = 0; i < NUM_DRONES; i++)
    {
        float e = _drones[i]->launched ? std::clamp(_tick.exploreGain[i] / EXPLORE_NORM, 0.0f, 1.0f) : 0.0f;
        explore[i] = e * _tick.shareFactor[i];
    }

    bool found = (s->detectedBy >= 0);
    bool dead = !found && (s->step >= MAX_STEPS);

    for (int i = 0; i < NUM_DRONES; i++)
    {
        Metis::TMULTIHEAD h((char*)_drones[i]->name.c_str());
        float r = 0.0f;
        if (_drones[i]->launched)
        {
            float team = 0.0f;
            for (int j = 0; j < NUM_DRONES; j++)
                if (j != i) team += explore[j];
            team /= (float)(NUM_DRONES - 1);

            // positive part normalized to [0,1] WITHOUT clipping: penalties are never hidden by a clamp
            r = (explore[i] + W_TEAM_EXPLORE * team) / (1.0f + W_TEAM_EXPLORE);
            r += _tick.overlapPen[i];
            r -= R_RESCAN_K * _tick.rescanHeat[i] / EXPLORE_NORM;   // re-scanning what was just scanned
            r += _tick.commPen[i];
            r += R_TIME;
            if (_drones[i]->hitBorder) r += R_BORDER;
            r = std::clamp(r, -2.0f, 1.0f); // safety limit; not reached with the current values
        }
        // terminal rewards (outside the clamp, as in SwarmDefenseTIE)
        if (found)
            r += (s->detectedBy == i) ? R_FOUND_FINDER : R_FOUND_TEAM;
        if (dead)
            r += R_DEAD;

        h.reward = r;
        lastReward[i] = r;
        episodeReward[i] += r;
        rewards.push_back(h);
    }

    if (found) *pDone = 1;
    else if (dead) *pDone = -1;

    // statistics (only once per episode)
    if (*pDone != 0 && !_episodeClosed)
    {
        _episodeClosed = true;
        outcomes.push_back(*pDone);
        while ((int)outcomes.size() > OUTCOME_WINDOW)
            outcomes.pop_front();

        // episode history metrics
        redundancyHistory.push_back(redundancyPct());
        contactTimeHistory.push_back(found ? (float)s->step : (float)MAX_STEPS);
        while ((int)redundancyHistory.size() > METRIC_HISTORY) redundancyHistory.pop_front();
        while ((int)contactTimeHistory.size() > METRIC_HISTORY) contactTimeHistory.pop_front();
    }
    return rewards;
}

int SeaEnvironment::successesLastN() const
{
    int n = 0;
    for (int o : outcomes)
        if (o > 0) n++;
    return n;
}

float SeaEnvironment::cellHeat(int viewDrone, int cx, int cy) const
{
    if (viewDrone >= 0 && viewDrone < NUM_DRONES)
        return belief[viewDrone][cy][cx];
    float m = 0.0f;
    for (int i = 0; i < NUM_DRONES; i++)
        m = std::max(m, belief[i][cy][cx]);
    return m;
}

/// risk of losing communications if the drone takes heading 'action' (heuristic only)
float SeaEnvironment::commRiskOfHeading(int i, int action) const
{
    // nearest teammate according to the last known position
    float best = 1e9f, tx = 0.0f, ty = 0.0f;
    for (int j = 0; j < NUM_DRONES; j++)
    {
        if (j == i) continue;
        float d = hypotf(lastKnownX[i][j] - _drones[i]->x, lastKnownY[i][j] - _drones[i]->y);
        if (d < best) { best = d; tx = lastKnownX[i][j]; ty = lastKnownY[i][j]; }
    }
    float ratio = best / COMM_RANGE;
    if (ratio < 0.7f || best < 1.0f) return 0.0f;
    float ang = action * 45.0f * SIM_DEG2RAD;
    float dirx = (tx - _drones[i]->x) / best, diry = (ty - _drones[i]->y) / best;
    float cosToMate = cosf(ang) * dirx + sinf(ang) * diry; // >0 approaching, <0 moving away
    float urgency = std::min(2.0f, (ratio - 0.7f) / 0.3f);
    return urgency * (1.0f - cosToMate); // 0 if heading towards the teammate, up to 4 if moving away
}

float SeaEnvironment::currentBearingDeg() const
{
    // screen: x East, y down (South). Nautical bearing: 0 = North, 90 = East, clockwise
    float deg = atan2f(currentVy, currentVx) / SIM_DEG2RAD + 90.0f;
    while (deg < 0.0f) deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;
    return deg;
}
