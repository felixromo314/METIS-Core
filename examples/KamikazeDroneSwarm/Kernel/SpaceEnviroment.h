#pragma once

#include "METIS-Core.h"

#include "Spacecraft.h"

#define TEAM_BLUE_ID 0
#define TEAM_RED_ID 1

#define DRON_1 0
#define DRON_2 1
#define SHUTTLE_ID 2

#define MAX_STEPS 2000
#define MIN_DISTANCE_TO_TARGET_POSITION 25
#define DELTA_TIME 0.2   ///0.05  // you can increase to 0.1 to accelarte the tranning
#define MAX_DRONES_XWING 2  // agents in the concoy
#define DRONE_XWING_INPUTS 14  // 14 drone inputs
#define TEAM_BLUE_INPUTS ( (DRONE_XWING_INPUTS) ) // inputs to the multi-head net
#define TEAM_RED_INPUTS 18
#define MIN_REACH_SCORTPOINT 10
#define MAX_PLAYERS 3

// 1. Structure with the physical data of a generic drone
typedef struct _dronePhysicalData
{
	float heading; // absolute in degrees (ideally normalized)
	float x;       // relative to the target
	float y;
	float vx;      // velocity relative to the target
	float vy;
	float isAlive; // 1.0f if alive, 0.0f if destroyed
	_dronePhysicalData()
	{
		memset(this, 0, sizeof(_dronePhysicalData));
	};

	void toVector(std::vector<float>& inputs)
	{
		inputs.push_back(heading);
		inputs.push_back(x);
		inputs.push_back(y);
		inputs.push_back(vx);
		inputs.push_back(vy);
		inputs.push_back(isAlive);
	}

} TDRONE_PHYSICAL_DATA;  // 6 inputs

//data a imperial shuttle that need the multi-head
typedef struct _obsShuttle
{
	float heading;
	float shuttle_turret_heading;
	float numberOf_DroneX_WingDetected;
	TDRONE_PHYSICAL_DATA droneDetected[MAX_DRONES_XWING];

	float shuttle_laser_cooldown; // 1.0 if it is hot and needs to cool down, 0.0 if it is ready to fire
	// info the state of laser
	float Laser_fired; //laser was fired in this moment
	float Laser_firing; // there is laser on flying

	void toVector(std::vector<float>& inputs)
	{
		inputs.push_back(heading);
		inputs.push_back(shuttle_turret_heading);
		inputs.push_back((float)numberOf_DroneX_WingDetected);

		for (int i = 0; i < MAX_DRONES_XWING; i++)
		{
			droneDetected[i].toVector(inputs);
		}

		inputs.push_back(shuttle_laser_cooldown);
		inputs.push_back(Laser_fired);
		inputs.push_back(Laser_firing);
	}
	_obsShuttle()
	{
		memset(this, 0, sizeof(_obsShuttle));
	};
}TOBS_SHUTTLE;  // num imputs 11

// 2. What is actually passed to the Neural Network on the Drone's turn
typedef struct _obsDroneXWING
{
	// A. ENEMY DATA (The Shuttle)
	// The drone needs to know this in order to dodge
	float shuttle_turret_heading;
	float shuttle_laser_cooldown; // 1.0 if it is hot and needs to cool down, 0.0 if it is ready to fire

	TDRONE_PHYSICAL_DATA me;
	TDRONE_PHYSICAL_DATA wingman;
	_obsDroneXWING()
	{
		memset(this, 0, sizeof(_obsDroneXWING));
	};
	void toVector(std::vector<float>& inputs)
	{

		inputs.push_back(shuttle_turret_heading);
		inputs.push_back(shuttle_laser_cooldown);
		me.toVector(inputs);
		wingman.toVector(inputs);

	}

} TOBS_DRON_XWING;  // 14 inputs


//data about all the agents in the enviroment
typedef struct _join_state
{
	TOBS_SHUTTLE _escortTIEs[MAX_DRONES_XWING];

	Metis::Vector2D _scortRelativPositionX_TIE1; // 2 fields
	Metis::Vector2D _scortRelativPositionX_TIE2; // 2 fields

	TOBS_DRON_XWING _X_Wing_figther; // 8 inputs

	int currentStep; // to measure the time passed
}TSPACEENVIROMENT;

