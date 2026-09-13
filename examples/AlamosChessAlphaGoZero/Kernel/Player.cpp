#include "stdafx.h"
#include "Player.h"
#include "Board.h"
#include "Figure.h"
#include "MovementsGenerator.h"
#include "METIS-Core.h"

#include <assert.h>

Player::Player(int color)
{
	int size = sizeof(Player);
	_color = color;

	
}
Player::Player(const Player& otherPlayer)
{
	int size = sizeof(Player);
	_color = otherPlayer._color;

	
	this->setID(otherPlayer.getID());
}
Player::~Player()
{

}

void Player::setBoard(Board* pBoard)
{
	_pBoard = pBoard;
}
Board* Player::getBoard()
{
	return _pBoard;
}
int Player::getColor()
{
	return _color;
}

void Player::ini()
{
	assert(_pBoard != NULL);

	if (_color == COLOR_WHITE)
	{

	}
	else
	{

	}

}

void Player::applyAction(TPIECEMOVEMENT& action)
{

	Figure figure = action.figure;
	_pBoard->moveFigure(action.oldX, action.oldY, action.newX, action.newY, &figure);
}
void Player::getAllMovements(std::vector< TPIECEMOVEMENT> * validMovements)
{
	MovementsGenerator generator;

	generator.generateFromPlayer(this,validMovements);

}
TPIECEMOVEMENT Player::selectAction()
{
	TPIECEMOVEMENT action;

	MovementsGenerator generator;

	std::vector<TPIECEMOVEMENT> validMovements;
	generator.generateFromPlayer(this,&validMovements);

	return action;
}