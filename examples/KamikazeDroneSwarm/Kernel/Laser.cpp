#include "Laser.h"
#include "Spacecraft.h"
#include "SpaceEnviroment.h"

#include <cmath>

using namespace Metis;

Laser::Laser(Spacecraft* pPlatform)
{
	_pPlatform = pPlatform;

	_icycle = 0;


	_maxTemperature = 100.0;
	_heatPerShot = 15.0;         // Each shot adds 10
	_coolingRate = 2.5;          // Per second
	_coolingDown = false;
	_active = true;

	_icycle = 0;
	_lastUpdateTime = 0;

	_timeFiring = 1000;
	_turretAngleDeg = 0.0;


}
Laser::~Laser()
{

}

void Laser::Rotate_Left()
{
	_turretAngleDeg += ROTATION_INC_DEG;

	_turretAngleDeg = normalizeAngleDeg(_turretAngleDeg);
}

double Laser::CalculateDistanceToBullets(Metis::Vector2D& position)
{
	std::deque<Laser_BULLET>::iterator it;

	double minDistance = 100000;
	for (it = _bullets.begin(); it != _bullets.end(); it++)
	{
		const Laser_BULLET& bullet = *it;

		Metis::Vector2D bulletPos(bullet.x, bullet.y);

		double dist = position.DistanceTo(bulletPos);
		if (dist < minDistance)
		{
			minDistance = dist;
		}
	}

	return minDistance;
}
void Laser::Rotate_Right()
{
	_turretAngleDeg -= ROTATION_INC_DEG;

	_turretAngleDeg = normalizeAngleDeg(_turretAngleDeg);

}



double Laser::calculateInterceptionTime(Spacecraft& target, double bulletSpeed)
{
	Vector2D targetPos = target.getPosition();
	Vector2D LaserPos = this->_pPlatform->getPosition(); // Assuming you have the Laser position

	// Relative distance vector
	Vector2D D = { targetPos.x - LaserPos.x, targetPos.y - LaserPos.y };

	// Missile velocity vector
	Vector2D dir;
	dir = target.getVelocity();
	dir = dir.Normalize();
	double speedM = target.getSpeed();
	Vector2D Vm;
	Vm.x = dir.x * speedM;
	Vm.y = dir.y * speedM;

	// Coefficients of the quadratic equation: at^2 + bt + c = 0
	double a = (Vm.x * Vm.x + Vm.y * Vm.y) - (bulletSpeed * bulletSpeed);
	double b = 2.0 * (Vm.x * D.x + Vm.y * D.y);
	double c = (D.x * D.x + D.y * D.y);

	// Calculate the discriminant (b^2 - 4ac)
	double discriminant = b * b - 4.0 * a * c;

	if (discriminant < 0) return -1.0; // The missile is unreachable

	// Use the quadratic formula to find t
	// Use the negative sign in the numerator to get the shortest time
	double t1 = (-b + sqrt(discriminant)) / (2.0 * a);
	double t2 = (-b - sqrt(discriminant)) / (2.0 * a);

	//Return the smallest positive time
	if (t1 > 0 && t2 > 0) return fmin(t1, t2);
	if (t1 > 0) return t1;
	if (t2 > 0) return t2;

	return -1.0;
}



Metis::Vector2D Laser::predictPositionTarget(Spacecraft& target, double timeInAdvance)
{

	timeInAdvance = calculateInterceptionTime(target, BULLET_SPEED);

	Vector2D targetPos = target.getPosition();

	Vector2D direction;
	direction = target.getVelocity();
	direction = direction.Normalize();
	double speed = target.getSpeed();

	Vector2D predictedPos;
	double x, y;

	x = speed * direction.x;
	y = speed * direction.y;

	predictedPos.x = targetPos.x + x;
	predictedPos.y = targetPos.y + y;

	return predictedPos;
}


