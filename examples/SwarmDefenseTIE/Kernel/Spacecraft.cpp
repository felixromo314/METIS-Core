#include "Spacecraft.h"
#include "SpaceEnviroment.h"

void DebugPrint5(const char* format, ...)
{
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	OutputDebugStringA(buffer);
}

double normalizeAngleDeg(double angulo)
{
	double resultado = fmod(angulo, 360.0);
	if (resultado < 0) resultado += 360.0;

	double tmp = angulo / 360;
	if (tmp > 1.0)
	{
		int x = 0;
	}
	return resultado;
}


Spacecraft::Spacecraft(): _laser(this)
{
	// configurar el mundo head

	_pHoloDecoy = NULL;
	_sensor.Set(this);

	clean();
}

Spacecraft::~Spacecraft()
{

}

bool Spacecraft::isCoveredThePosition(Metis::Vector2D &posToCover)
{
	Vector2D mypos = getPosition();
	Vector2D r = posToCover - mypos;
	bool bisCoveredThePosition = false;

	if (r.magnitude() < DISTANCE_MAX_TO_SHUTTLE)
	{
		bisCoveredThePosition = true;
	}

	return bisCoveredThePosition;
}
/*
* 0º in front
* 180 behind
* -90 to the left
* +90 to the right
*/

double Spacecraft::getATAangle(Vector2D &posTarget)
{
	double ATARad;
	double term1, term2;

	Metis::Vector2D posEnemy = posTarget;

	Metis::Vector2D LOS = posEnemy - _position;

	double distance = LOS.magnitude();

	Metis::Vector2D vBlue;
	vBlue = getVelocity();
	vBlue = vBlue.Normalize();
	term1 = vBlue.Dot(LOS);
	term2 = vBlue.magnitude() * distance;

	double cosVal = term1 / term2;
	if (term2 == 0.0)
	{
		ATARad = 0.0; // Or some reasonable default value
	}
	else
	{
		// Clamp to avoid NaN due to rounding
		cosVal = std::min(1.0, std::max(-1.0, cosVal));
		ATARad = acos(cosVal);
	}

	double ATAdeg;
	ATAdeg = ATARad * (180.0 / M_PI);


	return ATAdeg;
}
double Spacecraft::getATAangle(Spacecraft* pAgentTarget)
{
	double ATARad;
	double term1, term2;

	Metis::Vector2D posEnemy = pAgentTarget->getPosition();

	double ATAdeg = getATAangle(posEnemy);
	return ATAdeg;

}

void Spacecraft::setState(int state)
{
	_iState = state;

	if (_iState == attacked)
	{
		int x = 0;
	}

	if ((_iState == destroyedByMissil) || (_iState == destroyed) || ((_iState == collision)))
	{
		_speed = 0;
		_position.x = (-SpaceEnviroment::_maxSIZE_X) + 40.0;  //put on the corner
		_position.y = SpaceEnviroment::_maxSIZE_Y-50.0;
	}
}


void Spacecraft::setHeading(float headingDeg)
{
	_headingDeg = headingDeg;
}
void Spacecraft::setPosition(float x, float y)
{
	_position.x = x;
	_position.y = y;
}

void Spacecraft::setColor(char r, char g, char b)
{
	_r = r;
	_g = g;
	_b = b;
}
Metis::Vector2D Spacecraft::getVelocity()
{
	Metis::Vector2D vVeloc;

	double headingRad = wxDegToRad(_headingDeg);
	vVeloc.x = _speedMaxSpeed * sin(headingRad);
	vVeloc.y = _speedMaxSpeed * cos(headingRad);

	return vVeloc;
}
bool Spacecraft::ThereIsHoloDecoyFlying()
{
	bool bThereIsChaffFlying = false;
	if (_pHoloDecoy != NULL)
	{
		bThereIsChaffFlying = _pHoloDecoy->IsFlying();
	}

	return bThereIsChaffFlying;
}
bool Spacecraft::WasFireHoloDecoy()
{
	return _bTriggerHoloDecoy;
}
void Spacecraft::fireMissile()
{

	if (_pTarget != NULL) // existe un target detectado por el sensor
	{
		if (_pMissile == NULL)
		{
			_pMissile = new Missile(this);
		}
		else
		{
			// there is al ready a missil flying
		}
		
	}
}

