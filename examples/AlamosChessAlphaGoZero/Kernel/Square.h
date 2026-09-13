#pragma once
#include "Figure.h"


class Square
{
private:

	Figure _figure;

public:
	Square();
	~Square();

	Figure &getFigure();
	void setFigure(Figure &pFigure);
};

