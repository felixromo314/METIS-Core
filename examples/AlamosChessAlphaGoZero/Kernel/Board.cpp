#include "stdafx.h"
#include "Board.h"
#include "Player.h"
#include "Figure.h"
#include "MovementPiece.h"
#include "MovementsGenerator.h"
#include "ZobristKeys.h"

#include <cassert>
#include <iostream>

#include "MemoryLeaks.h"

int Board::_countBoard = 0;
int Board::_countBoardInmemory = 0;

Board::Board(Player* pWhites, Player* pBlacks)
{

	_pWhites = pWhites;
	_pBlacks = pBlacks;

	_pWhites->setBoard(this);
	_pBlacks->setBoard(this);

	_id = Board::_countBoard;
	Board::_countBoard++;

	Board::_countBoardInmemory++;

}

Board& Board::operator=(const Board& other)
{
	if (this != &other)
	{
		// 1. Copiar los tipos primitivos
		_currentPlayerID = other._currentPlayerID;
		_winPlayer = other._winPlayer;

		// 2. Copiar la matriz de casillas
		for (int r = 0; r < MAXROWS; ++r)
		{
			for (int c = 0; c < MAXROWS; ++c)
			{
				_squares[r][c] = other._squares[r][c];
			}
		}

		_id = Board::_countBoard;
		Board::_countBoard++;

		// 3. Reemplazar los jugadores destruyendo los antiguos y clonando los nuevos
		delete _pWhites;
		_pWhites = (other._pWhites != nullptr) ? MY_NEW Player(*other._pWhites) : nullptr;
		if (_pWhites != nullptr) _pWhites->setBoard(this);

		delete _pBlacks;
		_pBlacks = (other._pBlacks != nullptr) ? MY_NEW Player(*other._pBlacks) : nullptr;
		if (_pBlacks != nullptr) _pBlacks->setBoard(this);
	}
	return *this;
}

Board::Board(const Board& otherBoard)
{
	int size = sizeof(Board);

	_id = Board::_countBoard;
	Board::_countBoard++;
	
	_currentPlayerID = otherBoard._currentPlayerID;
	_winPlayer = otherBoard._winPlayer;
	_pWhites = NULL;
	_pBlacks = NULL;

		
	_lastBoardsHistory = otherBoard._lastBoardsHistory;

	//checkBoardSanity();
	for (int r = 0; r < MAXROWS; ++r)
	{
		for (int c = 0; c < MAXROWS; ++c)
		{
			_squares[r][c] = otherBoard._squares[r][c];
		}
	}

	if (otherBoard._pWhites != nullptr)
	{
		_pWhites = MY_NEW Player(*otherBoard._pWhites);
		_pWhites->setBoard(this);
	}

	if (otherBoard._pBlacks != nullptr)
	{
		_pBlacks = MY_NEW Player(*otherBoard._pBlacks);
		_pBlacks->setBoard(this);
	}
	Board::_countBoardInmemory++;

}
Board::~Board()
{
	if (_pWhites != NULL)
	{
		delete _pWhites;
		_pWhites = NULL;
	}
	if (_pBlacks != NULL)
	{
		delete _pBlacks;
		_pBlacks = NULL;
	}

	Board::_countBoardInmemory--;
}

void Board::setFigure(Piece piece, int x, int y,int color)
{

	Figure aFigure(piece, color);

	_squares[x][y].setFigure(aFigure);
}


