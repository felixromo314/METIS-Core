// HelloWorld_TreasureHunter.cpp
// A hello world example of using Metis-Core so you can learn better how to use Metis-Core
// 
// The example: 
//   /o\ : is the treasure hunter (ia agent) that should find the treasure [T]
//   [T] : treasure to be finded.

// Configuration: Debug and ReleaseCUDA


#include "Metis-Core.h"
#include <iostream>
#include <numeric>
#include <windows.h>

#define MAX_X_AXIS 80
#define MAX_STEPS 80

float calculateAverageReward(std::vector<float> vectorReward);

// render the enviroment of the treasure
void render(int agentPos, int treasurePos, int worldSize) 
{
	system("cls");
	
	printf("\n\n\n\n\n\n\n\n");
	

	std::string line = "";

	for (int i = 0; i < worldSize; ++i) {
		if (i == agentPos) 
		{
			if ( (agentPos % 2) == 0 )
				line += "\\o/"; // El Agente
			else line += "/o\\"; // El Agente
		}
		else if (i == treasurePos) {
			line += "[T]"; // El Tesoro
		}
		else {
			line += "_";   // El suelo
		}
	}
	printf("\n\n\n");
	// \r vuelve al inicio de la línea, std::flush asegura que se imprima ya
	std::cout << "\r" << line << std::flush;
}


#pragma region IAAgent

/// <summary>
/// IA Agent to hunter the treasure
/// </summary>
class treasureHunter : public Metis::Agent
{

public:

	int _currentNumberSteps; // current steps done to reach the treasure. The lower, the better.

	enum ACTIONS {
		RIGHT=0,
		LEFT,
	};

	int _positionX; // position along the X axis
	int _currentAction;

	// methods to be override from Metis-core
	virtual int getActionProcedural(Metis::State& state);
	virtual int update(double delta_time);
	virtual bool isValidAction(int iAction) { return true; };
};

// Useful for providing a 'Sparring Partner' or a baseline to train against.
int treasureHunter::getActionProcedural(Metis::State& state)
{
	// in this example there is no need of "Sparring Agent", so we leave empty the method
	return 0;
}

//Updates the environment's internal state and physics simulation.
//Useful for integrating velocity, handling timers, or procedural animations.
//param delta_time: The elapsed time since the last frame (in seconds).  #define INC_TIME 0.5 (TODO: to be configurable by api)
int treasureHunter::update(double delta_time)
{
	// in this example there is no need to update any physics, only the number of step done by the agent
	_currentNumberSteps++;
	return 0;
}
#pragma endregion

#pragma region Enviroment

// Number of inputs to the net
#define NUM_INPUTS 2

// 2 actions:
//  0: move to the RIGHT
//  1: move to the LEFT
#define NUM_ACTIONS 2


// State of the enviroment
// the reward function (float calculateReward(Metis::State& state, int* pDone)) should calculate the reward with the data in this TTREASURESTATE
typedef struct _TreasureState
{
	// si > 0 (treasure is to the right of the screen)
	// si < 0 (treasure is to the left of the screen)
	int distanteToTreasure;
	int steps; // number of steps need to reach the treasure
	_TreasureState()
	{
		memset(this, 0, sizeof(_TreasureState));
	}
}TTREASURESTATE;

//----------------------------------------------------------------
// Variables globales
int _treasurePositionX; // position of the treasure to be finded
treasureHunter iaTreasureHunter; // ia agent who will try to find the treasure

// Variables to save the best ia model to file
float gMaxReward = -1.0;  // to save the model with maxreward
int   gNumberMaxReachTreasure = -100;  // max numer of times that hunter get the treasure
//----------------------------------------------------------------
// We define our enviriment of the treasure deriving from Metis::Enviroment
// This class manages the world logic, state transitions, and reward signals.
class TreasureEnviroment : public Metis::Enviroment
{
public:

	virtual void getState(Metis::State* pState);
	virtual void serizalizeState(void* state, std::vector<float>* stateVector);
	virtual void applyAction(Metis::IAgent* pAgent, int actionId);
	virtual float calculateReward(Metis::State& state, int* pDone);
	virtual std::vector<Metis::TMULTIHEAD> calculateRewards(Metis::State& state, int* pDone);
	virtual void reset();
};
// in each step, this funcion copy the data of the enviroment, to the struct TTREASURESTATE which will be use to calculate the reward of the ia hunter treasure
void TreasureEnviroment::getState(Metis::State* pState)
{
	TTREASURESTATE treasureState;

	// si > 0 (tesoro está a la derecha de la pantalla)
	int distance =  _treasurePositionX - iaTreasureHunter._positionX;

	treasureState.distanteToTreasure = distance;
	treasureState.steps = iaTreasureHunter._currentNumberSteps;

	pState->copyState(&treasureState, sizeof(TTREASURESTATE)); // copy de data to the internal state of Metis-Core (using to save the Replay Buffer)
}
//this function get the inputs ready to be used by the DQN network
//note: std::vector<float>* stateVector will be the inputs to the DQN network. Better if all the inputs are normalizaed within the range -1.0 - 1.0
void TreasureEnviroment::serizalizeState(void* state, std::vector<float>* stateVector)
{
	// si > 0 (tesoro está a la derecha de la pantalla)
	int distance = _treasurePositionX - iaTreasureHunter._positionX;

	float normDistance = distance / (float) MAX_X_AXIS; // normalize the distance (better for the traning with normalized data)
	stateVector->push_back(normDistance);

	float nomrmSteps = iaTreasureHunter._currentNumberSteps / (float) MAX_STEPS;
	stateVector->push_back(nomrmSteps);

}
//this funcion is call in each step of the traning process. Apply the actions in the enviroment choose by the treasure hunter (ia agent)
// Note: Action identifiers correspond to the indices of the DQN output vector.
// These discrete integers (int) map to specific environment behaviors.
void TreasureEnviroment::applyAction(Metis::IAgent* pAgent, int actionId)
{
	treasureHunter* pHunterTreasure = (treasureHunter*)pAgent;
	switch (actionId)
	{
		case treasureHunter::ACTIONS::RIGHT:
		{
			pHunterTreasure->_currentAction = treasureHunter::ACTIONS::RIGHT;
			pHunterTreasure->_positionX++; // move to the right of the console window
			break;
		}
		case treasureHunter::ACTIONS::LEFT:
		{
			pHunterTreasure->_currentAction = treasureHunter::ACTIONS::LEFT;
			pHunterTreasure->_positionX--; // move to the left of the console window
			break;
		}
		default:
		{
			assert(false);
			break;
		}
	}
}
//used in multi-head net
std::vector<Metis::TMULTIHEAD> TreasureEnviroment::calculateRewards(Metis::State& state, int* pDone)
{
	std::vector<Metis::TMULTIHEAD> r_heads;

	return r_heads; // return empty
}
// calculate the reward based on the current values of the TTREASURESTATE.
// Note: with pDone you can say to the Metis-Core when the episode has reached the END.
//   *pDone = 1; To set that the episode has finished with a WIN
//   *pDone = -1; To set that the episode has finished with a LOSS
float TreasureEnviroment::calculateReward(Metis::State& state, int* pDone)
{
	float rewardDistance = 0.0;
	*pDone = 0; // mark end or not end of the training episode ( 0: not end episode)

	TTREASURESTATE* pTreasureState = (TTREASURESTATE*)state.getUserState();

	//the treasure is to the right
	if (pTreasureState->distanteToTreasure > 0)
	{
		if (iaTreasureHunter._currentAction == treasureHunter::ACTIONS::RIGHT)
		{
			rewardDistance = 0.5; // the agent choose the ok direction
		}
		else
		{
			rewardDistance = -0.5; // the agent choose the wrong direction
		}
	}
	else
	{
		//the treasure is to the left
		if (iaTreasureHunter._currentAction == treasureHunter::ACTIONS::RIGHT)
		{
			rewardDistance = -0.5; // the agent choose the wrong direction
		}
		else
		{
			rewardDistance = 0.5; // the agent choose the ok direction
		}
	}
	
	float distanceMagnitude = fabs(pTreasureState->distanteToTreasure);
	//hunter reach the treasure??
	if (distanceMagnitude <= 0)
	{
		rewardDistance += 10.0; // reward for getting the treasure
		*pDone = 1;
	}

	if ((iaTreasureHunter._positionX < 0) || (iaTreasureHunter._positionX >= MAX_X_AXIS) )
	{
		rewardDistance -= 10.0; // penalty for getting lost
		*pDone = -1; // hunter get loss in its search. -1 to apply a penalty (set the episode at finish)
	}
	
	double stepFactor = 0.01;
	double stepsPenalty = pTreasureState->steps * stepFactor;
	rewardDistance -= stepsPenalty; // penalizar por cada step

	//calculate
	if (pTreasureState->steps > MAX_STEPS)
	{
		rewardDistance -= 5.0; // penalty MAX NUMER OF STEPS
		*pDone = -1; // set the episode at finish
	}

	return rewardDistance;
}
// at the begin of each episode Metis-Core call this funtion to reset al the necesary values.
void TreasureEnviroment::reset()
{
	////-------------------------------------------------
	//for testing, fixed the treasure and agent in the same position always
	//_treasurePositionX = 10; // reset position of the treasure
	// reset the state of the hunter treasure
	//iaTreasureHunter._positionX = 20;
	////-------------------------------------------------

	// in each episode, calculate a random position for treasure and hunter
	_treasurePositionX = rand() % MAX_X_AXIS; // random position of the treasure
	iaTreasureHunter._positionX = rand() % MAX_X_AXIS; // random position of the hunter
	iaTreasureHunter._currentNumberSteps = 0;

	
}
#pragma endregion

