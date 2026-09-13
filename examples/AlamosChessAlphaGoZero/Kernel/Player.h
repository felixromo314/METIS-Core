#pragma once

#include "Figure.h"
#include "METIS-Core.h"

#define MAXROWS 6

#define NUM_SQUARES (MAXROWS * MAXROWS) // 64 casillas

// 12 piece types (6 white + 6 black) + 1 plane for the turn = 13
// note: the planes is using to get the state of the board in a vector<float>
#define NUM_PLANES 13 
// New inputs: 64 squares * 13 planes = 832 network inputs
#define MAXINPUTS (NUM_SQUARES * NUM_PLANES)

#define MAXACTIONS (NUM_SQUARES*NUM_SQUARES)

class Board;

typedef struct stPieceMovement
{
	unsigned long movementID;
	Figure figure;
	int oldX;
	int oldY;
	int newX;
	int newY;

	int toIndex(int currentPlayer) const
	{
		// If Black plays (1), we flip the Y-axis using MAXROWS - 1
		// to translate the move into the Neural Network's "language" (canonical perspective).
		int relOldY = oldY;
		int relNewY = newY;
		if (currentPlayer != -1)
		{
			relOldY = (currentPlayer == 1) ? (MAXROWS - 1 - oldY) : oldY;
			relNewY = (currentPlayer == 1) ? (MAXROWS - 1 - newY) : newY;
		}

		// Calculate the index using relative coordinates adapted to MAXROWS (6x6)
		int fromSquare = (relOldY * MAXROWS) + oldX;
		int toSquare = (relNewY * MAXROWS) + newX;

		// The action space is based on NUM_SQUARES (36 * 36)
		return (fromSquare * NUM_SQUARES) + toSquare;
	}
}TPIECEMOVEMENT;

class Player : public Metis::IAgent
{
private:

	Board* _pBoard;

	int _color;

public:

	Player(int color);
	Player(const Player& otherPlayer);

	~Player();

	void setBoard(Board *pBoard);
	Board* getBoard();
	int getColor();
	void ini();

	// override from Metis-Core
	TPIECEMOVEMENT selectAction();
	void getAllMovements(std::vector< TPIECEMOVEMENT>* validMovements);

	void applyAction(TPIECEMOVEMENT& action);
	
};

