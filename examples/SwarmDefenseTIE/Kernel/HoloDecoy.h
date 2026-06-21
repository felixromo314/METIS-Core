#pragma once

#include "METIS-Core.h"

#define MAX_HoloDecoy 1000
#define MAX_LIFE_HoloDecoy 20

class HoloDecoy
{
	private:

	Metis::Vector2D _position;
	double _life;
	bool _bDelete;

	public:

	HoloDecoy(int x, int y);
	HoloDecoy(const HoloDecoy& aHoloDecoy);

	Metis::Vector2D GetPosition();
	void Update();

	bool IsExpireLife();

	bool WasFire();
	bool IsFlying();
};

