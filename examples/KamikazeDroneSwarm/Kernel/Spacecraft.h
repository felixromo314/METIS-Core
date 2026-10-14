#pragma once
#include <wx/wx.h>
#include "METIS-Core.h"
#include "Laser.h"
#include "Sensor.h"


// export funcions used in this file
double normalizeAngleDeg(double angulo);

//#define NUM_ACTIONS 7
#define MAX_SPEED_SHUTTLE 13.0f
#define MAX_SPEED_DRON_XWING 30.0f
//#define MAX_SPEED_X_WINGS 40.0
#define INC_ANGLE_HEADING  15.0  // 5 // 3º increment of turning
#define INC_TURRET_ANGLE_HEADING 15 // 3º increment of turning

enum stateSpacecraft
{
	searched = 1,
	destroyed = 2,
	attacked = 3,
	attackedAndEvaded = 4,
	evaded = 5,
	disable = 6,
	tables = 7,
	standby = 8,
	evadeMissile = 9,
	collision,
	destroyedByMissil
};



enum class TEAM_BLUE_ACTIONS
{
	STOP = 0,
	TURN_LEFT,
	AHEAD,
	TURN_RIGHT,
	MAX_ACTIONS, // MAX ACTIONS for the IA agents
};
enum class TEAM_RED_ACTIONS
{
	NONE=0,
	ROTATE_LEFT_TURRET,
	ROTATE_RIGHT_TURRET,
	FIRE_LASER,
	MAX_ACTIONS, // MAX ACTIONS for the IA agents
};
enum class ShipType 
{
	Shuttle,
	XWingDrone
};

class SpaceEnviroment;

/// <summary>
/// Represents the agent the kamizake dron and the shuttle
/// </summary>
class Spacecraft : public Metis::IAgent
{
public:
	static const int DISTANCE_MIN_TO_SHUTTLE = 15;
	static const int DISTANCE_MAX_TO_SHUTTLE = RANGE_SENSOR;

	double _speedMaxSpeed;

	// equipment onboard
	Laser _laser;
	int _Laser_tried_fired;
	Sensor _sensor;
			

private:

	double _headingDeg;
	double _speed;
	Metis::Vector2D _position;
	Metis::Vector2D _scortPos;
	ShipType _typeShip;

	int _numTargets;
	std::vector<int> _targetsIDs;

	int _iState; //state used in the rol of X-Wing in the logical procedural

	bool isReadyToFire(Spacecraft* pEnemy);

public:
	Spacecraft();
	Spacecraft(Spacecraft& other);
	~Spacecraft();

	Spacecraft& operator=(Spacecraft& other);
	bool operator!=(const Spacecraft& other);

	unsigned char _r, _g, _b;
	SpaceEnviroment* _pEnv;
	Metis::Vector2D _idealScortPos;

	int _dronWingMan;

	// set/get
	Metis::Vector2D& getPosition() { return _position; };
	Metis::Vector2D getVelocity();
	double getHeading() { return _headingDeg; };
	double getSpeed() { return _speed; };
	void setSpeed(double speed) { _speed = speed; };

	void setShipType(ShipType type) { _typeShip = type; };
	ShipType getShipType() { return _typeShip; };
	
	
	void setPosition(float x, float y);
	void setColor(char r, char g, char b);
	void setHeading(float headingDeg);
	void setTurretHeading(float headingDeg);
	void setState(int state);
	int getState() { return _iState; };

	bool isAlive() { return (_iState != destroyed); };
	int getNumberDroneDetected();
	std::vector<int> getDronesDetected();

	float getLaserTemperature();
	float getTurrentHeading();

	
	void applyAction(int action);

	
	//Weapons
	bool wasLaserFired();
	bool thereIsLaserFlying();
	

	void clean();

	//Draw
	void DrawXWingDrone(wxDC* pDC, int cx, int cy, float scale);
	void DrawLambdaShuttle(wxDC* pDC, int cx, int cy, float scale);
	void Draw(wxDC* pDC);

	//Actions drones
	void TurnLeft();
	void GoHead();
	void TurnRight();
	void Stop();
	void FireLaser();

	//actions shuttle
	void Rotate_TurrentLeft();
	void Rotate_TurrentRight();
	

	bool isOnSensorRange(Metis::Vector2D& position);
	bool updateSensor(Sensor* pSensor);
	bool updateLaser(double delta_time);

	double getATAangle(Spacecraft* pEnemy);
	double getATAangle(Vector2D& posTarget);
	bool isCoveredThePosition(Metis::Vector2D& posToCover);

	// Functions to override from the Metis-core framework
	virtual int update(double delta_time);
	virtual int getActionProcedural(Metis::State& state);
	virtual bool isValidAction(int iAction);
	virtual void endStep();
};

