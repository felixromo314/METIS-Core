#pragma once

#include "METIS-Core.h"
#include <map>

class Car;

#define WINDOW_WIDTH  700 //1700  // 600
#define WINDOW_HEIGHT 500 //700 //250

// numero de inputs: 

typedef struct _UrbanState
{
	Metis::Vector2D policeSpeed;
	

	Metis::Vector2D relativeThiefPos;
	Metis::Vector2D thiefSpeed;

	double thiefDistanceToTarget;
	
	//std::vector<float> _values; // cada dato del estado será un float
	_UrbanState()
	{
		memset(this, 0, sizeof(_UrbanState));
	}
}TURBANSTATE;

enum ACTIONS 
{
	LEFT,
	AHEAD,
	RIGHT,
	STOP
};

#define NUM_INPUTS 7  // numero de campos en el TURBANSTATE
#define NUM_ACTIONS 4 // numero maximo de acciones

typedef struct _Action
{
	int _typeAction; // UP=-1, NOMOVE=0, DOWN=+1
	_Action()
	{
		memset(this, 0, sizeof(struct _Action));
	}
}TACTION;

class UrbanEnviroment : public Metis::Enviroment
{

public:
	UrbanEnviroment();
	virtual ~UrbanEnviroment();

	Metis::Vector2D _interceptPoint;


	virtual void reset();
	//virtual void getState(TURBANSTATE* state);
	virtual float calculateReward(Metis::State& state,int *pDone);

	virtual void setNumActions(int numActions) {};
	virtual void getState(Metis::State* pState);
	virtual void serizalizeState(void* state, std::vector<float>* stateVector);
	virtual void applyAction(Metis::IAgent* pAgent,int actionId);
	virtual bool isEpisodeDone();
	
};

