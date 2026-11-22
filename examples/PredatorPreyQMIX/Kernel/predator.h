#pragma once

#include "METIS-Core.h"


enum class PredatorAction : int
{
	UP = 0,   // y - 1
	DOWN = 1,   // y + 1
	LEFT = 2,   // x - 1
	RIGHT = 3,   // x + 1
	STAY = 4,   // stay still
	CAPTURE = 5,   // try to capture (only valid if the prey is adjacent)

	COUNT          // number of actions (not an action)
};

constexpr int N_ACTIONS = static_cast<int>(PredatorAction::COUNT);   // = 6

#define SIZE_WORD 7
#define AGENT_OBS_FEATURES 8                     // pos(2) + mateVisible(1) + matePos(2) + prayVisible(1) + prayPos(2)
#define AGENT_INPUT_SIZE   AGENT_OBS_FEATURES    // only the predator's observation
#define AGENT_HIDDEN       64

class predatorPrayEnv;

class predator : public Metis::IAgent
{
	private:

		Metis::Vector2D _pos;
		
	public:

		int _tryCaptured = 0;

		predatorPrayEnv *_pEnv;

		predator();
		~predator();
		void setPosition(Metis::Vector2D& pos);
		Metis::Vector2D getPosition();

		virtual void onPreStep();
		virtual void applyAction(int action);
		
};

