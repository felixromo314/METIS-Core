#include "stdafx.h"
#include "Player.h"
#include "MovementsGenerator.h"
#include "Board.h"
#include <cmath>
#include <algorithm>


unsigned long MovementsGenerator::_counterMovementsIDs=0;

MovementsGenerator::MovementsGenerator()
{
    
}
MovementsGenerator::~MovementsGenerator()
{

}

void MovementsGenerator::addMovement(std::vector<TPIECEMOVEMENT> *movements, Figure* pFigure,int x,int y,int newX,int newY)
{
    TPIECEMOVEMENT mov;


    mov.movementID = MovementsGenerator::_counterMovementsIDs;
    mov.figure = *pFigure;
    mov.oldX = x;
    mov.oldY = y;
    mov.newX = newX;
    mov.newY = newY;

    if (mov.movementID == 5)
    {
        int x = 0;
    }

    movements->push_back(mov);

    MovementsGenerator::_counterMovementsIDs++; // incrmentar
}



void MovementsGenerator::generateMovementForKnight(Board* pBoard, int x, int y, Figure* pFigure, int radio, std::vector<TPIECEMOVEMENT> *movements)
{

    int relativMovs[8][2] = {   {-2,1},
                                {-1,2},
                                {1,2},
                                {2,1},
                                {2,-1},
                                {+1,-2},
                                {-1,-2},
                                {-2,-1} };
    /*
    Meaning of the relative positions in relativMovs
    These are the relative moves a Knight can make


    x7x0x		0	=> relativMovs[0][0]=-2	relativMovs[0][1]=1
    6xxx1
    xxCxx
    5xxx2
    x4x3x
*/


    for (int indexMov = 0; indexMov < 8; indexMov++)
    {
        int newX = x + relativMovs[indexMov][0];
        int newY = y + relativMovs[indexMov][1];

        if (pBoard->isValidMovement(x, y, newX, newY, pFigure,false))
        {
            addMovement(movements,pFigure,x, y, newX, newY);
        }
    }

}

bool MovementsGenerator::isKingMoveLegal(int target_x, int target_y, int enemy_king_x, int enemy_king_y)
{
    // Calculate Chebyshev distance
    int distance_x = std::abs(target_x - enemy_king_x);
    int distance_y = std::abs(target_y - enemy_king_y);

    // Evaluate if the 1-square perimeter overlaps
    // If the maximum distance on any axis is <= 1, they are adjacent. // The distance is safe
    if (std::max(distance_x, distance_y) <= 1) {
        return false;
    }

    return true; // The distance is safe
}

void MovementsGenerator::generateMovementForKing(Board* pBoard, int x, int y, Figure* pFigure, std::vector<TPIECEMOVEMENT>* movements)
{
    int xEnemyKing, yEnemyKing;
    int color = pFigure->getColor();

    int colorEnemy;
    if (color == COLOR_WHITE)
        colorEnemy = COLOR_BLACK;
    else colorEnemy = COLOR_WHITE;
    pBoard->getPositionKing(colorEnemy, &xEnemyKing, &yEnemyKing);

    std::vector<TPIECEMOVEMENT> movHorVer;
    generateMovementForRook(pBoard, x, y, pFigure, 2,&movHorVer);

    std::vector<TPIECEMOVEMENT> movDiagonal;
    generateMovementForBishop(pBoard, x, y, pFigure, 2,&movDiagonal);

    std::vector<TPIECEMOVEMENT>::iterator it;

    for (it = movHorVer.begin(); it != movHorVer.end(); it++)
    {
        const TPIECEMOVEMENT& mov = *it;

        if (isKingMoveLegal(mov.newX, mov.newY, xEnemyKing, yEnemyKing))
        {
            movements->push_back(mov);
        }
        else
        {
            int x = 0;
        }

    }
    for (it = movDiagonal.begin(); it != movDiagonal.end(); it++)
    {
        const TPIECEMOVEMENT& mov = *it;

        if (isKingMoveLegal(mov.newX, mov.newY, xEnemyKing, yEnemyKing))
        {
            movements->push_back(mov);
        }
        else
        {
            int x = 0;
        }

    }

}

