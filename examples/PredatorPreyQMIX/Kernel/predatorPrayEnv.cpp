#include "pch.h"
//#include "predator.h"
#include "predatorPrayEnv.h"

#include <assert.h>
// is (x,y) inside the vision window centered on 'c'? (Chebyshev distance)
static bool isInSight(const Metis::Vector2D& c, int x, int y)
{
	return std::abs(x - c.x) <= MAX_RANGE_VISION && std::abs(y - c.y) <= MAX_RANGE_VISION;
}


predatorPrayEnv::predatorPrayEnv()
{

	/*_predators[0] = new predator();
	_predators[1] = new predator();*/

	_predators[0].setID(0);
	_predators[1].setID(1);
	_prey.setID(2);
	_prey._pEnv = this;
	_prey._pEnv = this;

	////create the brain of the each agent
	//_predators[0].createBrain(AGENT_INPUT_SIZE, N_ACTIONS);
	//_predators[0].createTargetBrain(); // each agent needs a target network

	//_predators[1].createBrain(AGENT_INPUT_SIZE, N_ACTIONS);
	//_predators[1].createTargetBrain();

	this->addAgent(&_predators[0]);
	this->addAgent(&_predators[1]);
	
	_predators[0]._pEnv = this;
	_predators[1]._pEnv = this;

	this->setProceduralAgent(&_prey);
	

}
predatorPrayEnv::~predatorPrayEnv()
{

}

void predatorPrayEnv::reset()
{
	Metis::Vector2D pos;
	pos.x = 1;
	pos.y = 0;
	_predators[0].setPosition(pos);
	
	
	//_predators[0]._id = 0;
	/*_predators[0]._tryCaptured = 0;
	_predators[0]._pEnv = this;*/

	pos.x = SIZE_WORD - 1;
	pos.y = SIZE_WORD - 1;
	_predators[1].setPosition(pos);
	/*_predators[1]._id = 1;
	_predators[1]._tryCaptured = 0;
	_predators[1]._pEnv = this;*/

	//_prey._id = 2;

	pos.x = SIZE_WORD / 2;
	pos.y = SIZE_WORD / 2;

	_prey.setPosition(pos);

	_penalty = false;
	_preyCaptured = false;
	_step = 0;
	_episodeReturn = 0;
	
}

int predatorPrayEnv::getJoinStateSize()
{
	return (int) JOINSTATE_SIZE;
}
int predatorPrayEnv::getNumberOfAgents()
{
	return MAX_PREDATORS;
}

Metis::IAgent* predatorPrayEnv::getAgentByIndex(int index)
{
	Metis::IAgent* pAgent;
	
	pAgent = (Metis::IAgent*)&_predators[index];
	return pAgent;
}

Metis::IAgent* predatorPrayEnv::getAgentByID(int agentID)
{
	Metis::IAgent* pAgentFound = NULL;
	for (int i = 0; i < MAX_PREDATORS; i++)
	{
		if (_predators[i].getID() == agentID)
		{
			pAgentFound = (Metis::IAgent*) &_predators[i];
		}
	}

	if (pAgentFound == NULL)
	{
		if (_prey.getID() == agentID)
		{
			pAgentFound = (Metis::IAgent*)&_prey;
		}
	}
	
	return pAgentFound;
}

Metis::Vector2D predatorPrayEnv::getPredatorPos(int i)
{
	return _predators[i].getPosition();
}
Metis::Vector2D predatorPrayEnv::getPreyPos()
{
	return _prey.getPosition();
}


void predatorPrayEnv::getJoinState(std::vector<float>* pJoinState)
{

	assert(pJoinState != NULL);

	const float norm = (float)(SIZE_WORD - 1);   // positions to [0,1]
	pJoinState->clear();
	pJoinState->reserve(JOINSTATE_SIZE);

	// real position of each predator (index = id)
	for (int i = 0; i < MAX_PREDATORS; i++)
	{
		Metis::Vector2D p = getPredatorPos(i);

		// check: always inside the map
		assert(p.x >= 0 && p.x < SIZE_WORD && p.y >= 0 && p.y < SIZE_WORD);

		pJoinState->push_back(p.x / norm);
		pJoinState->push_back(p.y / norm);
	}

	// real position of the prey (always known, even if no predator sees it)
	Metis::Vector2D prey = getPreyPos();
	assert(prey.x >= 0 && prey.x < SIZE_WORD && prey.y >= 0 && prey.y < SIZE_WORD);
	pJoinState->push_back(prey.x / norm);
	pJoinState->push_back(prey.y / norm);

	// episode progress
	pJoinState->push_back((float)_step / (float)MAX_STEPS);

	assert((int)pJoinState->size() == JOINSTATE_SIZE);
}

