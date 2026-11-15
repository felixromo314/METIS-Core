# SmartGridGuardian: Your First Asymmetric AlphaZero with METIS-Core

> **Four substations defend a power grid. Two hackers try to cause a blackout.**
> Each side sees a different world, has different actions and pursues opposite goals.
> And both learn to play **on their own**, by playing against each other again and again.

This tutorial shows you how to build that game with METIS-Core's **asymmetric multi-head AlphaZero** engine, in pure C++ and without a single line of Python. By the end, you will know how to apply the same pattern to your own domain: cybersecurity, tactical simulation, logistics or strategy games.

### Demo: SmartGridGuardian
Check out the SmartGridGuardian learning and in action! 🚀

[![Watch the demo on YouTube](https://img.youtube.com/vi/9rzoBaReL38/0.jpg)](https://www.youtube.com/watch?v=9rzoBaReL38)

> **Note:** Best experienced with audio! The video features a custom soundtrack composed for the Metis-Core project.

- **Video Link:** [https://www.youtube.com/watch?v=9rzoBaReL38)

---

## Table of Contents

1. [What you will build](#1-what-you-will-build)
2. [Why an *asymmetric* AlphaZero](#2-why-an-asymmetric-alphazero)
3. [Architecture at a glance](#3-architecture-at-a-glance)
4. [Step 1: Players and teams](#step-1-players-and-teams)
5. [Step 2: One action space per head](#step-2-one-action-space-per-head)
6. [Step 3: The agent](#step-3-the-agent)
7. [Step 4: The environment](#step-4-the-environment)
8. [Step 5: Build the two multi-head brains](#step-5-build-the-two-multi-head-brains)
9. [Step 6: Train with self-play](#step-6-train-with-self-play)
10. [Step 7: Play against the AI](#step-7-play-against-the-ai)
11. [Common pitfalls (and how to avoid them)](#common-pitfalls-and-how-to-avoid-them)
12. [Bring it to your own domain](#bring-it-to-your-own-domain)

---

## 1. What you will build

A 4-node ring power grid, fed by a power plant:

```
   [PLANT] -- A -------- B
              |          |
              C -------- D
```

| Side | Players | What it sees | What it can do |
|---|---|---|---|
| **Blue** (defense) | 4 substations: A, B, C, D | The **measured** grid, which the hackers can falsify | Shed load, restore, open or close lines, verify sensors, quarantine |
| **Red** (attack) | **Striker** and **Spoofer** | The **real** grid | Striker: inject fake load (botnet), trip breakers. Spoofer: freeze meters, freeze flows, raise false alarms |

- **One episode = one day** (288 steps of 5 minutes), with a demand profile that peaks in the evening.
- **Red wins** if the fraction of customers served drops below the blackout threshold.
- **Blue wins** if it keeps the grid up all day.

The interesting part of the game is **coordination**:
- The Striker overloads a node and, just in time, the Spoofer freezes its meter so Blue does not see it.
- Blue has to decide when to spend a turn **verifying** its sensors instead of acting.

Nobody programs these strategies: they **emerge** from self-play.

---

## 2. Why an *asymmetric* AlphaZero

Classic AlphaZero (Go, chess) assumes a **symmetric** game: two players with the same rules, the same actions and the same view of the board. A single network serves both.

Almost nothing in the real world works like that. METIS-Core extends AlphaZero to support:

- **Different teams:** 4 defenders against 2 attackers.
- **One policy head per agent:** each substation and each hacker has its **own action space**, with a different size in each case (A has 4 actions, D has 8, the Striker 12).
- **Asymmetric observations:** Blue sees data that may be falsified; Red sees the truth. This is genuinely imperfect information, not a shared board.
- **One brain per team:** a shared trunk with several policy heads plus a value head.
- **MCTS that simulates the opponent with its own model:** when Blue plans, it uses Red's network to predict how the attacker will respond.
- **Optional domain heuristic:** you can blend your expert knowledge with the network's evaluation during the search.

---

## 3. Architecture at a glance

```
                 ┌──────────────────────────────────────────┐
                 │      Metis::AlphaGoZeroAsymTrainer        │
                 │  self-play + MCTS + training              │
                 └──────────────┬───────────────────────────┘
                                │
          ┌─────────────────────┼─────────────────────┐
          ▼                                           ▼
 MultiHeadAgent (BLUE)                       MultiHeadAgent (RED)
 ├─ head "subA"  (4 actions)                 ├─ head "striker" (12)
 ├─ head "subB"  (6)                         ├─ head "spoofer" (11)
 ├─ head "subC"  (6)                         └─ valueHead
 ├─ head "subD"  (8)
 └─ valueHead
          ▲                                           ▲
          └─────────── GridEnvironment ───────────────┘
              (inherits Metis::IAlphaGoZeroState)
              6 GridAgent (inherit Metis::IAgent)
```

**You only write three things:**

1. **The agents** (`GridAgent`).
2. **The environment** (`GridEnvironment`): rules, observations, legal actions and who wins.
3. **A `main`** of a few lines that builds the teams and starts training.

The networks, MCTS, self-play, replay and training are provided by METIS-Core.

---

## Step 1: Players and teams

METIS-Core identifies each player with a **sequential ID `0..N-1`**. Define the IDs and a function that tells which team each one belongs to (`Kernel/Actions.h`):

```cpp
enum PlayerID : int {
    PID_A = 0, PID_B = 1, PID_C = 2, PID_D = 3,   // Blue (substations)
    PID_STRIKER = 4, PID_SPOOFER = 5,             // Red (hackers)
    MAX_PLAYERS = 6
};
enum TeamID : int { TEAM_BLUE_ID = 0, TEAM_RED_ID = 1, TEAM_DRAW_ID = -1 };

constexpr int teamDe(int pid) { return pid <= PID_D ? TEAM_BLUE_ID : TEAM_RED_ID; }
```

> 💡 **Rule:** IDs must be **consecutive and start at 0**. That is how the engine indexes turns, heads and statistics.

---

## Step 2: One action space per head

Each network head has **exactly** as many outputs (logits) as its agent has actions. The cleanest way is one `enum` per agent type, ending in `COUNT`:

```cpp
// BLUE: each substation has its own repertoire
enum class ACT_A : uint8_t { NoOp = 0, ToggleL1, ToggleL2, Verify, COUNT };                      // 4
enum class ACT_B : uint8_t { NoOp = 0, Shed10, Shed25, Restore, Verify, Quarantine, COUNT };      // 6 (B and C)
enum class ACT_D : uint8_t { NoOp = 0, Shed10, Shed25, Restore, ToggleL1, ToggleL2,
                             Verify, Quarantine, COUNT };                                         // 8

// RED
enum class STRIKER_ACTIONS : uint8_t {
    NoOp = 0,
    Botnet10_B = 1, Botnet10_C = 2, Botnet10_D = 3,
    Botnet25_B = 4, Botnet25_C = 5, Botnet25_D = 6,
    Release = 7,
    Trip_AB = 8, Trip_AC = 9, Trip_BD = 10, Trip_CD = 11,
    MAX_ACTIONS = 12
};
```

There are **two levels of legality**:

- **Static:** which actions *exist* for that agent. The `enum` itself defines it, and substation A has no `Shed`.
- **Dynamic:** which actions are legal *right now*, for example whether there is budget left or load to restore. The environment computes it at runtime (Step 4).

Red's actions also have a **cost**, which prevents attacking non-stop:

```cpp
constexpr int strikerCoste(STRIKER_ACTIONS a) {
    switch (a) {
    case STRIKER_ACTIONS::Botnet10_B: /* ... */ return 1;
    case STRIKER_ACTIONS::Botnet25_B: /* ... */ return 2;
    case STRIKER_ACTIONS::Trip_AB:    /* ... */ return 4;
    default:                                    return 0;
    }
}
```

> 💡 **Game design:** budgets recharge gradually (1 point every 12 steps) instead of in chunks. With chunked recharges, Red always attacked at the same moments and Blue learned to anticipate it. It is a good example of how self-play **exploits any regularity** in your rules.

---

## Step 3: The agent

Each player is a `Metis::IAgent`. In this example there is **a single** agent type with a different **role**, because the physics lives in the environment:

```cpp
class GridAgent : public Metis::IAgent {
public:
    Rol  rol = Rol::Subestacion;          // Substation, StrikerHacker or SpooferHacker
    int  busOHacker = -1;
    GridEnvironment* _pEnv = nullptr;     // re-pointed when cloning (critical for MCTS!)

    GridAgent() = default;
    GridAgent(GridAgent& other);
    GridAgent& operator=(GridAgent& other);

    int  update(double) override { return 0; }   // the environment resolves the physics
    bool isValidAction(int) override;
    void endStep() override {}
};
```

### ⚠️ Copying the agent: the most important point in this whole tutorial

MCTS **clones the environment thousands of times** per decision, and the agents with it. The `IAgent` base holds the pointer to the brain (`_pAgentImpl`). **The copy must never touch it:**

```cpp
GridAgent::GridAgent(GridAgent& other) : Metis::IAgent()   // base default-constructed
{
    rol        = other.rol;
    busOHacker = other.busOHacker;
    _pEnv      = other._pEnv;            // the environment clone re-points it to itself
    this->setID(other.getID());
    // _pAgentImpl is NOT copied: each clone has its own, so the brain is never shared or freed twice
}
```

If you copy `_pAgentImpl`, you will get brains shared between clones and a *double free* when they are destroyed. These errors are intermittent and very hard to debug.

---

## Step 4: The environment

The environment inherits from `Metis::IAlphaGoZeroState<TState, TAction>`. This is where **all** the knowledge of the game lives.

```cpp
struct TGridState  { int dummy = 0; };     // the real state lives in the members
struct TGridAction { int id = 0; int toIndex(int) const { return id; } };

class GridEnvironment : public Metis::IAlphaGoZeroState<TGridState, TGridAction> { ... };
```

### What you have to implement

| Method | What it does |
|---|---|
| `reset(int)` | Prepares a new day |
| `getCurrentPlayerID()` | Whose turn it is |
| `getTeamID(pid)` | Which team a player belongs to |
| `getState(pid)` | **The player's observation** (vector of floats) |
| `isStateRelativeToPlayer()` | `true` if each player sees something different |
| `getPossibleActions(pid, out)` | Actions **legal right now** (local indices of its head) |
| `getNumActions(pid)` | Width of that player's head |
| `applyAction(idx)` | Applies the current player's action and **passes the turn** |
| `update(dt)` | Advances the physics (here, when each round closes) |
| `isTerminal()` / `getWinner()` | End of game and winner |
| `getReward(teamID)` | Final reward in `[-1, 1]` (zero-sum) |
| `clone()` + copy constructor | **Deep copy** for MCTS |
| `getMaterialHeuristicValue(pid)` | (Optional) your domain heuristic |

### 4.1 Asymmetric observations

This is where the information asymmetry lives. Each side receives a vector of **fixed** size:

```cpp
#define GRID_BLOCK_INPUTS 28
#define TEAM_BLUE_INPUTS  (GRID_BLOCK_INPUTS + 4)               // 32
#define TEAM_RED_INPUTS   (GRID_BLOCK_INPUTS + 3 + kNumBuses)   // 35

std::vector<float> GridEnvironment::getState(int currentPlayer)
{
    std::vector<float> v;
    if (teamDe(currentPlayer) == TEAM_BLUE_ID)
    {
        BlueObs o;
        fillGridBlock(o.grid, /*measured=*/true);   // Blue sees what is MEASURED (spoofable)
        o.oneHotSub[currentPlayer] = 1.0f;          // which substation am I
        o.toVector(v);                              // -> 32 floats
    }
    else
    {
        RedObs o;
        fillGridBlock(o.grid, /*measured=*/false);  // Red sees the TRUTH
        // + budget, which hacker am I, and the "freshness" of each botnet
        o.toVector(v);                              // -> 35 floats
    }
    return v;
}
```

> 💡 **Recommended pattern:** describe each observation as a `struct` with its own `toVector()` (`BusObs`, `LineObs`, `GridBlockObs`, `BlueObs`, `RedObs`). The code documents itself and the size checks itself.

> 💡 **Coordination:** Red's vector includes `botnetReciente[bus]`, which is 1 right after the Striker attacks a node and decays over time. This is what lets the Spoofer **synchronize**: it learns to freeze *that* meter *in time*.

### 4.2 Legal actions

`getPossibleActions` returns only the actions that are legal at this moment, as **local** indices of that player's head:

```cpp
void GridEnvironment::getPossibleActions(int playerID, std::vector<TGridAction>& out)
{
    out.clear();
    int idx[kRedMaxActions];                          // the widest head
    const int n = legalActions(playerID, idx, kRedMaxActions);
    for (int j = 0; j < n; ++j) { TGridAction a; a.id = idx[j]; out.push_back(a); }
}
```

MCTS only explores these actions, and the network's policy is masked with them.

### 4.3 Turns and physics: the "round" pattern

The 6 players act **in turn** within the same round. Each action only records an **intent**; the physics is resolved **once per round**:

```cpp
void GridEnvironment::applyAction(int accionIdx)
{
    const int pid = _currentPlayer;
    // ... apply the effect of 'pid''s action (shed, botnet, freeze...) ...
    advanceToNextActivePlayer(false);                 // pass the turn
}

void GridEnvironment::update(float)
{
    if (_currentPlayer == PID_STRIKER)                // end of round
    {
        resolverFisicaFinDeRonda();                   // flows, cascades, demand served
        ++_step;                                      // advance the clock (5 minutes)
        // budget recharge and new demand for the time of day
    }
}
```

This turns a **simultaneous** game (everyone acts at the same instant) into a **turn-based** one, which is what MCTS needs, without changing the physical outcome.

### 4.4 End of game and reward

```cpp
bool GridEnvironment::isTerminal()
{
    if (_fraccionServida < kUmbralBlackout) { _winner = TEAM_RED_ID;  return true; }   // blackout
    if (_step >= MAX_STEPS)                 { _winner = TEAM_BLUE_ID; return true; }   // survived the day
    _winner = TEAM_DRAW_ID;
    return false;
}

double getReward(int teamID)
{
    // GRADED reward: Blue scores by the average service of the day, in [-1, 1]
    double avg   = _sumaFraccionServida / _rondasContadas;
    double vBlue = (_winner == TEAM_RED_ID) ? -1.0 : margenServicio(avg);
    return (teamID == TEAM_BLUE_ID) ? vBlue : -vBlue;   // zero-sum
}
```

> 💡 **Graded instead of binary reward:** with only +1 or −1, Blue cannot tell "I barely survived" from "I survived without breaking a sweat". Scoring the service margin gives a much richer learning signal.

### 4.5 Cloning for MCTS

`clone()` must return a **deep, independent copy**:

```cpp
Metis::IAlphaGoZeroStateBase* clone() override { return new GridEnvironment(*this); }

GridEnvironment::GridEnvironment(GridEnvironment& o)
{
    // ... copy ALL the state: physics, deceptions, budgets, counters ...
    for (int i = 0; i < kNumBuses; ++i) {
        _subestacion[i]       = o._subestacion[i];
        _subestacion[i]._pEnv = this;             // re-point to the clone!
        addAgent(&_subestacion[i]);               // register the clone's agents
    }
    _striker = o._striker;  _striker._pEnv = this;  addAgent(&_striker);
    _spoofer = o._spoofer;  _spoofer._pEnv = this;  addAgent(&_spoofer);
}
```

**Three rules:**

1. **Copy everything** that affects the future of the game. If you forget a field, the clones diverge from the real game and MCTS plans over a world that does not exist.
2. **Re-point** each agent's `_pEnv` to the clone.
3. **Register** the clone's agents with `addAgent`.

### 4.6 (Optional) Your expert knowledge: the heuristic

`getMaterialHeuristicValue` evaluates a state with your domain knowledge, and MCTS blends it with the network's value head. In SmartGridGuardian it penalizes Blue for:

- the service margin relative to the blackout threshold;
- congested lines (risk of a cascade);
- the **hidden threat**: an active botnet **and** a frozen meter on the same node, weighted by how well synchronized the attack is.

```cpp
float GridEnvironment::getMaterialHeuristicValue(int playerToEvaluate)
{
    float vBlue = margenServicio(_fraccionServida);
    // - visible congestion
    // - hidden threat (synchronized botnet + freeze)
    // ...
    return (teamDe(playerToEvaluate) == TEAM_BLUE_ID) ? vBlue : -vBlue;   // zero-sum
}
```

This greatly speeds up the early stages of training, when the network still knows nothing.

---

## Step 5: Build the two multi-head brains

With the environment ready, building the teams is declarative thanks to `MultiHeadBrainBuilder`:

```cpp
GridEnvironment env;
env.reset(0);
bool gpu = Metis::isCUDAavailable();

// --- BLUE TEAM: one head per substation + value head
Metis::MultiHeadBrainBuilder bBlue;
Metis::MultiHeadAgent* teamBlue = bBlue
    .addHead("subA", &env._subestacion[0], ACTIONS_SUB_A)   // 4 logits
    .addHead("subB", &env._subestacion[1], ACTIONS_SUB_B)   // 6
    .addHead("subC", &env._subestacion[2], ACTIONS_SUB_C)   // 6
    .addHead("subD", &env._subestacion[3], ACTIONS_SUB_D)   // 8
    .addValueHead("valueHead")
    .setNumberInputsOutputs(TEAM_BLUE_INPUTS)               // 32 inputs
    .useGPU(gpu).build();
teamBlue->setID(TEAM_BLUE_ID);

// --- RED TEAM
Metis::MultiHeadBrainBuilder bRed;
Metis::MultiHeadAgent* teamRed = bRed
    .addHead("striker", &env._striker, ACTIONS_STRIKER)     // 12
    .addHead("spoofer", &env._spoofer, ACTIONS_SPOOFER)     // 11
    .addValueHead("valueHead")
    .setNumberInputsOutputs(TEAM_RED_INPUTS)                // 35 inputs
    .useGPU(gpu).build();
teamRed->setID(TEAM_RED_ID);

// Player IDs (the same ones the environment uses)
for (int i = 0; i < 4; ++i) env._subestacion[i].setID(PID_A + i);
env._striker.setID(PID_STRIKER);
env._spoofer.setID(PID_SPOOFER);
```

- **`addHead(name, agent, nActions)`:** a policy head with its own width.
- **`addValueHead`:** the head that estimates who is winning.
- **`setNumberInputsOutputs(n)`:** the team's observation size. It must match `getState(pid).size()` **exactly**.

---

## Step 6: Train with self-play

```cpp
Metis::AlphaGoZeroAsymTrainer trainer;
trainer.setMaterialHeuristicWeight(0.3);                 // 30% heuristic + 70% network (0 = network only)
trainer.setCallbackPerStep(nullptr, onStepTraining);     // every real decision
trainer.setCallbackEndEpisode(nullptr, onEndEpisode);    // end of every game
trainer.training(&env, teamBlue, teamRed, gpu);          // blocking
```

The trainer handles the whole loop: self-play with asymmetric MCTS, storing the games, training the policy and value heads of **both teams**, and saving the `.ia` models.

### Callbacks: your window into training

**`onStepTraining`** is called on every **real** decision. Use it to draw, log actions or measure coordination:

```cpp
static void onStepTraining(void*, Metis::TSEARCHACTIONASYM* m)
{
    GridEnvironment* env = static_cast<GridEnvironment*>(m->pState);
    // m->playerID = who just acted
    GridRenderState s;
    env->fillRenderState(s, true);
    g_viz.dibujar(s);
}
```

**`onEndEpisode`** is called at the end of every game, with the metrics of both teams:

```cpp
static void onEndEpisode(void*, Metis::TSEARCHACTIONASYM* m)
{
    GridEnvironment* env = static_cast<GridEnvironment*>(m->pState);
    // env->getWinner(), m->metricsTeamBlue, m->metricsTeamRed, m->winRateModel ...
    // per-head loss: m->metricsTeamBlue._headMetric["subA"]._meanPolicyHeadLoss
}
```

> ⚠️ **Log only in the callbacks.** Never write logs inside `applyAction` or `getMaterialHeuristicValue`: those functions also run on the MCTS **clones**, which are hypothetical states, and you would fill your statistics with moves that never happened.

> 💡 **Save the best model, not the last one.** In an adversarial game, the win rate **oscillates**: when one side improves, the other adapts (an arms race). The example saves `blueTeam_best.ia` whenever Blue beats its own record of wins over a 50-game window:
>
> ```cpp
> if (blueWins50 > g_bestBlueWins50) {
>     g_bestBlueWins50 = blueWins50;
>     g_teamBlue->saveIAModel((char*)"blueTeam_best.ia");
> }
> ```

---

## Step 7: Play against the AI

Now **you** are the hacker. Build the teams just as in training, load the models and alternate between your turns and the AI's:

```cpp
teamBlue->loadIAModel((char*)"blueTeam.ia", gpu);
teamRed->loadIAModel((char*)"redTeam.ia", gpu);   // Blue's MCTS uses it to simulate the opponent
teamBlue->setTeamBlueRed(teamBlue, teamRed);      // the asymmetric search needs both models

while (!env.isTerminal())
{
    const int pid = env.getCurrentPlayerID();
    int accion;
    if (teamDe(pid) == TEAM_RED_ID)
        accion = pedirAccionHumanaRed(env, pid);          // you choose from a menu
    else
        accion = teamBlue->selectActionAsym(&env, pid);   // the AI defends with MCTS

    env.applyAction(accion);
    env.update(1.0f);
}
```

Notice `setTeamBlueRed`: even though only the Blue AI is playing, **it needs the Red model**. Its MCTS imagines what you would do in each branch of the tree, using what Red learned during self-play.

---

## Common pitfalls (and how to avoid them)

| Symptom | Likely cause | Fix |
|---|---|---|
| Dimension error in the forward pass | `getState(pid).size()` does not match `setNumberInputsOutputs` | Check the size with an `assert` at the end of `getState` |
| A head chooses impossible actions | `getPossibleActions` returns indices outside its head | Use **local** indices `0..nActions-1` of that head |
| Intermittent crashes or *double free* | The `IAgent` copy touches `_pAgentImpl` | Default-construct the base in the agent's copy constructor |
| MCTS makes absurd moves | The clone misses a field, or `_pEnv` points to the original | Copy **all** the state and re-point the agents |
| Impossible statistics | Logs inside `applyAction` or the heuristic (they run on the clones) | Log only in `onStepTraining` and `onEndEpisode` |
| One side always wins | Unbalanced rules (threshold, costs, budgets) | Tune the rules: self-play exploits any imbalance |
| Strange game values | Heuristic or reward that is not zero-sum | Return `v` for one team and `-v` for the other |

---

## Bring it to your own domain

The SmartGridGuardian pattern works for any **asymmetric team-vs-team** conflict:

| Domain | Blue | Red |
|---|---|---|
| Cybersecurity | SOC team | Coordinated attackers |
| Tactical simulation | Zone defense | Attacking force |
| Logistics | Distribution network | Disruptions and failures |
| Strategy games | Faction A | Faction B, with other units and other rules |

**Recipe:**

1. Define the **players** (IDs `0..N-1`) and the **teams**.
2. Define **one action `enum` per agent type**.
3. Write the **agent** (`IAgent`) with a copy that does not touch the brain.
4. Write the **environment** (`IAlphaGoZeroState`): per-player observations, legal actions, turns, zero-sum end and reward, and a deep clone.
5. Build the **brains** with `MultiHeadBrainBuilder`.
6. Run the **`AlphaGoZeroAsymTrainer`** and watch the strategies emerge.

---

*SmartGridGuardian is a METIS-Core example: native C++ multi-agent AI on top of LibTorch.*