bool Spacecraft::wasLaserFired()
{
	bool bwasLaserFired = false;

	if (_Laser_tried_fired >= 1)
	{
		bwasLaserFired = true;
	}

	return bwasLaserFired;
}
bool Spacecraft::thereIsLaserFlying()
{
	bool bthereIsLaserFlying = false;

	bthereIsLaserFlying = _laser.isFiring();
	
	return bthereIsLaserFlying;
}

bool Spacecraft::wasMissilFired()
{
	bool bwasMissilFired=false;
	if (_pMissile != NULL)
	{
		bwasMissilFired = _pMissile->WasFire();
	}
	return bwasMissilFired;
}
void Spacecraft::clean()
{
	if (_pMissile != NULL)
	{
		delete _pMissile;
		_pMissile = NULL;
	}
	if (_pHoloDecoy != NULL)
	{
		delete _pHoloDecoy;
		_pHoloDecoy = NULL;
	}

	_pTargetMissile = NULL;
	_pTarget = NULL;

	_laser.clean();
	_sensor.clean();
}
bool Spacecraft::thereIsMissileFlying()
{
	bool bwasMissilFired = false;
	if (_pMissile != NULL)
	{
		bwasMissilFired = true;
	}
	return bwasMissilFired;
}
bool Spacecraft::updateLaser(double delta_time)
{
	Spacecraft* pShuttle = (Spacecraft*)_pEnv->getAgentFromID(X_WING_ID);

	if (_pTargetMissile != NULL)
	{
		if ( (_pTargetMissile->_iState == Missile::MissilState::Destroyed) || (_pTargetMissile->_iState == Missile::MissilState::NoFuel) ||
			(_pTargetMissile->_iState == Missile::MissilState::HitTarget) )

		{
			_pTargetMissile = NULL;
			_laser.setTarget(NULL);
		}
		
	}
	else
	{
		_laser.setTarget(NULL);
	}

	//Missile *pEnemyMissile = pShuttle->getMissile();
	//if (pEnemyMissile == NULL)
	//{
	//	_laser.setTarget(NULL); // missil has been destroyed so laser has no target
	//}
	//else
	//{
	//	if (_laser.getTarget() != NULL)
	//	{
	//		_laser.setTarget(pEnemyMissile);
	//	}
	//}

	_laser.Update(delta_time);

	return true;
}
bool Spacecraft::updateSensor(Spacecraft* pTarget,Sensor* pSensor)
{
	bool bEnemyMissilDetected = false;
	bool bDetected = false;
	Missile *enemyMissile = pTarget->getMissile();
	if (enemyMissile != NULL)
	{
		//check if the sensor detect the missile
		bEnemyMissilDetected = pSensor->IsIluminate(enemyMissile);
		if (bEnemyMissilDetected)
		{
			if ((enemyMissile->_iState != Missile::MissilState::Destroyed) &&
				(enemyMissile->_iState != Missile::MissilState::HitTarget) &&
				(enemyMissile->_iState != Missile::MissilState::NoFuel))
			{
				_pTargetMissile = enemyMissile;
				bDetected = bEnemyMissilDetected;
			}
			
		}
		else
		{
			_pTargetMissile = NULL;
		}
	}
	else
	{
		_pTargetMissile = NULL;
	}
	if (!bEnemyMissilDetected)
	{


		bDetected = pSensor->IsIluminate(pTarget);
		if (bDetected)
		{
			_pTarget = pTarget;
		}
		else
		{
			_pTarget = NULL;
		}
	}


	return bDetected;
}
int Spacecraft::update(double delta_time)
{
	double vx, vy;

	double headingRad;

	headingRad = wxDegToRad(_headingDeg);
	vx = _speed * sin(headingRad) * delta_time;
	vy = _speed * cos(headingRad) * delta_time;

	double newX, newY;
	newX = _position.x + vx;
	newY = _position.y + vy;

	_position.x = newX;
	_position.y = newY;

	if (_pMissile != NULL)
	{
		_pMissile->Update(delta_time);
		if (_pMissile->_iState == Missile::MissilState::HitTarget)
		{
			this->getTarget()->setState(destroyed);
			if (_pMissile->CanBeDelete())
			{
				delete _pMissile;
				_pMissile = NULL;
			}
		}
		else
		{
			if ( (_pMissile->_iState == Missile::MissilState::Destroyed) || (_pMissile->_iState == Missile::NoFuel))
			{
				if (_pMissile->CanBeDelete())
				{
					delete _pMissile;
					_pMissile = NULL;
				}
			}
		}
	}
	if (_pHoloDecoy != NULL)
	{
		_pHoloDecoy->Update();
		if (_pHoloDecoy->IsExpireLife())
		{
			delete _pHoloDecoy;
			_pHoloDecoy = NULL;
		}
	}


	//update sensor for TIEs
	if ( (this->getID() == 1) || (this->getID() == 2))
	{
		Spacecraft* pXWing = (Spacecraft*)_pEnv->getAgentFromID(3); // get the target
		updateSensor(pXWing, &_sensor);

		updateLaser(delta_time);
	}
	if (this->getID() == 3 ) // update for the X-Wing
	{
		Spacecraft* pShuttle = (Spacecraft*)_pEnv->getAgentFromID(0); // get the target
		updateSensor(pShuttle, &_sensor);
	}

	return 0;
}

