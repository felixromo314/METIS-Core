
#ifndef __SENSOR__
	#define __SENSOR__

#include "METIS-Core.h"

using namespace Metis;

class Spacecraft;
class Missile;

#define RANGE_SENSOR 150
#define MISSILE_RANGE_SENSOR 200
#define FOV_SEARCH 90

class Sensor
{
private:

	Spacecraft* _owner;
	Spacecraft* _pTarget;
	float _range;
	float _FOV_search; // In degrees
	

	void GetTargetPosition(Vector2D* pTargetPos);

	Vector2D _lastTargetPos;

public:

	void clean();
	void Set(Spacecraft* pShip);
	bool IsIluminate(Spacecraft* pHeli);
	bool IsIluminate(Missile* pHeli);

	double GetRange() { return _range; };
	double GetFOV() { return _FOV_search; };

	void SetRange(double range);
	void SetFOV(double fov);

	Spacecraft* getTarget() {return _pTarget;};

	Sensor();
	~Sensor();
};

#endif

