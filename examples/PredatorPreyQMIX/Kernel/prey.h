#pragma once

#include "METIS-Core.h"


enum class PreyAction : int
{
	UP = 0,   // y - 1
	DOWN = 1, // y + 1
	LEFT = 2,   // x - 1
	RIGHT = 3,   // x + 1
	STAY = 4,   // stay still
	COUNT          // number of actions (not an action)
};

class predatorPrayEnv;

class prey : public Metis::IAgent
{
	private:
		Metis::Vector2D _pos;

		std::mt19937 _rng{ 1234 };
	public:

		predatorPrayEnv *_pEnv;

		void setPosition(Metis::Vector2D& pos);
		Metis::Vector2D getPosition();

		//Metis-core
		virtual int getActionProcedural();
		virtual void applyAction(int action);
};

