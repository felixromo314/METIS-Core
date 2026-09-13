#include "stdafx.h"
#include "Square.h"
#include "Figure.h"

Square::Square()
{

}
Square::~Square()
{

}

Figure& Square::getFigure()
{
	return _figure;
}

void Square::setFigure(Figure& pFigure)
{
	_figure = pFigure;
}

