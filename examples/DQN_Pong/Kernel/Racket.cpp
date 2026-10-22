#include "Racket.h"
#include "Ball.h"

#include <cassert>
#include <cmath>

Racket::Racket()
{
    _position.x = 0;
    _position.y = 0;
    _lenRacket = PADDEL_LEN;
    _hitBall = false;
    _paddel_vy = 0;
    _prevPositionY = 0;
    _directionPaddel = DIR_NOMOVE;
    _state = playing;
    _fieldTop = 0;
    _fieldBottom = 480;
    _pBall = nullptr;

    _gamesWin = 0;
    _numberHits = 0;
    _currentAction = ACTION_NOMOVE;
    _r = _g = _b = 255;
}

void Racket::Reset()
{
    _state = playing;
    _hitBall = false;
    _paddel_vy = 0;
    _directionPaddel = DIR_NOMOVE;
    _currentAction = ACTION_NOMOVE;
}

void Racket::SetPosition(double x, double y)
{
    _position.x = x;
    _position.y = y;
    _prevPositionY = y;     // placing the paddle is NOT a movement: it does not change the direction
}

void Racket::MoveVertical(int incMove)
{
    _position.y += incMove;

    if (incMove < 0)       _directionPaddel = DIR_UP;
    else if (incMove > 0)  _directionPaddel = DIR_DOWN;
    else                   _directionPaddel = DIR_NOMOVE;

    // the whole paddle inside the field (before, only the center was limited to 0..480)
    const double half = _lenRacket * 0.5;
    if (_position.y < _fieldTop + half)    _position.y = _fieldTop + half;
    if (_position.y > _fieldBottom - half) _position.y = _fieldBottom - half;
}

void Racket::ApplyActionId(int actionId)
{
    _currentAction = actionId;
    switch (actionId)
    {
    case ACTION_UP:     MoveVertical(-PADDEL_STEP_Y); break;
    case ACTION_NOMOVE: MoveVertical(0);              break;
    case ACTION_DOWN:   MoveVertical(+PADDEL_STEP_Y); break;
    default:            assert(false);                break;
    }
}

// Metis calls it every step: the paddle's own physics (vertical velocity)
int Racket::update(double delta_time)
{
    if (delta_time > 0.0)
        _paddel_vy = (_position.y - _prevPositionY) / delta_time;
    _prevPositionY = _position.y;
    return 0;
}

// "Hand-made" policy: follow the ball's Y when it is coming towards me.
// It is the opponent (right paddle) and serves as a baseline to compare with the DQN.
int Racket::getActionProcedural(Metis::State& /*state*/)
{
    if (_pBall == nullptr)
        return ACTION_NOMOVE;

    Metis::Vector2D ballPos = _pBall->GetPosition();
    Metis::Vector2D ballVel = _pBall->GetVectorSpeed();

    // the ball is coming towards me if it is getting closer in X
    const bool approaching = (ballVel.x > 0 && ballPos.x < _position.x) ||
        (ballVel.x < 0 && ballPos.x > _position.x);

    const double targetY = approaching ? ballPos.y : (_fieldTop + _fieldBottom) * 0.5;
    const double diff = targetY - _position.y;
    const double deadZone = PADDEL_STEP_Y * 0.5;   // avoids oscillating around the target

    if (diff < -deadZone) return ACTION_UP;
    if (diff > deadZone) return ACTION_DOWN;
    return ACTION_NOMOVE;
}