typedef struct _stAction
{
	int typeAction;
	int toIndex(int currentPlayer) const
	{
		return typeAction;
	}
}TACTION_AIRCRAFT;

// keep track of the min and maximum reward
typedef struct _maxminReward
{
	double minShuttle;
	double maxShuttle;
	double currentShuttle_R;
	std::vector<double> ShuttleRewardsList;

	double minTIE_1;
	double maxTIE_1;
	double currentTIE_1_R;
	std::vector<double> TIE_1_RewardsList;

	double minTIE_2;
	double maxTIE_2;
	double currentTIE_2_R;
	std::vector<double> TIE_2_RewardsList;

	double minXTotal;
	double maxXTotal;
	double currentTotal_R;
	std::vector<double> Total_RewardsList;

	_maxminReward()
	{
		minShuttle = 100.0;
		maxShuttle = -100.0;
		currentShuttle_R = 0.0;

		minTIE_1 = 100.0;
		maxTIE_1 = -100.0;
		currentTIE_1_R = 0.0;

		minTIE_2 = 100.0;
		maxTIE_2 = -100.0;
		currentTIE_2_R = 0.0;
	}


}TMINMAX_REWARD;

#define SHUTTLE_ID	2

#define NAME_SHUTTLE "ImperialShuttle"
#define NAME_XWINDRONE_1 "xwinDrone_1"
#define NAME_XWINDRONE_2 "xwinDrone_2"

class SpaceEnviroment : public Metis::IAlphaGoZeroState<TSPACEENVIROMENT, TACTION_AIRCRAFT>
{
private:

	void reset2(int type);
	void reset3(int type);

	double _minDistEverReached = 99999.0;

public:

	TMINMAX_REWARD _minmaxReward;
	int _currentPlayer;

	Metis::Vector2D _posTarget;
	static int _maxSIZE_X;
	static int _maxSIZE_Y;

	const int MAX_WINDOW_TO_FIRE_LASER = 8.0;

	int _currenteStep;
	int _currentEpisode;
	int _decoyWindowCounter = 0;
	int _laserWindowCounter = 0;
	int _teamWinner;

	//Players
	Spacecraft _drone_XWing_1;
	Spacecraft _drone_XWing_2;
	Spacecraft _ImperialShuttle;


	SpaceEnviroment();
	SpaceEnviroment(SpaceEnviroment& other);
	virtual ~SpaceEnviroment();

	bool isPlayerActive(int playerID);
	void advanceToNextActivePlayer(bool bIncludeCurrent);
	SpaceEnviroment& operator=(SpaceEnviroment& other);

	virtual double getDeltaTime();
	virtual bool isEqualState(IAlphaGoZeroStateBase* pOtherState);

	// derived from Metis
	// STATE & ENVIRONMENT LIFECYCLE
	void reset(int typeReset);
	void update(float incTime = 0.01);
	std::vector<float> getState();                 // Used by AlphaGoZeroTrainer class
	virtual std::vector<float> getState(int currentPlayer);
	virtual std::vector<float> getStateFromTeam(int teamID);
	IAlphaGoZeroStateBase* clone();
	bool isStateRelativeToPlayer() { return true; }; // Used by AlphaGoZeroTrainer class
    virtual int getNumActions(int currentPlayer);
    int getNumActions() override { return 0; };
	int getNumInputs() override { return 0; };

	// ACTION SPACE & EXECUTION
	void applyAction(int action);
	void getPossibleActions(int playerID, std::vector<TACTION_AIRCRAFT>& actions);
	bool is_valid_action(int actionID);



	// REWARD, TERMINAL STATE & HEURISTICS
	bool isTerminal();
	int getWinner();
	double getReward(int teamID);
	float getMaterialHeuristicValue(int playerToEvaluateID);

	// PLAYER & TEAM MANAGEMENT
	int getCurrentPlayerID() override { return _currentPlayer; };
	virtual int getTeamID(int currentPlayerID);
    int getRandomActionFromPlayer(int currentPlayer) override { return -1; };

};