void MovementsGenerator::generateMovementForQueen(Board* pBoard, int x, int y, Figure* pFigure, std::vector<TPIECEMOVEMENT>* movementsparam)
{
    std::vector<TPIECEMOVEMENT> movHorVer;
    generateMovementForRook(pBoard, x, y, pFigure,MAXROWS,&movHorVer);

    std::vector<TPIECEMOVEMENT> movDiagonal;
    generateMovementForBishop(pBoard, x, y, pFigure, MAXROWS,&movDiagonal);

    

    movementsparam->insert(movementsparam->end(), movHorVer.begin(), movHorVer.end());
    movementsparam->insert(movementsparam->end(), movDiagonal.begin(), movDiagonal.end());

}


void MovementsGenerator::generateMovementForBishop(Board* pBoard, int x, int y, Figure* pFigure, int radio, std::vector<TPIECEMOVEMENT> *movements)
{
    int relativMovs[4][2] = {   {-1,1},{1,1},
                                {1,-1},{-1,-1} };

    
    for (int radioIndex = 1; radioIndex < radio; radioIndex++)
    {

        for (int indexPoints = 0; indexPoints < 4; indexPoints++)
        {
            int newX = x + relativMovs[indexPoints][0] * radioIndex;
            int newY = y + relativMovs[indexPoints][1] * radioIndex;

            if (pBoard->isValidMovement(x, y, newX, newY,pFigure))
            {
                addMovement(movements, pFigure, x, y, newX, newY);
            }
        }
    }
}

// Generates the Rook-like moves that can be made from a given position determined by posi
// 'pieza' is the piece to be stored in the move list,
// and 'radio' is the number of squares it can move (range/radius)
// This method is used to generate moves for Rooks, Kings, and Queens
// For Rook  ==> pieza = ROOK   radio = 6
// For Queen ==> pieza = QUEEN  radio = 6
// For King  ==> pieza = KING   radio = 1

