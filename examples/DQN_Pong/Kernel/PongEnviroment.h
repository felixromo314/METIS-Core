#ifndef __PONG_ENVIROMENT_H__
#define __PONG_ENVIROMENT_H__

// ---------------------------------------------------------------------------
//  PongEnviroment.h - the Pong world for Metis-Core.
//
//  Renamed from Enviroment.h/.cpp: Metis has its own "Enviroment.h"
//  (included by AgentTrainer.h) and the class Metis::Enviroment. With the same
//  file name and an __ENVIROMENT_H__ style guard in both, one of the two
//  would be silently skipped depending on the include order.
//
//  Cycle of a step (done by AgentTrainerDQN):
//     getState -> serizalizeState -> (network chooses action) -> applyAction
//     -> agent update(dt) -> getState -> calculateReward
//  The ball physics is advanced inside applyAction() of the trained agent,
//  so it does not depend on the exact order in which Metis calls the rest.
// ---------------------------------------------------------------------------

#include "Metis-Core.h"
#include <vector>
#include <cstring>

class Racket;
class Ball;

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

// Inputs of the DQN network (see serizalizeState)
#define NUM_INPUTS 5

class PongEnviroment : public Metis::Enviroment
{
private:
    Racket* _pPlayerLeft;          // trained agent (blue)
    Racket* _pPlayerRight;         // sparring partner with procedural policy (red)
    Ball* _pBall;

    int _w;
    int _h;
    int _margin;

    void stepSimulation();         // moves ball + collisions + misses
    void checkHit(Racket* pRacket, bool isLeft);
    bool predictImpactY(float xLine, float* pY);

public:
    // Rewards (previously hard-coded in Reward())
    float rewardHit;               // hit the ball       -> end of episode (WIN)
    float rewardMiss;              // let it through     -> end of episode (LOSS)
    float pseudoReward;            // per-step shaping: move towards the impact point

    // Right paddle control:
    //   true  -> procedural policy (sparring during training)
    //   false -> the action is set by the human in rightManualAction (Play mode)
    bool rightProcedural;
    int  rightManualAction;

    PongEnviroment(int w, int h, Racket* pLeft, Racket* pRight, Ball* pBall);

    // ---------- Metis::Enviroment methods ----------------------------------
    virtual void  getState(Metis::State* pState) override;
    virtual void  serizalizeState(void* state, std::vector<float>* stateVector) override;
    virtual void  applyAction(Metis::IAgent* pAgent, int actionId) override;
    virtual float calculateReward(Metis::State& state, int* pDone) override;
    virtual std::vector<Metis::TMULTIHEAD> calculateRewards(Metis::State& state, int* pDone) override;
    virtual void  reset() override;
    virtual float getDeltaTime();

    void getStateFromEnviroment(TSTATE* state);
    int  getWidth() { return _w; }
    int  getHeight() { return _h; }

    // for the render (yellow prediction dot)
    bool getPredictedImpactLeft(float* pY);
};

#endif