#include "Laser.h"
#include "Spacecraft.h"
#include "Missile.h"

using namespace Metis;

Laser::Laser(Spacecraft* pPlatform)
{
	_pPlatform = pPlatform;
		
	_icycle = 0;


	_pTarget = NULL;
	_pTargetMissile = NULL;

	_maxTemperature = 100.0;
	_heatPerShot = 15.0;         // Each shot adds 10
	_coolingRate = 2.5;          // Per second
	_coolingDown = false;
	_active = true;

	_icycle = 0;
	_lastUpdateTime=0;

	_timeFiring = 1000;


	
}
Laser::~Laser()
{

}
double Laser::calculateInterceptionTime(Missile& targetMissile, double bulletSpeed)
{
	Metis::Vector2D targetPos = targetMissile.GetPosition();
	Metis::Vector2D LaserPos = _pPlatform->getPosition(); // Asumiendo que tienes la pos del Laser

	// relative distance vector for better learning
	Metis::Vector2D D = { targetPos.x - LaserPos.x, targetPos.y - LaserPos.y };

	// speed vector of the missile
	Metis::Vector2D dir;
	targetMissile.GetDirection(&dir);
	double speedM = targetMissile.GetSpeed();
	Metis::Vector2D Vm;
	Vm.x = dir.x * speedM;
	Vm.y = dir.y * speedM;
		 

	// Coefficients of the quadratic equation: at^2 + bt + c = 0
	double a = (Vm.x * Vm.x + Vm.y * Vm.y) - (bulletSpeed * bulletSpeed);
	double b = 2.0 * (Vm.x * D.x + Vm.y * D.y);
	double c = (D.x * D.x + D.y * D.y);

	//  Calculate the discriminant (b^2 - 4ac)
	double discriminant = b * b - 4.0 * a * c;

	if (discriminant < 0) return -1.0; //  The missile is unreachable

	// Use the quadratic formula to find t
	// Use the negative sign in the numerator to get the shortest time
	double t1 = (-b + sqrt(discriminant)) / (2.0 * a);
	double t2 = (-b - sqrt(discriminant)) / (2.0 * a);

	// Return the smallest positive time
	if (t1 > 0 && t2 > 0) return fmin(t1, t2);
	if (t1 > 0) return t1;
	if (t2 > 0) return t2;

	return -1.0;
}

