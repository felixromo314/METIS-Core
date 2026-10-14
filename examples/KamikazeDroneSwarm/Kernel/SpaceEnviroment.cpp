#include "SpaceEnviroment.h"
#include "Spacecraft.h"

#include "..\MemoryLeaks.h"

#include <cmath>
#include <algorithm>
#include <random>

using namespace std;

int SpaceEnviroment::_maxSIZE_X = 592;
int SpaceEnviroment::_maxSIZE_Y = 259;
double computePenalty(double distance, double maxdistance)
{
	if (maxdistance <= 0.0) {
		return 1.0;
	}
	double safeDistance = std::max(0.0, std::min(distance, maxdistance));
	double ratio = safeDistance / maxdistance;
	return ratio * ratio;
}
// Returns [0 - 360 degrees]
double normalizeAngleDeg3(double angulo)
{
	double resultado = fmod(angulo, 360.0);
	if (resultado < 0) resultado += 360.0;
	return resultado;
}

int randomInt(int minY, int maxY)
{
	int valueRnd = minY + rand() % (maxY - minY + 1);

	return valueRnd;
}


double calculateHeadingTowardScortPosition(Metis::Vector2D& shipVelocity, Metis::Vector2D& scortPosRelative)
{
	Metis::Vector2D scortPosRelativeRelative;

	scortPosRelativeRelative.x = -scortPosRelative.x;
	scortPosRelativeRelative.y = -scortPosRelative.y;

	Metis::Vector2D  scortPosRelativeNorm = scortPosRelative.Normalize();

	/// 1. The dot product is directly the cosine in unit vectors
	double cosTheta = shipVelocity.x * scortPosRelativeNorm.x + shipVelocity.y * scortPosRelativeNorm.y;

	// 2. Limit due to floating-point precision to avoid NaN in acos
	if (cosTheta > 1.0) cosTheta = 1.0;
	else if (cosTheta < -1.0) cosTheta = -1.0;

	// 3. Get the angle (in radians or degrees)
	double angleRad = acos(cosTheta);
	double angleReturn = angleRad * 180.0 / M_PI; // Returns 0° to 180°

	return angleReturn;
}


// Returns [0 - 360 degrees]
double normalizeAngleDeg2(double angulo)
{
	double resultado = fmod(angulo, 360.0);
	if (resultado < 0) resultado += 360.0;
	return resultado;
}

double getDegreeAproxDist(double distance, double maxdistance)
{
	if (maxdistance <= 0.0)
		return 0.0; // avoid division by zero

	// Clamp only from below
	if (distance < 0.0)
		distance = 0.0;

	// Linear mapping:
	//  distance = 0           ->  1
	//  distance = maxdistance ->  0
	//  distance > maxdistance -> <0
	return 1.0 - (distance / maxdistance);
}

/**
* Degree of membership to the rear cone (Stern).
* @param angleATAnorm Normalized angle from 0 to 360.
* @param targetAngle The center of the cone (usually 180.0 for the stern).
* @param halfConeWidth Half of the cone width (45.0 for a 90-degree cone).
* @return 1.0 at 180 degrees, 0.0 at the boundaries (135 and 225), and 0 outside.
*/
double getBackConeMembership(double angleATAnorm, double targetAngle, double halfConeWidth)
{
	// 1. Calculate the deviation from the center (180 degrees)
	double diff = std::fabs(angleATAnorm - targetAngle);

	// 2. If it is outside the [135, 225] range, membership is 0
	if (diff >= halfConeWidth)
	{
		return 0.0;
	}

	// 3. Linear formula:
	// If diff is 0 (it is at 180°), it returns 1.0
	// If diff is 45 (it is at 135° or 225°), it returns 0.0
	return 1.0 - (diff / halfConeWidth);
}

/*
* Calculates the degree of membership to the front cone.
* @param angleATA The current ATA angle of the ship.
* @param halfConeWidth Half of the cone width (e.g., 30.0 for a 60-degree cone).
* @return Value between 0.0 and 1.0.
*/
double getConeMembership(double angleATA, double halfConeWidth)
{
	// We use the absolute value because the cone is symmetric (±30)
	double absAngle = std::fabs(angleATA);

	// If it is outside the cone, membership is zero
	if (absAngle >= halfConeWidth)
	{
		return 0.0;
	}

	// Linear formula: 1.0 - (current_angle / maximum_angle)
	// When absAngle is 0 -> results in 1.0
	// When absAngle is 30 -> results in 0.0
	return 1.0 - (absAngle / halfConeWidth);
}