void Laser::clean()
{
	_bullets.clear();

	_icycle = 0;

	_targetsIDs.clear();



	_maxTemperature = 100.0;
	_heatPerShot = 15.0;         // Each shot adds 10
	_coolingRate = 2.5;          // Per second
	_coolingDown = false;
	_active = true;

	_icycle = 0;
	_lastUpdateTime = 0;

	_timeFiring = 1000;
}
bool Laser::isReadyToFire()
{

	bool bisReadyToFire = true;
	if ((_targetsIDs.size() > 0) && (_bullets.size() >= MAX_BULLETS))
	{
		bisReadyToFire = false;
	}

	//-------------------- temperature
	if (!_active)
	{
		bisReadyToFire = false;
	}

	return  bisReadyToFire;
}
bool Laser::operator!=(const Laser& other)
{
	bool bNotEqual = false;

	if (_timeFiring != other._timeFiring) bNotEqual = true;
	if (_bulletPerFire != other._bulletPerFire) bNotEqual = true;

	assert(_pPlatform != other._pPlatform);
	// if the pointer points to the same object, we flag it as not equal, this would not be correct
	if (_pPlatform == other._pPlatform) bNotEqual = true;

	if (_temperature != other._temperature) bNotEqual = true;
	if (_maxTemperature != other._maxTemperature) bNotEqual = true;
	if (_heatPerShot != other._heatPerShot) bNotEqual = true;
	if (_coolingRate != other._coolingRate) bNotEqual = true;
	if (_coolingDown != other._coolingDown) bNotEqual = true;
	if (_active != other._active) bNotEqual = true;

	if (_bullets != other._bullets)
		bNotEqual = true;
	if (_targetsIDs != other._targetsIDs) bNotEqual = true;

	//if (_lastUpdateTime != other._lastUpdateTime) bNotEqual = true;
	if (_turretAngleDeg != other._turretAngleDeg) bNotEqual = true;
	if (_minBulletDistanceToTarget != other._minBulletDistanceToTarget) bNotEqual = true;


	return bNotEqual;
}
Laser& Laser::operator=(const Laser& other)
{
	if (this == &other)
	{
		return *this;
	}

	// 2. Copy primitive and scalar types
	_timeFiring = other._timeFiring;
	_bulletPerFire = other._bulletPerFire;
	_temperature = other._temperature;
	_maxTemperature = other._maxTemperature;
	_heatPerShot = other._heatPerShot;
	_coolingRate = other._coolingRate;
	_coolingDown = other._coolingDown;
	_active = other._active;
	_icycle = other._icycle;
	_lastUpdateTime = other._lastUpdateTime;
	_turretAngleDeg = other._turretAngleDeg;
	_minBulletDistanceToTarget = other._minBulletDistanceToTarget;

	
	
	// If the Laser class owned this object, you would need to clone it or manage the memory beforehand.
	_pPlatform = other._pPlatform;

	
	// std::deque performs a deep copy of all its elements (Laser_BULLET).
	_bullets = other._bullets;
	if (_bullets.size() > 0)
	{
		int x = 0;
	}

	
	// This is correct if the targets are shared or managed externally.
	_targetsIDs = other._targetsIDs;
	if (_targetsIDs.size())
	{
		int x = 0;
	}
		
	return *this;
}

double Laser::getTemperatureValue()
{
	return _temperature;
}
double Laser::getTemperature()
{
	double t;

	t = _temperature / _maxTemperature;

	return t;
}
void Laser::checkTemperature()
{
	_temperature += _heatPerShot;
	if (_temperature >= _maxTemperature)
	{
		_active = false;
		_coolingDown = true;
	}
}

void Laser::updateTemperature(double currentTime)
{
	double delta = currentTime - (double)_lastUpdateTime;
	_lastUpdateTime = currentTime;

	if (_coolingDown)
	{
		_temperature -= _coolingRate * delta;
		if (_temperature <= 30.0)
		{
			_temperature = 30.0;
			_coolingDown = false;
			_active = true;
		}
	}
	else
	{
		// Passive cooling when not firing
		_temperature -= (_coolingRate / 2.0) * delta;
		if (_temperature < 0.0)
			_temperature = 0.0;
	}
}