double Laser::calculateInterceptionTime(Spacecraft& target, double bulletSpeed)
{
	Vector2D targetPos = target.getPosition();
	Vector2D LaserPos = this->_pPlatform->getPosition(); // Asumiendo que tienes la pos del Laser

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

	if (discriminant < 0) return -1.0; // El misil es inalcanzable

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


Metis::Vector2D Laser::predictPositionTarget(Missile& targetMissile, double timeInAdvance)
{

	timeInAdvance = calculateInterceptionTime(targetMissile, BULLET_SPEED);
	

	Metis::Vector2D targetPos = targetMissile.GetPosition();

	Metis::Vector2D direction;
	targetMissile.GetDirection(&direction);
	double speed = targetMissile.GetSpeed();

	Vector2D predictedPos;
	double x, y;

	x = timeInAdvance * speed * direction.x;
	y = timeInAdvance * speed * direction.y;

	predictedPos.x = targetPos.x + x;
	predictedPos.y = targetPos.y + y;

	return predictedPos;
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
void Laser::setTarget(Missile* pTargetMissile)
{
	_pTargetMissile = pTargetMissile;
}
void Laser::Fire(Missile* pTargetMissile)
{
	if (_bullets.size() >= MAX_BULLETS)
	{
		return;
	}

	//-------------------- temperatura
	if (!_active)
	{
		return;
	}
	checkTemperature();

	//------------------------
	_pTargetMissile = pTargetMissile;
		
	double timeInAdvanceRnd = getRnd(0.0, 3.0);
	Vector2D targetPos = predictPositionTarget(*pTargetMissile, timeInAdvanceRnd);

	Vector2D positionPlatform = _pPlatform->getPosition();
	// Calculate the direction vector to the target
	float dx;
	float dy;

	dx = targetPos.x - positionPlatform.x;
	dy = targetPos.y - positionPlatform.y;

	float desiredDirection = std::atan2(dy, dx); // Angle in radians
	desiredDirection = (M_PI / 2.0) - desiredDirection;
	Vector2D dirBurts(dx, dy);
	double separacionPiexeles = 9.0; // Espacio entre balas de la ráfaga
	// Fire a burst
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		Laser_BULLET bullet;

		Vector2D position(positionPlatform.x, positionPlatform.y);
		Vector2D vNormalize = dirBurts.Normalize();

		double xx = vNormalize.x * i * separacionPiexeles;
		double yy = vNormalize.y * i * separacionPiexeles;

		bullet.x = position.x + xx;
		bullet.y = position.y + yy;
		bullet.headingDeg = wxRadToDeg(desiredDirection);


		bullet._speed = BULLET_SPEED;
		bullet._distanceTravel = BULLET_MAX_DISTANCE_TRAVEL;
		bullet.bHitTarget = 0;

		_bullets.push_back(bullet);
	}

	_timeFiring = 0;
}

void Laser::clean()
{
	_bullets.clear();
		
	_icycle = 0;


	_pTarget = NULL;
	_pTargetMissile = NULL;

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
	if(_pTarget != NULL)
	{
		int x = 0;
	}
	bool bisReadyToFire = true;
	if ( (_pTarget != NULL) && (_bullets.size() >= MAX_BULLETS))
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
void Laser::Fire(Spacecraft*pTarget)
{

	
	if (_bullets.size() >= MAX_BULLETS)
	{
		return;
	}

	//-------------------- temperature
	if (!_active)
	{
		return;
	}
	checkTemperature();

	Vector2D targetPos;
	if (pTarget != NULL)
	{
		_pTarget = pTarget;
		double timeInAdvanceRnd = getRnd(0.0, 2.0);
		targetPos = predictPositionTarget(*pTarget, timeInAdvanceRnd);
	}
	else
	{
		targetPos.x = 10000000;
		targetPos.y = 10000000;
	}

	Vector2D positionPlatform = _pPlatform->getPosition();
	// Calculate the direction vector to the target
	float dx;
	float dy;
		

	dx = targetPos.x - positionPlatform.x;
	dy = targetPos.y - positionPlatform.y;

	float desiredDirection = std::atan2(dy, dx); // Angle in radians
	desiredDirection = (M_PI / 2.0) - desiredDirection;
	float directionDeg = wxRadToDeg(desiredDirection);
	Vector2D dirBurts(dx, dy);
	double separacionPiexeles = 9.0; // Espacio entre balas de la ráfaga
	// Fire a burst
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		Laser_BULLET bullet;

		Vector2D position(positionPlatform.x, positionPlatform.y);
		Vector2D vNormalize  = dirBurts.Normalize();


		double xx = vNormalize.x * i* separacionPiexeles;
		double yy = vNormalize.y * i* separacionPiexeles;

		bullet.x = position.x + xx;
		bullet.y = position.y + yy;
		bullet.headingDeg = wxRadToDeg(desiredDirection);
		

		bullet._speed = BULLET_SPEED;
		bullet._distanceTravel = BULLET_MAX_DISTANCE_TRAVEL;
		bullet.bHitTarget = 0;

		_bullets.push_back(bullet);
	}

	_timeFiring = 0;
}

bool Laser::HitTarget(Laser_BULLET *pBullet, Vector2D& posTarget)
{
	bool bHitTarget = false;

	Vector2D posBullet(pBullet->x, pBullet->y);

	double distance = posBullet.DistanceTo(posTarget);
	if (distance < 5)
	{
		bHitTarget = true;
	}
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

	if ( (_bullets.size() > 0) && ((_timeFiring > 1)))
	{
		return true;
	}
	else return false;
}


void Laser::Update(double incTime)
{
	std::deque<Laser_BULLET>::iterator it;

	updateTemperature(_icycle);

	_icycle++;
	
	int numBufferHits = 0;
	for (it = _bullets.begin(); it != _bullets.end(); it++)
	{
		Laser_BULLET& pBullet = (Laser_BULLET &)*it;

		double dheadingRad = wxDegToRad(pBullet.headingDeg);
		double speedBullet = pBullet._speed;
		
					
		// Update the missile's position based on the new direction
		double vx = sin(dheadingRad)* incTime * speedBullet;
		double vy = cos(dheadingRad)* incTime * speedBullet;

		pBullet.x = pBullet.x + vx;
		pBullet.y = pBullet.y + vy;

		pBullet._speed *= 0.99;
		pBullet._distanceTravel = pBullet._distanceTravel - speedBullet*incTime;

		Vector2D posTarget;
		if (_pTargetMissile != NULL)
		{
			posTarget = _pTargetMissile->GetPosition();
		}
		else
		{
			if (_pTarget != NULL)
			{
				posTarget = _pTarget->getPosition();
			}
		}
		
		
		if ((pBullet.bHitTarget == 0) && HitTarget(&pBullet, posTarget))
		{
			numBufferHits++;
			if (_pTargetMissile != NULL)
			{
				_pTargetMissile->SetState(Missile::MissilState::Destroyed);
				_pTargetMissile = NULL;
			}
			else
			{
				if (_pTarget != NULL)
				{
					//_pTarget->AddDamage(25);  // in case you want to make a little damage
					_pTarget->setState(destroyed);
				}
			}
				
			
			pBullet.bHitTarget=3;
			pBullet._speed = 0;
		}
		else
		{
			if (pBullet.bHitTarget > 0)
			{
				pBullet.bHitTarget--;
				if (pBullet.bHitTarget <= 0)
				{
					it = _bullets.erase(it);
					if (it == _bullets.end())
					{
						break;
					}
				}
			}
			

		}
		if ( (pBullet._distanceTravel <= 0) || (pBullet._speed <= 10.0))
		{
			it = _bullets.erase(it);
			if (it == _bullets.end())
			{
				break;
			}
		}
	}
	if (_bullets.size() > 0)
		_timeFiring++;
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
	wxPen pen(wxColour(r, g, b),2);
	wxPen oldPen = pDC->GetPen();

	pDC->SetBrush(wxBrush(wxColour(r, g, b))); // Cuerpo gris oscuro
		

	int x, y;
	double headingDeg;
	headingDeg = bullet.headingDeg;

	x = bullet.x;
	y = bullet.y;
		
	pDC->SetPen(pen);
	pDC->DrawCircle(x, y, 3);

	if (bullet.bHitTarget > 0)
	{
		drawSpark(pDC,bullet);
	}
		
	pDC->SetPen(oldPen);
}
void Laser::Draw(wxDC* pDC)
{
	wxSize sizeWindow = pDC->GetSize();
	wxPoint center;
	center.x = sizeWindow.x / 2;
	center.y = sizeWindow.y / 2;
	std::deque<Laser_BULLET>::iterator it;

	wxAffineMatrix2D matrixBackup, matrix;

	matrixBackup = pDC->GetTransformMatrix();
	//matrixBackup = matrix;

	Vector2D platformPos =  _pPlatform->getPosition();
	matrix.Translate(center.x, center.y);
	matrix.Scale(1.0, -1.0);
	pDC->SetTransformMatrix(matrix);
		
	char r, g, b;
	r = 255;
	g = 255;
	b = 255;
	wxPen pen(wxColour(r, g, b), 2);
	wxPen oldPen = pDC->GetPen();

	int numBufferHits = 0;
	for (it = _bullets.begin(); it != _bullets.end(); it++)
	{
		Laser_BULLET& bullet = (Laser_BULLET&)*it;

		drawLaserBullet(pDC, bullet);
	}

	pDC->SetTransformMatrix(matrixBackup);
}
