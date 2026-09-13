#include "stdafx.h"
#include "Figure.h"

// Helper function or LUT to convert Piece -> int [0..11]
int pieceToIndex(Piece piece) 
{
	static int lut[256];
	static bool initialized = false;

	if (!initialized) 
	{
		// Initialize everything to -1 (works byte-by-byte for -1 in two's complement)
		std::memset(lut, -1, sizeof(lut));

		// Whites (0..5)
		lut[(unsigned char)Piece::WhitePawn] = 0;
		lut[(unsigned char)Piece::WhiteKnight] = 1;
		lut[(unsigned char)Piece::WhiteBishop] = 2;
		lut[(unsigned char)Piece::WhiteRook] = 3;
		lut[(unsigned char)Piece::WhiteQueen] = 4;
		lut[(unsigned char)Piece::WhiteKing] = 5;

		// Blacks (6..11)
		lut[(unsigned char)Piece::BlackPawn] = 6;
		lut[(unsigned char)Piece::BlackKnight] = 7;
		lut[(unsigned char)Piece::BlackBishop] = 8;
		lut[(unsigned char)Piece::BlackRook] = 9;
		lut[(unsigned char)Piece::BlackQueen] = 10;
		lut[(unsigned char)Piece::BlackKing] = 11;

		initialized = true;
	}

	return lut[(unsigned char)piece];
}

int pieceToColorIndex(Piece piece) 
{
	switch (piece) {
	case Piece::WhitePawn: case Piece::WhiteKnight: case Piece::WhiteBishop:
	case Piece::WhiteRook: case Piece::WhiteQueen:  case Piece::WhiteKing:
		return 0; // White
	case Piece::BlackPawn: case Piece::BlackKnight: case Piece::BlackBishop:
	case Piece::BlackRook: case Piece::BlackQueen:  case Piece::BlackKing:
		return 1; // Black
	default: return -1;
	}
}

Figure::Figure()
{
	_piece = Piece::Empty;
	_color = -1;
}
Figure::Figure(Piece piece, int color)
{

	if (color == COLOR_WHITE)
	{
		char charPiece = (char)piece;
		if ((charPiece >= (char)Piece::BlackPawn) && (charPiece <= (char)Piece::BlackKing))
		{
			int x = 0;  // se esta asignado color blanco a una pieze negra
		}
	}

	if (color == COLOR_BLACK)
	{
		char charPiece = (char)piece;
		if ((charPiece >= (char)Piece::WhitePawn) && (charPiece <= (char)Piece::WhiteKing))
		{
			int x = 0;  // se esta asignado color blanco a una pieze negra
		}
	}


	_piece = piece;
	_color = color;

	
}

Piece Figure::getPiece()
{
	return _piece;
}
int Figure::getColor()
{
	return _color;
}

bool Figure::isEmpty()
{
	if (_piece == Piece::Empty)
	{
		return true;
	}
	else return false;
}
bool Figure::isKnight()
{
	if ((_piece == Piece::BlackKnight) || (_piece == Piece::WhiteKnight))
	{
		return true;
	}
	else return false;
}
