
#ifndef __SENSOR__
	#define __SENSOR__

#include "METIS-Core.h"

using namespace Metis;

class Spacecraft;

#define RANGE_SENSOR 200
#define FOV_SEARCH 90

class Sensor
{
private:

	Spacecraft* _owner;
	float _range;
	float _FOV_search; // In degrees
	Vector2D _lastTargetPos;

	void GetTargetPosition(Vector2D* pTargetPos);

	

public:

	Sensor();
	~Sensor();

	void clean();
	void Set(Spacecraft* pShip);
	bool IsIluminate(Spacecraft* pHeli);
	
	double GetRange() { return _range; };
	double GetFOV() { return _FOV_search; };

	void SetRange(double range);
	void SetFOV(double fov);
		
	bool operator!=(const Sensor& other);

	
};

#endif