bool Spacecraft::isReadyToFire(Spacecraft* pEnemy)
{
	bool bisReadyToFire = false;

	Vector2D enemyPos = pEnemy->getPosition();
	Vector2D rToEnemy = enemyPos - _position;

	double distance = rToEnemy.magnitude();

	if ((distance < 300) && (this->_pTarget != NULL) && (_pMissile == NULL))
	{
		if (this->_pTarget == pEnemy)
		{
			bisReadyToFire = true;
		}
		else
		{
			int x = 0;
		}

	}

	return bisReadyToFire;
}
int Spacecraft::evade()
{
	int bestAction;
	Spacecraft* pTIEfiring = _pEnv->getTIEfiringLaser();

	Vector2D posLaser = pTIEfiring->getPosition();

	// 1. Vector from Laser to Heli
	Vector2D escapeDir;
	escapeDir.x = _position.x - posLaser.x;
	escapeDir.y = _position.y - posLaser.y;
	escapeDir.Normalize();

	// 2. Create a perpendicular vector (the "side")
	Vector2D sideStep;
	sideStep.x = -escapeDir.y;
	sideStep.y = escapeDir.x;

	static double currentTime = 0.0;
	double frequency = 5.0;
	// 3. Oscillate using time (Sine) to create a Zig-Zag
	// Make sure currentTime and frequency are double
	double wave = std::sin(currentTime * frequency);
	currentTime += METIS_INC_TIME;

	Vector2D velocity;
	// 4. Combine: Move away from the Laser + Move side to side
	velocity.x = (escapeDir.x + sideStep.x * wave) * _speed;
	velocity.y = (escapeDir.y + sideStep.y * wave) * _speed;

	// 1. Calculate the desired angle (where we want to go)
	// atan2 returns the angle in radians between -PI and PI
	double targetAngle = std::atan2(velocity.y, velocity.x);

	// 2. Obtener el ángulo actual del heli (supongamos que lo tienes en radianes)
	double currentAngle = wxDegToRad(this->getHeading());

	// 3. Calculate the angle difference
	double angleDiff = targetAngle - currentAngle;

	// Normalize the difference so it is always between -PI and PI
	while (angleDiff > M_PI)  angleDiff -= 2.0 * M_PI;
	while (angleDiff < -M_PI) angleDiff += 2.0 * M_PI;

	// 4. Determine the action
	if (std::abs(angleDiff) > 0.1) // Tolerance threshold
	{
		if (angleDiff > 0)
		{
			bestAction = LEFT;
		}
		else
		{
			bestAction = RIGHT;
		}
	}
	else
	{
		bestAction = AHEAD;
	}

	return bestAction;
}

