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

Spacecraft::Spacecraft(Spacecraft& other) :_laser(other._laser)
{
	_speedMaxSpeed = other._speedMaxSpeed;

	// equipment onboard
	_Laser_tried_fired = other._Laser_tried_fired;
	_sensor = other._sensor;

	_headingDeg = other._headingDeg;
	_speed = other._speed;
	_position = other._position;
	_scortPos = other._scortPos;
	_typeShip = other._typeShip;


	_numTargets = other._numTargets;
	_targetsIDs = other._targetsIDs;

	_dronWingMan = other._dronWingMan;

	_iState = other._iState; //state used in the rol of X-Wing in the logical procedural

	this->setID(other.getID());
}

bool Spacecraft::operator!=(const Spacecraft& other)
{
	bool bNotEqual = false;


	if (_headingDeg != other._headingDeg) bNotEqual = true;
	if (_position != other._position) bNotEqual = true;
	if (_speed != other._speed) bNotEqual = true;
	if (_typeShip != other._typeShip) bNotEqual = true;
	if (_numTargets != other._numTargets) bNotEqual = true;
	if (_targetsIDs != other._targetsIDs) bNotEqual = true;
	if (_iState != other._iState) bNotEqual = true;
	if (_speedMaxSpeed != other._speedMaxSpeed) bNotEqual = true;


	if (_laser != other._laser)
	{
		bNotEqual = true;
	}
	if (_sensor != other._sensor)
	{
		bNotEqual = true;
	}

	return bNotEqual;
}
Spacecraft& Spacecraft::operator=(Spacecraft& other)
{
	if (this != &other)
	{
		_speedMaxSpeed = other._speedMaxSpeed;

		// equipment onboard
		_Laser_tried_fired = other._Laser_tried_fired;
		_sensor = other._sensor;
		_sensor.Set(this);
		_laser = other._laser;

		_headingDeg = other._headingDeg;
		_speed = other._speed;
		_position = other._position;
		_scortPos = other._scortPos;
		_typeShip = other._typeShip;


		_numTargets = other._numTargets;
		_targetsIDs = other._targetsIDs;

		_dronWingMan = other._dronWingMan;

		_iState = other._iState; //state used in the rol of X-Wing in the logical procedural

		this->setID(other.getID());

	}
	return *this;
}


Spacecraft::Spacecraft() : _laser(this)
{

	_sensor.Set(this);
	_numTargets = 0;
	_iState = searched;

	clean();
}

Spacecraft::~Spacecraft()
{

}

bool Spacecraft::isCoveredThePosition(Metis::Vector2D& posToCover)
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

double Spacecraft::getATAangle(Vector2D& posTarget)
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

	if (_iState == collision)
	{
		int x = 0;
	}

	if ((_iState == destroyedByMissil) || (_iState == destroyed) || ((_iState == collision)))
	{
		_speed = 0;
	}
}


std::vector<int> Spacecraft::getDronesDetected()
{
	std::vector<int> targetIDs;

	_laser.getTargetsID(&targetIDs);
	return targetIDs;
}

