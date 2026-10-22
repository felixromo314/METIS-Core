#include "Ball.h"
#include "Racket.h"

#include <cmath>

static const double PI_D = 3.14159265358979323846;

static double degreesToRadians(double degrees) { return degrees * (PI_D / 180.0); }
static double radiansToDegrees(double radians) { return radians * (180.0 / PI_D); }

// normalizes an angle to [0, 360)
static double normalizeDeg(double deg)
{
    deg = std::fmod(deg, 360.0);
    if (deg < 0.0) deg += 360.0;
    return deg;
}

Ball::Ball()
{
    _position.x = 0;
    _position.y = 0;
    _speed = 0;
    _radius = 6;
    _dheadingDeg = 180;
    _fieldTop = 0;
    _fieldBottom = 480;
}

Metis::Vector2D Ball::GetPosition() { return _position; }
int    Ball::GetRadius() { return _radius; }
double Ball::GetHeading() { return _dheadingDeg; }
double Ball::GetVelocity() { return _speed; }
void   Ball::SetVelocity(int velocity) { _speed = (float)velocity; }
void   Ball::SetHeading(int headingDeg) { _dheadingDeg = (float)normalizeDeg(headingDeg); }

void Ball::SetPosition(int x, int y)
{
    _position.x = x;
    _position.y = y;
}

Metis::Vector2D Ball::GetVectorSpeed()
{
    double headingRad = degreesToRadians(_dheadingDeg);
    Metis::Vector2D v(_speed * cos(headingRad), _speed * sin(headingRad));
    return v;
}

void Ball::Update()
{
    double headingRad = degreesToRadians(_dheadingDeg);
    double vx = _speed * cos(headingRad);
    double vy = _speed * sin(headingRad);

    _position.x = _position.x + vx;
    _position.y = _position.y + vy;

    // Bounce on ceiling/floor.
    // The ball is moved back inside and the sign of vy is forced: in the previous
    // version vy was only inverted and, if the ball was still outside on the next
    // tick, it was inverted again (it got stuck "trembling" against the wall).
    bool bounce = false;
    if (_position.y < _fieldTop + _radius)
    {
        _position.y = _fieldTop + _radius;
        vy = fabs(vy);              // downwards
        bounce = true;
    }
    else if (_position.y > _fieldBottom - _radius)
    {
        _position.y = _fieldBottom - _radius;
        vy = -fabs(vy);             // upwards
        bounce = true;
    }

    if (bounce)
        _dheadingDeg = (float)normalizeDeg(radiansToDegrees(atan2(vy, vx)));
}

void Ball::HitWith(Racket* pRacket)
{
    pRacket->_numberHits++;

    double headingRad = degreesToRadians(_dheadingDeg);
    double vx = -_speed * cos(headingRad);   // inverts the X component (bounce)
    double vy = _speed * sin(headingRad);

    _dheadingDeg = (float)normalizeDeg(radiansToDegrees(atan2(vy, vx)));
}