void Board::clear()
{
	int i, j;
	// initialize the entire board to empty
	for (i = 0; i < MAXROWS; i++)
	{
		for (j = 0; j < MAXROWS; j++)
		{
			setFigure(Piece::Empty, i, j, COLOR_EMPTY);
		}
	}
}
// Los Alamos Chess (6x6) - Initial classic setup
// No bishops: King, Queen, 2 Rooks, 2 Knights, and 6 Pawns per side.
// Board size: 6x6 (Rows/Columns from 0 to 5)
void Board::iniLosAlamosChess()
{
	this->clear();

	// White Player
	setFigure(Piece::WhiteRook, 0, 0, COLOR_WHITE); // a1
	setFigure(Piece::WhiteKnight, 1, 0, COLOR_WHITE); // b1
	setFigure(Piece::WhiteQueen, 2, 0, COLOR_WHITE); // c1
	setFigure(Piece::WhiteKing, 3, 0, COLOR_WHITE); // d1
	setFigure(Piece::WhiteKnight, 4, 0, COLOR_WHITE); // e1
	setFigure(Piece::WhiteRook, 5, 0, COLOR_WHITE); // f1

	for (int x = 0; x < 6; x++) 
	{
		setFigure(Piece::WhitePawn, x, 1, COLOR_WHITE);
	}

	// Black Player
	for (int x = 0; x < 6; x++) {
		setFigure(Piece::BlackPawn, x, 4, COLOR_BLACK);
	}

	setFigure(Piece::BlackRook, 0, 5, COLOR_BLACK); // a6
	setFigure(Piece::BlackKnight, 1, 5, COLOR_BLACK); // b6
	setFigure(Piece::BlackQueen, 2, 5, COLOR_BLACK); // c6
	setFigure(Piece::BlackKing, 3, 5, COLOR_BLACK); // d6
	setFigure(Piece::BlackKnight, 4, 5, COLOR_BLACK); // e6
	setFigure(Piece::BlackRook, 5, 5, COLOR_BLACK); // f6
}
void Board::iniRandom()
{
	ini(); // standard board setup


	// Choose how many random moves to make: between 1 and 3
	int numRandomMoves = (std::rand() % 3) + 1;

	_currentPlayerID = 0;

	for (int i = 0; i < numRandomMoves; i++)
	{
		std::vector<TPIECEMOVEMENT> legalMoves;
		getPossibleActions(_currentPlayerID, legalMoves);

		// in case there is no movements (check,...)
		if (legalMoves.empty())
		{
			break;
		}

		// Choose a random index within the vector size
		int randomIndex = std::rand() % legalMoves.size();
		TPIECEMOVEMENT chosenMove = legalMoves[randomIndex];

		//apply the movement in the board
		int actionRnd = chosenMove.toIndex(_currentPlayerID);
		this->applyAction(actionRnd);
	}

	_currentPlayerID = 0; // Player 0 ini the first movement
}
void Board::ini()
{

	if ((_pWhites != NULL) && (_pBlacks != NULL))
	{
		// set up the board
		clear();
		iniLosAlamosChess();
	}

	_winPlayer = -1;
	_currentPlayerID = 0; // Juegan Blancas
	
	int color;
	if (_currentPlayerID == 0)
		color = COLOR_WHITE;
	else color = COLOR_BLACK;

	//calculate the hash of the board
	_currentHash = buildZobristHash(color); // Solo se llama 1 vez al inicio

	_lastBoardsHistory.clear();
}


bool Board::isPathClear(int x, int y, int newX, int newY)
{

	if (!isInsideBoard(newX, newY))
	{
		return false;
	}

	// Calculate the move direction
	// dx and dy will be -1, 0, or 1
	int dx = (newX > x) ? 1 : (newX < x) ? -1 : 0;
	int dy = (newY > y) ? 1 : (newY < y) ? -1 : 0;

	
	// Start the traversal from the cell NEXT to the origin
	int curX = x + dx;
	int curY = y + dy;

	// loop to go the the new position
	while (curX != newX || curY != newY)
	{
		// has curX,curY a piece (obstacule)
		if (!this->isEmpty(curX,curY))
		{
			return false; //  There is an obstruction
		}

		// next step
		curX += dx;
		curY += dy;
	}

	return true; // Path clear
}

