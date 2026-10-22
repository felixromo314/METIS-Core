#ifndef __RACKET_H__
#define __RACKET_H__

// ---------------------------------------------------------------------------
//  Racket.h - Pong paddle as a Metis-Core agent.
//
//  All the DQN code that used to live here (QNetwork, target network, replay buffer,
//  epsilon, train, softUpdate, MSE...) is GONE: Metis does it.
//  The paddle only keeps its physics and what Metis requires from an IAgent:
//     - getActionProcedural(): "hand-made" policy (sparring / baseline)
//     - update(dt):            the agent's own physics on every step
//     - isValidAction():       valid actions
//  The network is created with createBrain() and saved with saveIAModel() (inherited).
// ---------------------------------------------------------------------------

#include "Metis-Core.h"

#define PADDEL_STEP_Y  20       // pixels the paddle moves per action
#define PADDEL_LEN     50       // paddle length in pixels

class Ball;

enum stateRacket
{
    playing = 1,
    gameover = 2,
    win = 3,
};

class Racket : public Metis::IAgent
{
public:
    // Actions = output indices of the DQN network (0..NUM_ACTIONS-1).
    // No more transformIndexActionToAction / transformAction_To_IndexAction.
    enum ACTIONS
    {
        ACTION_UP = 0,
        ACTION_NOMOVE = 1,
        ACTION_DOWN = 2,
        NUM_ACTIONS = 3
    };

    // Direction of the last movement (used by the reward shaping)
    enum DIRECTION
    {
        DIR_UP = -1,
        DIR_NOMOVE = 0,
        DIR_DOWN = 1
    };

private:
    Metis::Vector2D _position;     // x = paddle edge, y = vertical center
    int    _lenRacket;
    bool   _hitBall;
    double _paddel_vy;             // vertical velocity (pixels / time)
    double _prevPositionY;
    int    _directionPaddel;       // DIRECTION
    int    _state;                 // stateRacket

    int    _fieldTop;              // vertical limits (before: 0 and 480 hard-coded)
    int    _fieldBottom;

    Ball* _pBall;                 // only for the procedural policy

public:
    int  _gamesWin;
    int  _numberHits;
    int  _currentAction;           // last applied action (ACTIONS)
    unsigned char _r, _g, _b;      // color (read by the render)

    Racket();

    // --------- Metis::IAgent methods ---------------------------------------
    virtual int  getActionProcedural(Metis::State& state) override;
    virtual int  update(double delta_time) override;
    virtual bool isValidAction(int iAction) override { return iAction >= 0 && iAction < NUM_ACTIONS; }

    // --------- physics ----------------------------------------------------
    void Reset();
    void SetBall(Ball* pBall) { _pBall = pBall; }
    void SetFieldLimits(int top, int bottom) { _fieldTop = top; _fieldBottom = bottom; }
    void SetColor(unsigned char r, unsigned char g, unsigned char b) { _r = r; _g = g; _b = b; }
    void SetPosition(double x, double y);
    void SetState(int state) { _state = state; }
    int  GetState() { return _state; }

    Metis::Vector2D& GetPosition() { return _position; }
    int  GetLen() { return _lenRacket; }
    int  GetDirection() { return _directionPaddel; }
    double GetVelocityPaddel() { return _paddel_vy; }

    void HitTheBall(bool bHit) { _hitBall = bHit; }
    bool ThereIsHitBall() { return _hitBall; }

    void MoveVertical(int incMove);
    void ApplyActionId(int actionId);   // translates action index -> movement
};

#endif