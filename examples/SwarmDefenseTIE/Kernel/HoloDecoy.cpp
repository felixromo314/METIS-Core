#include "SpaceEnviroment.h"
#include "HoloDecoy.h"

HoloDecoy::HoloDecoy(int x, int y)
{

	_position.x = x;
	_position.y = y;

	_life = MAX_LIFE_HoloDecoy;
	_bDelete = false;
}
HoloDecoy::HoloDecoy(const HoloDecoy& aHoloDecoy)
{
	_position.x = aHoloDecoy._position.x;
	_position.y = aHoloDecoy._position.y;

	_life = aHoloDecoy._life;
	_bDelete = _bDelete;
}

bool HoloDecoy::IsFlying()
{
	if (_bDelete)
	{
		return false;
	}
	return !WasFire();
}
bool HoloDecoy::WasFire()
{
	bool bWasFire = false;
	double diff = MAX_LIFE_HoloDecoy - _life;
	if (diff <= 0)
	{
		bWasFire = true;
	}
	return bWasFire;
}

Metis::Vector2D HoloDecoy::GetPosition()
{
	return _position;
}

void HoloDecoy::Update()
{
	_life -= METIS_INC_TIME;
	if (_life < 0)
	{
		_bDelete = true;
	}
}

bool HoloDecoy::IsExpireLife()
{
	return _bDelete;
}