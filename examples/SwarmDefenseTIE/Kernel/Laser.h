#pragma once

#include <wx/wx.h>
#include <queue>
#include "Vector2D.h"
#include "METIS-Core.h"
#include "Sensor.h"


#define BULLET_SPEED 75  // 100
#define BULLET_MAX_DISTANCE_TRAVEL 120*2

class Spacecraft;
class Missile;

typedef struct _Laser_bullet
{
	double x, y;
	int headingDeg;
	double _speed; // Velocity decays as it moves

	int _distanceTravel; //Bullet distance traveled
	int bHitTarget; // true if it hit the target

}Laser_BULLET;

#define MAX_BULLETS 5

class Laser
{
private:

	double _timeFiring;
	int _bulletPerFire;
	Spacecraft* _pPlatform;

	// temperatura
	double _temperature;
	double _maxTemperature = 100.0;
	double _heatPerShot = 25.0;         // Each shot adds 10
	double _coolingRate = 2.5;          // Per second
	bool _coolingDown = false;
	bool _active = true;
		
	std::deque<Laser_BULLET> _bullets;

	Spacecraft* _pTarget;
	Missile* _pTargetMissile;

	bool HitTarget(Laser_BULLET* pBullet, Metis::Vector2D &posTarget);

	void drawBullet(wxDC*pDC, Laser_BULLET &bullet);
	void drawLaserBullet(wxDC* pDC, Laser_BULLET& bullet);
	void drawSpark(wxDC* pDC, Laser_BULLET& bullet);

	double calculateInterceptionTime(Missile& targetMissile, double bulletSpeed);
	double calculateInterceptionTime(Spacecraft& target, double bulletSpeed);
	Metis::Vector2D predictPositionTarget(Spacecraft& target, double timeInAdvance);
	Metis::Vector2D predictPositionTarget(Missile& targetMissile, double timeInAdvance);

	double getRnd(double min, double max);

	int _icycle;
	int _lastUpdateTime;
	void updateTemperature(double currentTime);
public:
	Laser(Spacecraft* pPlatform);
	~Laser();

	void clean();
	bool isReadyToFire();
	void Fire(Missile* pTargetMissile);
	void setTarget(Missile* pTargetMissile);
	Missile* getTarget() { return _pTargetMissile; };

	bool isFiring();
	bool wasFired();
	bool isHitTarget();
	void checkTemperature();
	double getTemperature();
	void Fire(Spacecraft* pTarget);
	void Update(double incTime);

	void Draw(wxDC* pDC);
};

