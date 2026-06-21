#pragma once
#include <wx/wx.h>
#include "METIS-Core.h"
#include "Laser.h"
#include "Missile.h"
#include "Sensor.h"
#include "HoloDecoy.h"

#define NUM_ACTIONS 7
#define MAX_SPEED_SHUTTLE 15.0
#define MAX_SPEED_FIGHTER_TIE 30.0
#define MAX_SPEED_X_WINGS 40.0
#define INC_ANGLE_HEADING 5 // 3º increment of turning
#define MAX_HOLY_DECOY 5
#define RANGE_HOLO_DECOY 50

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


/// <summary>
/// All actions, same of then are invalid in same agents.
/// </summary>
enum ACTIONS
{
	LEFT=0,
	AHEAD,
	RIGHT,
	STOP,
	LAUNCH_DECOY, // only valid in the Shuttle
	FIRE_LASER, // valid only in X-Wings
	MAX_ACTIONS, // MAX ACTIONS for the IA agents
	FIRE_MISSILE // only use in the x-wing in selecting a procedure action
};

class SpaceEnviroment;

/// <summary>
/// Represents the agent that groups the spacecraft forming the escort of the Imperial Shuttle
/// cargo spacecraft Shuttle: Must carry its Beskar cargo from planet Sullust to Death Star.
/// TIE_1: Will defend the cargo ship at close range from the front
/// TIE_2: Will defend the back
/// </summary>
class Spacecraft : public Metis::Agent
{
public:
	static const int DISTANCE_MIN_TO_SHUTTLE = 15;
	static const int DISTANCE_MAX_TO_SHUTTLE = RANGE_SENSOR;

	double _speedMaxSpeed;

	// equipment onboard
	Laser _laser;
	int _Laser_tried_fired;
	Sensor _sensor;
	Missile* _pMissile;
	HoloDecoy* _pHoloDecoy;

	int _countHolyDecoyLaunches;
	bool _bTriggerHoloDecoy;


private:

	double _headingDeg;
	double _speed;
	Metis::Vector2D _position;
	Metis::Vector2D _scortPos;


	Spacecraft* _pTarget;
	Missile* _pTargetMissile;

	int _iState; //state used in the rol of X-Wing in the logical procedural

	bool isReadyToFire(Spacecraft* pEnemy);
	int evade();

public:
	Spacecraft();
	~Spacecraft();

	unsigned char _r, _g, _b;
	SpaceEnviroment* _pEnv;
	Metis::Vector2D _idealScortPos;

	// set/get
	Metis::Vector2D& getPosition() { return _position; };
	Metis::Vector2D getVelocity();
	double getHeading() { return _headingDeg; };
	double getSpeed() { return _speed; };
	Missile* getMissile(){ return _pMissile;};
	
	void setPosition(float x, float y);
	void setColor(char r, char g, char b);
	void setHeading(float headingDeg);
	void setState(int state);
	int getState() { return _iState; };
	Spacecraft* getTarget() { return _pTarget; };
	void drawHoloDecoy(wxDC* pDC);
	HoloDecoy* GetHoloDecoyLaunched() { return _pHoloDecoy; };

	// Decoy
	bool ThereIsHoloDecoyFlying();
	bool WasFireHoloDecoy();
	void fireMissile();
	//Weapons
	bool wasLaserFired();
	bool thereIsLaserFlying();
	bool wasMissilFired();
	bool thereIsMissileFlying();

	void clean();

	//Draw
	void DrawXWing(wxDC* pDC, int cx, int cy, float scale);
	void DrawTIEFighter(wxDC* pDC, int cx, int cy, float scale);
	void DrawLambdaShuttle(wxDC* pDC, int cx, int cy, float scale);
	void Draw(wxDC* pDC);

	//Actions
	void TurnLeft();
	void GoHead();
	void TurnRight();
	void Stop();
	void FireLaser();
	void LaunchHoloDecoy();

	bool isOnSensorRange(Metis::Vector2D& position);
	bool updateSensor(Spacecraft* pTarget, Sensor* pSensor);
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