// use by the x-wing which is used as an attacker to the imperial convoy
int Spacecraft::getActionProcedural(Metis::State& state)
{
	int bestAction=0;
		
	TSPACEENVIROMENT* pEnv = (TSPACEENVIROMENT*)state.getUserState();

	Spacecraft* pShuttle = (Spacecraft*)_pEnv->getAgentFromID(0);

	if ((_iState == destroyed) || ((_iState == destroyedByMissil)))
	{
		bestAction = STOP;
	}
	
	if (_iState == evaded)
	{
		Vector2D shuttlePos = pShuttle->getPosition();
		Vector2D LoS = shuttlePos - _position;
		Vector2D vSpeed;
		vSpeed = this->getSpeed();
		vSpeed = vSpeed.Normalize();

		double angleAlfa = LoS.angleWith(vSpeed); // Angle that the LoS makes with the velocity direction
		double angleAlfaDeg = wxRadToDeg(angleAlfa);
		bestAction = AHEAD;
		if (angleAlfaDeg <= 20)
		{
			bestAction = RIGHT;
		}


	}
	if (_iState == attacked)
	{
		//if state SEARCH
		double ATAangleDeg = getATAangle(pShuttle);

		if (fabs(ATAangleDeg) < INC_ANGLE_HEADING)
		{
			bestAction = AHEAD;

		}
		else
		{
			if (ATAangleDeg > INC_ANGLE_HEADING)
			{
				bestAction = LEFT;
			}
			else
			{
				bestAction = RIGHT;
			}
		}

		if (isReadyToFire(pShuttle))
		{
			if (!pShuttle->ThereIsHoloDecoyFlying()) // If the target has not deployed chaffs
			{
				double randValue = static_cast<double>(rand()) / RAND_MAX;
				if (randValue < 0.3) // 70% of the time, it will decide not to fire in a cycle
				{
					bestAction = FIRE_MISSILE;
				}

			}



		}
	}

	Spacecraft* pTIEfiring = _pEnv->getTIEfiringLaser();
	if (pTIEfiring != NULL)
	{
		Vector2D tiePos = pTIEfiring->getPosition();
		Vector2D r = tiePos - _position;
		if (r.magnitude() < RANGE_SENSOR)
		{
			bestAction = evade();
		}
	}

	return bestAction;
}

/// <summary>
/// Each spacecraf has is own actions so somae of them it will not be valid
/// </summary>
/// <param name="iAction"></param>
/// <returns>return true if the action iAction is valid for the agent</returns>
bool Spacecraft::isValidAction(int iAction)
{
	bool bIsValidAction = true;

	if (this->getID() == 0) // Shuttle
	{
		//shuttle not fire laser
		if (FIRE_LASER == iAction)
		{
			bIsValidAction = false;
		}
	}
	if ( (getID() == 1) || (getID() == 2)) // fighter_TIE_1 or _fighter_TIE_2
	{
		if (LAUNCH_DECOY == iAction) // fighter_TIE not launch HoloDecoy
		{
			bIsValidAction = false;
		}
	}
	return bIsValidAction;

}