void predatorPrayEnv::getAgentObservation(int indexAgent, std::vector<float>* pObs)
{
	assert(pObs != nullptr);
	assert(indexAgent >= 0 && indexAgent < MAX_PREDATORS);

	const float norm = (float)(SIZE_WORD - 1);   // positions to [0,1]
	pObs->clear();
	pObs->reserve(AGENT_INPUT_SIZE);

	Metis::Vector2D me = getPredatorPos(indexAgent);

	// --- own data (always known)
	pObs->push_back(me.x / norm);
	pObs->push_back(me.y / norm);

	// --- teammate: the first one in sight (with MAX_PREDATOR = 2 there is only one)
	int mateVisible = 0;
	Metis::Vector2D matePos = { 0, 0 };
	for (int j = 0; j < MAX_PREDATORS; j++)
	{
		if (j == indexAgent) continue;
		Metis::Vector2D mate = getPredatorPos(j);
		if (isInSight(me, mate.x, mate.y))
		{
			mateVisible = 1;
			matePos = mate;
			break;
		}
	}
	pObs->push_back((float)mateVisible);
	pObs->push_back(mateVisible ? matePos.x / norm : 0.0f);   // not visible: neutral position
	pObs->push_back(mateVisible ? matePos.y / norm : 0.0f);

	// --- prey: only if it is inside the window
	Metis::Vector2D prey = getPreyPos();
	int prayVisible = isInSight(me, prey.x, prey.y) ? 1 : 0;
	pObs->push_back((float)prayVisible);
	pObs->push_back(prayVisible ? prey.x / norm : 0.0f);      // not visible: the network knows via prayVisible = 0
	pObs->push_back(prayVisible ? prey.y / norm : 0.0f);

	assert((int)pObs->size() == AGENT_INPUT_SIZE);   // 8
}

// Agent's valid-action mask: validActions[a] = 1 if action a is valid, 0 otherwise.
// Size is always N_ACTIONS (same format as nextValidActions and masked_fill in the learner).
std::vector<int> predatorPrayEnv::getValidActionsFromAgent(int indexAgent)
{
	assert(indexAgent >= 0 && indexAgent < MAX_PREDATORS);

	std::vector<int> validActions(N_ACTIONS, 0);   // all invalid by default

	Metis::Vector2D me = getPredatorPos(indexAgent);
	Metis::Vector2D prey = getPreyPos();

	// positions as integers (in case Vector2D stores floats)
	const int mx = (int)me.x, my = (int)me.y;
	const int px = (int)prey.x, py = (int)prey.y;

	// --- moves: inside the map and not stepping on the prey's cell
	const int dx[4] = { 0, 0, -1, 1 };   // UP, DOWN, LEFT, RIGHT
	const int dy[4] = { -1, 1, 0, 0 };
	for (int a = 0; a < 4; a++)
	{
		int nx = mx + dx[a];
		int ny = my + dy[a];
		bool insideMap = (nx >= 0 && nx < SIZE_WORD && ny >= 0 && ny < SIZE_WORD);
		bool onPrey = (nx == px && ny == py);
		validActions[a] = (insideMap && !onPrey) ? 1 : 0;
	}

	// --- stay: always valid (guarantees at least one action)
	validActions[(int)PredatorAction::STAY] = 1;

	// --- capture: only if the prey is adjacent (Manhattan distance 1)
	int dist = std::abs(mx - px) + std::abs(my - py);
	validActions[(int)PredatorAction::CAPTURE] = (dist == 1) ? 1 : 0;

	return validActions;
}
double predatorPrayEnv::getJoinReward(bool* pbDone,bool *win)
{
	double reward = STEP_PENALTY;                      // -0.1: cost per step

	*win = false;

	*pbDone = false;
	if (_step > MAX_STEPS)
	{
		*pbDone = true;
		return reward;
	}

	// --- how many predators tried to capture this step
	int nCapturing = 0;
	for (int i = 0; i < MAX_PREDATORS; i++)
	{
		if (_predators[i]._tryCaptured)
		{
			// check: the mask must prevent capturing without the prey adjacent
			const Metis::Vector2D& p = this->_predators[i].getPosition();
			int dist = std::abs(p.x - _prey.getPosition().x) + std::abs(p.y - _prey.getPosition().y);
			if ( dist <= 1)
				nCapturing++;
		}
	}

	// --- joint resolution
	_preyCaptured = (nCapturing >= 2);                 // cooperative capture
	_penalty = (nCapturing == 1);                 // solo attack

	if (_preyCaptured)
	{
		*win = true;
		_countCaptures++;
		reward += 10.0;                      // +10
		*pbDone = true;
	}
	else if (_penalty)
	{
		reward += SOLO_CAPTURE_PENALTY;                // -2
	}

	// --- for the renderer and statistics
	_lastReward = (float)reward;
	_episodeReturn += (float)reward;


	return reward;
}

int predatorPrayEnv::getAgentInputSize()
{
	return AGENT_INPUT_SIZE;
}
int predatorPrayEnv::getAgentActionsSize()
{
	return N_ACTIONS;
}

bool predatorPrayEnv::isCellFree(int nx, int ny)
{
	int i;
	bool bIsFree = true;
	for (int i = 0; i < MAX_PREDATORS; i++)
	{
		Metis::Vector2D pos = this->_predators[i].getPosition();
		if ((pos.x == nx) && (pos.y == ny))
		{
			bIsFree = false;
		}
	}

	return bIsFree;
}