double Laser::getRnd(double min, double max)
{
	// Generate a factor between 0.0 and 1.0
	double factor = (double)rand() / RAND_MAX;

	// Scale it to the desired range
	return min + factor * (max - min);
}

void Laser::Fire()
{


	if (_bullets.size() >= MAX_BULLETS)
	{
		return;
	}

	//-------------------- Temperature and state control
	if (!_active)
	{
		return;
	}
	checkTemperature();

	_minBulletDistanceToTarget = 10000.0;

	// 1. Get the origin position
	Vector2D positionPlatform = _pPlatform->getPosition();



	// 2. Convert the physical turret angle to radians
	float angleRad = _turretAngleDeg * M_PI / 180.0f;

	// 3. Compute the direction vector (already normalized by definition thanks to cos/sin)
	Vector2D vNormalize(sin(angleRad), cos(angleRad));

	double separacionPiexeles = 9.0; // Spacing between bullets in the burst

	// 4. Fire the burst in the direction of the cannon
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		Laser_BULLET bullet;

		// The initial offset of each bullet to form the burst line
		double xx = vNormalize.x * i * separacionPiexeles;
		double yy = vNormalize.y * i * separacionPiexeles;

		bullet.x = positionPlatform.x + xx;
		bullet.y = positionPlatform.y + yy;

		// The bullet keeps the same heading as the turret
		bullet.headingDeg = _turretAngleDeg;

		bullet._speed = BULLET_SPEED;
		bullet._distanceTravel = BULLET_MAX_DISTANCE_TRAVEL;
		bullet.bHitTarget = 0; // 0 means it is still in the air

		_bullets.push_back(bullet);
	}

	_timeFiring = 0;
}

bool Laser::HitTarget(Laser_BULLET* pBullet, Vector2D& posTarget)
{
	bool bHitTarget = false;

	Vector2D posBullet(pBullet->x, pBullet->y);

	double distance = posBullet.DistanceTo(posTarget);
	if (distance < 18)
	{
		bHitTarget = true;
	}

	_minBulletDistanceToTarget = std::min(_minBulletDistanceToTarget, distance);

	return bHitTarget;
}

bool Laser::isHitTarget()
{
	bool bIsHitTarget = false;
	std::deque<Laser_BULLET>::iterator it;

	int numBufferHits = 0;
	for (it = _bullets.begin(); it != _bullets.end(); it++)
	{
		Laser_BULLET& pBullet = (Laser_BULLET&)*it;
		if (pBullet.bHitTarget > 0)
		{
			bIsHitTarget = true;
		}
	}

	return bIsHitTarget;
}
bool Laser::wasFired()
{
	if (_timeFiring <= 1)
	{
		return true;
	}
	else return false;
}
bool Laser::isFiring()
{

	if ((_bullets.size() > 0) && ((_timeFiring > 1)))
	{
		return true;
	}
	else return false;
}


