#pragma once

#include <wx/wx.h>
#include <queue>
#include "Vector2D.h"
#include "METIS-Core.h"
#include "Sensor.h"


#define BULLET_SPEED 75  // 100
#define BULLET_MAX_DISTANCE_TRAVEL 120*2
#define ROTATION_INC_DEG 20.0

class Spacecraft;


typedef struct _Laser_bullet
{
	double x, y;
	int headingDeg;
	double _speed; // Velocity decays as it moves

	int _distanceTravel; //Bullet distance traveled
	int bHitTarget; // true if it hit the target
		
	bool operator==(const _Laser_bullet& other) const {
		return x == other.x &&
			y == other.y &&
			headingDeg == other.headingDeg &&
			_speed == other._speed &&
			_distanceTravel == other._distanceTravel &&
			bHitTarget == other.bHitTarget;
	}
		
	bool operator!=(const _Laser_bullet& other) const {
		return !(*this == other);
	}

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
	double _coolingRate = 3.5;          // Per second
	bool _coolingDown = false;
	bool _active = true;
		
	std::deque<Laser_BULLET> _bullets;
	std::vector<int> _targetsIDs;
	int _icycle;
	int _lastUpdateTime;
	double _turretAngleDeg; // angle of the torret of the laser
	double _minBulletDistanceToTarget;

	bool HitTarget(Laser_BULLET* pBullet, Metis::Vector2D &posTarget);

	void drawBullet(wxDC*pDC, Laser_BULLET &bullet);
	void drawLaserBullet(wxDC* pDC, Laser_BULLET& bullet);
	void drawSpark(wxDC* pDC, Laser_BULLET& bullet);
	void drawTurret(wxDC* pDC, const Vector2D& pos, float angleDeg, float scale);
		
	double calculateInterceptionTime(Spacecraft& target, double bulletSpeed);
	Metis::Vector2D predictPositionTarget(Spacecraft& target, double timeInAdvance);
	

	double getRnd(double min, double max);


	void updateTemperature(double currentTime);

	

public:
	Laser(Spacecraft* pPlatform);
	~Laser();

	void clean();
	bool isReadyToFire();

	double getTurretHeading() { return _turretAngleDeg; };
	void setTurretHeading(double heading) { _turretAngleDeg = heading; };
	
	bool isFiring();
	bool wasFired();
	bool isHitTarget();
	void checkTemperature();
	double getTemperature();
	double getTemperatureValue();
	void setPlatform(Spacecraft* pPlatform) { _pPlatform = pPlatform; };
		

	Laser& operator=(const Laser& other);
	bool operator!=(const Laser& other);

	//actions
	void Fire();
	void Rotate_Left();
	void Rotate_Right();

	double getBulletMinDistanceToTarget() { return _minBulletDistanceToTarget; };
	double CalculateDistanceToBullets(Metis::Vector2D& position);


	void Update(double incTime);

	void setTargets(std::vector<int>& targets);
	void getTargetsID(std::vector<int>* pTargetIDs);

	void Draw(wxDC* pDC);
};

