# Building your first DQN application with Metis-Core

### A step-by-step guide based on the `DQN_Pong` example

This guide shows how to train an agent with **Deep Q-Learning (DQN)** using **Metis-Core**, taking the `DQN_Pong` example as a reference. By the end you will know which pieces Metis-Core asks you to write, how they fit together and how to adapt them to your own problem.

You do **not** need to implement any of the DQN machinery yourself: the Q-network, target network, replay buffer, ε-greedy exploration, loss and optimizer are all handled by Metis-Core. Your job is to describe **the world** and **the agent**.

### Demo: DQN_Pong

Check out the IA player in action! 🚀

[![Watch the demo on YouTube](https://img.youtube.com/vi/iRe3HKzdlzA/0.jpg)](https://www.youtube.com/watch?v=iRe3HKzdlzA)

> **Note:** Best experienced with audio! The video features a custom soundtrack composed for the Metis-Core project.

- **Video Link:** [https://youtu.be/iRe3HKzdlzA](https://youtu.be/iRe3HKzdlzA)



---

## Table of contents

1. [Who is this guide for](#1-who-is-this-guide-for)
2. [DQN in two minutes](#2-dqn-in-two-minutes)
3. [The big picture: what Metis does and what you do](#3-the-big-picture-what-metis-does-and-what-you-do)
4. [The DQN_Pong example](#4-the-dqn_pong-example)
5. [Step 1 – Define the raw state](#5-step-1--define-the-raw-state)
6. [Step 2 – Write the agent (`Metis::IAgent`)](#6-step-2--write-the-agent-metisiagent)
7. [Step 3 – Write the environment (`Metis::Enviroment`)](#7-step-3--write-the-environment-metisenviroment)
8. [Step 4 – Design the reward](#8-step-4--design-the-reward)
9. [Step 5 – Train](#9-step-5--train)
10. [Step 6 – Use the trained model](#10-step-6--use-the-trained-model)
11. [Checklist for your own application](#11-checklist-for-your-own-application)
12. [Common pitfalls](#12-common-pitfalls)
13. [Minimal template](#13-minimal-template)

---

## 1. Who is this guide for

- **Programmers** who want to add a learning agent to a game or simulation without writing a DQN from scratch.
- **Reinforcement learning researchers** who want a quick C++ test bed for new environments and reward designs.
- **Beginners** who know some C++ and want to see a complete, working DQN they can modify.

Prerequisites: basic C++ (classes, virtual methods, pointers). For the visual part, `DQN_Pong` uses [raylib](https://www.raylib.com/), but Metis-Core itself does not depend on any graphics library.

---

## 2. DQN in two minutes

In reinforcement learning an **agent** interacts with an **environment** in a loop:

```
        state s ──────────►  AGENT  ──────────► action a
           ▲                                       │
           │                                       ▼
           └──── new state s', reward r ◄──── ENVIRONMENT
```

The agent wants to maximize the sum of future rewards. **DQN** learns a function **Q(s, a)**: "how good is it to take action `a` in state `s`". It is a neural network whose:

- **inputs** are the numbers that describe the state (in Pong: paddle position, ball position, ball direction),
- **outputs** are one value per possible action (in Pong: 3 outputs → UP, NOMOVE, DOWN).

The agent picks the action with the highest Q value (plus some random exploration at the start). Every transition `(s, a, r, s', done)` is stored in a **replay buffer**, and the network is trained on random batches from it.

With Metis-Core you only need to answer three questions about your problem:

| Question | Where you answer it |
|---|---|
| What does the agent see? | the **state** and its serialization |
| What can the agent do? | the **actions** |
| What is good and what is bad? | the **reward** |

---

## 3. The big picture: what Metis does and what you do

| Metis-Core does | You write |
|---|---|
| Creates the Q-network (`createBrain`) | A class derived from **`Metis::IAgent`** (your agent) |
| Exploration (ε-greedy), replay buffer, target network | A class derived from **`Metis::Enviroment`** (your world) |
| Training loop, loss, optimizer | A plain struct with the **raw state** |
| Saving / loading the model (`saveIAModel` / `loadIAModel`) | The **reward** function |
| Training metrics (`TAGENTMETRICS`) and callbacks | Optional callbacks to draw or log progress |

### The cycle of one training step

`Metis::AgentTrainerDQN` runs this sequence on every step:

```
getState  ->  serizalizeState  ->  (network chooses action)  ->  applyAction
          ->  agent update(dt)  ->  getState  ->  calculateReward
```

- `getState` / `serizalizeState`: read the world and turn it into the network inputs.
- `applyAction`: apply the chosen action and **advance the world one tick**.
- `update(dt)`: the agent's own per-step logic; `dt` comes from `getDeltaTime()` of your environment.
- `calculateReward`: return the reward and tell Metis whether the episode has ended.
- `reset`: called at the start of every episode.

---

## 4. The DQN_Pong example

The left paddle (blue) is the agent. It learns to hit the ball against a procedural opponent (red) that simply follows the ball.

| File | Role |
|---|---|
| `DQN_Pong.cpp` | `main()`, menu, training callbacks and Play mode |
| `Kernel/Racket.h/.cpp` | **The agent**: the paddle, derived from `Metis::IAgent` |
| `Kernel/PongEnviroment.h/.cpp` | **The environment**: derived from `Metis::Enviroment` |
| `Kernel/Ball.h/.cpp` | Ball physics (plain C++, no Metis) |
| `PongRender.h/.cpp` | Drawing with raylib (optional) |

When you run it you get a menu:

- **[T] Training**: the left paddle learns. Every time it improves, the model is saved to `pongModel.ia`.
- **[P] Play**: the trained network plays against you (UP / DOWN arrows).

---

## 5. Step 1 – Define the raw state

The **raw state** is a plain struct with everything you need to know about the world at a given moment: what the network will see, plus anything you need to compute the reward.

```cpp
// "Raw" state of the environment. Metis stores it with memcpy (State::copyState)
// in the replay buffer -> it must be trivially copyable and < 1000 bytes.
typedef struct _state
{
    float leftPlayerPositionY;
    float ballPosX, ballPosY;
    float ballVelX, ballVelY;      // normalized velocity (unit vector)
    int   leftDirection;           // Racket::DIRECTION after the action
    int   leftHit;                 // 1 if the left paddle hit the ball in this step
    int   leftMiss;                // 1 if the ball went past the left paddle
    int   rightMiss;               // 1 if the ball went past the right paddle
    _state() { memset(this, 0, sizeof(struct _state)); }
} TSTATE;
```

> ⚠️ **Rules for the raw state**
> - Metis copies it byte by byte (`memcpy`). Use only plain types (`int`, `float`, fixed arrays). **No** `std::vector`, `std::string` or pointers.
> - Keep it **under 1000 bytes**.

---

## 6. Step 2 – Write the agent (`Metis::IAgent`)

Your agent class derives from `Metis::IAgent` and keeps its own physics. Metis requires three methods:

```cpp
class Racket : public Metis::IAgent
{
public:
    // Actions = output indices of the DQN network (0..NUM_ACTIONS-1)
    enum ACTIONS
    {
        ACTION_UP     = 0,
        ACTION_NOMOVE = 1,
        ACTION_DOWN   = 2,
        NUM_ACTIONS   = 3
    };

    // --------- Metis::IAgent methods ---------
    virtual int  getActionProcedural(Metis::State& state) override;
    virtual int  update(double delta_time) override;
    virtual bool isValidAction(int iAction) override { return iAction >= 0 && iAction < NUM_ACTIONS; }

    // ... your own physics: position, movement, etc.
};
```

| Method | What it is for |
|---|---|
| `getActionProcedural(state)` | A "hand-made" policy. In Pong it is used by the **opponent** (it follows the ball) and works as a **baseline** to compare with the DQN. |
| `update(dt)` | The agent's own logic on every step (in Pong: computes the paddle's vertical velocity). |
| `isValidAction(i)` | Tells Metis which action indices are valid. |

> 💡 **Actions are just indices.** Output `0` of the network is action `0`, output `1` is action `1`, and so on. Define them as an `enum` that starts at 0 and ends with `NUM_ACTIONS`; you will need that number to create the network.

The network itself is **inherited**: you create it with `createBrain()` and save/load it with `saveIAModel()` / `loadIAModel()`. You never touch it directly.

---

## 7. Step 3 – Write the environment (`Metis::Enviroment`)

The environment owns the world (the ball, both paddles) and implements these methods:

```cpp
class PongEnviroment : public Metis::Enviroment
{
public:
    virtual void  getState(Metis::State* pState) override;
    virtual void  serizalizeState(void* state, std::vector<float>* stateVector) override;
    virtual void  applyAction(Metis::IAgent* pAgent, int actionId) override;
    virtual float calculateReward(Metis::State& state, int* pDone) override;
    virtual std::vector<Metis::TMULTIHEAD> calculateRewards(Metis::State& state, int* pDone) override;
    virtual void  reset() override;
    virtual float getDeltaTime();
};
```

### `reset()`: start a new episode

Put the world back in a starting position. Add randomness so the agent sees varied situations:

```cpp
void PongEnviroment::reset()
{
    const int middleY = _h / 2;

    _pPlayerLeft->Reset();
    _pPlayerLeft->SetPosition(_margin, middleY);
    _pPlayerLeft->SetState(playing);

    _pPlayerRight->Reset();
    _pPlayerRight->SetPosition(_w - _margin, middleY);
    _pPlayerRight->SetState(playing);

    // serve from the right towards the agent, random height and angle
    const int y       = 10 + rand() % (_h - 20 + 1);
    const int heading = 150 + rand() % (60 + 1);     // 180 ± 30 degrees

    _pBall->SetPosition(_w - 15, y);
    _pBall->SetVelocity(6);
    _pBall->SetHeading(heading);
}
```

### `getState()`: hand the raw state to Metis

Fill your `TSTATE` and copy it into the `Metis::State`:

```cpp
void PongEnviroment::getState(Metis::State* pState)
{
    TSTATE pongState;
    getStateFromEnviroment(&pongState);          // your own helper that fills TSTATE
    pState->copyState(&pongState, sizeof(TSTATE));
}
```

### `serizalizeState()`: the network inputs

This is **what the network sees**. Push one `float` per input, normalized to roughly **[-1, 1]**:

```cpp
void PongEnviroment::serizalizeState(void* /*state*/, std::vector<float>* stateVector)
{
    TSTATE s;
    getStateFromEnviroment(&s);

    const float halfW = _w * 0.5f;
    const float halfH = _h * 0.5f;

    stateVector->push_back((s.leftPlayerPositionY - halfH) / halfH);  // [0] paddle
    stateVector->push_back((s.ballPosX - halfW) / halfW);             // [1] ball X
    stateVector->push_back((s.ballPosY - halfH) / halfH);             // [2] ball Y
    stateVector->push_back(s.ballVelX);                               // [3] direction X
    stateVector->push_back(s.ballVelY);                               // [4] direction Y
    // (NUM_INPUTS = 5; if you add inputs, update NUM_INPUTS)
}
```

> ⚠️ **The number of values you push must be exactly the number of inputs of the network** (`NUM_INPUTS`, used in `createBrain`). If you add an input here, update that constant.

> 💡 **Why normalize?** A neural network learns much better when all inputs are in similar ranges. Raw pixel positions (0..800) next to directions (-1..1) make training slow and unstable.

### `applyAction()`: apply the action and advance the world

```cpp
void PongEnviroment::applyAction(Metis::IAgent* pAgent, int actionId)
{
    Racket* pRacket = static_cast<Racket*>(pAgent);

    pRacket->HitTheBall(false);
    pRacket->ApplyActionId(actionId);

    // Only the trained agent advances the world (1 action = 1 tick)
    if (pRacket == _pPlayerLeft)
    {
        // the opponent: procedural policy (Training) or human (Play)
        int rightAction = rightManualAction;
        if (rightProcedural)
        {
            Metis::State dummy;
            rightAction = _pPlayerRight->getActionProcedural(dummy);
        }
        _pPlayerRight->HitTheBall(false);
        _pPlayerRight->ApplyActionId(rightAction);

        stepSimulation();   // moves the ball, checks hits and misses
    }
}
```

> 💡 **Advance the physics inside `applyAction()` of the trained agent.** This way one action is always exactly one tick of the world, and your simulation does not depend on the exact order in which Metis calls the other methods.

### `calculateReward()`: reward and end of episode

See the next section.

### `calculateRewards()`: only for multi-head networks

For a single DQN agent, return an empty vector:

```cpp
std::vector<Metis::TMULTIHEAD> PongEnviroment::calculateRewards(Metis::State&, int*)
{
    return std::vector<Metis::TMULTIHEAD>();
}
```

### `getDeltaTime()`: the time step

Since **Metis-Core 0.4.1**, the trainer reads the time step from your environment (`pEnv->getDeltaTime()`) and passes it to the agent's `update(dt)`:

```cpp
float PongEnviroment::getDeltaTime()
{
    return 2.0;
}
```

---

## 8. Step 4 – Design the reward

`calculateReward` returns the reward of the last step and sets `*pDone`:

| `*pDone` | Meaning |
|---|---|
| `0` | the episode continues |
| `1` | the episode ends with a **WIN** |
| `-1` | the episode ends with a **LOSS** |

```cpp
float PongEnviroment::calculateReward(Metis::State& /*state*/, int* pDone)
{
    *pDone = 0;

    // The CURRENT state of the environment is read (after applyAction)
    TSTATE s;
    getStateFromEnviroment(&s);

    // --- terminal states ---
    if (s.leftHit)   { *pDone =  1; return rewardHit;  }   // +1.0
    if (s.leftMiss)  { *pDone = -1; return rewardMiss; }   // -1.0
    if (s.rightMiss) { *pDone =  1; return 0.0f;       }   // the opponent missed

    // --- shaping: move towards where the ball will arrive ---
    float impactY = 0.0f;
    const bool comingToMe = getPredictedImpactLeft(&impactY);
    const float halfLen = _pPlayerLeft->GetLen() / 2.0f;

    int wantedDir = Racket::DIR_NOMOVE;
    if (comingToMe && impactY >= 0 && impactY <= _h)
    {
        const float diffY = impactY - s.leftPlayerPositionY;
        if (diffY >  halfLen) wantedDir = Racket::DIR_DOWN;
        if (diffY < -halfLen) wantedDir = Racket::DIR_UP;
    }

    return (s.leftDirection == wantedDir) ? pseudoReward : -pseudoReward;   // ±0.1
}
```

The Pong reward has two parts:

1. **Terminal reward** (+1 / −1): the real goal. Hit the ball = win, miss it = loss.
2. **Shaping reward** (±0.1 per step): a small hint on every step. The paddle gets +0.1 when it moves towards the point where the ball will arrive, and −0.1 otherwise.

> 💡 **Why shaping?** With only the terminal reward, the agent receives feedback once per episode and learns very slowly at the beginning. A small per-step hint speeds up learning a lot. Keep it **much smaller** than the terminal reward (here 0.1 vs 1.0), so that the agent still cares about the real goal and does not just "farm" the hint.

> 💡 **Read the current state of the environment** in `calculateReward` (as above). It is computed right after `applyAction`, so it always reflects the result of the action.

---

## 9. Step 5 – Train

### Create the network and register the agent

```cpp
int main()
{
    srand((unsigned)time(nullptr));     // seed only once

    // the network: NUM_INPUTS inputs (serizalizeState) and 3 actions (UP, NOMOVE, DOWN)
    leftPaddle.createBrain(NUM_INPUTS, Racket::NUM_ACTIONS);
    environment.addAgent((Metis::IAgent*)&leftPaddle);

    // ...
}
```

`createBrain(inputs, outputs)`: **inputs** = values pushed in `serizalizeState`, **outputs** = number of actions.

### Launch the trainer

```cpp
void doTraining()
{
    Metis::AgentTrainerDQN trainer;
    trainer.setCallbackPerStep(onStepTraining);       // called on every step
    trainer.setCallbackEndEpisode(onEndEpisode);      // called at the end of every episode
    trainer.training(&environment, &leftPaddle, nullptr);   // blocks until stopped
}
```

`training()` **blocks** until training stops. Use the callbacks to draw, log and save.

### The callbacks and `TAGENTMETRICS`

Callbacks are plain C functions. Metis passes a `TAGENTMETRICS*` with the training progress:

| Field | Meaning |
|---|---|
| `episode` | current episode number |
| `_meanLoss` | mean loss of the network |
| `countSuccesX50episodes` | number of wins (`*pDone == 1`) in the last 50 episodes |
| `bForceStopTraining` | set it to `true` to stop training |

```cpp
// Metis calls this on every training step
void onStepTraining(void* /*pSender*/, Metis::TAGENTMETRICS* pMetrics)
{
    drawScene(TextFormat("TRAINING  ep:%d  loss:%.2f  hits/50 %d",
                         pMetrics->episode, pMetrics->_meanLoss, pMetrics->countSuccesX50episodes));

    // ESC or close the window -> stop the training
    if (WindowShouldClose())
        pMetrics->bForceStopTraining = true;
}

// Metis calls this at the end of each episode
void onEndEpisode(void* /*pSender*/, Metis::TAGENTMETRICS* pMetrics)
{
    // save the model when the number of hits in 50 episodes improves
    if (pMetrics->countSuccesX50episodes > gMaxHitsX50)
    {
        gMaxHitsX50 = pMetrics->countSuccesX50episodes;
        leftPaddle.saveIAModel((char*)MODEL_FILE);
    }
}
```

> 💡 **Save the best model, not the last one.** DQN training is noisy: performance can go up and down. Saving only when `countSuccesX50episodes` improves keeps the best network you have seen.

> 💡 **Drawing slows training down.** In `DQN_Pong` you can press SPACE to hide the drawing and train faster.

> ℹ️ Callbacks are C functions without context, which is why `DQN_Pong` keeps the game objects (ball, paddles, environment) as **global variables**.

---

## 10. Step 6 – Use the trained model

To use the network without exploration (Play mode), load the model and ask it for the best action:

```cpp
leftPaddle.loadIAModel((char*)MODEL_FILE);

while (!WindowShouldClose())
{
    // the network decides, without exploration
    Metis::State state;
    environment.getState(&state);
    environment.serizalizeState(&state, &state._stateVector);
    int aiAction = leftPaddle.predictAction(state);

    // one world tick
    environment.applyAction(&leftPaddle, aiAction);
    leftPaddle.update(0.5);

    // ... draw, check points, etc.
}
```

The model you load must have been created with **the same number of inputs and outputs** (`createBrain` must be called with the same values before `loadIAModel`).

---

## 11. Checklist for your own application

1. **Actions**: list them as an `enum` starting at 0 and ending with `NUM_ACTIONS`.
2. **Raw state**: a plain struct (`TSTATE`) with everything you need, under 1000 bytes, no pointers.
3. **Agent**: derive from `Metis::IAgent`; implement `getActionProcedural`, `update` and `isValidAction`.
4. **Environment**: derive from `Metis::Enviroment`; implement `reset`, `getState`, `serizalizeState`, `applyAction`, `calculateReward`, `calculateRewards` (empty) and `getDeltaTime`.
5. **Inputs**: decide what the network sees, normalize to ~[-1, 1] and set `NUM_INPUTS` to the number of values.
6. **Reward**: a clear terminal reward (win / loss with `*pDone`) plus, if needed, a small shaping reward.
7. **main**: `srand` once, `createBrain(NUM_INPUTS, NUM_ACTIONS)`, `addAgent`, `AgentTrainerDQN::training`.
8. **Callbacks**: log progress, save the best model, allow stopping with `bForceStopTraining`.
9. **Baseline**: compare the trained agent with your `getActionProcedural` policy. If the DQN does not beat a simple hand-made rule, review the inputs and the reward.

---

## 12. Common pitfalls

| Symptom | Likely cause | Fix |
|---|---|---|
| Crash or garbage when training | `NUM_INPUTS` does not match the values pushed in `serizalizeState` | Count the `push_back` calls and update `NUM_INPUTS` |
| The agent never improves | Inputs not normalized (pixels vs. -1..1) | Normalize every input to ~[-1, 1] |
| The agent never improves | Only terminal reward, episodes are long | Add a small shaping reward |
| The agent learns a "trick" instead of the task | Shaping reward too large compared to the terminal reward | Keep shaping ~10 times smaller than the terminal reward |
| Many episodes start exactly the same | `srand(time(NULL))` inside `reset()` (1 s resolution) | Seed **once** in `main()` |
| Physics runs twice per step, or in the wrong order | The world is advanced outside `applyAction` | Advance it inside `applyAction` of the trained agent |
| Your header is silently ignored | Your file is called `Enviroment.h` with the same include guard as Metis' own `Enviroment.h` | Use a unique file name and guard (e.g. `PongEnviroment.h`, `__PONG_ENVIROMENT_H__`) |
| Name clashes with raylib (`CloseWindow`, `DrawText`, `Rectangle`...) | `<windows.h>` and `raylib.h` included in the same file | Include raylib in a single `.cpp` and exchange plain data (see `PongRender.h`) |
| Corrupted states in the replay buffer | `std::vector`, `std::string` or pointers inside `TSTATE` | Use only plain types and fixed arrays |

---

## 13. Minimal template

A skeleton to start a new DQN application. Replace the `TODO`s with your problem.

```cpp
// ---------------- MyEnviroment.h ----------------
#include "Metis-Core.h"
#include <vector>
#include <cstring>

typedef struct _myState
{
    float agentX, agentY;            // TODO: everything that describes your world
    float targetX, targetY;
    int   reachedTarget;             // 1 = win in this step
    int   failed;                    // 1 = loss in this step
    _myState() { memset(this, 0, sizeof(struct _myState)); }
} TMYSTATE;

#define NUM_INPUTS 4                 // TODO: number of values pushed in serizalizeState

class MyAgent : public Metis::IAgent
{
public:
    enum ACTIONS { ACTION_LEFT = 0, ACTION_RIGHT, ACTION_UP, ACTION_DOWN, NUM_ACTIONS };

    virtual int  getActionProcedural(Metis::State& state) override { return ACTION_LEFT; } // TODO: simple baseline
    virtual int  update(double delta_time) override { return 0; }                          // TODO: per-step logic
    virtual bool isValidAction(int iAction) override { return iAction >= 0 && iAction < NUM_ACTIONS; }

    float x = 0, y = 0;              // TODO: your agent's own data
};

class MyEnviroment : public Metis::Enviroment
{
public:
    MyAgent* pAgent = nullptr;

    void readState(TMYSTATE* s) { /* TODO: fill s from your world */ }

    virtual void reset() override
    {
        // TODO: random starting position
    }

    virtual void getState(Metis::State* pState) override
    {
        TMYSTATE s;
        readState(&s);
        pState->copyState(&s, sizeof(TMYSTATE));
    }

    virtual void serizalizeState(void* /*state*/, std::vector<float>* v) override
    {
        TMYSTATE s;
        readState(&s);
        v->push_back(s.agentX);      // TODO: normalized to ~[-1, 1]
        v->push_back(s.agentY);
        v->push_back(s.targetX);
        v->push_back(s.targetY);     // exactly NUM_INPUTS values
    }

    virtual void applyAction(Metis::IAgent* agent, int actionId) override
    {
        // TODO: apply the action to the agent AND advance the world one tick
    }

    virtual float calculateReward(Metis::State& /*state*/, int* pDone) override
    {
        *pDone = 0;
        TMYSTATE s;
        readState(&s);
        if (s.reachedTarget) { *pDone =  1; return  1.0f; }   // win
        if (s.failed)        { *pDone = -1; return -1.0f; }   // loss
        return -0.01f;                                         // TODO: optional shaping
    }

    virtual std::vector<Metis::TMULTIHEAD> calculateRewards(Metis::State&, int*) override
    {
        return std::vector<Metis::TMULTIHEAD>();               // single agent: not used
    }

    virtual float getDeltaTime() { return 1.0f; }
};

// ---------------- main.cpp ----------------
MyAgent      agent;
MyEnviroment environment;
int          gBest = -1;

void onStep(void*, Metis::TAGENTMETRICS* m)
{
    // TODO: draw / log; set m->bForceStopTraining = true to stop
}

void onEndEpisode(void*, Metis::TAGENTMETRICS* m)
{
    if (m->countSuccesX50episodes > gBest)
    {
        gBest = m->countSuccesX50episodes;
        agent.saveIAModel((char*)"myModel.ia");
    }
}

int main()
{
    srand((unsigned)time(nullptr));

    environment.pAgent = &agent;
    agent.createBrain(NUM_INPUTS, MyAgent::NUM_ACTIONS);
    environment.addAgent((Metis::IAgent*)&agent);

    Metis::AgentTrainerDQN trainer;
    trainer.setCallbackPerStep(onStep);
    trainer.setCallbackEndEpisode(onEndEpisode);
    trainer.training(&environment, &agent, nullptr);   // blocks until stopped
    return 0;
}
```

---

**Next steps:** run `DQN_Pong` in Training mode, watch `hits/50` grow, then try Play mode against the network. After that, change one thing at a time (an input, the shaping reward, the opponent) and see how learning changes. That is the fastest way to build intuition for DQN, and Metis-Core lets you do it without touching the learning code.
