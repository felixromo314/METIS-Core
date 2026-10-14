#include "Sensor.h"
#include "Spacecraft.h"


Sensor::Sensor()
{
	_range = RANGE_SENSOR; // RANGE_SENSOR pixels of range
	_FOV_search = FOV_SEARCH; // In degrees
	
}
Sensor::~Sensor()
{

}

bool Sensor::operator!=(const Sensor& other)
{
    bool bNotEqual = false;

    assert(_owner != other._owner);
    if (_owner == other._owner) bNotEqual = true;
    if (_range != other._range) bNotEqual = true;
    if (_FOV_search != other._FOV_search) bNotEqual = true;
    if (_lastTargetPos != other._lastTargetPos) bNotEqual = true;

    
    return bNotEqual;
}
void Sensor::Set(Spacecraft* pJetFighter)
{
    _owner = pJetFighter;
}
void Sensor::clean()
{
    //_pTarget = NULL;
}

void Sensor::SetRange(double range)
{
    _range = range;
}

void Sensor::SetFOV(double fovDeg)
{

    _FOV_search = fovDeg;
}


bool Sensor::IsIluminate(Spacecraft* pTarget)
{
    bool bIsIluminate = false;
        

    Vector2D positionEnemy = pTarget->getPosition();
    Vector2D myPosition = _owner->getPosition();

    Vector2D vDirection;
    vDirection = _owner->getVelocity();
    vDirection = vDirection.Normalize();

    float distance = myPosition.DistanceTo(positionEnemy);
    if (distance <= _range) 
    {
        bIsIluminate = true;
        //_pTarget = pTarget;
    }

    
    return bIsIluminate;
}