// Function to draw a TIE Fighter from above
void Spacecraft::DrawTIEFighter(wxDC* pDC, int cx, int cy, float scale)
{
	// Aseguramos que el grosor no desaparezca al hacer zoom out
	int mainThickness = std::max(1, (int)(2 * scale));
	int panelThickness = std::max(1, (int)(3 * scale));
	int laserThickness = std::max(1, (int)(2 * scale));

	// ==========================================
	// 1. BRUSH CONFIGURATION (Hostile Mode)
	// ==========================================
	// Electric Red border (Enemy threat) and dark maroon fill
	wxPen penHull(wxColour(255, 50, 50), mainThickness);
	wxBrush brushHull(wxColour(60, 0, 0));
	pDC->SetPen(penHull);
	pDC->SetBrush(brushHull);

	// ==========================================
	// 2. CONNECTION PYLONS (The horizontal arms)
	// ==========================================
	// Draw a rectangle that crosses the center
	int pylonWidth = (int)(40 * scale);
	int pylonHeight = (int)(8 * scale);
	pDC->DrawRectangle(cx - pylonWidth / 2,
		cy - pylonHeight / 2,
		pylonWidth,
		pylonHeight);

	// ==========================================
	// 3. SOLAR PANELS (The vertical wings)
	// ==========================================
	// The panels are two long and thin rectangles on the sides
	int panelWidth = (int)(6 * scale);
	int panelHeight = (int)(46 * scale);
	int leftPanelX = cx - (int)(22 * scale) - panelWidth / 2;
	int rightPanelX = cx + (int)(22 * scale) - panelWidth / 2;
	int panelY = cy - panelHeight / 2;

	pDC->SetPen(wxPen(wxColour(255, 50, 50), panelThickness));

	// Left Panel
	pDC->DrawRectangle(leftPanelX, panelY, panelWidth, panelHeight);
	// Right Panel
	pDC->DrawRectangle(rightPanelX, panelY, panelWidth, panelHeight);

	// ==========================================
	// 4. CENTRAL COCKPIT (The "Eye")
	// ==========================================
	pDC->SetPen(penHull);

	// Main fuselage sphere
	int cockpitRadius = std::max(1, (int)(10 * scale));
	pDC->DrawCircle(wxPoint(cx, cy), cockpitRadius);

	// Classic octagonal window (simplified as a black inner circle with a cross)
	pDC->SetBrush(*wxBLACK_BRUSH);
	int windowRadius = std::max(1, (int)(6 * scale));
	pDC->DrawCircle(wxPoint(cx, cy), windowRadius);

	// Cockpit glass details (Thin red lines)
	pDC->SetPen(wxPen(wxColour(255, 50, 50), 1));
	pDC->DrawLine(cx - windowRadius, cy, cx + windowRadius, cy);
	pDC->DrawLine(cx, cy - windowRadius, cx, cy + windowRadius);

	// ==========================================
	// 5. IMPERIAL LASER CANNONS (Green Lines)
	// ==========================================
	// The Empire uses green lasers, so we use pure Neon Green
	wxPen penLaser(wxColour(50, 255, 50), laserThickness);
	pDC->SetPen(penLaser);

	// The TIE cannons are very close together, right under the cockpit, pointing forward
	int cannonSpacing = (int)(4 * scale);
	int cannonStart = cy - (int)(10 * scale);
	int cannonEnd = cy - (int)(22 * scale);

	pDC->DrawLine(cx - cannonSpacing, cannonStart, cx - cannonSpacing, cannonEnd);
	pDC->DrawLine(cx + cannonSpacing, cannonStart, cx + cannonSpacing, cannonEnd);
}