void Laser::getTargetsID(std::vector<int>* pTargetIDs)
{
	std::vector<int>::iterator it;
	std::map<int, double> distanceTarget;

	double minDistance = 1000;

	for (it = _targetsIDs.begin(); it != _targetsIDs.end(); it++)
	{
		int id = *it;
		Spacecraft* pAgent = (Spacecraft*)_pPlatform->_pEnv->getAgentByID(id);

		Metis::Vector2D pos = pAgent->getPosition();
		double distToShuttle = pos.magnitude();

		distanceTarget[id] = distToShuttle;


	}

	// 1. Copy the map contents into a vector of pairs
	std::vector<std::pair<int, double>> sortedTargets(distanceTarget.begin(), distanceTarget.end());

	// 2. Sort the vector using std::sort and a lambda function comparing the 'second' (the double)
	std::sort(sortedTargets.begin(), sortedTargets.end(),
		[](const std::pair<int, double>& a, const std::pair<int, double>& b) {
			return a.second < b.second; // Ascending order
		}
	);

	std::vector<std::pair<int, double>>::iterator it2;

	for (it2 = sortedTargets.begin(); it2 != sortedTargets.end(); it2++)
	{
		int id = it2->first;
		pTargetIDs->push_back(id);
	}


}
void Laser::setTargets(std::vector<int>& targets)
{
	_targetsIDs = targets;
}
void Laser::Update(double incTime)
{
	updateTemperature(_icycle);
	_icycle++;

	int numBufferHits = 0;

	// We use a loop with a manually controlled iterator to handle safe erasures
	for (auto it = _bullets.begin(); it != _bullets.end(); )
	{
		Laser_BULLET& pBullet = *it;

		// 1. Update the bullet position and physics
		double dheadingRad = wxDegToRad(pBullet.headingDeg);
		double speedBullet = pBullet._speed;

		double vx = sin(dheadingRad) * incTime * speedBullet;
		double vy = cos(dheadingRad) * incTime * speedBullet;

		pBullet.x += vx;
		pBullet.y += vy;

		pBullet._speed *= 0.99;
		pBullet._distanceTravel -= speedBullet * incTime;

		// 2. Check collisions with the agents
		bool bulletRemoved = false;
		int numAgents = _pPlatform->_pEnv->getNumAgents();

		for (int iAgent = 0; iAgent < numAgents; iAgent++)
		{
			Spacecraft* pTarget = (Spacecraft*)_pPlatform->_pEnv->getAgentByID(iAgent);

			if (pTarget && pTarget->getShipType() != ShipType::Shuttle)
			{
				Vector2D posTarget = pTarget->getPosition();

				if (pBullet.bHitTarget == 0 && HitTarget(&pBullet, posTarget))
				{
					numBufferHits++;
					pTarget->setState(destroyed);
					pBullet.bHitTarget = 3; // Persistence time after the impact
					pBullet._speed = 0;
					break; // Target found! We exit the agents loop for this bullet
				}
			}
		}

		// 3. Manage the impact timer if it already hit before
		if (pBullet.bHitTarget > 0)
		{
			pBullet.bHitTarget--; // Decremented only once per frame
		}

		// 4. Check whether the bullet must be destroyed
		// (if it ran out of distance, out of speed, or if the post-impact timer expired)
		if ((pBullet._distanceTravel <= 0) || (pBullet._speed <= 10.0) || (pBullet.bHitTarget < 0))
		{
			// erase() automatically returns a valid iterator to the next element
			it = _bullets.erase(it);
			bulletRemoved = true;
		}

		// 5. If we did NOT erase the bullet, we advance the iterator normally
		if (!bulletRemoved)
		{
			++it;
		}
	}

	if (!_bullets.empty())
	{
		_timeFiring++;
	}
}
void Laser::drawSpark(wxDC* pDC, Laser_BULLET& bullet)
{

	pDC->SetPen(wxPen(*wxRED, 2));

	int x, y;

	x = bullet.x;
	y = -bullet.y;

	pDC->DrawLine(x - 5, y - 5, x + 5, y + 5);
	pDC->DrawLine(x - 5, y + 5, x + 5, y - 5);

}
void Laser::drawLaserBullet(wxDC* pDC, Laser_BULLET& bullet)
{
	// Save the original brush for safety
	wxPen oldPen = pDC->GetPen();

	// ==========================================
	// LASER CONFIGURATION
	// ==========================================
	// Intense orange. Increased thickness to 3 so the laser looks bolder
	wxPen penLaser(wxColour(50, 255, 50), 3);
	pDC->SetPen(penLaser);

	int x = bullet.x;
	int y = bullet.y;

	// ==========================================
	// LIGHT BEAM TRIGONOMETRY
	// ==========================================
	// 1. Convert projectile degrees to radians
	double headingRad = bullet.headingDeg * M_PI / 180.0;

	// 2. Laser burst length in pixels (you can adjust this number)
	int laserLength = 6;

	// 3. Calculate the laser "tail" coordinate
	int tailX = x - (int)(sin(headingRad) * laserLength);
	int tailY = y - (int)(cos(headingRad) * laserLength);

	/* Tactical note: Depending on how your radar coordinate system works,
	   if you notice the laser draws "forward" instead of leaving a trail behind,
	   simply change the '-' signs to '+' in the tailX and tailY calculations.
	*/

	// 4. Draw the line from the tail to the head of the laser
	pDC->DrawLine(tailX, tailY, x, y);

	if (bullet.bHitTarget > 0)
	{
		drawSpark(pDC, bullet);  // IMPACT EFFECT
	}

	// Restore DC state
	pDC->SetPen(oldPen);
}

