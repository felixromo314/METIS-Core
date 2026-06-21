#pragma once

#include "METIS-Core.h"

#define MAX_STEPS 2000
#define MIN_DISTANCE_TO_TARGET_POSITION 25
#define DELTA_TIME 0.05  // you can increase to 0.1 to accelarte the tranning
#define MAX_ESCORT_TIES 3  // agents in the concoy
#define FIGHTER_TIE_INPUTS 12
#define FIGHTER_XWING_INPUTS 9
#define MAX_JOINT_INPUTS 4 //_scortRelativPositionX_TIE1 + _scortRelativPositionX_TIE2
#define CONVOY_INPUTS ((FIGHTER_TIE_INPUTS*MAX_ESCORT_TIES) + MAX_JOINT_INPUTS + FIGHTER_XWING_INPUTS+1 /*CURRENTSTEP*/) // inputs to the multi-head net
#define MIN_REACH_SCORTPOINT 10

//data a fighter TIE that need the multi-head
typedef struct _obsTIE
{
	int id; // like a rol
	float heading;
	float x; //position in pixels
	float y;
	float vx; // velocity
	float vy;
	float speed; //scalar

	char X_WingDetected;

	char holoDecoy_fired;
	char holoDecoyOnAir;

	// info the state of laser
	char Laser_fired; //laser was fired in this moment
	char Laser_firing; // there is laser on flying
	_obsTIE()
	{
		memset(this, 0, sizeof(_obsTIE));
	};
}TOBS_TIE;

//data a fighter TIE that need the multi-head
typedef struct _obsXWING
{
	int id; // like a rol
	float heading;
	float x; //position in pixels
	float y;
	float vx; // velocity
	float vy;

	// info the state of misil
	char Missil_fired;
	char Missil_flying;
	float missil_x;
	float missil_y;
	_obsXWING()
	{
		memset(this, 0, sizeof(_obsXWING));
	};
}TOBS_XWING;


//data about all the agents in the enviroment
typedef struct _join_state
{
	TOBS_TIE _escortTIEs[MAX_ESCORT_TIES];

	Metis::Vector2D _scortRelativPositionX_TIE1; // 2 fields
	Metis::Vector2D _scortRelativPositionX_TIE2; // 2 fields

	TOBS_XWING _X_Wing_figther; // 8 inputs

	int currentStep; // to measure the time passed
}TSPACEENVIROMENT;

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
		currentShuttle_R=0.0;

		minTIE_1 = 100.0;
		maxTIE_1 = -100.0;
		currentTIE_1_R = 0.0;

		minTIE_2 = 100.0;
		maxTIE_2 = -100.0;
		currentTIE_2_R = 0.0;
	}


}TMINMAX_REWARD;

class Spacecraft;

#define SHUTTLE_ID	0
#define TIE_1_ID	1
#define TIE_2_ID	2
#define X_WING_ID	3

#define NAME_SHUTTLE "Shuttle"
#define NAME_TIE1 "TIE_1"
#define NAME_TIE2 "TIE_2"

class SpaceEnviroment : public Metis::Enviroment
{
public:

	TMINMAX_REWARD _minmaxReward;

	Metis::Vector2D _posTarget;
	static int _maxSIZE_X;
	static int _maxSIZE_Y;

	const int MAX_WINDOW_TO_FIRE_LASER = 8.0;
	
	int _currenteStep;
	float _totalMeanReward;
	int _decoyWindowCounter = 0;
	int _laserWindowCounter = 0;


	SpaceEnviroment();
	~SpaceEnviroment();

	Spacecraft *getTIEfiringLaser();

	bool isValidSituationToUseLaser_TIE(int TIE_ID,TSPACEENVIROMENT* pSpaceEnv);
	bool isShuttleThreat(TSPACEENVIROMENT* pSpaceEnv);
	float calculate_rewardShuttle(TSPACEENVIROMENT* pSpaceEnv);
	float calculate_reward_X_TIE1(TSPACEENVIROMENT* pSpaceEnv);
	float calculate_reward_X_TIE2(TSPACEENVIROMENT* pSpaceEnv);

	float calculate_reward_Laser(TSPACEENVIROMENT* pSpaceEnv, Spacecraft *pTIE,TOBS_TIE &objTIE);

	//methods override from Metis
	virtual void reset();
	virtual float calculateReward(Metis::State& state, int* pDone) { return 0.0; };
	virtual std::vector<Metis::TMULTIHEAD> calculateRewards(Metis::State& state, int* pDone);
	virtual void setNumActions(int numActions) {};
	virtual void getState(Metis::State* pState);
	virtual void serizalizeState(void* state, std::vector<float>* stateVector);
	virtual void applyAction(Metis::IAgent* pAgent, int actionId);
	virtual bool isEpisodeDone() { return false; };
	virtual float getDeltaTime();
};