void Spacecraft::DrawXWing(wxDC* pDC, int cx, int cy, float scale)
{
	// Calculate thicknesses dynamically so lines don't disappear if scale is too small
	int mainThickness = std::max(1, (int)(2 * scale));

	// 0. STATE: DESTROYED
	if (getState() == destroyed) 
	{ 
		wxPen pen2;
		pen2.SetColour(wxColour(255, 0, 0));
		pen2.SetWidth(2);

		pDC->SetPen(pen2); // Blue border
		pDC->DrawLine(-10, -10, 10, 10); // Main rotor
		pDC->DrawLine(-10, 10, 10, -10); // Main rotor

		pDC->SetPen(wxPen(wxColour(100, 100, 100), 1)); // Blue border
		pDC->DrawLine(5, -1, 5, -15); // Main rotor
		pDC->DrawLine(-5, -1, -5, -15); // Main rotor

		return;
	}

	int laserThickness = std::max(1, (int)(2 * scale));
	int r2Thickness = std::max(1, (int)(1 * scale));

	
	// 1. BRUSH CONFIGURATION (Intact Ship)
	wxPen penHull(wxColour(0, 255, 255), mainThickness);
	wxBrush brushHull(wxColour(0, 60, 60));
	pDC->SetPen(penHull);
	pDC->SetBrush(brushHull);

	
	// 2. DRAW THE FUSELAGE
	wxPoint fuselage[5];
	fuselage[0] = wxPoint(cx, cy - (int)(30 * scale));
	fuselage[1] = wxPoint(cx + (int)(6 * scale), cy - (int)(10 * scale));
	fuselage[2] = wxPoint(cx + (int)(6 * scale), cy + (int)(20 * scale));
	fuselage[3] = wxPoint(cx - (int)(6 * scale), cy + (int)(20 * scale));
	fuselage[4] = wxPoint(cx - (int)(6 * scale), cy - (int)(10 * scale));
	pDC->DrawPolygon(5, fuselage);

	
	// 3. DRAW THE WINGS
	// Left Wing
	wxPoint leftWing[4];
	leftWing[0] = wxPoint(cx - (int)(6 * scale), cy - (int)(5 * scale));
	leftWing[1] = wxPoint(cx - (int)(35 * scale), cy + (int)(5 * scale));
	leftWing[2] = wxPoint(cx - (int)(35 * scale), cy + (int)(15 * scale));
	leftWing[3] = wxPoint(cx - (int)(6 * scale), cy + (int)(15 * scale));
	pDC->DrawPolygon(4, leftWing);

	// Right Wing
	wxPoint rightWing[4];
	rightWing[0] = wxPoint(cx + (int)(6 * scale), cy - (int)(5 * scale));
	rightWing[1] = wxPoint(cx + (int)(35 * scale), cy + (int)(5 * scale));
	rightWing[2] = wxPoint(cx + (int)(35 * scale), cy + (int)(15 * scale));
	rightWing[3] = wxPoint(cx + (int)(6 * scale), cy + (int)(15 * scale));
	pDC->DrawPolygon(4, rightWing);

	
	// 4. DRAW ENGINES AND ASTROMECH
	pDC->SetBrush(*wxBLACK_BRUSH);
	// Engines
	int motorRadius = std::max(1, (int)(4 * scale));
	pDC->DrawCircle(wxPoint(cx - (int)(12 * scale), cy + (int)(15 * scale)), motorRadius);
	pDC->DrawCircle(wxPoint(cx + (int)(12 * scale), cy + (int)(15 * scale)), motorRadius);

	// Droide R2
	pDC->SetPen(wxPen(wxColour(255, 0, 0), r2Thickness));
	pDC->SetBrush(*wxRED_BRUSH);
	int r2Radius = std::max(1, (int)(2 * scale));
	pDC->DrawCircle(wxPoint(cx, cy - (int)(2 * scale)), r2Radius);

	
	// 5. LASER CANNONS
	wxPen penLaser(wxColour(255, 255, 0), laserThickness);
	pDC->SetPen(penLaser);

	pDC->DrawLine(cx - (int)(35 * scale), cy + (int)(5 * scale),
		cx - (int)(35 * scale), cy - (int)(15 * scale));

	pDC->DrawLine(cx + (int)(35 * scale), cy + (int)(5 * scale),
		cx + (int)(35 * scale), cy - (int)(15 * scale));
}

