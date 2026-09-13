#pragma once
#include "Square.h"
#include "Player.h"


typedef struct stBoard
{
	int stateBoard;
}TBOARD;

class Board : public Metis::IAlphaGoZeroState<TBOARD,TPIECEMOVEMENT>
{

	static int _countBoard;
	static int _countBoardInmemory;
	int _id;
	Square _squares[MAXROWS][MAXROWS];

	uint64_t _currentHash;

	std::map<uint64_t, int> _lastBoardsHistory; // <board hash state, number of times this state has been repeated>

	Player *_pWhites;
	Player *_pBlacks;
	int _currentPlayerID;


	// 1: win 1 : 2:win 3:draw
	int _winPlayer;

	void setFigure(Piece piece, int x, int y, int color);
	bool isPathClear(int x, int y, int newX, int newY);

	bool isInsideBoard(int x, int y);

	std::string buildHashFromBoard(int color);
	uint64_t buildZobristHash(int color);
public:


	Board(Player* pWhites, Player* pBlacks);
	Board(const Board &otherBoard);
	~Board();

	Board& operator=(const Board& other);

	int getSquareColor(int x, int y);
	Figure *getFigure(int x, int y);
	bool isEmpty(int x, int y);
	bool canCapture(int colorPiece, int x, int y);
	bool isValidMovement(int x, int y, int newX, int newY, Figure* pFigure,bool checkClearPath=true);

	void moveFigure(int x, int y, int newX, int newY, Figure* pFigure);
	bool isKingInCheck(Player* player);
	bool getPositionKing(int color, int* x, int* y);
	bool isOnlyTwoKings();
	bool canFigureDoTheMoveToSquare(Figure* pFig, int x, int y, int newX, int newY);
	TPIECEMOVEMENT convertToMovement(int actionID);
	bool isCheckmate(Player* player);
	Player* getPlayerByID(int playerID);
	int pieceToTypeID(Piece p);

	void clear();
	void iniLosAlamosChess();

	void iniRandom();
	void ini();
	void iniInMateCheck();
	bool checkBoardSanity();
	
	
	//-------------------------------------------------------------------------------
	// override from Metis Core
	virtual float getMaterialHeuristicValue(int playerToEvaluateID);
	virtual bool isStateRelativeToPlayer();
	virtual int getNumInputs();
	virtual int getNumActions();
	virtual bool isTerminal();
	virtual double getReward(int playerID);
	virtual int getWinner();
	virtual void applyAction(int action);
	virtual void getPossibleActions(int playerID,std::vector<TPIECEMOVEMENT>& actions);
	virtual int getCurrentPlayerID();
	virtual void reset(int typeReset);
	virtual void update(float incTime = 0.01);
	virtual IAlphaGoZeroStateBase* clone(); // Vital para la simulación
	virtual std::vector<float> getState();
	virtual bool is_valid_action(int actionID);
	virtual int  getRandomActionFromPlayer(int currentPlayer);
	//-------------------------------------------------------------------------------
	
};