void Laser::drawBullet(wxDC* pDC, Laser_BULLET& bullet)
{
	wxAffineMatrix2D matrixBackup, matrix;
	char r, b, g;

	// orange
	r = 255;
	g = 135;
	b = 0;
	wxPen pen(wxColour(r, g, b), 2);
	wxPen oldPen = pDC->GetPen();

	pDC->SetBrush(wxBrush(wxColour(r, g, b))); // Dark gray body


	int x, y;
	double headingDeg;
	headingDeg = bullet.headingDeg;

	x = bullet.x;
	y = bullet.y;

	pDC->SetPen(pen);
	pDC->DrawCircle(x, y, 3);

	if (bullet.bHitTarget > 0)
	{
		drawSpark(pDC, bullet);
	}

	pDC->SetPen(oldPen);
}


//void Laser::drawTurret(wxDC* pDC, const Vector2D& pos, float angleDeg, float scale)
//{
//	// 1. DYNAMIC DIMENSIONS (Scaled the same as the Shuttle)
//	int baseRadius = std::max(2, (int)(8 * scale));
//	int cannonLength = (int)(25 * scale);
//	int cannonThickness = std::max(2, (int)(4 * scale));
//
//	// Save the original matrix state
//	wxAffineMatrix2D matrixBackup = pDC->GetTransformMatrix();
//	wxAffineMatrix2D matrix;
//	//matrix.Translate(pos.x, -pos.y);
//	//matrix.Scale(1.0, -1.0);
//	//pDC->SetTransformMatrix(matrix);
//
//	// 2. TURRET BASE (Elite Imperial Hull)
//	// Dark crimson red fill and bright red border (identical to brushHull and penHull)
//	pDC->SetBrush(wxBrush(wxColour(130, 15, 15)));
//	pDC->SetPen(wxPen(wxColour(255, 50, 50), std::max(1, (int)(2 * scale))));
//	pDC->DrawCircle(pos.x, -pos.y, baseRadius);
//
//	// Central axis (Inspired by the cockpit and the dorsal fin of the ship)
//	// Black center with pure white border to give mechanical contrast
//	pDC->SetBrush(wxBrush(wxColour(0, 0, 0)));
//	pDC->SetPen(wxPen(wxColour(255, 255, 255), std::max(1, (int)(1 * scale))));
//	pDC->DrawCircle(pos.x, -pos.y, std::max(1, baseRadius / 2));
//
//	// 3. TRIGONOMETRIC CALCULATION
//	angleDeg = angleDeg - 90.0;
//	float angleRad = angleDeg * M_PI / 180.0f;
//	float dirX = cos(angleRad);
//	float dirY = sin(angleRad);
//	float endX = pos.x + cannonLength * dirX;
//	float endY = pos.y + cannonLength * dirY;
//
//	// 4. DRAW THE CANNON
//	// Outer cannon casing (Bright red so it stands out against the black)
//	pDC->SetPen(wxPen(wxColour(255, 50, 50), cannonThickness));
//	pDC->DrawLine(pos.x, -pos.y, endX, endY);
//
//	// Inner accent: Laser Energy (Imperial Green, identical to the wings' 'penLaser')
//	// This visually integrates the turret weapon with the ship's weapons
//	pDC->SetPen(wxPen(wxColour(50, 255, 50), std::max(1, cannonThickness / 2)));
//	pDC->DrawLine(pos.x, -pos.y, endX, endY);
//
//	//pDC->SetTransformMatrix(matrixBackup);
//}
void Laser::Draw(wxDC* pDC)
{
	wxSize sizeWindow = pDC->GetSize();
	wxPoint center(sizeWindow.x / 2, sizeWindow.y / 2);

	// Save the original matrix state
	wxAffineMatrix2D matrixBackup = pDC->GetTransformMatrix();
	wxAffineMatrix2D matrix;

	// Center and adapt the Y axis to the mathematical coordinate format
	matrix.Translate(center.x, center.y);
	matrix.Scale(1.0, -1.0);
	pDC->SetTransformMatrix(matrix);

	Vector2D platformPos = _pPlatform->getPosition();

	// SCALE: We force the scale 0.8 so the turret has the same 
	// proportional size as the Shuttle drawn in Spacecraft.
	float scale = 1.0f;



	// --- 2. DRAW THE BULLETS IN THE AIR ---
	// We save the old pen for safety
	wxPen oldPen = pDC->GetPen();

	std::deque<Laser_BULLET>::iterator it;
	for (it = _bullets.begin(); it != _bullets.end(); it++)
	{
		Laser_BULLET& bullet = (Laser_BULLET&)*it;
		drawLaserBullet(pDC, bullet);
	}

	// Restore state
	pDC->SetPen(oldPen);
	pDC->SetTransformMatrix(matrixBackup);

	// --- 1. DRAW THE TURRET AND THE CANNON ---
	//_turretAngleDeg = 90.0;
	drawTurret(pDC, platformPos, _turretAngleDeg, scale);
}

