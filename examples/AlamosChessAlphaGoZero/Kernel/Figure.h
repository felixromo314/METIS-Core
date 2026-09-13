#pragma once

#define COLOR_EMPTY 0
#define COLOR_WHITE 1
#define COLOR_BLACK 2


enum class Piece : char {
    // Empty space
    Empty = '.',

    // White Pieces (Uppercase) 
    WhitePawn = 'P',
    WhiteKnight = 'N',
    WhiteBishop = 'B',
    WhiteRook = 'R',
    WhiteQueen = 'Q',
    WhiteKing = 'K',

    // Black Pieces (Lowercase)
    BlackPawn = 'p',
    BlackKnight = 'n',
    BlackBishop = 'b',
    BlackRook = 'r',
    BlackQueen = 'q',
    BlackKing = 'k'
};

int pieceToIndex(Piece piece);
int pieceToColorIndex(Piece piece);

class Figure
{
private:

	int _color;
    Piece _piece;

public:

    Figure();
	Figure(Piece piece,int color);
	//~Figure();

    Piece getPiece();
    int getColor();
    bool isKnight();
    bool isEmpty();

};