void Board::moveFigure(int x, int y, int newX, int newY, Figure* pFigure)
{
	Figure* pFigureTmp = getFigure(newX, newY);
	if (pFigureTmp == NULL)
	{
		return;
	}

	if (!pFigureTmp->isEmpty())
	{
		if (pFigureTmp->getColor() == pFigure->getColor())
		{
			assert(false);
		}
	}

	const ZobristKeys& keys = ZobristKeys::instance();

	// Datos de la pieza que se mueve
	Piece movingPiece = pFigure->getPiece();
	int movingType = pieceToIndex(movingPiece);
	int movingColor = pieceToColorIndex(movingPiece);
	int colorPiece = pFigure->getColor();



	
	// Incremental Zobrist hash update
	// Remove piece from origin square (x, y)
	if (movingType != -1 && movingColor != -1) {
		_currentHash ^= keys.pieces[x][y][movingType][movingColor];
	}

	// If it's a capture, remove the captured piece from the destination square (newX, newY)
	bool isCapture = !pFigureTmp->isEmpty();
	if (isCapture) 
	{
		Piece capturedPiece = pFigureTmp->getPiece();
		int capturedType = pieceToIndex(capturedPiece);
		int capturedColor = pieceToColorIndex(capturedPiece);

		if (capturedType != -1 && capturedColor != -1) {
			_currentHash ^= keys.pieces[newX][newY][capturedType][capturedColor];
		}
	}

	// Place the moved piece on the destination square (newX, newY)
	if (movingType != -1 && movingColor != -1) {
		_currentHash ^= keys.pieces[newX][newY][movingType][movingColor];
	}

	// Toggle the turn in the hash
	int currentTurnColor = movingColor; // Color que acaba de mover
	int nextTurnColor = (currentTurnColor == 0) ? 1 : 0;
	_currentHash ^= keys.turn[currentTurnColor];
	_currentHash ^= keys.turn[nextTurnColor];

	
	// apply the movement
	bool isPawnMove = (movingPiece == Piece::WhitePawn || movingPiece == Piece::BlackPawn);

	setFigure(Piece::Empty, x, y, COLOR_EMPTY);
	setFigure(movingPiece, newX, newY, colorPiece);

	bool isPromotion = false;
	Piece promotedPiece = movingPiece;
	if (movingPiece == Piece::WhitePawn && newY == (MAXROWS-1))
	{
		promotedPiece = Piece::WhiteQueen;
		setFigure(Piece::WhiteQueen,newX,newY,colorPiece);
		isPromotion = true;
	}
	
	if (movingPiece == Piece::BlackPawn && newY == 0)
	{ 
		promotedPiece = Piece::BlackQueen;
		setFigure(Piece::BlackQueen, newX, newY, colorPiece);
		isPromotion = true;
	}

	if (isPromotion)
	{
		_currentHash ^= keys.pieces[newX][newY][movingType][movingColor];
		int promotedType = pieceToIndex(promotedPiece);
		_currentHash ^= keys.pieces[newX][newY][promotedType][movingColor];
	}

	// Irreversibility rule: a capture or pawn move clears the 3-fold repetition history
	if (isCapture || isPawnMove)
	{
		_lastBoardsHistory.clear();
	}

	// Store the updated hash directly
	_lastBoardsHistory[_currentHash]++;


}

bool Board::isValidMovement(int x,int y,int newX, int newY,Figure *pFigure, bool checkClearPath)
{
	if (!isInsideBoard(newX, newY))
	{
		return false;
	}

	bool bisValidMovement = false;
	int colorPlayer = pFigure->getColor();

	Figure* pFigureOn = getFigure(newX, newY);
	int colorOnXY = pFigureOn->getColor();

	if ( isEmpty(x, y))
	{
		return false; // si casisa origen vacia, no es valido
	}
	else
	{
		
	}

	if (!isEmpty(newX, newY))
	{
		if (colorOnXY == colorPlayer)  //si es hay pieza de mi color, no se valido
			return false;
	}

	if (checkClearPath)
		bisValidMovement = isPathClear(x, y, newX, newY);
	else bisValidMovement = true;

	//If the move captures a piece of the same color, the move is invalid
	Figure* pFigureOld = this->getFigure(x, y);
	Figure* pFigureNew = this->getFigure(newX, newY);
	if (!pFigureNew->isEmpty())
	{
		if (pFigureNew->getColor() == pFigureOld->getColor())
		{
			bisValidMovement = false;
		}
	}
	return bisValidMovement;
}

uint64_t Board::buildZobristHash(int color)
{
	uint64_t hash = 0ULL;
	ZobristKeys &keys = ZobristKeys::instance();

	for (int r = 0; r < MAXROWS; ++r)
	{
		for (int c = 0; c < MAXROWS; ++c)
		{
			if (!isEmpty(r, c))
			{
				Piece piece = _squares[r][c].getFigure().getPiece();

				int pIdx = pieceToIndex(piece);
				if (pIdx != -1) 
				{ 
					long long value = (long long) keys.pieces[r][c][pIdx];
					hash ^= value;
				}
			}
		}
	}

	hash ^= keys.turn[color];

	return hash;
}

