#pragma once
#include <vector>

class Player;

class MovementsGenerator
{
private:
	static unsigned long _counterMovementsIDs;

	void generateMovementForPawn(Board* pBoard, int x, int y, Figure* pFigure,bool movementUP, std::vector<TPIECEMOVEMENT> *pPieceMovement);
	void generateMovementForRook(Board* pBoard, int x, int y, Figure* pFigure,int radio, std::vector<TPIECEMOVEMENT>* pPieceMovement);
	void generateMovementForBishop(Board* pBoard, int x, int y, Figure* pFigure, int radio, std::vector<TPIECEMOVEMENT>* pPieceMovement);
	void generateMovementForKnight(Board* pBoard, int x, int y, Figure* pFigure, int radio, std::vector<TPIECEMOVEMENT>* pPieceMovement);
	void generateMovementForQueen(Board* pBoard, int x, int y, Figure* pFigure, std::vector<TPIECEMOVEMENT>* pPieceMovement);
	void generateMovementForKing(Board* pBoard, int x, int y, Figure* pFigure, std::vector<TPIECEMOVEMENT>* pPieceMovement);
	bool isKingMoveLegal(int target_x, int target_y, int enemy_king_x, int enemy_king_y);

	void addMovement(std::vector<TPIECEMOVEMENT> *movements, Figure* pFigure, int x, int y, int newX, int newY);
public:
	MovementsGenerator();
	~MovementsGenerator();

	void generateMovementForFigure(Board* pBoard, Figure* pFigure, int x, int y, std::vector<TPIECEMOVEMENT>* pPieceMovement);
	void generateFromPlayer(Player* pPlayer, std::vector<TPIECEMOVEMENT>* pPieceMovement);
};