void Laser::drawTurret(wxDC* pDC, const Vector2D& pos, float angleDeg, float scale)
{
	// 1. DYNAMIC DIMENSIONS (Scaled the same as the Shuttle)
	int baseRadius = std::max(2, (int)(8 * scale));
	int cannonLength = (int)(25 * scale);
	int cannonThickness = std::max(2, (int)(4 * scale));

	// 2. TURRET BASE (Elite Imperial Hull)
	// Dark crimson red fill and bright red border (identical to brushHull and penHull)
	pDC->SetBrush(wxBrush(wxColour(130, 15, 15)));
	pDC->SetPen(wxPen(wxColour(255, 50, 50), std::max(1, (int)(2 * scale))));
	pDC->DrawCircle(pos.x, pos.y, baseRadius);

	// Central axis (Inspired by the cockpit and the dorsal fin of the ship)
	// Black center with pure white border to give mechanical contrast
	pDC->SetBrush(wxBrush(wxColour(0, 0, 0)));
	pDC->SetPen(wxPen(wxColour(255, 255, 255), std::max(1, (int)(1 * scale))));
	pDC->DrawCircle(pos.x, pos.y, std::max(1, baseRadius / 2));

	// 3. TRIGONOMETRIC CALCULATION
	angleDeg = angleDeg - 90.0;
	float angleRad = angleDeg * M_PI / 180.0f;
	float dirX = cos(angleRad);
	float dirY = sin(angleRad);
	float endX = pos.x + cannonLength * dirX;
	float endY = pos.y + cannonLength * dirY;

	// 4. DRAW THE CANNON
	// Outer cannon casing (Bright red so it stands out against the black)
	pDC->SetPen(wxPen(wxColour(255, 50, 50), cannonThickness));
	pDC->DrawLine(pos.x, pos.y, endX, endY);

	// Inner accent: Laser Energy (Imperial Green, identical to the wings' 'penLaser')
	// This visually integrates the turret weapon with the ship's weapons
	pDC->SetPen(wxPen(wxColour(50, 255, 50), std::max(1, cannonThickness / 2)));
	pDC->DrawLine(pos.x, pos.y, endX, endY);
}