std::string Board::buildHashFromBoard(int color)
{
	
	char cHash[320];
	int index = 0;

	for (int r = 0; r < MAXROWS; ++r)
	{
		for (int c = 0; c < MAXROWS; ++c)
		{
			if (!isEmpty(r, c))
			{
				Figure& figure = _squares[r][c].getFigure();

				cHash[index++] = (char)('0' + r);
				cHash[index++] = (char)('0' + c);
				cHash[index++] = (char)('0' + figure.getColor());
				cHash[index++] = (char)figure.getPiece();
				cHash[index++] = '|';
			}
		}
	}

	cHash[index++] = 'T';
	cHash[index++] = (char)('0' + color);

	cHash[index] = '\0';

	return std::string(cHash, index);
}
bool Board::isInsideBoard(int x, int y)
{
	if (x < 0 || y < 0 || x >= MAXROWS || y >= MAXROWS)
	{
		return false;
	}
	else return true;
}
bool Board::canCapture(int colorPiece,int x, int y)
{
	if (!isInsideBoard(x,y))
	{
		return false;
	}

	Figure *pFigure = this->getFigure(x, y);
	if (pFigure->getPiece() == Piece::Empty)
	{
		return false;
	}
	if (pFigure->getColor() == colorPiece)
	{
		return false; // same color as mine
	}

	if (pFigure->getColor() != colorPiece)
	{
		return true; // there is a piece from the opposite (whites or blacks)
	}

	return true;
}

bool Board::isEmpty(int x, int y)
{
	Figure* pFigure = getFigure(x, y);
	if (pFigure == NULL)
	{
		return false;
	}

	if (pFigure->getPiece() == Piece::Empty)
	{
		return true;
	}
	else return false;
}
Figure *Board::getFigure(int x, int y)
{
	if (!isInsideBoard(x, y))
	{
		return NULL;
	}
	Square& squareCell = _squares[x][y];
	Figure& aFigure = squareCell.getFigure();

	return &aFigure;
}
int Board::getSquareColor(int x, int y)
{
	int color = COLOR_EMPTY;

	Square& squareCell = _squares[x][y];

	Figure &aFigure = squareCell.getFigure();
	
	color = aFigure.getColor();
	
	return color;
}


int Board::getNumInputs()
{
	int maxInputs;

	maxInputs = MAXINPUTS;
	return maxInputs;
}
int Board::getNumActions()
{
	int maxActions;

	maxActions = MAXACTIONS;
	return maxActions;
}

bool Board::canFigureDoTheMoveToSquare(Figure *pFig,int x,int y,int newX,int newY)
{
	MovementsGenerator generator;
	bool bCanFigureDoTheMoveToSquare = false;

	std::vector<TPIECEMOVEMENT> figureMovs;
	generator.generateMovementForFigure(this, pFig, x, y,&figureMovs);

	std::vector<TPIECEMOVEMENT>::iterator it;
	for (it = figureMovs.begin(); (it != figureMovs.end()) && !bCanFigureDoTheMoveToSquare; ++it)
	{
		const TPIECEMOVEMENT& mov = *it;

		if ((mov.newX == newX) && (mov.newY == newY))
		{
			bCanFigureDoTheMoveToSquare = true;
		}
	}

	return bCanFigureDoTheMoveToSquare;
}

bool Board::isOnlyTwoKings()
{
	int kingX = -1;
	int kingY = -1;
	Piece targetKingPiece = Piece::WhiteKing;
	int countFiguresWhites = 0;
	int countFiguresBlack = 0;
	// Find the current position of the player's King
	for (int y = 0; y < MAXROWS; ++y)
	{
		for (int x = 0; x < MAXROWS; ++x)
		{
			Figure* pFig = getFigure(x, y);
			if (!pFig->isEmpty() && (pFig->getColor() == COLOR_WHITE) )
			{
				countFiguresWhites++;

			}
			if (!pFig->isEmpty() && (pFig->getColor() == COLOR_BLACK))
			{
				countFiguresBlack++;

			}
		}
	}

	if ((countFiguresWhites == 1) && (countFiguresBlack == 1))
	{
		return true;
	}
	else return false;


}

bool Board::getPositionKing(int color, int* xx, int* yy)
{
	
	*xx = -1;
	*yy = -1;
	Piece targetKingPiece = (color == COLOR_WHITE) ? Piece::WhiteKing : Piece::BlackKing;
	// Find the current position of the player's King
	for (int y = 0; y < MAXROWS; ++y)
	{
		for (int x = 0; x < MAXROWS; ++x)
		{
			Figure* pFig = getFigure(x, y);
			if (pFig != nullptr && pFig->getPiece() == targetKingPiece)
			{
				*xx = x;
				*yy = y;
				break;
			}
		}
		if (*xx != -1) break;
	}

	if (*xx == -1 || *yy == -1)
	{
		return false;
	}

	return true;
}

