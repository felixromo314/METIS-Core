#include "pch.h"
#include "predator.h"
#include "predatorPrayEnv.h"

predator::predator()
{

	this->createBrain(AGENT_INPUT_SIZE, N_ACTIONS);
	this->createTargetBrain();  // same inputs and actions as the online one
	
}
predator::~predator()
{


}
void predator::setPosition(Metis::Vector2D& pos)
{
	_pos = pos;
}
Metis::Vector2D predator::getPosition()
{
	return _pos;
}

void predator::onPreStep()
{
	_tryCaptured = 0;
}

void predator::applyAction(int action)
{
	switch ((PredatorAction)action)
	{
		case PredatorAction::UP:
		{
			if (_pos.y > 0)
				_pos.y--;
			break;
		}
		case PredatorAction::DOWN:
		{
			if (_pos.y < (SIZE_WORD - 1))
				_pos.y++;
			break;
		}
		case PredatorAction::LEFT:
		{
			if (_pos.x > 0)
				_pos.x--;
			break;
		}
		case PredatorAction::RIGHT:
		{
			if (_pos.x < (SIZE_WORD - 1))
				_pos.x++;
			break;
		}
		case PredatorAction::STAY:
		{
			break;
		}
		case PredatorAction::CAPTURE:
		{
			Metis::Vector2D preyPos = _pEnv->getPreyPos();
			int dist = std::abs(_pos.x - preyPos.x) + std::abs(_pos.y - preyPos.y);
			if (dist == 1)
			{
				_tryCaptured++;   // capture intent; resolved after the loop
			}

			break;
		}
	}

	assert(_pos.x >= 0 && _pos.x < SIZE_WORD && _pos.y >= 0 && _pos.y < SIZE_WORD);
}