void MovementsGenerator::generateMovementForRook(Board * pBoard, int x, int y, Figure * pFigure,int radio, std::vector<TPIECEMOVEMENT>* movements)
{
    int relativMovs[4][2] = {   {0,1},{1,0},
                                {0,-1},{-1,0} 
                            };


    /*
    Meaning of the relative positions in relativMovs
    These are the relative moves a Rook can make

    x3x
    2R0
    x1x
    R	=> Rook
    0	=> relativMovs[0][0]=0	relativMovs[0][1]=1
    1	=> relativMovs[1][0]=1	relativMovs[1][1]=0
*/

    for (int radioIndex = 1; radioIndex < radio; radioIndex++)
    {
        
            for (int indexPoints = 0; indexPoints < 4; indexPoints++)
            {
                int relativX = relativMovs[indexPoints][0];
                int relativY = relativMovs[indexPoints][1];

                int newX = x + relativX * radioIndex;
                int newY = y + relativY * radioIndex;

                if (pBoard->isValidMovement(x,y,newX, newY, pFigure))
                {
                    addMovement(movements, pFigure, x, y, newX, newY);

                }
            }
    }


}
void MovementsGenerator::generateMovementForPawn(Board *pBoard,int x,int y,Figure *pFigure, bool movementUP, std::vector<TPIECEMOVEMENT>* pPieceMovement)
{
    
    int distanceMovement = 0;
    distanceMovement = (movementUP == true) ? 1 : -1;

    int newY = y + distanceMovement;// según el valor de sumar se suma o se resta.

    if (pBoard->isEmpty(x, newY))
    {
        addMovement(pPieceMovement, pFigure, x, y, x, newY);
    }

    //if ( (y == 1) || (y == (MAXROWS - 1)))
    //{
    //    newY = y + distanceMovement + distanceMovement;// según el valor de sumar se suma o se resta.
    //    if (pBoard->isEmpty(x, newY))
    //    {
    //        addMovement(pPieceMovement, pFigure, x, y, x, newY);
    //    }
    //}
    
    // Check if we can capture an enemy piece
    int newXDiagonal;
    int newYDiagonal;
    if (movementUP)
    {
        newXDiagonal = x + 1;
        newYDiagonal = y + 1;
        int colorPiece = pFigure->getColor();
        if (pBoard->canCapture(colorPiece,newXDiagonal, newYDiagonal))
        {
            addMovement(pPieceMovement, pFigure, x, y, newXDiagonal, newYDiagonal);
        }

        newXDiagonal = x - 1;
        newYDiagonal = y + 1;
        if (pBoard->canCapture(colorPiece, newXDiagonal, newYDiagonal))
        {
            addMovement(pPieceMovement, pFigure, x, y, newXDiagonal, newYDiagonal);
        }

    }
    else
    {
        newXDiagonal = x + 1;
        newYDiagonal = y - 1;
        int colorPiece = pFigure->getColor();
        if (pBoard->canCapture(colorPiece, newXDiagonal, newYDiagonal))
        {
            addMovement(pPieceMovement, pFigure, x, y, newXDiagonal, newYDiagonal);
        }
        newXDiagonal = x - 1;
        newYDiagonal = y - 1;
        if (pBoard->canCapture(colorPiece, newXDiagonal, newYDiagonal))
        {
            addMovement(pPieceMovement, pFigure, x, y, newXDiagonal, newYDiagonal);
        }
    }
    
}
void MovementsGenerator::generateMovementForFigure(Board *pBoard,Figure *pFigure,int x,int y, std::vector<TPIECEMOVEMENT>* pPieceMovement)
{

	Piece aPiece = pFigure->getPiece();
    

    switch (aPiece)
    {
    case Piece::Empty:
    {
        
        break;
    }
    case Piece::BlackPawn:
    {
        generateMovementForPawn(pBoard, x, y, pFigure,false, pPieceMovement);
        break;
    }
        
    case Piece::WhitePawn:
    {
        generateMovementForPawn(pBoard, x, y, pFigure,true, pPieceMovement);
        break;
    }
    case Piece::BlackRook:
    case Piece::WhiteRook:
    {
        generateMovementForRook(pBoard, x, y, pFigure,MAXROWS, pPieceMovement);
        break;
    }
    case Piece::BlackKnight:
    case Piece::WhiteKnight:
    {
        generateMovementForKnight(pBoard, x, y, pFigure, MAXROWS, pPieceMovement);
        break;
    }
    case Piece::BlackBishop:
    case Piece::WhiteBishop:
    {
        generateMovementForBishop(pBoard, x, y, pFigure, MAXROWS, pPieceMovement);
        break;
    }
    case Piece::BlackQueen:
    case Piece::WhiteQueen:
    {
        generateMovementForQueen(pBoard, x, y, pFigure, pPieceMovement);
        break;
    }
    case Piece::BlackKing:
    case Piece::WhiteKing:
    {
        generateMovementForKing(pBoard, x, y, pFigure, pPieceMovement);
        break;
    }
    }
	
}

void MovementsGenerator::generateFromPlayer(Player* pPlayer, std::vector<TPIECEMOVEMENT>* movements)
{
	Board *pBoard = pPlayer->getBoard();
	int colorPlayer = pPlayer->getColor();

	int x, y;

	for (x = 0; x < MAXROWS; x++)
	{
		for (y = 0; y < MAXROWS; y++)
		{
			Figure *pFigure = pBoard->getFigure(x, y);
			if (pFigure->getPiece() != Piece::Empty)
			{
				if (pFigure->getColor() == colorPlayer)
				{
					// generate movement por this piece
                    std::vector<TPIECEMOVEMENT> pieceMovement;
                    generateMovementForFigure(pBoard, pFigure, x, y, &pieceMovement);
                    movements->insert(movements->end(), pieceMovement.begin(), pieceMovement.end());
                    
				}
			}
		}
	}

}