// Draw a Lambda Shuttle from above
void Spacecraft::DrawLambdaShuttle(wxDC* pDC, int cx, int cy, float scale)
{
	// Dynamic thickness calculations
	int mainThickness = std::max(1, (int)(2 * scale));
	int finThickness = std::max(1, (int)(4 * scale));
	int laserThickness = std::max(1, (int)(2 * scale));

	// 1. BRUSH CONFIGURATION (Imperial Command Mode)
	// Bright White/Grey border and gunmetal grey fill
	wxPen penHull(wxColour(220, 220, 220), mainThickness);
	wxBrush brushHull(wxColour(60, 60, 60));
	pDC->SetPen(penHull);
	pDC->SetBrush(brushHull);

	
	// 2. SIDE WINGS IN FLIGHT MODE (Deployed)
	// The Lambda wings from above are large swept-back triangles
	// Left Wing
	wxPoint leftWing[3];
	leftWing[0] = wxPoint(cx - (int)(10 * scale), cy - (int)(5 * scale));  // Raíz frontal
	leftWing[1] = wxPoint(cx - (int)(45 * scale), cy + (int)(15 * scale)); // Punta del ala
	leftWing[2] = wxPoint(cx - (int)(15 * scale), cy + (int)(20 * scale)); // Raíz trasera
	pDC->DrawPolygon(3, leftWing);

	// Right Wing
	wxPoint rightWing[3];
	rightWing[0] = wxPoint(cx + (int)(10 * scale), cy - (int)(5 * scale));
	rightWing[1] = wxPoint(cx + (int)(45 * scale), cy + (int)(15 * scale));
	rightWing[2] = wxPoint(cx + (int)(15 * scale), cy + (int)(20 * scale));
	pDC->DrawPolygon(3, rightWing);

	
	// 3. MAIN FUSELAGE (Nose and body)
	// Elongated polygon: the cockpit is very narrow in front and widens in the back
	wxPoint fuselage[4];
	fuselage[0] = wxPoint(cx - (int)(4 * scale), cy - (int)(35 * scale)); // Morro izquierdo
	fuselage[1] = wxPoint(cx + (int)(4 * scale), cy - (int)(35 * scale)); // Morro derecho
	fuselage[2] = wxPoint(cx + (int)(15 * scale), cy + (int)(20 * scale)); // Base derecha
	fuselage[3] = wxPoint(cx - (int)(15 * scale), cy + (int)(20 * scale)); // Base izquierda
	pDC->DrawPolygon(4, fuselage);

	// Front glass cockpit detail (Black transverse line)
	pDC->SetPen(wxPen(wxColour(0, 0, 0), mainThickness));
	pDC->DrawLine(cx - (int)(5 * scale), cy - (int)(25 * scale),
		cx + (int)(5 * scale), cy - (int)(25 * scale));

	
	// 4. CENTRAL DORSAL FIN (Top-down view)
	// Viewed from above, the huge upper fin is just a thick line in the center
	wxPen penFin(wxColour(255, 255, 255), finThickness); // Blanco puro para destacar
	pDC->SetPen(penFin);
	pDC->DrawLine(cx, cy - (int)(15 * scale), cx, cy + (int)(15 * scale));

	
	// 5. REAR ION ENGINES (Blue Glow)
	// The Lambda has three blue engines in the back
	wxPen penEngine(wxColour(50, 150, 255), mainThickness + 1); // Azul propulsión
	pDC->SetPen(penEngine);
	pDC->DrawLine(cx - (int)(10 * scale), cy + (int)(20 * scale),
		cx + (int)(10 * scale), cy + (int)(20 * scale));

	
	// 6. LASER CANNONS (Imperial Green)
	// It has dual cannons at the wing joints
	wxPen penLaser(wxColour(50, 255, 50), laserThickness);
	pDC->SetPen(penLaser);

	int cannonStart = cy - (int)(5 * scale);
	int cannonEnd = cy - (int)(20 * scale);
	int cannonOffset = (int)(12 * scale); // Justo donde el ala se une al fuselaje

	// Left Lasers
	pDC->DrawLine(cx - cannonOffset, cannonStart, cx - cannonOffset, cannonEnd);
	// Right Lasers
	pDC->DrawLine(cx + cannonOffset, cannonStart, cx + cannonOffset, cannonEnd);
}
void Spacecraft::drawHoloDecoy(wxDC* pDC)
{
	if (_pHoloDecoy == NULL)
	{
		return;
	}

	wxPen pen(wxColour(0, 255, 0), 2);
	pDC->SetPen(pen);


	Vector2D positionDecoy = _pHoloDecoy->GetPosition();

	wxAffineMatrix2D matrixBackup, matrix;
	matrix = pDC->GetTransformMatrix();
	matrixBackup = matrix;

	matrix.Translate(positionDecoy.x, -positionDecoy.y);
	pDC->SetTransformMatrix(matrix);
		
	pDC->DrawCircle(-3, -3, 2);
	pDC->DrawCircle(-3, +3, 2);
	pDC->DrawCircle(+3, -3, 2);
	pDC->DrawCircle(+3, +3, 2);
	pDC->DrawCircle(+3, +3, 2);


	pDC->SetTransformMatrix(matrixBackup);
}