TreasureEnviroment aTreasureEnviroment; // enviroment of the treasure
std::vector<float> gRewardHistory; // to do the mean each 50 episodes
double gMeanRewardX50Episode = 0.0f;

#pragma region  CALLBACKS_OF_TRAINING
// callback when Metis-Core execute a step in the traning
void onStepTraining(void* pSender, Metis::TAGENTMETRICS* pMetrics)
{

	//------------------------------------------------
	// Comment this block of code if you want fast trainer
	render(iaTreasureHunter._positionX, _treasurePositionX, MAX_X_AXIS);
	int episode = pMetrics->episode;
	int step = pMetrics->step;
	printf("\n\n\nEpisode:%d  step:%d  reward:%.2f\n", pMetrics->episode, pMetrics->step, pMetrics->reward);
	printf("gMeanRewardX50Episode:%f\n", gMeanRewardX50Episode);
	printf("gMaxReward:%.2f gNumberMaxReachTreasure:%d\n",gMaxReward, gNumberMaxReachTreasure);
	fflush(stdout);
//-----------------------------------------------------------------------

	if (gRewardHistory.size() >= 50)
	{
		// remove the the lastest reward
		gRewardHistory.erase(gRewardHistory.begin());
	}
	gRewardHistory.push_back(pMetrics->reward);
}
//callback when the Metis-Core ends a episode
void onEndEpisode(void* pSender, Metis::TAGENTMETRICS* pMetrics)
{
	// each 50 eposide, write to a log file and calculate how good is the training
	if ((pMetrics->episode % 50) == 0)
	{
		gMeanRewardX50Episode = calculateAverageReward(gRewardHistory);

		printf("Episode:%d  step:%d  reward:%.2f\n", pMetrics->episode, pMetrics->step, pMetrics->reward);
		printf("gMeanRewardX50Episode:%f   countSuccesX50episodes:%d\n", gMeanRewardX50Episode, pMetrics->countSuccesX50episodes);
		fflush(stdout);

		static FILE* logFile = NULL;
		if (logFile == NULL)
		{
			logFile = fopen("trainingLog.txt", "w+t");
		}
		fprintf(logFile, "episode:%d  gMeanRewardX50Episode:%2f  countSuccesX50episodes:%d\n", pMetrics->episode, gMeanRewardX50Episode, pMetrics->countSuccesX50episodes);
		fprintf(logFile, "		_meanLoss:%.4f  _backbone_norm:%.2f   _gradient_norm:%.3f\n", pMetrics->_meanLoss, pMetrics->_backbone_norm, pMetrics->_gradient_norm);

		///--------------  block to check how good is the model, if it is good -> save to file
		// save the model with max rewards in 50 episodes  (TODO: configurable by api this number of 50)
		if (gMeanRewardX50Episode > gMaxReward)
		{
			gMaxReward = gMeanRewardX50Episode;
			iaTreasureHunter.saveIAModel((char*)"treasureHunterModel.ia");
			fprintf(logFile, "		Save IA model gMaxReachTreasure:%.2f\n", gMaxReward);
		}
		//if in 50 episode the hunter get more tresasure, the model is good -> save it
		if (pMetrics->countSuccesX50episodes > gNumberMaxReachTreasure)
		{
			gNumberMaxReachTreasure = pMetrics->countSuccesX50episodes;
			iaTreasureHunter.saveIAModel((char*)"treasureHunterModel.ia");
			fprintf(logFile, "		Save IA model gNumberMaxReachTreasure:%d\n", gNumberMaxReachTreasure);
			
		}
		fflush(logFile);
	}
}