float calculateATAangle(Metis::Vector2D& originPos, Metis::Vector2D& originDirection, Metis::Vector2D& targetPos)
{
	//Metis::Vector2D LOS = targetPos - originPos;  // if we work in absulute coordinates
	Metis::Vector2D LOS = originPos; //originPos it is already a vector relative to targetPos

	double distance = LOS.magnitude();

	double ATARad;
	double term1, term2;
	term1 = originDirection.Dot(LOS);
	term2 = originDirection.magnitude() * distance;

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

// maths
double computeAlignmentRewardRelativ(double rel_x, double rel_y, double headingDeg, double pos_target_x, double pos_target_y)
{
	// 1. Convert relative input to the 'toTarget' vector
	// If rel_x = ship.x - target.x, then -rel_x = target.x - ship.x
	double toTargetX = -rel_x; // rel_x is just a relative coordinate
	double toTargetY = -rel_y;

	// 2. Normalize the vector to the target
	double dist = std::sqrt(toTargetX * toTargetX + toTargetY * toTargetY);
	if (dist < 0.0001) return 1.0;

	double toTargetNormX = toTargetX / dist;
	double toTargetNormY = toTargetY / dist;

	// 3. Ship direction vector (according to your Sin/Cos convention)
	double headingRad = headingDeg * M_PI / 180.0;
	double shipDirX = std::sin(headingRad);
	double shipDirY = std::cos(headingRad);

	// No need to normalize shipDir if sin^2 + cos^2 = 1, but we do it for safety
	double magDir = std::sqrt(shipDirX * shipDirX + shipDirY * shipDirY);
	shipDirX /= magDir;
	shipDirY /= magDir;

	// 4. Dot Product
	double cosAngle = (shipDirX * toTargetNormX) + (shipDirY * toTargetNormY);
	cosAngle = std::clamp(cosAngle, -1.0, 1.0);

	// 5. Calculate angle and reward/penalty
	double angle = std::acos(cosAngle);
	const double threshold = 0.261799; // 15 degrees (30-degree cone = 15º*2)

	if (angle <= threshold) {
		return cosAngle;
	}
	else {
		// 5. Calculate angle and reward/penalty
		double penalty = -std::pow(angle / M_PI, 2);
		return penalty;
	}
}
double getDegreeAproxDist_01(double distance, double maxdistance)
{
	if (maxdistance <= 0.0)
		return 0.0; // Avoid division by zero or invalid values

	// Clamp the distance to the [0, maxdistance] range
	if (distance < 0.0) distance = 0.0;
	if (distance > maxdistance) distance = maxdistance;

	// Maps from [0,maxdistance] to [1,0]
	return 1.0 - (distance / maxdistance);
}




SpaceEnviroment::SpaceEnviroment()
{
	_currentPlayer = 0;
	_currentEpisode = 0;
	_currenteStep = 0;
	_teamWinner = -1;
	srand(time(NULL));

	_drone_XWing_1.setID(0);
	_drone_XWing_2.setID(1);
	_ImperialShuttle.setID(2);

	this->addAgent(&_drone_XWing_1);
	this->addAgent(&_drone_XWing_2);
	this->addAgent(&_ImperialShuttle);
}

SpaceEnviroment& SpaceEnviroment::operator=(SpaceEnviroment& other)
{
	if (this != &other)
	{
		_minmaxReward = other._minmaxReward;
		_minDistEverReached = other._minDistEverReached;

		_posTarget = other._posTarget;

		_currentPlayer = other._currentPlayer;
		_currenteStep = other._currenteStep;
		_decoyWindowCounter = other._decoyWindowCounter;
		_laserWindowCounter = other._laserWindowCounter;

		_drone_XWing_1 = other._drone_XWing_1;
		_drone_XWing_2 = other._drone_XWing_2;
		_ImperialShuttle = other._ImperialShuttle;

		this->addAgent(&_drone_XWing_1);
		this->addAgent(&_drone_XWing_2);
		this->addAgent(&_ImperialShuttle);

	}
	return *this;
}


SpaceEnviroment::SpaceEnviroment(SpaceEnviroment& other)
{

	_minmaxReward = other._minmaxReward;
	_minDistEverReached = other._minDistEverReached;

	_posTarget = other._posTarget;

	_currentPlayer = other._currentPlayer;
	_currenteStep = other._currenteStep;
	_decoyWindowCounter = other._decoyWindowCounter;
	_laserWindowCounter = other._laserWindowCounter;

	_drone_XWing_1 = other._drone_XWing_1;
	_drone_XWing_2 = other._drone_XWing_2;
	_ImperialShuttle = other._ImperialShuttle;

	_drone_XWing_1._pEnv = this;
	_drone_XWing_2._pEnv = this;
	_ImperialShuttle._pEnv = this;
	_ImperialShuttle._laser.setPlatform(&_ImperialShuttle);

	_drone_XWing_1._laser.setPlatform(&_drone_XWing_1);
	_drone_XWing_2._laser.setPlatform(&_drone_XWing_2);


	this->addAgent(&_drone_XWing_1);
	this->addAgent(&_drone_XWing_2);
	this->addAgent(&_ImperialShuttle);

}

IAlphaGoZeroStateBase* SpaceEnviroment::clone()
{

	SpaceEnviroment* pEnvCloned = new SpaceEnviroment(*this);

	return (IAlphaGoZeroStateBase*)pEnvCloned;
}

SpaceEnviroment::~SpaceEnviroment()
{
	//TODO: delete the players
}

double SpaceEnviroment::getDeltaTime()
{
	return (double)DELTA_TIME;
}

bool SpaceEnviroment::isEqualState(IAlphaGoZeroStateBase* pOtherState)
{
	bool bIsEqualState = true;

	SpaceEnviroment* pOtherEnv = (SpaceEnviroment*)pOtherState;

	//if (_currentPlayer != pOtherEnv->_currentPlayer) bIsEqualState = false;
	if (_posTarget != pOtherEnv->_posTarget) bIsEqualState = false;
	//if (_currenteStep != pOtherEnv->_currenteStep) bIsEqualState = false;
	//if (_currentEpisode != pOtherEnv->_currentEpisode) bIsEqualState = false;
	if (_decoyWindowCounter != pOtherEnv->_decoyWindowCounter) bIsEqualState = false;
	if (_laserWindowCounter != pOtherEnv->_laserWindowCounter) bIsEqualState = false;
	//if (_teamWinner != pOtherEnv->_teamWinner) bIsEqualState = false;

	if (_drone_XWing_1 != pOtherEnv->_drone_XWing_1)
	{
		bIsEqualState = false;
	}
	if (_drone_XWing_2 != pOtherEnv->_drone_XWing_2)
	{
		bIsEqualState = false;
	}
	if (_ImperialShuttle != pOtherEnv->_ImperialShuttle)
	{
		bIsEqualState = false;
	}

	return bIsEqualState;
}


std::vector<float> SpaceEnviroment::getState()
{

	std::vector<float> vectorState;

	vectorState.push_back(1.0f);
	vectorState.push_back(1.0f);
	vectorState.push_back(1.0f);

	return vectorState;
}

int SpaceEnviroment::getTeamID(int currentPlayerID)
{
	int teamID = -1;

	if ((currentPlayerID == 0) || (currentPlayerID == 1))
	{
		teamID = TEAM_BLUE_ID;
	}
	else
	{
		if (currentPlayerID == SHUTTLE_ID)
			teamID = TEAM_RED_ID;
	}


	return teamID;
}

bool SpaceEnviroment::is_valid_action(int actionID)
{
	return true;
}


int SpaceEnviroment::getNumActions(int playerID)
{
	int numActions = 0;
	if (playerID <= 1)
	{
		numActions = (int)TEAM_BLUE_ACTIONS::MAX_ACTIONS;
	}
	else numActions = (int)TEAM_RED_ACTIONS::MAX_ACTIONS;

	return numActions;
}
std::vector<float> SpaceEnviroment::getState(int currentPlayer)
{
	std::vector<float> spaceVector;

	float maxSize = 600.0;

	double MAX_RELATIVE_SPEED = MAX_SPEED_DRON_XWING + MAX_SPEED_SHUTTLE;

	int teamID = this->getTeamID(currentPlayer);

	if (teamID == TEAM_BLUE_ID) // TEAM BLUE
	{
		Spacecraft* pShuttle = (Spacecraft*)this->getAgentByID(SHUTTLE_ID);
		Metis::Vector2D posShuttle = pShuttle->getPosition();
		Metis::Vector2D velocShuttle = pShuttle->getVelocity();

		Spacecraft* pDroneXWing = (Spacecraft*)this->getAgentByID(currentPlayer);
		Spacecraft* pDroneWinMan = (Spacecraft*)this->getAgentByID(pDroneXWing->_dronWingMan);

		Metis::Vector2D posDron1 = pDroneXWing->getPosition();
		Metis::Vector2D velocDron1 = pDroneXWing->getVelocity();


		Metis::Vector2D relativPosDron = posDron1 - posShuttle;
		Metis::Vector2D relativVelocDron = velocDron1 - velocShuttle;
		relativVelocDron.x /= MAX_RELATIVE_SPEED;
		relativVelocDron.y /= MAX_RELATIVE_SPEED;

		// wingman data
		Metis::Vector2D posDron2 = pDroneWinMan->getPosition();
		Metis::Vector2D velocDron2 = pDroneWinMan->getVelocity();
		Metis::Vector2D relativPosDron2 = posDron2 - posShuttle;

		Metis::Vector2D relativVelocDron2 = velocDron2 - velocShuttle;
		relativVelocDron2.x /= MAX_RELATIVE_SPEED;
		relativVelocDron2.y /= MAX_RELATIVE_SPEED;

		TOBS_DRON_XWING xwindroncurrent;
		xwindroncurrent.me.heading = pDroneXWing->getHeading() / 360.0;
		xwindroncurrent.me.x = relativPosDron.x / maxSize;
		xwindroncurrent.me.y = relativPosDron.y / maxSize;
		xwindroncurrent.me.vx = relativVelocDron.x;
		xwindroncurrent.me.vy = relativVelocDron.y;
		xwindroncurrent.me.isAlive = (float)pDroneXWing->isAlive();

		xwindroncurrent.wingman.heading = pDroneWinMan->getHeading() / 360.0;
		xwindroncurrent.wingman.x = relativPosDron2.x / maxSize;
		xwindroncurrent.wingman.y = relativPosDron2.y / maxSize;
		xwindroncurrent.wingman.vx = relativVelocDron2.x;
		xwindroncurrent.wingman.vy = relativVelocDron2.y;
		xwindroncurrent.wingman.isAlive = (float)pDroneWinMan->isAlive();

		xwindroncurrent.shuttle_turret_heading = pShuttle->getTurrentHeading() / 360.0;
		xwindroncurrent.shuttle_laser_cooldown = pShuttle->getLaserTemperature();


		xwindroncurrent.toVector(spaceVector);


	}
	else
	{
		if (teamID == TEAM_RED_ID) // TEAM RED
		{
			Spacecraft* pShuttle = (Spacecraft*)this->getAgentByID(currentPlayer);

			Metis::Vector2D posShuttle = pShuttle->getPosition();
			Metis::Vector2D velocShuttle = pShuttle->getVelocity();

			float numDronesDetected = (float)pShuttle->getNumberDroneDetected();

			float laserTemperature = (float)pShuttle->getLaserTemperature();
			float turretHeading = (float)pShuttle->getTurrentHeading();


			TOBS_SHUTTLE shuttle;

			shuttle.heading = pShuttle->getHeading() / 360.0;

			shuttle.shuttle_turret_heading = turretHeading / 360.0;
			shuttle.numberOf_DroneX_WingDetected = numDronesDetected / (float)MAX_DRONES_XWING;

			// sorted by danger..the first one is the closest
			std::vector<int> dronesDetected = pShuttle->getDronesDetected();

			if (dronesDetected.size() != numDronesDetected)
			{
				int x = 0;
			}

			for (int i = 0; i < MAX_DRONES_XWING; i++)
			{
				memset(&shuttle.droneDetected[i], 0, sizeof(TDRONE_PHYSICAL_DATA));
			}

			int idDrones = 0;
			for (int i = 0; i < numDronesDetected; i++)
			{
				TDRONE_PHYSICAL_DATA dronData;

				int droneID = dronesDetected[i];
				Spacecraft* pDrone = (Spacecraft*)this->getAgentByID(droneID);

				if (pDrone->isAlive())
				{
					Metis::Vector2D posDron = pDrone->getPosition();
					Metis::Vector2D velocDron = pDrone->getVelocity();

					Metis::Vector2D relativPosDron = posDron - posShuttle;
					Metis::Vector2D relativVelocDron = velocDron - velocShuttle;
					relativVelocDron.x /= MAX_RELATIVE_SPEED;
					relativVelocDron.y /= MAX_RELATIVE_SPEED;

					dronData.heading = pDrone->getHeading() / 360.0;
					dronData.x = relativPosDron.x / maxSize;
					dronData.y = relativPosDron.y / maxSize;
					dronData.vx = relativVelocDron.x;
					dronData.vy = relativVelocDron.y;
					dronData.isAlive = (float)pDrone->isAlive();
					shuttle.droneDetected[i] = dronData;
				}

			}



			shuttle.shuttle_laser_cooldown = laserTemperature; // already normalized
			shuttle.Laser_fired = pShuttle->wasLaserFired();
			shuttle.Laser_fired = 0.0;
			shuttle.Laser_firing = pShuttle->thereIsLaserFlying();

			shuttle.toVector(spaceVector);

		}
	}

	return spaceVector;
}
std::vector<float> SpaceEnviroment::getStateFromTeam(int teamID)
{
	std::vector<float> spaceVector;


	if (teamID == TEAM_BLUE_ID) // TEAM BLUE
	{
		Spacecraft* pDroneXWing1 = (Spacecraft*)this->getAgentByID(0);
		Spacecraft* pDroneXWing2 = (Spacecraft*)this->getAgentByID(1);

		TOBS_DRON_XWING xwindron1;
		TOBS_DRON_XWING xwindron2;

		xwindron1.toVector(spaceVector);
		xwindron2.toVector(spaceVector);


	}
	else
	{
		if (teamID == TEAM_RED_ID) // TEAM RED
		{
			Spacecraft* pShuttle = (Spacecraft*)this->getAgentByID(2);

			TOBS_SHUTTLE shuttle;

			shuttle.toVector(spaceVector);

		}
	}

	return spaceVector;
}

int SpaceEnviroment::getWinner()
{
	return _teamWinner;
}
double SpaceEnviroment::getReward(int teamID)
{
	double reward = 0.0;
	if (_teamWinner != -1)
	{
		if (teamID == _teamWinner)
		{
			reward = 1.0;
		}
		else
		{
			reward = -1.0;
		}
	}
	else
	{
		reward = 0.0;
	}

	return reward;

}
bool SpaceEnviroment::isTerminal()
{
#ifdef _DEBUG
	static FILE* fpTerminal = NULL;
	if (fpTerminal == NULL) fpTerminal = fopen("terminal.txt", "w+t");
#endif

	Spacecraft* pDrone1 = (Spacecraft*)getAgentByID(0);
	Spacecraft* pDrone2 = (Spacecraft*)getAgentByID(1);
	Spacecraft* pShuttle = (Spacecraft*)getAgentByID(2);

	// 1. Check the time limit (Real draw)
	if (_currenteStep > MAX_STEPS)
	{
		_teamWinner = -1; // Draw
#ifdef _DEBUG
		fprintf(fpTerminal, "tablas: exceso de steps:%d\n", _currenteStep);
		fflush(fpTerminal);
#endif
		return true;
	}

	// 2. Evaluate Drone -> Shuttle interception collisions
	Metis::Vector2D posShuttle = pShuttle->getPosition();
	double dist1 = pDrone1->getPosition().DistanceTo(posShuttle);
	double dist2 = pDrone2->getPosition().DistanceTo(posShuttle);

	if ((dist1 < 25) || (dist2 < 25))
	{
		pShuttle->setState(destroyed);
#ifdef _DEBUG
		if (dist1 < 25) fprintf(fpTerminal, "WIN: DRON 1 derribo shuttle: distancia:%f\n", dist1);
		if (dist2 < 25) fprintf(fpTerminal, "WIN: DRON 2 derribo shuttle: distancia:%f\n", dist2);
		fprintf(fpTerminal, "WIN: TEAM_BLUE_ID\n");
		fflush(fpTerminal);
#endif
		_teamWinner = TEAM_BLUE_ID;
		return true; // We finish here, the shuttle cannot reach the goal if it is destroyed
	}

	// 3. Evaluate whether the Shuttle reached the target (It only gets here if it was not destroyed above)
	if (posShuttle.DistanceTo(_posTarget) < 10.0)
	{
		_teamWinner = TEAM_RED_ID;
#ifdef _DEBUG
		fprintf(fpTerminal, "WIN: TEAM_RED_ID. llego al target ileso!!\n");
		fflush(fpTerminal);
#endif
		return true;
	}

	// 4. Evaluate the disabled states of the Drones
	int state1 = pDrone1->getState();
	int state2 = pDrone2->getState();

	bool dron1Out = (state1 == destroyed || state1 == collision);
	bool dron2Out = (state2 == destroyed || state2 == collision);

	// If BOTH drones are out of action (either destroyed or collided), 
	// the Blue team failed its mission. The Red team (Shuttle) wins.
	if (dron1Out && dron2Out)
	{
		_teamWinner = TEAM_RED_ID;
#ifdef _DEBUG
		fprintf(fpTerminal, "WIN: TEAM_RED_ID (Ambos drones inoperativos)\n");
		fflush(fpTerminal);
#endif
		return true;
	}

	// If we get here, the game continues
	return false;
}

// Dirección normalizada segura: vector casi nulo -> (0,0), así el producto escalar queda neutro en vez de NaN
static inline Metis::Vector2D dirSegura(Metis::Vector2D v)
{
	double m = v.magnitude();
	return (m > 1e-6) ? Metis::Vector2D(v.x / m, v.y / m) : Metis::Vector2D(0.0, 0.0);
}

float SpaceEnviroment::getMaterialHeuristicValue(int playerToEvaluateID)
{
	Spacecraft* pShuttle = (Spacecraft*)this->getAgentByID(SHUTTLE_ID);
	if (pShuttle == nullptr) return 0.0f;

	Spacecraft* drones[2] = { (Spacecraft*)this->getAgentByID(0), (Spacecraft*)this->getAgentByID(1) };
	const float maxDist = (float)SpaceEnviroment::_maxSIZE_X;
	const Metis::Vector2D posShuttle = pShuttle->getPosition();

	// Dirección de la torreta
	float turretRad = (float)pShuttle->_laser.getTurretHeading() * (float)M_PI / 180.0f;
	Metis::Vector2D turretFwd(sinf(turretRad), cosf(turretRad));

	// ---- Medidas por dron (todas en [0, 1]) ----
	bool vivo[2] = { false, false };
	float cerca[2] = { 0, 0 };        // 1 = pegado al shuttle, 0 = a maxDist o más
	float aproxima[2] = { 0, 0 };     // 1 = velocidad apuntando al shuttle, 0 = alejándose o de lado
	float expuesto[2] = { 0, 0 };     // 1 = justo en la línea de la torreta
	float dist[2] = { maxDist, maxDist };
	Metis::Vector2D haciaDron[2];
	int vivos = 0;

	for (int i = 0; i < 2; i++)
	{
		if (drones[i] == nullptr || drones[i]->getState() == destroyed) continue;
		vivo[i] = true; vivos++;

		Metis::Vector2D vDiff = drones[i]->getPosition() - posShuttle;   // shuttle -> dron
		dist[i] = (float)vDiff.magnitude();
		haciaDron[i] = dirSegura(vDiff);

		cerca[i] = 1.0f - std::min(1.0f, dist[i] / maxDist);

		// Aproximación: velocidad del dron hacia el shuttle (solo cuenta si se acerca)
		Metis::Vector2D dirVel = dirSegura(drones[i]->getVelocity());
		float dotAprox = -(float)(dirVel.x * haciaDron[i].x + dirVel.y * haciaDron[i].y);   // haciaDron apunta al dron: invertimos
		aproxima[i] = std::max(0.0f, dotAprox);

		// Exposición: solo cuenta dentro del cono de la torreta (potencia 8: ±15° ≈ 0.76, ±30° ≈ 0.32, ±60° ≈ 0)
		float dotTorreta = (float)(turretFwd.x * haciaDron[i].x + turretFwd.y * haciaDron[i].y);
		expuesto[i] = powf(std::max(0.0f, dotTorreta), 8.0f);
	}

	// ---- Medidas conjuntas ----
	float separacion = 0.0f;          // 0 = juntos, 1 = lados opuestos del shuttle
	float sincronia = 0.0f;           // 1 = llegan a la vez (misma distancia)
	if (vivos == 2)
	{
		float dotPinza = (float)(haciaDron[0].x * haciaDron[1].x + haciaDron[0].y * haciaDron[1].y);
		separacion = (1.0f - dotPinza) / 2.0f;
		const float D_SYNC = 0.25f * maxDist;                       // diferencia de distancia tolerada
		sincronia = 1.0f - std::min(1.0f, fabsf(dist[0] - dist[1]) / D_SYNC);
	}

	// ---- Calidad del ataque azul en [0, 1] ----
	float calidadAzul = 0.0f;
	float amenaza = 0.0f;             // peligro de la torreta para el dron más expuesto, en [0, 1]
	if (vivos == 2)
	{
		float aprox = 0.5f * ((0.5f * cerca[0] + 0.5f * aproxima[0]) + (0.5f * cerca[1] + 0.5f * aproxima[1]));
		float aproxPonderada = aprox * (0.25f + 0.75f * separacion);       // juntos solo cuenta el 25 %
		float pinza = separacion * (0.6f + 0.4f * sincronia);
		calidadAzul = 0.4f * pinza + 0.6f * aproxPonderada;

		// La amenaza a un dron pesa menos si el otro llega por el lado opuesto
		amenaza = std::max(expuesto[0] * cerca[0], expuesto[1] * cerca[1]) * (1.0f - 0.5f * separacion);
	}
	else if (vivos == 1)
	{
		int i = vivo[0] ? 0 : 1;
		calidadAzul = 0.5f * cerca[i] + 0.5f * aproxima[i];
		amenaza = expuesto[i] * cerca[i];                                  // sin compañero, sin descuento
	}

	// ---- Puntería del rojo (solo cuenta si la bala pasa muy cerca) ----
	float punteria = 0.0f;
	if (vivos > 0 && pShuttle->_laser.isFiring())
	{
		const double R_PUNTERIA = 0.10 * (double)RANGE_SENSOR;
		double distBala = pShuttle->_laser.getBulletMinDistanceToTarget();
		punteria = (float)std::max(0.0, std::min(1.0, 1.0 - distBala / R_PUNTERIA));
	}
	float calidadRojo = 0.8f * amenaza + 0.2f * punteria;

	// ---- Ventaja del azul: material relativo (0 sin bajas) + posición ----
	int destruidos = 2 - vivos;
	float material = -(float)destruidos / 2.0f;                            // 0, -0.5, -1
	float ventajaAzul = 0.3f * material + 0.7f * (calidadAzul - calidadRojo);
	ventajaAzul = std::max(-1.0f, std::min(1.0f, ventajaAzul));

	// Suma cero: el rojo recibe exactamente la contraria
	return (this->getTeamID(playerToEvaluateID) == TEAM_BLUE_ID) ? ventajaAzul : -ventajaAzul;
}
// returns true if the player can act (not destroyed nor collided)
bool SpaceEnviroment::isPlayerActive(int playerID)
{
	Spacecraft* pPlayer = (Spacecraft*)getAgentByID(playerID);
	if (pPlayer == NULL) return false;

	int state = pPlayer->getState();
	return (state != destroyed) && (state != collision);
}

// advances _currentPlayer to the next active player (starting from the one after the current one if bIncludeCurrent = false)
// if nobody is active, leaves _currentPlayer as it was (the episode will be terminal)
void SpaceEnviroment::advanceToNextActivePlayer(bool bIncludeCurrent)
{
	int start = bIncludeCurrent ? 0 : 1;

	for (int i = start; i <= MAX_PLAYERS; i++)
	{
		int candidate = (_currentPlayer + i) % MAX_PLAYERS;
		if (isPlayerActive(candidate))
		{
			_currentPlayer = candidate;
			return;
		}
		else
		{
			int x = 0;
		}
	}
	// nobody active: we do not touch _currentPlayer, isTerminal() will close the episode
}
void SpaceEnviroment::applyAction(int action)
{
	Spacecraft* pPlayer = (Spacecraft*)getAgentByID(_currentPlayer);
	assert(pPlayer != NULL);

	// only applies the action if the player is still active
	if (isPlayerActive(_currentPlayer))
	{
		pPlayer->applyAction(action);
	}
	else
	{
		int x = 0;
	}


	// we pass the turn to the next active player
	advanceToNextActivePlayer(false);
}
// returns the actions of the player playerID
void SpaceEnviroment::getPossibleActions(int playerID, std::vector<TACTION_AIRCRAFT>& actions)
{
	actions.clear();

	//here
	int teamID = this->getTeamID(playerID);
	if (teamID == TEAM_BLUE_ID)
	{

		for (int iAction = 0; iAction < ((int)TEAM_BLUE_ACTIONS::MAX_ACTIONS); iAction++)
		{
			TACTION_AIRCRAFT action;

			action.typeAction = iAction;

			actions.push_back(action);
		}

	}
	else
	{
		if (teamID == TEAM_RED_ID)
		{
			for (int iAction = 0; iAction < ((int)TEAM_RED_ACTIONS::MAX_ACTIONS); iAction++)
			{
				TACTION_AIRCRAFT action;

				action.typeAction = iAction;

				actions.push_back(action);
			}
		}
	}



	// Get all pseudo-legal moves for the current player
	//std::vector<TPIECEMOVEMENT> pseudoMoves;
	//movingPlayer->getAllMovements(&pseudoMoves);
}

void SpaceEnviroment::update(float incTime)
{
	int numberPlayers = getNumAgents();

	for (int i = 0; i < numberPlayers; i++)
	{
		Spacecraft* pPlayer = (Spacecraft*)getAgentByID(i);
		pPlayer->update(DELTA_TIME);

		if (pPlayer->getState() == searched)
		{
			Metis::Vector2D pos = pPlayer->getPosition();
			if ((pos.x <= -SpaceEnviroment::_maxSIZE_X) || (pos.x >= SpaceEnviroment::_maxSIZE_X))
				pPlayer->setState(collision);
			if ((pos.y <= -SpaceEnviroment::_maxSIZE_Y) || (pos.y >= SpaceEnviroment::_maxSIZE_Y))
				pPlayer->setState(collision);
		}
	}

	Spacecraft* pDron1 = (Spacecraft*)getAgentByID(0);
	Spacecraft* pDron2 = (Spacecraft*)getAgentByID(1);
	Spacecraft* pShuttle = (Spacecraft*)getAgentByID(2);
	Spacecraft* drones[2];
	drones[0] = pDron1;
	drones[1] = pDron2;

	for (int i = 0; i < 2; i++) {
		if (drones[i] != nullptr) {
			double d = (drones[i]->getPosition() - pShuttle->getPosition()).magnitude();
			if (d < _minDistEverReached)
			{
				_minDistEverReached = d;
			}
		}
	}

	_currenteStep++;
}

void SpaceEnviroment::reset2(int type)
{
	_currentEpisode++;
	_currenteStep = 0;

	//_currentEpisode = 601;

	// ---------------------------------------------------------
	// 1. Shuttle turret rotation logic
	// ---------------------------------------------------------
	const int EPISODES_PER_ROTATION = 25;
	const double BASE_TURRET_HEADING = 180.0;
	const double ROTATION_STEP = 45.0;

	long long rotationSteps = _currentEpisode / EPISODES_PER_ROTATION;
	double turretHeading = BASE_TURRET_HEADING + (rotationSteps * ROTATION_STEP);
	turretHeading = fmod(turretHeading, 360.0);
	if (turretHeading < 0.0) {
		turretHeading += 360.0;
	}

	// ---------------------------------------------------------
	// 2. Initial Shuttle configuration
	// ---------------------------------------------------------
	Metis::Vector2D posShuttle(-400.0, 0.0);
	Spacecraft* pShuttle = (Spacecraft*)this->getAgentByID(2);

	pShuttle->setPosition(posShuttle.x, posShuttle.y);
	pShuttle->setHeading(90.0);
	pShuttle->setTurretHeading(turretHeading);
	pShuttle->setSpeed(MAX_SPEED_SHUTTLE);
	pShuttle->clean();

	// ---------------------------------------------------------
	// 3. CURRICULUM LEARNING: Drone Separation Calculation
	// ---------------------------------------------------------
	const int EPISODES_PER_PHASE = 100;
	const int MAX_PHASES = 10;

	int phase = std::min(_currentEpisode / EPISODES_PER_PHASE, MAX_PHASES);
	double difficultyFactor = (double)phase / (double)MAX_PHASES;

	// Interpolation of the distance BETWEEN the drones
	const double START_SEPARATION = 80.0; // Easy: large separation (episode 0)
	const double END_SEPARATION = 30.0;    // Hard: very close together (episode 1000)

	// As difficultyFactor goes from 0 to 1, the separation drops from 300 to 30
	double currentSeparation = START_SEPARATION + (END_SEPARATION - START_SEPARATION) * difficultyFactor;
	double halfSeparation = currentSeparation / 2.0;

	// ---------------------------------------------------------
	// 4. Random Generation of the "Formation Center"
	// ---------------------------------------------------------
	static std::random_device rd;
	static std::mt19937 gen(rd());

	// Distance of the group relative to the Shuttle (we keep a random radius to avoid overfitting)
	std::uniform_real_distribution<double> dis_radius(120.0, 250.0);
	std::uniform_real_distribution<double> dis_angle(0.0, 2.0 * M_PI);

	// Noise to rotate the drone formation (so they are not always aligned horizontally)
	std::uniform_real_distribution<double> dis_formation_angle(0.0, 2.0 * M_PI);

	double formationRadius = dis_radius(gen);
	double formationTheta = dis_angle(gen);

	// Central (X, Y) coordinate of the drone squadron
	double centerX = posShuttle.x + formationRadius * cos(formationTheta);
	double centerY = posShuttle.y + formationRadius * sin(formationTheta);

	// Angle at which the two drones will be aligned relative to their center
	double formationAlignAngle = dis_formation_angle(gen);

	// ---------------------------------------------------------
	// 5. Drone Deployment
	// ---------------------------------------------------------
	Spacecraft* pDron1 = (Spacecraft*)this->getAgentByID(0);
	Spacecraft* pDron2 = (Spacecraft*)this->getAgentByID(1);

	// DRONE 1: Placed at -halfSeparation from the center at the formation angle
	double d1_x = centerX + halfSeparation * cos(formationAlignAngle);
	double d1_y = centerY + halfSeparation * sin(formationAlignAngle);
	pDron1->setPosition(d1_x, d1_y);

	// Orient Drone 1 pointing at the Shuttle (Note: Standard signature atan2(y, x))
	double d1_heading = atan2(posShuttle.x - d1_x, posShuttle.y - d1_y) * 180.0 / M_PI;
	if (d1_heading < 0.0) d1_heading += 360.0;
	pDron1->setHeading(d1_heading);
	pDron1->clean();

	// DRONE 2: Placed at +halfSeparation from the center (opposite angle)
	double d2_x = centerX + halfSeparation * cos(formationAlignAngle + M_PI);
	double d2_y = centerY + halfSeparation * sin(formationAlignAngle + M_PI);
	pDron2->setPosition(d2_x, d2_y);

	// Orient Drone 2 pointing at the Shuttle (Note: Standard signature atan2(y, x))
	double d2_heading = atan2(posShuttle.x - d2_x, posShuttle.y - d2_y) * 180.0 / M_PI;
	if (d2_heading < 0.0) d2_heading += 360.0;
	pDron2->setHeading(d2_heading);
	pDron2->clean();
}
void SpaceEnviroment::reset3(int type)
{
	_currentEpisode++;
	_currenteStep = 0;

	// ---------------------------------------------------------
	// 1. Turret rotation logic
	// ---------------------------------------------------------
	const int EPISODES_PER_ROTATION = 25;
	const double BASE_TURRET_HEADING = 180.0;
	const double ROTATION_STEP = 45.0;

	long long rotationSteps = _currentEpisode / EPISODES_PER_ROTATION;
	double turretHeading = BASE_TURRET_HEADING + (rotationSteps * ROTATION_STEP);
	turretHeading = fmod(turretHeading, 360.0);
	if (turretHeading < 0.0) {
		turretHeading += 360.0;
	}

	// ---------------------------------------------------------
	// 2. Initial Shuttle configuration
	// ---------------------------------------------------------
	Metis::Vector2D posShuttle(-350.0, 0.0);
	Spacecraft* pShuttle = (Spacecraft*)this->getAgentByID(2);

	pShuttle->setPosition(posShuttle.x, posShuttle.y);
	pShuttle->setHeading(90.0);
	pShuttle->setTurretHeading(turretHeading);
	//pShuttle->setTurretHeading(0.0); // fromo
	pShuttle->setSpeed(MAX_SPEED_SHUTTLE);
	//pShuttle->setSpeed(0);  // fromo
	pShuttle->clean();

	// ---------------------------------------------------------
	// 3. CURRICULUM LEARNING: Difficulty and Pincer Offset
	// ---------------------------------------------------------
	const int EPISODES_PER_PHASE = 100;
	const int MAX_PHASES = 10;

	int phase = std::min(_currentEpisode / EPISODES_PER_PHASE, MAX_PHASES);
	double difficultyFactor = (double)phase / (double)MAX_PHASES;

	// ---------------------------------------------------------
	// 4. Radius Configuration (Outside the 200px sensor)
	// ---------------------------------------------------------
	// Strict requested range: Min 205.0 px, Max 240.0 px
	const double MIN_RADIUS = 205.0;
	const double MAX_RADIUS = 240.0;

	// Optional: The maximum radius scales slightly with difficulty within that safe range
	double currentMaxRadius = MIN_RADIUS + (MAX_RADIUS - MIN_RADIUS) * difficultyFactor;

	// ---------------------------------------------------------
	// 5. Uniform Random Generation
	// ---------------------------------------------------------
	static std::random_device rd;
	static std::mt19937 gen(rd());

	std::uniform_real_distribution<double> dis_radius(MIN_RADIUS, currentMaxRadius);
	std::uniform_real_distribution<double> dis_angle(0.0, 2.0 * M_PI);
	std::uniform_real_distribution<double> dis_noise(-0.2, 0.2); // Small random noise

	Spacecraft* pDron1 = (Spacecraft*)this->getAgentByID(0);
	Spacecraft* pDron2 = (Spacecraft*)this->getAgentByID(1);

	// ---------------------------------------------------------
	// 6. Deployment: Drone 1
	// ---------------------------------------------------------
	double r1 = dis_radius(gen);
	double theta1 = dis_angle(gen);

	r1 = -215.0;// -230.0;
	theta1 = wxDegToRad(10.0);

	double d1_x = posShuttle.x + r1 * cos(theta1);
	double d1_y = posShuttle.y + r1 * sin(theta1);
	pDron1->setPosition(d1_x, d1_y);
	pDron1->setSpeed(MAX_SPEED_DRON_XWING);

	double d1_heading = atan2(posShuttle.x - d1_x, posShuttle.y - d1_y) * 180.0 / M_PI;
	if (d1_heading < 0.0) d1_heading += 360.0;
	pDron1->setHeading(d1_heading);
	pDron1->clean();

	// ---------------------------------------------------------
	// 7. Deployment: Drone 2 (With dynamic pincer angle)
	// ---------------------------------------------------------
	double r2 = dis_radius(gen);

	// Interpolation of the angular offset according to the difficulty:
	// - difficultyFactor = 0.0 -> 180° offset (Full pincer)
	// - difficultyFactor = 1.0 -> 15° offset (Grouped attack)
	const double INITIAL_OFFSET = M_PI;
	const double FINAL_OFFSET = wxDegToRad(15.0);
	double currentOffset = INITIAL_OFFSET + (FINAL_OFFSET - INITIAL_OFFSET) * difficultyFactor;

	double theta2 = theta1 + currentOffset + dis_noise(gen);

	r2 = -215.0; // -230.0;
	theta2 = wxDegToRad(-10.0);

	double d2_x = posShuttle.x + r2 * cos(theta2);
	double d2_y = posShuttle.y + r2 * sin(theta2);
	pDron2->setPosition(d2_x, d2_y);

	double d2_heading = atan2(posShuttle.x - d2_x, posShuttle.y - d2_y) * 180.0 / M_PI;
	if (d2_heading < 0.0) d2_heading += 360.0;
	pDron2->setHeading(d2_heading);
	pDron2->clean();
	pDron2->setSpeed(MAX_SPEED_DRON_XWING);
}

void SpaceEnviroment::reset(int type)
{

	_minDistEverReached = 1000000.0;
	reset3(type);
	return;

	_currentEpisode++;


	_currenteStep = 0;

	// ---------------------------------------------------------
	// 1. Turret rotation logic
	// ---------------------------------------------------------
	const int EPISODES_PER_ROTATION = 25;
	const double BASE_TURRET_HEADING = 180.0;
	const double ROTATION_STEP = 45.0;

	long long rotationSteps = _currentEpisode / EPISODES_PER_ROTATION;
	double turretHeading = BASE_TURRET_HEADING + (rotationSteps * ROTATION_STEP);
	turretHeading = fmod(turretHeading, 360.0);
	if (turretHeading < 0.0) {
		turretHeading += 360.0;
	}

	// ---------------------------------------------------------
	// 2. Initial Shuttle configuration
	// ---------------------------------------------------------
	Metis::Vector2D posShuttle(-350.0, 0.0);
	Spacecraft* pShuttle = (Spacecraft*)this->getAgentByID(2);

	pShuttle->setPosition(posShuttle.x, posShuttle.y);
	pShuttle->setHeading(90.0);
	pShuttle->setTurretHeading(turretHeading);
	pShuttle->setSpeed(MAX_SPEED_SHUTTLE);
	pShuttle->clean();

	// ---------------------------------------------------------
	// 3. CURRICULUM LEARNING: Radius R and Difficulty Calculation
	// ---------------------------------------------------------
	const int EPISODES_PER_PHASE = 100;
	const int MAX_PHASES = 10;

	int phase = std::min(_currentEpisode / EPISODES_PER_PHASE, MAX_PHASES);
	double difficultyFactor = (double)phase / (double)MAX_PHASES;


	// Spawn Radius
	const double MIN_RADIUS = 110.0;  // Safe zone to avoid instant collision
	const double MAX_RADIUS_START = MIN_RADIUS + 0.0;
	const double MAX_RADIUS_END = MIN_RADIUS + 130.0;

	double currentMaxRadius = MAX_RADIUS_START + (MAX_RADIUS_END - MAX_RADIUS_START) * difficultyFactor;

	// ---------------------------------------------------------
	// 4. Uniform Random Generation
	// ---------------------------------------------------------
	static std::random_device rd;
	static std::mt19937 gen(rd());

	std::uniform_real_distribution<double> dis_radius(MIN_RADIUS, currentMaxRadius);
	std::uniform_real_distribution<double> dis_angle(0.0, 2.0 * M_PI);
	std::uniform_real_distribution<double> dis_noise(-0.2, 0.2); // Small random noise

	Spacecraft* pDron1 = (Spacecraft*)this->getAgentByID(0);
	Spacecraft* pDron2 = (Spacecraft*)this->getAgentByID(1);

	// ---------------------------------------------------------
	// 5. Deployment: Drone 1
	// ---------------------------------------------------------
	double r1 = dis_radius(gen);
	double theta1 = dis_angle(gen);

	double d1_x = posShuttle.x + r1 * cos(theta1);
	double d1_y = posShuttle.y + r1 * sin(theta1);
	pDron1->setPosition(d1_x, d1_y);

	double d1_heading = atan2(posShuttle.x - d1_x, posShuttle.y - d1_y) * 180.0 / M_PI;
	if (d1_heading < 0.0) d1_heading += 360.0;
	pDron1->setHeading(d1_heading);
	pDron1->clean();

	// ---------------------------------------------------------
	// 6. Deployment: Drone 2 (With dynamic pincer angle)
	// ---------------------------------------------------------
	double r2 = dis_radius(gen);

	// Interpolation of the angular offset according to the difficulty:
	// - difficultyFactor = 0.0 -> 180° offset (M_PI / Full pincer)
	// - difficultyFactor = 1.0 -> 15° offset (~0.26 rad / Grouped attack)
	const double INITIAL_OFFSET = M_PI;
	const double FINAL_OFFSET = wxDegToRad(15.0);
	double currentOffset = INITIAL_OFFSET + (FINAL_OFFSET - INITIAL_OFFSET) * difficultyFactor;

	double theta2 = theta1 + currentOffset + dis_noise(gen);

	double d2_x = posShuttle.x + r2 * cos(theta2);
	double d2_y = posShuttle.y + r2 * sin(theta2);
	pDron2->setPosition(d2_x, d2_y);

	double d2_heading = atan2(posShuttle.x - d2_x, posShuttle.y - d2_y) * 180.0 / M_PI;
	if (d2_heading < 0.0) d2_heading += 360.0;
	pDron2->setHeading(d2_heading);
	pDron2->clean();
}