bool Board::isKingInCheck(Player* player)
{

	int kingX = -1;
	int kingY = -1;
	Piece targetKingPiece = (player->getColor() == COLOR_WHITE) ? Piece::WhiteKing : Piece::BlackKing;

	for (int y = 0; y < MAXROWS; ++y)
	{
		for (int x = 0; x < MAXROWS; ++x)
		{
			Figure* pFig = getFigure(x, y);
			if (pFig != nullptr && pFig->getPiece() == targetKingPiece)
			{
				kingX = x;
				kingY = y;
				break;
			}
		}
		if (kingX != -1) break;
	}
		
	if (kingX == -1 || kingY == -1)
	{
		return true;
	}
		

	// Check if ANY opponent piece can attack the King's square
	Player* opponent = (player == _pWhites) ? _pBlacks : _pWhites;

	for (int y = 0; y < MAXROWS; ++y)
	{
		for (int x = 0; x < MAXROWS; ++x)
		{
			Figure* pFig = getFigure(x, y);
			//  If the square contains an opponent's piece
			if ( !pFig->isEmpty() && pFig->getColor() == opponent->getColor())
			{
				if (canFigureDoTheMoveToSquare(pFig, x, y, kingX, kingY))
				{
					return true; // El rey esta en jaque!
				}
			}
		}
	}

	return false; // the king is safe
}

bool Board::isCheckmate(Player* player)
{
	bool bisKingInCheck = false;
	// Condition A: The King must be in check
	if (!isKingInCheck(player))
	{
		return false; // Si no esta en jaque, nunca puede ser jaque mate
	}

	// 2. Condition B: Check if the player has any legal moves available
	std::vector<TPIECEMOVEMENT> actions;
	getPossibleActions(player->getID(),actions); // (Asume que devuelve los movimientos estrictamente legales)

	// If in check AND the legal moves list is empty -> Checkmate!
	bool isEmpty = actions.empty();
	if (isEmpty)
	{
		int x = 0;
	}
	return isEmpty;
}
bool Board::isTerminal()
{
	
	_winPlayer = -1;

	// CHECK THREEFOLD REPETITION AND 50-MOVE RULE
	std::map<uint64_t, int>::iterator it;

	it = _lastBoardsHistory.find(_currentHash);
	if (it != _lastBoardsHistory.end())
	{
		int countSameBoard = _lastBoardsHistory[_currentHash];
		if (countSameBoard >= 3)
		{
			_winPlayer = 3; // 3 significa Empate / Tablas
			return true;    // Salimos pido y ahorramos CPU!
		}
	}

	bool bCheckmateW = isCheckmate(_pWhites);
	bool bCheckmateB = isCheckmate(_pBlacks);

	bool bCheckmate = bCheckmateW || bCheckmateB;

	// 2. Check for Stalemate for the active player
	// (If not in check but has no legal moves, the game ends in a draw)
	std::vector<TPIECEMOVEMENT> currentActions;
	Player *pCurrentPlayer = getPlayerByID(_currentPlayerID);
	getPossibleActions(_currentPlayerID, currentActions);

	if (!isKingInCheck(pCurrentPlayer) && currentActions.empty())
	{
		_winPlayer = 3;
		return true; //  It's a stalemate (Draw)! The game also ends here.
	}

	if (bCheckmateW)
	{
		_winPlayer = 1;
	}
	if (bCheckmateB)
	{
		_winPlayer = 0;
	}

	if (isOnlyTwoKings())
	{
		_winPlayer = 3;

		bCheckmate = true;
	}

	return bCheckmate;
}
int Board::getWinner()
{
	return _winPlayer;
}
double Board::getReward(int playerID)
{
	double reward = 0.0;

	
	if ( (playerID == 0) && (_winPlayer == 0))
	{
		reward = 1.0;
	}
	if ((playerID == 0) && (_winPlayer == 1))
	{
		reward = -1.0;
	}

	if ((playerID == 1) && (_winPlayer == 1))
	{
		reward = 1.0;
	}
	if ((playerID == 1) && (_winPlayer == 0))
	{
		reward = -1.0;
	}
	if (_winPlayer == 3)
	{
		reward = 0.0; // valor ganador 0.5
	}

	return reward;
}
void Board::applyAction(int action)
{
	if (action < 0) // si accion negativa, no es valida
	{
		return; 
	}
	TPIECEMOVEMENT mov = convertToMovement(action);

	_currentPlayerID = (int)!_currentPlayerID;
	Figure figure = mov.figure;

	moveFigure(mov.oldX, mov.oldY, mov.newX, mov.newY, &figure);
}