float Spacecraft::getTurrentHeading()
{
	float turrentHeading = 0.0;
	if (_typeShip == ShipType::Shuttle)
	{
		turrentHeading = _laser.getTurretHeading();
	}

	return turrentHeading;
}
float Spacecraft::getLaserTemperature()
{
	float temperature = 0.0;
	if (_typeShip == ShipType::Shuttle)
	{
		temperature = _laser.getTemperature();
	}

	return temperature;
}
int Spacecraft::getNumberDroneDetected()
{
	int droneDetected = 0;
	if (_typeShip == ShipType::Shuttle)
	{
		droneDetected = _numTargets;
	}

	return droneDetected;
}
void Spacecraft::setHeading(float headingDeg)
{
	_headingDeg = headingDeg;
}
void Spacecraft::setTurretHeading(float headingDeg)
{
	_laser.setTurretHeading(headingDeg);
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
	vVeloc.x = _speed * sin(headingRad);
	vVeloc.y = _speed * cos(headingRad);

	return vVeloc;
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

void Spacecraft::clean()
{

	_iState = searched;

	_laser.clean();
	_sensor.clean();
}

bool Spacecraft::updateLaser(double delta_time)
{

	_laser.Update(delta_time);

	return true;
}
bool Spacecraft::updateSensor(Sensor* pSensor)
{
	_numTargets = 0;
	bool bEnemyMissilDetected = false;
	bool bDetected = false;
	std::vector<int> targetsIDs;

	int numAgents = _pEnv->getNumAgents();


	for (int i = 0; i < numAgents; i++)
	{
		Spacecraft* pAgent = (Spacecraft*)_pEnv->getAgentByID(i);
		if (pAgent->getID() != this->getID())
		{
			if ((pAgent->getState() != destroyed) && (pAgent->getState() != collision))
			{
				bDetected = pSensor->IsIluminate(pAgent);
				if (bDetected)
				{
					int idTarget = pAgent->getID();

					targetsIDs.push_back(idTarget);
					_numTargets++;

					bool existe = std::find(_targetsIDs.begin(), _targetsIDs.end(), idTarget) != _targetsIDs.end();
					if (!existe)
					{
						_targetsIDs.push_back(pAgent->getID());
					}
					else
					{
						int x = 0;
					}

				}
			}
			else
			{
				int x = 0;
			}
		}
	}

	_laser.setTargets(targetsIDs);

	return bDetected;
}
int Spacecraft::update(double delta_time)
{
	double vx, vy;

	if (this->getState() != searched)
	{
		return 0;
	}

	double headingRad;

	headingRad = wxDegToRad(_headingDeg);
	vx = _speed * sin(headingRad) * delta_time;
	vy = _speed * cos(headingRad) * delta_time;

	double newX, newY;
	newX = _position.x + vx;
	newY = _position.y + vy;

	_position.x = newX;
	_position.y = newY;


	if (_typeShip == ShipType::Shuttle)
	{
		//update laser turret movement
		updateSensor(&_sensor);
		updateLaser(delta_time);
	}

	return 0;
}

bool Spacecraft::isReadyToFire(Spacecraft* pEnemy)
{
	bool bisReadyToFire = false;

	Vector2D enemyPos = pEnemy->getPosition();
	Vector2D rToEnemy = enemyPos - _position;

	double distance = rToEnemy.magnitude();

	if ((distance < 300) && (_numTargets > 0))
	{
		bisReadyToFire = true;
	}

	return bisReadyToFire;
}
// use by the x-wing which is used as an attacker to the imperial convoy
int Spacecraft::getActionProcedural(Metis::State& state)
{
	int bestAction = 0;

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

	return bIsValidAction;

}

void Spacecraft::DrawXWingDrone(wxDC* pDC, int cx, int cy, float scale)
{
	int mainThickness = std::max(1, (int)(2 * scale));

	// 0. STATE: DESTROYED
	if (getState() == destroyed)
	{
		wxPen pen2(wxColour(255, 50, 50), 2);
		pDC->SetPen(pen2);
		pDC->DrawLine(cx - (int)(10 * scale), cy - (int)(10 * scale),
			cx + (int)(10 * scale), cy + (int)(10 * scale));
		pDC->DrawLine(cx - (int)(10 * scale), cy + (int)(10 * scale),
			cx + (int)(10 * scale), cy - (int)(10 * scale));

		pDC->SetPen(wxPen(wxColour(100, 100, 100), 1));
		pDC->DrawLine(cx + (int)(5 * scale), cy - (int)(1 * scale),
			cx + (int)(5 * scale), cy - (int)(15 * scale));
		pDC->DrawLine(cx - (int)(5 * scale), cy - (int)(1 * scale),
			cx - (int)(5 * scale), cy - (int)(15 * scale));
		return;
	}

	// 1. BRUSH CONFIGURATION (Blue Squadron Palette)
	// Dark navy blue border for contrast and tactical white/gray fill
	wxPen penHull(wxColour(20, 60, 140), mainThickness);
	wxBrush brushHull(wxColour(220, 220, 225));
	pDC->SetPen(penHull);
	pDC->SetBrush(brushHull);

	// 2. FUSELAGE (Shorter and flatter, without pilot cockpit)
	wxPoint fuselage[5];
	fuselage[0] = wxPoint(cx, cy - (int)(18 * scale));
	fuselage[1] = wxPoint(cx + (int)(8 * scale), cy - (int)(5 * scale));
	fuselage[2] = wxPoint(cx + (int)(8 * scale), cy + (int)(18 * scale));
	fuselage[3] = wxPoint(cx - (int)(8 * scale), cy + (int)(18 * scale));
	fuselage[4] = wxPoint(cx - (int)(8 * scale), cy - (int)(5 * scale));
	pDC->DrawPolygon(5, fuselage);

	// 3. DRONE TELEMETRY ANTENNAS
	pDC->SetPen(wxPen(wxColour(100, 100, 100), std::max(1, (int)(1 * scale))));
	pDC->DrawLine(cx - (int)(4 * scale), cy - (int)(18 * scale),
		cx - (int)(6 * scale), cy - (int)(26 * scale));
	pDC->DrawLine(cx + (int)(4 * scale), cy - (int)(18 * scale),
		cx + (int)(6 * scale), cy - (int)(26 * scale));

	// 4. X-WING WINGS
	pDC->SetPen(penHull);

	wxPoint leftWing[4];
	leftWing[0] = wxPoint(cx - (int)(8 * scale), cy - (int)(2 * scale));
	leftWing[1] = wxPoint(cx - (int)(32 * scale), cy + (int)(5 * scale));
	leftWing[2] = wxPoint(cx - (int)(32 * scale), cy + (int)(15 * scale));
	leftWing[3] = wxPoint(cx - (int)(8 * scale), cy + (int)(15 * scale));
	pDC->DrawPolygon(4, leftWing);

	wxPoint rightWing[4];
	rightWing[0] = wxPoint(cx + (int)(8 * scale), cy - (int)(2 * scale));
	rightWing[1] = wxPoint(cx + (int)(32 * scale), cy + (int)(5 * scale));
	rightWing[2] = wxPoint(cx + (int)(32 * scale), cy + (int)(15 * scale));
	rightWing[3] = wxPoint(cx + (int)(8 * scale), cy + (int)(15 * scale));
	pDC->DrawPolygon(4, rightWing);

	// 5. RED SQUADRON MARKINGS
	wxPen blueStripePen(wxColour(30, 120, 220), std::max(2, (int)(3 * scale)));
	pDC->SetPen(blueStripePen);
	pDC->DrawLine(cx - (int)(15 * scale), cy + (int)(10 * scale),
		cx - (int)(25 * scale), cy + (int)(10 * scale));
	pDC->DrawLine(cx + (int)(15 * scale), cy + (int)(10 * scale),
		cx + (int)(25 * scale), cy + (int)(10 * scale));

	// 6. ENGINES
	pDC->SetPen(wxPen(wxColour(50, 50, 50), 1));
	pDC->SetBrush(wxBrush(wxColour(80, 80, 80)));
	int motorRadius = std::max(1, (int)(3 * scale));
	pDC->DrawCircle(wxPoint(cx - (int)(12 * scale), cy + (int)(15 * scale)), motorRadius);
	pDC->DrawCircle(wxPoint(cx + (int)(12 * scale), cy + (int)(15 * scale)), motorRadius);

	// 7. DRONE OPTICAL VISOR
	pDC->SetPen(wxPen(wxColour(20, 20, 20), 1));
	pDC->SetBrush(*wxBLACK_BRUSH);
	pDC->DrawRectangle(cx - (int)(5 * scale), cy - (int)(8 * scale),
		(int)(10 * scale), (int)(6 * scale));

	pDC->SetPen(wxPen(wxColour(255, 50, 50), 1));
	pDC->SetBrush(wxBrush(wxColour(255, 50, 50)));
	int sensorRadius = std::max(1, (int)(1.5 * scale));
	pDC->DrawCircle(wxPoint(cx, cy - (int)(5 * scale)), sensorRadius);

	// 8. EXPLOSIVE PAYLOAD (Replaces the cannons)
	// Warheads attached to the wing tips (Warning Orange/Yellow)
	wxPen warheadPen(wxColour(255, 140, 0), std::max(1, (int)(2 * scale)));
	wxBrush warheadBrush(wxColour(255, 200, 0));
	pDC->SetPen(warheadPen);
	pDC->SetBrush(warheadBrush);

	// Left warhead
	pDC->DrawRectangle(cx - (int)(34 * scale), cy - (int)(2 * scale),
		(int)(4 * scale), (int)(10 * scale));
	// Right warhead
	pDC->DrawRectangle(cx + (int)(30 * scale), cy - (int)(2 * scale),
		(int)(4 * scale), (int)(10 * scale));

	// 9. ARMING CORE (Visual indicator of suicide drone)
	pDC->SetPen(wxPen(wxColour(255, 255, 0), 1));
	pDC->SetBrush(*wxYELLOW_BRUSH);
	int coreRadius = std::max(1, (int)(2.5 * scale));
	pDC->DrawCircle(wxPoint(cx, cy + (int)(6 * scale)), coreRadius);
}

// Draw a Lambda Shuttle from above
void Spacecraft::DrawLambdaShuttle(wxDC* pDC, int cx, int cy, float scale)
{
	// Dynamic thickness calculations
	int mainThickness = std::max(1, (int)(2 * scale));
	int finThickness = std::max(1, (int)(4 * scale));
	int laserThickness = std::max(1, (int)(2 * scale));

	// 1. BRUSH CONFIGURATION (Royal Guard / Elite Imperial Shuttle)
	// Bright red border so it stands out against the black background
	wxPen penHull(wxColour(255, 50, 50), mainThickness);
	// Dark crimson red fill, imposing and very "Sith"
	wxBrush brushHull(wxColour(130, 15, 15));
	pDC->SetPen(penHull);
	pDC->SetBrush(brushHull);


	// 2. SIDE WINGS IN FLIGHT MODE (Deployed)
	// The Lambda wings from above are large swept-back triangles
	// Left Wing
	wxPoint leftWing[3];
	leftWing[0] = wxPoint(cx - (int)(10 * scale), cy - (int)(5 * scale));  // Front root
	leftWing[1] = wxPoint(cx - (int)(45 * scale), cy + (int)(15 * scale)); // Wing tip
	leftWing[2] = wxPoint(cx - (int)(15 * scale), cy + (int)(20 * scale)); // Rear root
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
	fuselage[0] = wxPoint(cx - (int)(4 * scale), cy - (int)(35 * scale)); // Left nose
	fuselage[1] = wxPoint(cx + (int)(4 * scale), cy - (int)(35 * scale)); // Right nose
	fuselage[2] = wxPoint(cx + (int)(15 * scale), cy + (int)(20 * scale)); // Right base
	fuselage[3] = wxPoint(cx - (int)(15 * scale), cy + (int)(20 * scale)); // Left base
	pDC->DrawPolygon(4, fuselage);

	// Front glass cockpit detail (Black transverse line)
	pDC->SetPen(wxPen(wxColour(0, 0, 0), mainThickness));
	pDC->DrawLine(cx - (int)(5 * scale), cy - (int)(25 * scale),
		cx + (int)(5 * scale), cy - (int)(25 * scale));


	// 4. CENTRAL DORSAL FIN (Top-down view)
	// Viewed from above, the huge upper fin is just a thick line in the center
	wxPen penFin(wxColour(255, 255, 255), finThickness); // Pure white to stand out
	pDC->SetPen(penFin);
	pDC->DrawLine(cx, cy - (int)(15 * scale), cx, cy + (int)(15 * scale));


	// 5. REAR ION ENGINES (Blue Glow)
	// The Lambda has three blue engines in the back
	wxPen penEngine(wxColour(50, 150, 255), mainThickness + 1); // Propulsion blue
	pDC->SetPen(penEngine);
	pDC->DrawLine(cx - (int)(10 * scale), cy + (int)(20 * scale),
		cx + (int)(10 * scale), cy + (int)(20 * scale));


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
	float scale = 0.8;
	if (getShipType() == ShipType::Shuttle)
	{
		DrawLambdaShuttle(pDC, 0.0, 0.0, scale);
	}
	else
	{
		if (getShipType() == ShipType::XWingDrone)
		{
			DrawXWingDrone(pDC, 0.0, 0.0, 0.5);
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

		pDC->DrawCircle(wxPoint(0, 0), RANGE_SENSOR);

		pDC->SetBrush(oldBrush);
	}

	// Restore matrix
	pDC->SetTransformMatrix(matrixBackup);
	pDC->SetPen(oldPen);


	// We save the current text color just in case and set a visible one (e.g. white or yellow)
	wxColour oldTextForeground = pDC->GetTextForeground();
	pDC->SetTextForeground(*wxYELLOW);
	// Create and assign the new font
	// Parameters: Size (e.g. 10), Family, Style (Normal/Italic), Weight (Normal/Bold)
	wxFont debugFont(5, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	pDC->SetFont(debugFont);

	// Get the turret heading (adjust "_laser.getHeading()" to the actual function you use)
	double turretHeading = 0.0;
	wxString debugLabel;
	if (_typeShip == ShipType::Shuttle)
	{
		turretHeading = _laser.getTurretHeading();
		debugLabel = wxString::Format(wxT("Pos: %.1f, %.1f\nHdg: %.1fº\nTrt: %.1fº"),
			_position.x, _position.y,
			dHeadingDeg, turretHeading);
	}
	else
	{
		debugLabel = wxString::Format(wxT("Pos: %.1f, %.1f\nHdg: %.1fº\n"),
			_position.x, _position.y, dHeadingDeg);
	}




	// Draw it with an offset (+20 in X/Y) relative to the ship so it does not cover the sprite
	// We keep the inverted Y logic (-_position.y) to position it correctly in the world
	pDC->DrawText(debugLabel, _position.x + 20.0, -_position.y + 20.0);

	// Restore original text color
	pDC->SetTextForeground(oldTextForeground);


	if (_typeShip == ShipType::Shuttle)
		_laser.Draw(pDC);



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
void Spacecraft::endStep()
{

	_Laser_tried_fired = 0;
}

void Spacecraft::FireLaser()
{
	_Laser_tried_fired++;

	if (_targetsIDs.size() > 0)
	{
		_laser.Fire();

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

void Spacecraft::Rotate_TurrentLeft()
{

	_laser.Rotate_Left();


}
void Spacecraft::Rotate_TurrentRight()
{
	_laser.Rotate_Right();
}

void Spacecraft::applyAction(int action)
{
	if (getState() == destroyed)
	{
		return;
	}

	if (_typeShip == ShipType::XWingDrone)
	{
		switch ((TEAM_BLUE_ACTIONS)action)
		{
		case TEAM_BLUE_ACTIONS::STOP:
		{
			Stop();
			break;
		}
		case TEAM_BLUE_ACTIONS::TURN_LEFT:
		{
			TurnLeft();
			break;
		}
		case TEAM_BLUE_ACTIONS::AHEAD:
		{
			// do nothing
			break;
		}
		case TEAM_BLUE_ACTIONS::TURN_RIGHT:
		{
			TurnRight();
			break;
		}
		}
	}

	if (_typeShip == ShipType::Shuttle)
	{
		switch ((TEAM_RED_ACTIONS)action)
		{
		case TEAM_RED_ACTIONS::NONE:
		{

			break;
		}
		case TEAM_RED_ACTIONS::ROTATE_LEFT_TURRET:
		{
			Rotate_TurrentLeft();
			break;
		}
		case TEAM_RED_ACTIONS::ROTATE_RIGHT_TURRET:
		{
			Rotate_TurrentRight();
			break;
		}
		case TEAM_RED_ACTIONS::FIRE_LASER:
		{
			FireLaser();
			break;
		}
		}
	}
}