void Spacecraft::Draw(wxDC* pDC)
{
	// 1. Prepare colors and matrix backup
	wxPen pen(wxColour(_r, _g, _b));
	wxPen oldPen = pDC->GetPen();
	pDC->SetPen(pen);

	wxAffineMatrix2D matrix = pDC->GetTransformMatrix();
	wxAffineMatrix2D matrixBackup = matrix;

	// 2. Apply transformation (Position and Rotation)
	matrix.Translate(_position.x, -_position.y);
	pDC->SetTransformMatrix(matrix);
	
	if (this->getID() != 0) // If it is not the Shuttle, draw the escort point
	{
		//draw the optimal point of formation
		wxPen penScort(wxColour(_r, _g, _b), 3);

		pDC->SetPen(penScort);
		pDC->DrawCircle(_idealScortPos.x, -_idealScortPos.y, 2.0);

	}

	pDC->SetTransformMatrix(matrixBackup);

	double dHeadingDeg = getHeading();

	matrix.Rotate(wxDegToRad(dHeadingDeg));
	pDC->SetTransformMatrix(matrix);


	//-----  draw the spacecraf depend of their role (ID)
	float scale = 0.42;
	if (this->getID() == 0)
	{
		DrawLambdaShuttle(pDC, 0.0, 0.0, scale);
	}
	else
	{
		if (this->getID() == 3)
		{
			DrawXWing(pDC,0.0,0.0,0.5);
		}
		else
		{
			DrawTIEFighter(pDC, 0.0, 0.0, scale);
		}
		
	}
	
	if (this->getID() == SHUTTLE_ID) // draw decoy range
	{
		// draw the sensor
		wxPen penScort(wxColour(_r, _g, _b), 1);
		pDC->SetPen(penScort);

		wxBrush oldBrush = pDC->GetBrush();
		// Use the brush to paint a rectangle
		pDC->SetBrush(*wxTRANSPARENT_BRUSH);

		pDC->DrawCircle(wxPoint(0, 0), RANGE_HOLO_DECOY);

		pDC->SetBrush(oldBrush);
	}

	if (this->getID() != 0) // only X-Wing has sensors
	{
		// draw the sensor
		wxPen penScort(wxColour(_r, _g, _b), 1);
		pDC->SetPen(penScort);

		wxBrush oldBrush = pDC->GetBrush();
		// Use the brush to paint a rectangle
		pDC->SetBrush(*wxTRANSPARENT_BRUSH);

		pDC->DrawCircle(wxPoint(0, 0), RANGE_SENSOR);

		pDC->SetBrush(oldBrush);
	}

	
	// Restore matrix
	pDC->SetTransformMatrix(matrixBackup);
	pDC->SetPen(oldPen);

	// draw the missile if exits
	if (_pMissile != NULL)
	{
		_pMissile->Draw(pDC, _r, _g, _b);
	}
	
	_laser.Draw(pDC);
	
	drawHoloDecoy(pDC);
}

void Spacecraft::TurnLeft()
{
	_speed = _speedMaxSpeed;
	_headingDeg -= INC_ANGLE_HEADING;

	_headingDeg = normalizeAngleDeg(_headingDeg);
}
void Spacecraft::GoHead()
{
	_speed = _speedMaxSpeed;
}
void Spacecraft::TurnRight()
{
	_speed = _speedMaxSpeed;
	_headingDeg += INC_ANGLE_HEADING;

	_headingDeg = normalizeAngleDeg(_headingDeg);
}
void Spacecraft::Stop()
{
	_speed = 0.0;
}
void Spacecraft::LaunchHoloDecoy()
{
	_bTriggerHoloDecoy = true;
	if (_pHoloDecoy == NULL)
	{
		Vector2D direction;
		direction = this->getVelocity();
		direction = direction.Normalize();

		Vector2D rotateDir = direction.rotate(wxDegToRad(90.0));

		double xx = _position.x + rotateDir.x * 10;
		double yy = _position.y + rotateDir.y * 10;

		_pHoloDecoy = new HoloDecoy(xx, yy);
	}
}
void Spacecraft::endStep()
{
	_bTriggerHoloDecoy = false;
	_Laser_tried_fired = 0;
}

void Spacecraft::FireLaser()
{
	_Laser_tried_fired++;
	if (_pTargetMissile != NULL)
	{
		_laser.Fire(_pTargetMissile);
	}
	else
	{
		Spacecraft* pTargetXWing = _sensor.getTarget();
		if (pTargetXWing)
		{
			if (_sensor.IsIluminate(pTargetXWing))
				_laser.Fire(pTargetXWing);
			else
			{
				// do not fire if there is no target
			}
		}
	}
}

bool Spacecraft::isOnSensorRange(Metis::Vector2D& position)
{
	Metis::Vector2D r;
	bool bisOnSensorRange = false;

	r = position - _position;

	double distance = r.magnitude();
	if (distance < RANGE_SENSOR)
	{
		bisOnSensorRange = true;
	}
	return bisOnSensorRange;
}