Player* Board::getPlayerByID(int playerID)
{
	if (playerID == 0)
	{
		return _pWhites;
	}
	else return _pBlacks;
}
void Board::getPossibleActions(int playerID, std::vector<TPIECEMOVEMENT>& actions)
{
	actions.clear();



	// Save the current player to check their King later
	Player* movingPlayer = this->getPlayerByID(playerID);
	// Get all pseudo-legal moves for the current player
	std::vector<TPIECEMOVEMENT> pseudoMoves;
	movingPlayer->getAllMovements(&pseudoMoves);

	for (std::vector<TPIECEMOVEMENT>::iterator it = pseudoMoves.begin(); it != pseudoMoves.end(); ++it)
	{
		TPIECEMOVEMENT mov = *it;

		// Save the destination square state BEFORE moving
		// (Vital in case of a capture, so we can restore the piece later)
		Figure* pCapturedFig = getFigure(mov.newX, mov.newY);
		Piece capturedPiece = !pCapturedFig->isEmpty() ? pCapturedFig->getPiece() : Piece::Empty;
		int capturedColor =   !pCapturedFig->isEmpty() ? pCapturedFig->getColor() : COLOR_EMPTY;

		// Save the piece being moved
		Figure* pMovingFig = getFigure(mov.oldX, mov.oldY);
		Piece movingPiece = pMovingFig->getPiece();
		int movingColor = pMovingFig->getColor();

		// OPTIMIZATION 3: MAKE PATTERN (Make the move FAST directly on this board)
		// We do not use the full moveFigure() if it adds things to _lastBoardsHistory, 
		// we use a lightweight version just for the matrix.
		setFigure(Piece::Empty, mov.oldX, mov.oldY, COLOR_EMPTY); // Vaciamos origen
		setFigure(movingPiece, mov.newX, mov.newY, movingColor);  // Llenamos destino

		// Check if the King is in check on our own board
		if (!isKingInCheck(movingPlayer))
		{
			// is legal
			actions.push_back(mov);
		}
		else
		{
			int x = 0;
		}

		// 5. UNMAKE PATTERN (Undo the move and leave everything as it was)
		setFigure(movingPiece, mov.oldX, mov.oldY, movingColor);
		setFigure(capturedPiece, mov.newX, mov.newY, capturedColor);
	}
}
int Board::getCurrentPlayerID()
{
	return _currentPlayerID;
}
void Board::reset(int typeReset)
{
	if (typeReset == 0)
	{
		ini();
	}
	else
	{
		iniRandom();
	}

	
}
void Board::update(float incTime)
{

}
Metis::IAlphaGoZeroStateBase* Board::clone()
{
	int size = sizeof(Board);
	Board* pBoardCloned = MY_NEW Board(*this);

	return pBoardCloned;

}

int pieceToNumber(Piece aPiece, int color)
{
	int valueNumberPiece = -1;
	switch (aPiece)
	{
	case Piece::Empty:
	{
		break;
	}
	case Piece::BlackPawn:
	case Piece::WhitePawn:
	{
		valueNumberPiece = 1;
		break;
	}
	case Piece::BlackRook:
	case Piece::WhiteRook:
	{
		valueNumberPiece = 2;
		break;
	}
	case Piece::BlackKnight:
	case Piece::WhiteKnight:
	{
		valueNumberPiece = 3;
		break;
	}
	case Piece::BlackQueen:
	case Piece::WhiteQueen:
	{
		valueNumberPiece = 4;
		break;
	}
	case Piece::BlackKing:
	case Piece::WhiteKing:
	{
		valueNumberPiece = 5;
		break;
	}
	}

	if (color == COLOR_BLACK)
	{
		valueNumberPiece = valueNumberPiece * (-1);
	}

	return valueNumberPiece;
}

TPIECEMOVEMENT Board::convertToMovement(int actionID)
{
	TPIECEMOVEMENT mov;

	
	int fromSquare = actionID / NUM_SQUARES;
	int toSquare = actionID % NUM_SQUARES;

	
	mov.oldX = fromSquare % MAXROWS;
	mov.oldY = fromSquare / MAXROWS;
	mov.newX = toSquare % MAXROWS;
	mov.newY = toSquare / MAXROWS;

	Figure* pFigure = getFigure(mov.oldX, mov.oldY);
	if (pFigure != NULL)
	{
		mov.figure = *pFigure;
	}
	else
	{
		int x = 0;
		
	}
	
	
	return mov;
}
bool Board::is_valid_action(int actionID)
{
	
	int fromSquare = actionID / NUM_SQUARES;
	int toSquare = actionID % NUM_SQUARES;

	
	int oldX = fromSquare % MAXROWS;
	int oldY = fromSquare / MAXROWS;
	int newX = toSquare % MAXROWS;
	int newY = toSquare / MAXROWS;

	
	if (oldX < 0 || oldX >= MAXROWS || oldY < 0 || oldY >= MAXROWS ||
		newX < 0 || newX >= MAXROWS || newY < 0 || newY >= MAXROWS)
	{
		return false;
	}

	Figure* pFigure = this->getFigure(oldX, oldY);
	bool clearPath = true;
	if (pFigure->isKnight())
	{
		clearPath = false;
	}

	bool bIsLegalMoved = isValidMovement(oldX, oldY, newX, newY, pFigure,clearPath);
	return  bIsLegalMoved;
}


