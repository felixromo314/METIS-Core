#pragma once

#include <deque>
#include "METIS-Core.h"
#include "prey.h"
#include "predator.h"
#include "predatorPrayEnv.h"


#define SIZE_WORD 7
#define MAX_PREDATORS 2
#define MAX_STEPS 80
#define MAX_RANGE_VISION 2

#define JOINSTATE_SIZE (2 * MAX_PREDATORS + 2 + 1)   // predators(x,y) + prey(x,y) + step  = 7
#define CAPTURE_REWARD        10.0f   // joint capture (2 predators at once)
#define SOLO_CAPTURE_PENALTY  -0.5f   // solo capture attempt
#define STEP_PENALTY          -0.1f   // cost per step (so they hurry)

struct TPredatorAction
{
    int id = 0;
    int toIndex(int currentPlayer) const
    {
        return id;
    }
};




// size: 
typedef struct _stPredatorInfo
{
    int id;
    Metis::Vector2D pos;

}TPREDATORINFO;

typedef struct _stJoinState
{
    TPREDATORINFO predator[MAX_PREDATORS];
    Metis::Vector2D prayPos; // real position is always visible here
    int step;

}TJOINSTATE;

class predatorPrayEnv : public Metis::IQMIXEnviroment
{
    private:

        bool  _preyCaptured = false;
        bool  _penalty = false;                 // a hunter tried to capture alone
        float _lastReward = 0.0f;
        float _episodeReturn = 0.0f;
        int   _lastActions[MAX_PREDATORS] = {};
    
        
        
    public:

        std::deque<int> _success50;
        std::deque<float> meanGlobalReward;
        int _countCaptures = 0;
        predator    _predators[MAX_PREDATORS];
        prey        _prey;

        predatorPrayEnv();
        ~predatorPrayEnv();

        Metis::Vector2D getPredatorPos(int i);
        Metis::Vector2D getPreyPos();
        int   getLastAction(int i) const { return _lastActions[i]; }
        bool  lastWasPenalty() const { return _penalty; }
        int   getEpisode() const { return this->_episode; }
        int   getStep() const { return this->_step; }
        float getEpisodeReturn() const { return _episodeReturn; }
        int   getSightRange() const { return MAX_RANGE_VISION; };
        float getLastReward() const { return _lastReward; }
        void  setLastReward(float r) { _lastReward = r; }
        bool  isPreyCaptured() const { return _preyCaptured; };
        bool  isCellFree(int nx, int ny);

        //Metis-Core
        virtual void reset();
        virtual int getJoinStateSize();
        virtual int getNumberOfAgents();
        virtual int getAgentInputSize();
        virtual int getAgentActionsSize();

        virtual Metis::IAgent* getAgentByIndex(int index);
        virtual Metis::IAgent* getAgentByID(int index);
        virtual void getJoinState(std::vector<float>* joinState);
        virtual void getAgentObservation(int indexAgent, std::vector<float>* agentObs);
        virtual std::vector<int> getValidActionsFromAgent(int indexAgent);
        virtual double getJoinReward(bool* pDone,bool *win);
        

};

