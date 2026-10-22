#ifndef __BALL__
#define __BALL__

// ---------------------------------------------------------------------------
//  Ball.h - Pong ball (physics only, no drawing: drawing goes in PongRender)
// ---------------------------------------------------------------------------

#include "Metis-Core.h"

class Racket;

class Ball
{
private:
    Metis::Vector2D _position;
    int    _radius;
    float  _speed;          // pixels per tick
    float  _dheadingDeg;    // degrees [0, 360)

    int    _fieldTop;       // vertical limits of the field (before: 398 hard-coded)
    int    _fieldBottom;

public:
    Ball();

    void SetFieldLimits(int top, int bottom) { _fieldTop = top; _fieldBottom = bottom; }

    int    GetRadius();
    Metis::Vector2D GetPosition();
    double GetHeading();
    void   SetPosition(int x, int y);
    void   SetVelocity(int velocity);
    double GetVelocity();
    void   SetHeading(int headingDeg);
    Metis::Vector2D GetVectorSpeed();

    void Update();                  // advances one tick and bounces on ceiling/floor
    void HitWith(Racket* pRacket);  // bounce against a paddle
};

#endif