// thte state is return relative to the current player, so the board will be always relative the the player.
std::vector<float> Board::getState()
{
	std::vector<float> stateVector(MAXINPUTS, 0.0f);

	int currentPlayer = getCurrentPlayerID(); // 0 = whites, 1 = black
	bool flipBoard = (currentPlayer == 1);    // If it is Black's turn, flip vertically

	for (int x = 0; x < MAXROWS; x++)
	{
		for (int y = 0; y < MAXROWS; y++)
		{
			if (!this->isEmpty(x, y))
			{
				Figure* pFigure = this->getFigure(x, y);
				int pieceColor = pFigure->getColor();
				Piece aPiece = pFigure->getPiece();

				
				int pieceOwnerID = (pieceColor == COLOR_WHITE) ? 0 : 1;

				// index of the piece
				int pieceTypeID = pieceToTypeID(aPiece);

				
				int planeID = (pieceOwnerID == currentPlayer) ? pieceTypeID : pieceTypeID + 5;

				// Vertical flip if it is Black's turn: my own side always "on the bottom"
				int relX = x;
				int relY = flipBoard ? (MAXROWS - 1 - y) : y;

				int squareIndex = (relX * MAXROWS) + relY;
				int finalIndex = (planeID * NUM_SQUARES) + squareIndex;
				stateVector[finalIndex] = 1.0f;
			}
		}
	}

	// Turn plane
	int turnPlaneID = 10;
	float colorValue = 1.0f;
	for (int i = 0; i < NUM_SQUARES; i++)
	{
		stateVector[(turnPlaneID * NUM_SQUARES) + i] = colorValue;
	}

	assert(stateVector.size() == MAXINPUTS);
	return stateVector;
}

int Board::pieceToTypeID(Piece p)
{
	switch (p)
	{
	case Piece::WhiteKing:   case Piece::BlackKing:   return 0;
	case Piece::WhiteQueen:  case Piece::BlackQueen:  return 1;
	case Piece::WhiteRook:   case Piece::BlackRook:   return 2;
	case Piece::WhiteKnight: case Piece::BlackKnight: return 3;
	case Piece::WhitePawn:   case Piece::BlackPawn:   return 4;
	default: assert(false); return -1;
	}
}

int Board::getRandomActionFromPlayer(int currentPlayer)
{
	std::vector<TPIECEMOVEMENT> actions;

	
	getPossibleActions(currentPlayer, actions);
	
	// Protection against empty lists (Checkmate or Stalemate)
	if (actions.empty())
	{
		return -1;
	}

	
	int randomIndex = rand() % actions.size();

	// get random movement
	TPIECEMOVEMENT randomMovement = actions[randomIndex];

	// translate the struct movement to a index number (int actionID)
	int actionID = randomMovement.toIndex(currentPlayer);
	return actionID;
}