#pragma endregion

// make the treasure hunter to find the treasure
void doPlayTreasureHunter()
{

	iaTreasureHunter.loadIAModel((char *)"treasureHunterModel.ia");

	int numberTreasureFinded = 0;
	int episodes = 0;
	double rateOfSuccess = 0.0;
	while (1)
	{
		episodes++;
		_treasurePositionX = rand() % MAX_X_AXIS; // random position of the treasure
		iaTreasureHunter._positionX = rand() % MAX_X_AXIS; // random position of the hunter

		// loop to find the treasure
		bool bFind = false;
		int step = 0;
		while (!bFind && (step < MAX_STEPS))
		{
			Metis::State treasureState;
			// get the state from the enviroment
			aTreasureEnviroment.getState(&treasureState);
			aTreasureEnviroment.serizalizeState(&treasureState,&treasureState._stateVector);  // serialize de enviroment state into a vector (to the DQN network)
			int action = iaTreasureHunter.predictAction(treasureState); // HERE!. the hunter try to find the best action to find the treasure

			aTreasureEnviroment.applyAction(&iaTreasureHunter, action); // apply the action into the enviroment

			// render the enviroment
			render(iaTreasureHunter._positionX, _treasurePositionX, MAX_X_AXIS);
			printf("\n\n\n\n episodes:%d\n", episodes);
			printf("Number of treasure finded:%d  rateOfSucces:%.2f\n", numberTreasureFinded, rateOfSuccess);
			
			if (iaTreasureHunter._positionX == _treasurePositionX)
			{
				bFind = true;
			}
			step++;
		}

		if (bFind)
		{
			numberTreasureFinded++;
		}
		rateOfSuccess = (double)numberTreasureFinded / (double) episodes;


	}
}
int main()
{
	SetConsoleTitleA("[A Hello World of METIS-Core]      Example TreasureHunter");

	printf("Choose an option [and press ENTER]:\n");
	printf("	1: Training treasure hunter\n");
	printf("			[INFO] Infinite loop training. Data logged in trainingLog.txt.\n");
	printf("			[INFO] Model saved in file 'treasureHunterModel.ia'\n");
	printf("	2: Check if treasure hunter has learned to hunt\n");
	printf("			[INFO] Loading pre-trained weights from 'treasureHunterModel.ia'...\n");
	printf("			[INFO] Initializing Agent and Treasure at stochastic positions.\n");
	int option;
	scanf("%d", &option);

	iaTreasureHunter.createBrain(NUM_INPUTS, NUM_ACTIONS);
	aTreasureEnviroment.addAgent((Metis::IAgent*)&iaTreasureHunter);

	if (option == 1)
	{
		Metis::AgentTrainerDQN aPirateTrainer;

		aPirateTrainer.setCallbackPerStep(onStepTraining);
		aPirateTrainer.setCallbackEndEpisode(onEndEpisode);
		aPirateTrainer.training(&aTreasureEnviroment, &iaTreasureHunter,NULL /*there is no enemy agent*/);
	}
	if (option == 2)
	{
		doPlayTreasureHunter();
	}


}

//----------------------------------------------------------------
//  maths
float calculateAverageReward(std::vector<float> vectorReward)
{
	if (vectorReward.empty()) {
		return 0.0;
	}
	double sum = std::accumulate(vectorReward.begin(), vectorReward.end(), 0.0);
	return sum / static_cast<float>(vectorReward.size());
}