bool Board::checkBoardSanity()
{
	// Contadores para Blancas
	int wKings = 0, wQueens = 0, wRooks = 0, wBishops = 0, wKnights = 0, wPawns = 0;
	// Contadores para Negras
	int bKings = 0, bQueens = 0, bRooks = 0, bBishops = 0, bKnights = 0, bPawns = 0;

	// Recorremos el tablero entero
	for (int i = 0; i < MAXROWS; i++)
	{
		for (int j = 0; j < MAXROWS; j++)
		{
			int color = getSquareColor(i, j);
			if (color == COLOR_EMPTY) continue;

			Figure* pFigure = getFigure(i, j);
			if (pFigure != nullptr)
			{
				
				
				Piece pieceType = pFigure->getPiece();

				// Asumimos que 0 es Blanco y 1 es Negro, o usa tus constantes (ej. COLOR_WHITE)
				if (color == COLOR_WHITE) // Blancas
				{
					if (pieceType == Piece::WhiteKing) wKings++;
					else if (pieceType == Piece::WhiteQueen) wQueens++;
					else if (pieceType == Piece::WhiteRook) wRooks++;
					else if (pieceType == Piece::WhiteBishop) wBishops++;
					else if (pieceType == Piece::WhiteKnight) wKnights++;
					else if (pieceType == Piece::WhitePawn) wPawns++;
				}
				else // Negras
				{
					if (pieceType == Piece::BlackKing) bKings++;
					else if (pieceType == Piece::BlackQueen) bQueens++;
					else if (pieceType == Piece::BlackRook) bRooks++;
					else if (pieceType == Piece::BlackBishop) bBishops++;
					else if (pieceType == Piece::BlackKnight) bKnights++;
					else if (pieceType == Piece::BlackPawn) bPawns++;
				}
			}
		}
	}

	// --- CHEQUEOS ESTRICTOS DE CANTIDAD ---
	bool bIsValid = true;

	// 1. Reyes (Debe haber exactamente 1 por bando)
	if (wKings != 1 || bKings != 1) {
		std::cout << "[ERROR MCTS] Reyes invidos. Blancos: " << wKings << ", Negros: " << bKings << "\n";
		bIsValid = false;
	}

	// 2. Peones (Mximo 8)
	if (wPawns > 8 || bPawns > 8) {
		std::cout << "[ERROR MCTS] Demasiados peones. Blancos: " << wPawns << ", Negros: " << bPawns << "\n";
		bIsValid = false;
	}

	// 3. Piezas mayores/menores (Mximo original sin contar coronacn)
	if (wRooks > 2 || bRooks > 2) {
		std::cout << "[ERROR MCTS] Demasiadas torres. Blancas: " << wRooks << ", Negras: " << bRooks << "\n";
		bIsValid = false;
	}
	if (wBishops > 2 || bBishops > 2) {
		std::cout << "[ERROR MCTS] Demasiados alfiles. Blancos: " << wBishops << ", Negros: " << bBishops << "\n";
		bIsValid = false;
	}
	if (wKnights > 2 || bKnights > 2) {
		std::cout << "[ERROR MCTS] Demasiados caballos. Blancos: " << wKnights << ", Negros: " << bKnights << "\n";
		bIsValid = false;
	}
	if (wQueens > 1 || bQueens > 1) {
		std::cout << "[ERROR MCTS] Demasiadas reinas. Blancas: " << wQueens << ", Negras: " << bQueens << "\n";
		bIsValid = false;
	}

	// Si quieres que el programa se detenga automticamente en Debug si esto falla:
	assert(bIsValid && "Error de corrupcion en el tablero (Piezas duplicadas)");

	return bIsValid;
}

// return true is the information of the state of the board is relative to the player
bool Board::isStateRelativeToPlayer()
{
	return true; // return true becoause on getState() we return the information always relative to the current player
}
// return a heuristic value of the value of the board from the point of view of the playerToEvaluateID
// range: -1.0 to +1.0
// used in the MCTS in Metis::Core to help a little bit to learn faster
float Board::getMaterialHeuristicValue(int playerToEvaluateID)
{
	float scoreWhite = 0.0f;
	float scoreBlack = 0.0f;

	// Iterate through the board and sum standard piece values
	for (int r = 0; r < MAXROWS; ++r) {
		for (int c = 0; c < MAXROWS; ++c) {
			if (!isEmpty(r, c)) 
			{
				Figure* pFig = getFigure(r, c);
				float value = 0.0f;

				switch (pFig->getPiece()) 
				{
					case Piece::WhitePawn: case Piece::BlackPawn: value = 1.0f; break;
					case Piece::WhiteKnight: case Piece::BlackKnight:
					case Piece::WhiteBishop: case Piece::BlackBishop: value = 3.0f; break;
					case Piece::WhiteRook: case Piece::BlackRook: value = 5.0f; break;
					case Piece::WhiteQueen: case Piece::BlackQueen: value = 9.0f; break;
					default: break;
				}

				if (pFig->getColor() == COLOR_WHITE) 
					scoreWhite += value;
				else scoreBlack += value;
			}
		}
	}

	// Calculate advantage for the requested player
	float advantage = (playerToEvaluateID == 0) ? (scoreWhite - scoreBlack) : (scoreBlack - scoreWhite);

	// Normalize to an approximate range of [-1.0, 1.0].
	float normalizedScore = advantage / 15.0f;

	if (normalizedScore > 1.0f) normalizedScore = 1.0f;
	if (normalizedScore < -1.0f) normalizedScore = -1.0f;

	return normalizedScore;
}
