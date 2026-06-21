#include "SpaceEnviroment.h"
#include "Spacecraft.h"

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
	double angleReturn = angleRad * 180.0 / M_PI; // Retorna 0° a 180°

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
		return 0.0; // evitar división por cero

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

float calculateATAangle(Metis::Vector2D &originPos, Metis::Vector2D& originDirection, Metis::Vector2D &targetPos)
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
	const double threshold = 0.261799; // 15 grados (cono de 30 grados = 15º*2)

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
	srand(time(NULL));
}
SpaceEnviroment::~SpaceEnviroment()
{

}

float SpaceEnviroment::getDeltaTime()
{
	return (float) DELTA_TIME;
}

void SpaceEnviroment::reset()
{
	Spacecraft *pShuttle = (Spacecraft *)this->getAgentFromID(0); // get shuttle
	Spacecraft* pTIE1 = (Spacecraft *)this->getAgentFromID(1); // get TIE1
	Spacecraft* pTIE2 = (Spacecraft *)this->getAgentFromID(2); // get TIE2
	Spacecraft* pXWing = (Spacecraft*)this->getAgentFromID(3); // get X-wing

	pShuttle->setState(searched);
	int x = randomInt(-400, -300);
	//int x = 0; test
	int y = randomInt(-100, 100);
	double heading = (double) randomInt(45, 90+45);
	pShuttle->setPosition(x, y);
	pShuttle->setHeading(heading);
	pShuttle->clean();

	/*pShuttle->setPosition(0, 0);  //test
	pShuttle->setHeading(90.0);*/

	Metis::Vector2D posShuttle = pShuttle->getPosition();

	double distMean = (Spacecraft::DISTANCE_MIN_TO_SHUTTLE + Spacecraft::DISTANCE_MAX_TO_SHUTTLE) / 2.0;
	int xOffset = randomInt(-20, 20);
	int yOffset = randomInt(-20, -20);
	heading = (double)randomInt(45, 90 + 45);
	
	pTIE1->setPosition(posShuttle.x + distMean+ xOffset, posShuttle.y+ yOffset);
	pTIE1->setHeading(heading);
	pTIE1->clean();

	xOffset = randomInt(-20, 20);
	yOffset = randomInt(-20, -20);
	heading = (double)randomInt(45, 90 + 45);

	pTIE2->setPosition(posShuttle.x - distMean+ xOffset, posShuttle.y+ yOffset);
	pTIE2->setHeading(heading);
	pTIE2->clean();

	pXWing->setState(attacked);
	yOffset = randomInt(-150, 150);
	pXWing->setPosition(600 - 10, yOffset);
	//pXWing->setPosition(600-10, 100);

	heading = (double)randomInt(-90-15, -90 - 15);
	pXWing->setHeading(heading);
	//pXWing->setHeading(-90.0);
	pXWing->GoHead();
	pXWing->clean();
	//for testing
	//pXWing->setPosition(-600+10, 100); // set on the up left corner
	pXWing->setPosition(150, 0); // set on the up left corner

	//position target where the shuttle should reach to get success
	_posTarget.x = 600;
	_posTarget.y = 0;

	_currenteStep = 0;
	_totalMeanReward = 0.0; // reset the total mean reward

}

Spacecraft* SpaceEnviroment::getTIEfiringLaser()
{
	Spacecraft* pTIEfiringLaser=NULL;

	std::list<IAgent*> agents;
	
	this->getAgents(&agents);

	std::list<IAgent*>::iterator it;
	for (it = agents.begin(); it != agents.end(); it++)
	{
		Spacecraft* pAgent = (Spacecraft*)*it;
		if (pAgent->thereIsLaserFlying())
		{
			pTIEfiringLaser = pAgent;
		}
	}

	
	

	return pTIEfiringLaser;
}
void SpaceEnviroment::getState(Metis::State* pState)
{

	TSPACEENVIROMENT spaceEnv;
	
	std::list<Metis::IAgent*> agents;
	std::list<Metis::IAgent*>::iterator it;

	Metis::Vector2D vVelocityNorm;
	Metis::Vector2D posShuttle;

	Spacecraft* pXWing = (Spacecraft*)getAgentFromID(3);
	Metis::Vector2D XWing_pos = pXWing->getPosition();

	this->getAgents(&agents);
	int i = 0;
	for (it = agents.begin(); it != agents.end(); it++)
	{
		Spacecraft* pAgent = (Spacecraft*)*it;
		int id = pAgent->getID();
		if (id == X_WING_ID)
		{
			continue;
		}

		TOBS_TIE obsCraft;
		
		Metis::Vector2D pos = pAgent->getPosition();
		Metis::Vector2D veloc = pAgent->getVelocity();
		double speed = pAgent->getSpeed();

		//distance the x-wing
		Metis::Vector2D r = XWing_pos - pos;
		double distanceToXWing = r.magnitude();
		obsCraft.X_WingDetected = 0;
		if (distanceToXWing < RANGE_SENSOR)
		{
			obsCraft.X_WingDetected = 1;
		}

		obsCraft.heading = pAgent->getHeading();
		obsCraft.id = id;
		// position relative to the target when SHUTTLE
		if (id == 0)
		{
			posShuttle = pos;
			obsCraft.x = pos.x - _posTarget.x;
			obsCraft.y = pos.y - _posTarget.y;
		}
		else
		{
			// X-wing position relative to the shuttle (for effency to learn)
			obsCraft.x = pos.x - posShuttle.x;
			obsCraft.y = pos.y - posShuttle.y;
		}

		obsCraft.vx = veloc.x;
		obsCraft.vy = veloc.y;
		obsCraft.speed = speed;
		

		if (pAgent->getID() == 0)  // Shuttle
		{
			posShuttle = pAgent->getPosition();
			Metis::Vector2D vVelocity = pAgent->getVelocity();
			vVelocityNorm = vVelocity.Normalize();

			obsCraft.holoDecoy_fired = pAgent->WasFireHoloDecoy();
			obsCraft.holoDecoyOnAir = pAgent->ThereIsHoloDecoyFlying();
			

		}
		if (pAgent->getID() == 1)  // X-wing 1
		{
			Metis::Vector2D xWingPos = pAgent->getPosition();

			//calculate theh ideal position of scorting  (in front of the shuttle)
			double distMean = (Spacecraft::DISTANCE_MIN_TO_SHUTTLE + Spacecraft::DISTANCE_MAX_TO_SHUTTLE) / 2.0;
			Metis::Vector2D idealPosScort; // ideal position for the x-wing1 to scort the frigate
			idealPosScort.x = posShuttle.x + vVelocityNorm.x * distMean;
			idealPosScort.y = posShuttle.y + vVelocityNorm.y * distMean;
			Metis::Vector2D idealPosRelativ;
			idealPosRelativ = idealPosScort - xWingPos; // to draw in the screen, vector referencing about the position of the X-wing 1
			pAgent->_idealScortPos = idealPosRelativ;

			obsCraft.Laser_fired = pAgent->wasLaserFired();
			obsCraft.Laser_firing = pAgent->thereIsLaserFlying();

			// calculate the relative position of the x-wing in the formation
			spaceEnv._scortRelativPositionX_TIE1 = idealPosRelativ; // copy to the enviroment the ideal point of scort
		}
		if (pAgent->getID() == 2)  // X-wing 2
		{
			Metis::Vector2D xWingPos = pAgent->getPosition();
			//calculate theh ideal position of scorting (behind)
						//calculate theh ideal position of scorting  (in front of the shuttle)
			double distMean = (Spacecraft::DISTANCE_MIN_TO_SHUTTLE + Spacecraft::DISTANCE_MAX_TO_SHUTTLE) / 2.0;
			Metis::Vector2D idealPosScort; // ideal position for the x-wing1 to scort the frigate
			idealPosScort.x = posShuttle.x - vVelocityNorm.x * distMean;
			idealPosScort.y = posShuttle.y - vVelocityNorm.y * distMean;
			Metis::Vector2D idealPosRelativ;
			idealPosRelativ = idealPosScort - xWingPos; // to draw in the screen
			pAgent->_idealScortPos = idealPosRelativ;
			obsCraft.Laser_fired = pAgent->wasLaserFired();
			obsCraft.Laser_firing = pAgent->thereIsLaserFlying();

			// calculate the relative position of the x-wing in the formation
			spaceEnv._scortRelativPositionX_TIE2 = idealPosRelativ; // copy to the enviroment the ideal point of scort
		}

		spaceEnv._escortTIEs[i] = obsCraft; // added to the enviroment
		i++;
	}

	spaceEnv.currentStep = this->_currenteStep;

	TOBS_XWING obsXWing;

	Metis::Vector2D pos = pXWing->getPosition();
	Metis::Vector2D veloc = pXWing->getVelocity();

	obsXWing.heading = pXWing->getHeading();
	obsXWing.id = 3;
	obsXWing.x = pos.x - posShuttle.x;
	obsXWing.y = pos.y - posShuttle.y;
	obsXWing.vx = veloc.x;
	obsXWing.vy = veloc.y;
	obsXWing.Missil_fired = pXWing->wasMissilFired();
	obsXWing.Missil_flying = pXWing->thereIsMissileFlying();
	if (obsXWing.Missil_flying)
	{
		Vector2D missilePos = pXWing->getMissile()->GetPosition();
		obsXWing.missil_x = missilePos.x;
		obsXWing.missil_y = missilePos.y;
	}

	spaceEnv._X_Wing_figther = obsXWing;

	pState->copyState(&spaceEnv, sizeof(TSPACEENVIROMENT));
}
void SpaceEnviroment::serizalizeState(void* state, std::vector<float>* stateVector)
{
	TSPACEENVIROMENT *spaceEnv;
	Metis::State* pState = (Metis::State*)state;


	const float WINDOW_WIDTH = 1200.0;
	const float WINDOW_HEIGHT = 600.0;

	spaceEnv = (TSPACEENVIROMENT*)pState->getUserState();

	std::list<Metis::IAgent*> agents;
	std::list<Metis::IAgent*>::iterator it;

	this->getAgents(&agents);

	for (it = agents.begin(); it != agents.end(); it++)
	{
		Spacecraft* pImperialAgent = (Spacecraft *)*it;
				
		int id = pImperialAgent->getID();
		if (id <= 2) // id 0: shuttle, 1: TIE1  2: TIE 2
		{
			double heading = pImperialAgent->getHeading();
			Metis::Vector2D pos = pImperialAgent->getPosition();
			Metis::Vector2D veloc = pImperialAgent->getVelocity();
			char bwasLaserFired = pImperialAgent->wasLaserFired();
			char bthereIsLaserFlying = pImperialAgent->thereIsLaserFlying();
			char chaff_fired = pImperialAgent->WasFireHoloDecoy();
			char chaffOnAir = pImperialAgent->ThereIsHoloDecoyFlying();

			stateVector->push_back(id); // use as a rol
			float valueNorm;
			valueNorm = pos.x / WINDOW_WIDTH;
			stateVector->push_back(valueNorm);
			valueNorm = pos.y / WINDOW_HEIGHT;
			stateVector->push_back(valueNorm);

			valueNorm = veloc.x / MAX_SPEED_FIGHTER_TIE;
			stateVector->push_back(valueNorm);
			valueNorm = veloc.y / MAX_SPEED_FIGHTER_TIE;
			stateVector->push_back(valueNorm);

			valueNorm = pImperialAgent->getSpeed();
			valueNorm = valueNorm / MAX_SPEED_FIGHTER_TIE;
			stateVector->push_back(valueNorm);

			valueNorm = heading / 360.0;
			stateVector->push_back(valueNorm);

			stateVector->push_back(chaff_fired);
			stateVector->push_back(chaffOnAir);

			stateVector->push_back(bwasLaserFired);
			stateVector->push_back(bthereIsLaserFlying);

			stateVector->push_back(spaceEnv->_escortTIEs[id].X_WingDetected);
		}

	}

	Spacecraft *pX_TIE1 = (Spacecraft*)this->getAgentFromID(1);
	Spacecraft* pX_TIE2 = (Spacecraft*)this->getAgentFromID(2);
	// the relative position of formation of the scorts X-Wing

	// add to the input vector for the mult-head net the values of the relative scort position of the x-wing
	float valueNorm = spaceEnv->_scortRelativPositionX_TIE1.x / WINDOW_WIDTH;
	stateVector->push_back(valueNorm);
	valueNorm = spaceEnv->_scortRelativPositionX_TIE1.y / WINDOW_HEIGHT;
	stateVector->push_back(valueNorm);

	valueNorm = spaceEnv->_scortRelativPositionX_TIE2.x / WINDOW_WIDTH;
	stateVector->push_back(valueNorm);
	valueNorm = spaceEnv->_scortRelativPositionX_TIE2.y / WINDOW_HEIGHT;
	stateVector->push_back(valueNorm);

	// filled the data of the X-Wing
	Spacecraft* pXWing = (Spacecraft*)this->getAgentFromID(3);
	double headingXWing = pXWing->getHeading();
	Metis::Vector2D posXWing = pXWing->getPosition();
	Metis::Vector2D velocXWing = pXWing->getVelocity();
	char bwasMissilFired = pXWing->wasMissilFired();
	char bthereIsMissileFlying = pXWing->thereIsMissileFlying();
		
	valueNorm = posXWing.x / WINDOW_WIDTH;
	stateVector->push_back(valueNorm);
	valueNorm = posXWing.y / WINDOW_HEIGHT;
	stateVector->push_back(valueNorm);
	valueNorm = velocXWing.x / MAX_SPEED_FIGHTER_TIE;
	stateVector->push_back(valueNorm);
	valueNorm = velocXWing.y / MAX_SPEED_FIGHTER_TIE;
	stateVector->push_back(valueNorm);
	valueNorm = headingXWing / (float) 360.0;
	stateVector->push_back(valueNorm);
	stateVector->push_back(bwasMissilFired);
	stateVector->push_back(bthereIsMissileFlying);
	if (bthereIsMissileFlying)
	{
		Vector2D missilePos = pXWing->getMissile()->GetPosition();
		valueNorm = missilePos.x / WINDOW_WIDTH;
		stateVector->push_back(valueNorm);
		valueNorm = missilePos.y / WINDOW_HEIGHT;
		stateVector->push_back(valueNorm);
	}
	else
	{
		stateVector->push_back(0.0);
		stateVector->push_back(0.0);
	}

	valueNorm = spaceEnv->currentStep / MAX_STEPS;
	stateVector->push_back(valueNorm);

	int tmpInputsConvoy = CONVOY_INPUTS;
	assert(CONVOY_INPUTS == stateVector->size());
	
}
void SpaceEnviroment::applyAction(Metis::IAgent* pAgent, int actionId)
{
	Spacecraft* pCraft = (Spacecraft*)pAgent;

	if (pCraft->getState() == destroyed) //if destroyed no action to be applied
	{
		return;
	}

	switch (actionId)
	{
		case ACTIONS::LEFT:
		{
			pCraft->TurnLeft();
			break;
		}
		case ACTIONS::AHEAD:
		{
			pCraft->GoHead();
			break;
		}
		case ACTIONS::RIGHT:
		{
			pCraft->TurnRight();
			break;
		}
		case ACTIONS::STOP:
		{
			pCraft->Stop();
			break;
		}
		case ACTIONS::LAUNCH_DECOY:
		{
			pCraft->LaunchHoloDecoy();
			break;
		}
		case ACTIONS::FIRE_LASER:
		{
			pCraft->FireLaser();
			break;
		}
		case ACTIONS::FIRE_MISSILE:
		{
			pCraft->fireMissile();
			break;
		}
			
	}
}
bool SpaceEnviroment::isValidSituationToUseLaser_TIE(int TIE_ID, TSPACEENVIROMENT* pSpaceEnv)
{
	bool bisValidSituationToUseLaser_TIE=false;

	const TOBS_TIE &tieOBS = pSpaceEnv->_escortTIEs[TIE_ID];
	

	Spacecraft* pShuttle = (Spacecraft*)getAgentFromID(SHUTTLE_ID);
	Spacecraft* paTIE = (Spacecraft*)getAgentFromID(TIE_ID);

	Metis::Vector2D posShuttle = pShuttle->getPosition();
	Metis::Vector2D posTIE = paTIE->getPosition();

	Metis::Vector2D r = posShuttle - posTIE;

	
	
	if (tieOBS.X_WingDetected)
	{
		bisValidSituationToUseLaser_TIE = true;
	}
	if (pSpaceEnv->_X_Wing_figther.Missil_flying) // if a enemy missile towards the shuttle
	{

		Spacecraft* pXWing = (Spacecraft*)getAgentFromID(TIE_ID);

		Metis::Vector2D missilePos;
		if (pXWing->getMissile() != NULL)
		{
			Metis::Vector2D missilePos = pXWing->getMissile()->GetPosition();

			Metis::Vector2D rMissile = missilePos - posShuttle;

			double distanceMissile = rMissile.magnitude();

			if (distanceMissile < Spacecraft::DISTANCE_MAX_TO_SHUTTLE)
			{
				bisValidSituationToUseLaser_TIE = true;
			}
			else
			{
				int x = 0;
			}
		}
		else
		{
			int x = 0;
		}
		
	}
	


	return bisValidSituationToUseLaser_TIE;
}
bool SpaceEnviroment::isShuttleThreat(TSPACEENVIROMENT* pSpaceEnv)
{
	bool bIsShuttleThreat = false;

	Spacecraft* pShuttle = (Spacecraft*)getAgentFromID(SHUTTLE_ID);
	Spacecraft* pXwing = (Spacecraft*)getAgentFromID(X_WING_ID);
	Metis::Vector2D shuttlePos = pShuttle->getPosition();
	
	Missile* pXWingMissile = pXwing->getMissile();
	if (pXWingMissile != NULL)
	{
		Metis::Vector2D missilePos = pXWingMissile->GetPosition();

		Metis::Vector2D r = missilePos - shuttlePos;
		double maxDistHoloDecoy = RANGE_HOLO_DECOY * 2.0;
		if (r.magnitude() < maxDistHoloDecoy)
		{
			bIsShuttleThreat = true;
		}
	}


	return bIsShuttleThreat;
}
float SpaceEnviroment::calculate_rewardShuttle(TSPACEENVIROMENT* pSpaceEnv)
{
	const double WEIGHT_ALIGNMENT = 3.0;
	const double WEIGHT_DISTANCE = 1.0;
	const double WEIGHT_SPEED = 2.0;
	const double WEIGHT_HOLODECOY = 4.0;
	const double WEIGHT_ATA_BONUS = 2.0;

	// Divisor FIJO
	double current_total_weight = WEIGHT_ALIGNMENT + WEIGHT_DISTANCE + WEIGHT_SPEED + WEIGHT_ATA_BONUS + WEIGHT_HOLODECOY;

	float r_shuttle = 0.0f;

	// 1. VELOCIDAD
	float speed = pSpaceEnv->_escortTIEs[0].speed;
	r_shuttle += std::clamp(speed / 10.0, 0.0, 1.0) * WEIGHT_SPEED;

	// 2. ALINEACIÓN (con clamp garantizado)
	double r_alignment = computeAlignmentRewardRelativ(pSpaceEnv->_escortTIEs[0].x,      // Posición X del Shuttle
		pSpaceEnv->_escortTIEs[0].y,      // Posición Y del Shuttle
		pSpaceEnv->_escortTIEs[0].heading,// Heading actual del Shuttle
		_posTarget.x,                      // Posición X del objetivo
		_posTarget.y);
	r_alignment = std::clamp(r_alignment, -1.0, 1.0);
	r_shuttle += r_alignment * WEIGHT_ALIGNMENT;

	// 3. DISTANCIA
	Metis::Vector2D relativPosToTarget(pSpaceEnv->_escortTIEs[0].x, pSpaceEnv->_escortTIEs[0].y);
	double distToTarget = relativPosToTarget.magnitude();
	r_shuttle += getDegreeAproxDist_01(distToTarget, SpaceEnviroment::_maxSIZE_X * 2) * WEIGHT_DISTANCE;

	// 4. BONUS ATA (SIEMPRE se suma en el divisor)
	Spacecraft* pShuttle = (Spacecraft*)getAgentFromID(0);
	double angleATAWithMerchant = pShuttle->getATAangle(_posTarget);
	if (speed > 0)
	{
		double bonus = std::max(0.0, 1.0 - (std::abs(angleATAWithMerchant) / 15.0));
		r_shuttle += bonus * WEIGHT_ATA_BONUS;
	}
	else
	{
		r_shuttle += 0.0 * WEIGHT_ATA_BONUS;  // ← Mantener el peso en el divisor
	}

	double r_decoy = 0.0;
	// 5. HOLODECOY (Dinámico pero sin afectar divisor)
	if (pSpaceEnv->_X_Wing_figther.Missil_flying)
	{
		Metis::Vector2D posMissil(pSpaceEnv->_X_Wing_figther.missil_x, pSpaceEnv->_X_Wing_figther.missil_y);
		double missilDistance = (posMissil - pShuttle->getPosition()).magnitude();
		if (missilDistance < RANGE_HOLO_DECOY)
		{
			if (pSpaceEnv->_escortTIEs[0].holoDecoyOnAir)
				r_decoy += 0.2 * WEIGHT_HOLODECOY;
			else if (pSpaceEnv->_escortTIEs[0].holoDecoy_fired)
			{
				r_decoy += 1.0 * WEIGHT_HOLODECOY;
				_decoyWindowCounter = 0; // reset the window
			}
			else
			{
				_decoyWindowCounter++;
				if (_decoyWindowCounter > 4)
				{
					r_decoy -= 1.0 * WEIGHT_HOLODECOY;
				}
			}
				
		}
		else
		{
			if (pSpaceEnv->_escortTIEs[0].holoDecoy_fired)
			{
				r_decoy -= 0.5 * WEIGHT_HOLODECOY;
			}
			else
			{
				if (pSpaceEnv->_escortTIEs[0].holoDecoyOnAir)
				{
					r_decoy -= 0.1 * WEIGHT_HOLODECOY;
				}
			}
		}
	}
	else
	{
		_decoyWindowCounter = 0;
		if (pSpaceEnv->_escortTIEs[0].holoDecoy_fired)
		{
			r_decoy -= 1.0 * WEIGHT_HOLODECOY;
		}
		else
		{
			if (pSpaceEnv->_escortTIEs[0].holoDecoyOnAir)
			{
				r_decoy -= 0.5 * WEIGHT_HOLODECOY;
			}
		}
	}

	r_shuttle += r_decoy;
	// NO linear time penalty - use episodic reward
	// If you want to incentivize efficiency, use:
	// double time_bonus = std::max(0.0, 1.0 - (currentStep / (double)MAX_STEPS * 0.5));
	// r_shuttle += time_bonus * 0.1;  // Pequeño bonus, no penalización

	// 6. NORMALIZACIÓN FINAL
	float r_shuttleNorm = (float)(r_shuttle / current_total_weight);
	r_shuttleNorm = std::clamp(r_shuttleNorm, -1.0f, 1.0f);

	_minmaxReward.currentShuttle_R = r_shuttleNorm;
	_minmaxReward.minShuttle = fmin(_minmaxReward.minShuttle, r_shuttleNorm);
	_minmaxReward.maxShuttle = fmax(_minmaxReward.maxShuttle, r_shuttleNorm);
	_minmaxReward.ShuttleRewardsList.push_back(r_shuttleNorm);

	return r_shuttleNorm;
}

float SpaceEnviroment::calculate_reward_Laser(TSPACEENVIROMENT* pSpaceEnv, Spacecraft* pTIE, TOBS_TIE& objTIE)
{
	double r_laser = 0.0;
	bool targetInRange = false;
	bool isTargetMissile = false;

	Metis::Vector2D tiePos = pTIE->getPosition();
	double distanceToTarget = 0.0;
	Spacecraft* pXWing = (Spacecraft*)getAgentFromID(3);

	
	// 1. OBJECTIVE EVALUATION (Missile > Fighter)
	if (pSpaceEnv->_X_Wing_figther.Missil_flying)
	{
		Metis::Vector2D posMissil(pSpaceEnv->_X_Wing_figther.missil_x, pSpaceEnv->_X_Wing_figther.missil_y);
		distanceToTarget = (posMissil - tiePos).magnitude();

		if (distanceToTarget < RANGE_SENSOR)
		{
			targetInRange = true;
			isTargetMissile = true;
		}
	}

	// Only look for the X-Wing if there is no missile threatening
	if (!targetInRange && objTIE.X_WingDetected)
	{
		Metis::Vector2D posXWing = pXWing->getPosition();
		distanceToTarget = (posXWing - tiePos).magnitude();

		if (distanceToTarget < RANGE_SENSOR)
		{
			targetInRange = true;
			isTargetMissile = false;
		}
	}

	
	// 2. REWARD CALCULATION
	if (targetInRange)
	{
		double baseReward = 0.0;

		if (objTIE.Laser_fired)
		{
			_laserWindowCounter = 0; // RESET
			if (!objTIE.Laser_firing)
			{
				baseReward = 1.3; // Large reward for the timely click
				
			}
			else
			{
				baseReward = -0.01; // if there is bullet on air, there is no need to fire more.
			}
			
		}
		else if (objTIE.Laser_firing)
		{
			baseReward = 0.01;  // Small reward for keeping the laser on the target
			_laserWindowCounter = 0; // RESET
		}
		else
		{
			// THE ENEMY IS IN SIGHT AND IS NOT FIRING!
			_laserWindowCounter++;

			if (_laserWindowCounter > MAX_WINDOW_TO_FIRE_LASER) // Ventana de gracia superada
			{
				baseReward = -0.8; // Harsh punishment for ignoring the threat
			}
		}

		// Apply the priority multiplier
		if (isTargetMissile) {
			r_laser += (baseReward * 2.0); // +2.6 si acierta, -1.6 si lo ignora
		}
		else {
			r_laser += baseReward;
		}
	}
	else
	{
		// 3. PEACETIME(No threats)
		_laserWindowCounter = 0; // Only reset the counter when it is safe

		// Penalize BOTH the initial shot and holding the trigger down over empty space
		if (objTIE.Laser_fired || objTIE.Laser_firing)
		{
			r_laser = -1.0;
		}
	}
	
	r_laser = std::clamp((double)r_laser, -1.0, 1.0);

	return r_laser;
}
float SpaceEnviroment::calculate_reward_X_TIE1(TSPACEENVIROMENT* pSpaceEnv)
{
	const double WEIGHT_ALIGNMENT = 2.0;
	const double WEIGHT_INSIDECONE = 2.0;
	const double WEIGHT_DISTANCE = 2.0;
	const double WEIGHT_COVER = 1.0;
	const double WEIGHT_LASER = 3.0;

	// FIXED divisor (always includes all weights)
	double current_total_weight = WEIGHT_ALIGNMENT + WEIGHT_INSIDECONE + WEIGHT_DISTANCE + WEIGHT_COVER + WEIGHT_LASER;

	float r_TIE1 = 0.0;
	const double ANGLE_CONE = 30.0;

	TOBS_TIE& shuttle = pSpaceEnv->_escortTIEs[0];
	TOBS_TIE& obsTIE1 = pSpaceEnv->_escortTIEs[1];
	Spacecraft* pShuttle = (Spacecraft*)getAgentFromID(0);
	Spacecraft* pTIE1 = (Spacecraft*)getAgentFromID(1);

	// 1. ALIGNMENT 
	Metis::Vector2D vDir(obsTIE1.vx, obsTIE1.vy);
	Metis::Vector2D normalizeRelativScort = pSpaceEnv->_scortRelativPositionX_TIE1.Normalize();
	vDir = vDir.Normalize();
	double diffAngleToScortPoint = calculateHeadingTowardScortPosition(vDir, normalizeRelativScort);
	double R_heading = cos(wxDegToRad(diffAngleToScortPoint));
	r_TIE1 += R_heading * WEIGHT_ALIGNMENT;

	// 2. ESCORT ZONE
	double angleATAdeg = pShuttle->getATAangle(pTIE1);
	double distanceToTargetPosScort = pSpaceEnv->_scortRelativPositionX_TIE1.magnitude();

	if (angleATAdeg >= -ANGLE_CONE && angleATAdeg <= ANGLE_CONE)
	{
		if (distanceToTargetPosScort < MIN_REACH_SCORTPOINT)
		{
			r_TIE1 += 1.0 * WEIGHT_INSIDECONE;
			r_TIE1 += 1.0 * WEIGHT_DISTANCE;

			// Formation bonus as a separate component (but without inflation)
			double diff = obsTIE1.heading - shuttle.heading;
			double normalizedDiff = normalizeAngleDeg3(diff);
			if (normalizedDiff > 180.0) normalizedDiff -= 360.0;
			double formation_bonus = std::max(0.0, cos(wxDegToRad(normalizedDiff)));
			r_TIE1 += formation_bonus * 0.3;  // Small bonus, not inflated
		}
		else
		{
			double insideConeDegree = getConeMembership(angleATAdeg, ANGLE_CONE);
			r_TIE1 += insideConeDegree * WEIGHT_INSIDECONE;
			double rPointStort = getDegreeAproxDist(distanceToTargetPosScort, Spacecraft::DISTANCE_MAX_TO_SHUTTLE);
			r_TIE1 += rPointStort * WEIGHT_DISTANCE;
		}
	}
	else
	{
		double penalty_r = computePenalty(distanceToTargetPosScort, SpaceEnviroment::_maxSIZE_X);
		r_TIE1 -= penalty_r * WEIGHT_DISTANCE;
	}

	// 3. COVERAGE (PROGRESSIVE, not binary)
	double coverage_degree  = getDegreeAproxDist(distanceToTargetPosScort, SpaceEnviroment::_maxSIZE_X * 2);
	coverage_degree = std::clamp(coverage_degree, 0.0, 1.0);
	double cover_reward = (coverage_degree * 2.0) - 1.0;  // -1.0 a +1.0
	r_TIE1 += cover_reward * WEIGHT_COVER;
	
	bool agentFiredLaser = obsTIE1.Laser_fired || obsTIE1.Laser_firing;
	// 4. LASER (Without affecting divisor)
	if (isValidSituationToUseLaser_TIE(TIE_1_ID, pSpaceEnv) || agentFiredLaser)
	{
		float r_laser = calculate_reward_Laser(pSpaceEnv, pTIE1, obsTIE1);
		r_TIE1 += r_laser * WEIGHT_LASER;
	}
	
	// 5. NORMALIZATION
	r_TIE1 = r_TIE1 / current_total_weight;
	r_TIE1 = std::clamp((double)r_TIE1, -1.0, 1.0);

	_minmaxReward.currentTIE_1_R = r_TIE1;
	_minmaxReward.minTIE_1 = fmin(_minmaxReward.minTIE_1, r_TIE1);
	_minmaxReward.maxTIE_1 = fmax(_minmaxReward.maxTIE_1, r_TIE1);
	_minmaxReward.TIE_1_RewardsList.push_back(r_TIE1);

	return r_TIE1;
}
float SpaceEnviroment::calculate_reward_X_TIE2(TSPACEENVIROMENT* pSpaceEnv)
{
	
	// WEIGHT DEFINITION (No dynamic divisor changes)
	const double WEIGHT_ALIGNMENT = 2.0;
	const double WEIGHT_INSIDECONE = 2.0;  // Cono trasero [150, 210]
	const double WEIGHT_DISTANCE = 2.0;
	const double WEIGHT_COVER = 1.0;
	const double WEIGHT_LASER = 3.0;

	// FIXED DIVISOR (always includes ALL weights)
	const double TOTAL_WEIGHT = WEIGHT_ALIGNMENT + WEIGHT_INSIDECONE + WEIGHT_DISTANCE + WEIGHT_COVER + WEIGHT_LASER;

	float r_TIE2 = 0.0f;
	const double ANGLE_CONE = 30.0;
	const double BACK_CONE_CENTER = 180.0;
	const double MIN_ANGLE_CONE = BACK_CONE_CENTER - ANGLE_CONE;  // 150
	const double MAX_ANGLE_CONE = BACK_CONE_CENTER + ANGLE_CONE;  // 210

	
	// GET REFERENCES TO ACTORS
	TOBS_TIE& shuttle = pSpaceEnv->_escortTIEs[0];
	TOBS_TIE& obsTIE2 = pSpaceEnv->_escortTIEs[2];
	Spacecraft* pShuttle = (Spacecraft*)getAgentFromID(0);
	Spacecraft* pTIE2 = (Spacecraft*)getAgentFromID(2);

	if (pShuttle == NULL || pTIE2 == NULL)
	{
		fprintf(stderr, "ERROR: Null pointer in calculate_reward_X_TIE2\n");
		return 0.0f;
	}

	
	// 1. ALIGNMENT REWARD (Heading towards the rear escort point)
	Metis::Vector2D vDir(obsTIE2.vx, obsTIE2.vy);
	Metis::Vector2D normalizeRelativScort = pSpaceEnv->_scortRelativPositionX_TIE2.Normalize();

	vDir = vDir.Normalize();
	double diffAngleToScortPoint = calculateHeadingTowardScortPosition(vDir, normalizeRelativScort);
	double R_heading = std::clamp(cos(wxDegToRad(diffAngleToScortPoint)), -1.0, 1.0);

	r_TIE2 += R_heading * WEIGHT_ALIGNMENT;

	
	// 2. ESCORT POSITION REWARD (Rear cone + Distance)
	double angleATAdeg2 = pShuttle->getATAangle(pTIE2);
	double angleATAnorm = normalizeAngleDeg2(angleATAdeg2);
	double distanceToTargetPosScort = pSpaceEnv->_scortRelativPositionX_TIE2.magnitude();

	bool isInBackCone = (angleATAnorm >= MIN_ANGLE_CONE && angleATAnorm <= MAX_ANGLE_CONE);

	if (isInBackCone)
	{
		// INSIDE THE REAR CONE
		if (distanceToTargetPosScort < MIN_REACH_SCORTPOINT)
		{
			// GOAL REACHED: Reward maximum cone and distance
			r_TIE2 += 1.0 * WEIGHT_INSIDECONE;
			r_TIE2 += 1.0 * WEIGHT_DISTANCE;

			// FORMATION BONUS: Maintain same heading as shuttle (explicit, small)
			double diff = obsTIE2.heading - shuttle.heading;
			double normalizedDiff = normalizeAngleDeg3(diff);
			if (normalizedDiff > 180.0) normalizedDiff -= 360.0;

			double formation_alignment = std::clamp(cos(wxDegToRad(normalizedDiff)), 0.0, 1.0);
			double formation_bonus = formation_alignment * 0.3;  // Pequeño bonus (0-0.3)
			r_TIE2 += formation_bonus;
		}
		else
		{
			// IN TRANSITION: Smooth approach towards the escort point
			double insideBackConeDegree = getBackConeMembership(angleATAnorm, BACK_CONE_CENTER, ANGLE_CONE);
			insideBackConeDegree = std::clamp(insideBackConeDegree, 0.0, 1.0);
			r_TIE2 += insideBackConeDegree * WEIGHT_INSIDECONE;

			double rPointScort = getDegreeAproxDist(distanceToTargetPosScort, Spacecraft::DISTANCE_MAX_TO_SHUTTLE);
			rPointScort = std::clamp(rPointScort, 0.0, 1.0);
			r_TIE2 += rPointScort * WEIGHT_DISTANCE;
		}
	}
	else
	{
		// OUTSIDE THE REAR CONE: Progressive penalty
		double penalty_r = computePenalty(distanceToTargetPosScort, SpaceEnviroment::_maxSIZE_X);
		penalty_r = std::clamp(penalty_r, 0.0, 1.0);
		r_TIE2 -= penalty_r * WEIGHT_DISTANCE;
	}

	
	// 3. COVERAGE REWARD (PROGRESSIVE, not binary)
	// Get coverage percentage (0.0 = not covered, 1.0 = fully covered)
	double coverage_degree = getDegreeAproxDist(distanceToTargetPosScort, SpaceEnviroment::_maxSIZE_X * 2);
	coverage_degree = std::clamp(coverage_degree, 0.0, 1.0);

	// Convert to range [-1, 1]: 
	// coverage_degree=0 -> -1.0 (no coverage is bad)
	// coverage_degree=0.5 -> 0.0 (neutral)
	// coverage_degree=1.0 -> +1.0 (full coverage is good)
	double cover_reward = (coverage_degree * 2.0) - 1.0;
	r_TIE2 += cover_reward * WEIGHT_COVER;

	
	// 4. LASER REWARD 
	bool agentFiredLaser = obsTIE2.Laser_fired || obsTIE2.Laser_firing;
	if (isValidSituationToUseLaser_TIE(TIE_2_ID, pSpaceEnv) || agentFiredLaser)
	{
		float r_laser = calculate_reward_Laser(pSpaceEnv, pTIE2, obsTIE2);
		r_laser = std::clamp((double)r_laser, -1.0, 1.0);
		r_TIE2 += r_laser * WEIGHT_LASER;
	}
	
	
	// 5. FINAL NORMALIZATION
	float r_TIE2_normalized = (float)(r_TIE2 / TOTAL_WEIGHT);
	r_TIE2_normalized = std::clamp(r_TIE2_normalized, -1.0f, 1.0f);

	// 6. TELEMETRY
	_minmaxReward.currentTIE_2_R = r_TIE2_normalized;
	_minmaxReward.minTIE_2 = fmin(_minmaxReward.minTIE_2, r_TIE2_normalized);
	_minmaxReward.maxTIE_2 = fmax(_minmaxReward.maxTIE_2, r_TIE2_normalized);
	_minmaxReward.TIE_2_RewardsList.push_back(r_TIE2_normalized);

	return r_TIE2_normalized;
}
vector<Metis::TMULTIHEAD> SpaceEnviroment::calculateRewards(Metis::State& state, int* pDone)
{
	vector<Metis::TMULTIHEAD> rewards;

	_currenteStep++;

	TSPACEENVIROMENT *pSpaceEnv = (TSPACEENVIROMENT *) state.getUserState();

	Spacecraft* pShuttle = (Spacecraft*)getAgentFromID(SHUTTLE_ID);
	Spacecraft* pTIE1 = (Spacecraft*)getAgentFromID(TIE_1_ID);
	Spacecraft* pTIE2 = (Spacecraft*)getAgentFromID(TIE_2_ID);
	Metis::Vector2D posShuttle = pShuttle->getPosition();


	// the size of the rewards vector will be the same as the number of heads
		
	Metis::TMULTIHEAD r_Shuttle((char *)NAME_SHUTTLE);
	Metis::TMULTIHEAD r_X_TIE_1((char*)NAME_TIE1);
	Metis::TMULTIHEAD r_X_TIE_2((char*)NAME_TIE2);

	r_Shuttle.reward = calculate_rewardShuttle(pSpaceEnv);
	r_X_TIE_1.reward = calculate_reward_X_TIE1(pSpaceEnv);
	r_X_TIE_2.reward = calculate_reward_X_TIE2(pSpaceEnv);
	 
	Metis::Vector2D posMerchantRelativToTarget(pSpaceEnv->_escortTIEs[0].x, pSpaceEnv->_escortTIEs[0].y);
	if (posMerchantRelativToTarget.magnitude() < MIN_DISTANCE_TO_TARGET_POSITION)
	{
		*pDone = -1; // in case shuttle is not scorted by the two x-wings
		//check if the shuttle was covered by the X-Wings
		Spacecraft* pXWing1 = (Spacecraft*)getAgentFromID(1);
		Spacecraft* pXWing2 = (Spacecraft*)getAgentFromID(2);
		r_Shuttle.reward += 1.0;
		
		bool bOnSensorXWing1 = false;
		bool bOnSensorXWing2 = false;
		if ( pXWing1->isOnSensorRange(posShuttle) )
		{
			r_Shuttle.reward += 2.5;
			r_X_TIE_1.reward += 2.5;
			bOnSensorXWing1 = true;
		}
		if ( pXWing2->isOnSensorRange(posShuttle) )
		{
			r_Shuttle.reward += 2.5;
			r_X_TIE_2.reward += 2.5;
			bOnSensorXWing2 = true;
		}
		if (bOnSensorXWing1 && bOnSensorXWing2)
		{
			*pDone = 1; // succes only if the shuttle is scorted by the two X-Wings
		}
	}

	if ( (fabs(posShuttle.x) > SpaceEnviroment::_maxSIZE_X) ||
		 (fabs(posShuttle.y) > SpaceEnviroment::_maxSIZE_Y) )
	{
		*pDone = -1; // out of the world
		r_Shuttle.reward -= 5.0;
	}

	Metis::Vector2D tie1Pos = pTIE1->getPosition();
	Metis::Vector2D tie2Pos = pTIE2->getPosition();

	if ( (tie1Pos.x < -SpaceEnviroment::_maxSIZE_X) ||  // no destroyed if beyond of SpaceEnviroment::_maxSIZE_X
		(fabs(tie1Pos.y) > SpaceEnviroment::_maxSIZE_Y))
	{
		*pDone = -1; // reach the tarte
		r_X_TIE_1.reward -= 3.0;
	}
	if ((fabs(tie2Pos.x) > SpaceEnviroment::_maxSIZE_X) ||
		(fabs(tie2Pos.y) > SpaceEnviroment::_maxSIZE_Y))
	{
		*pDone = -1; // reach the tarte
		r_X_TIE_2.reward -= 3.0;
	}

	if (_currenteStep > MAX_STEPS)
	{
		*pDone = -1; // reach the tarte
		r_Shuttle.reward -= 6.0;
		r_X_TIE_1.reward -= 6.0;
		r_X_TIE_2.reward -= 6.0;
	}
	if (pShuttle->getState() == destroyed)
	{
		*pDone = -1;
		r_Shuttle.reward -= 10.0;
		r_X_TIE_1.reward -= 10.0;
		r_X_TIE_2.reward -= 10.0;
	}
	
	_totalMeanReward = (r_Shuttle.reward + r_X_TIE_1.reward + r_X_TIE_2.reward) / (double) 3.0;

	//to keep track of the min and max rewards
	_minmaxReward.currentTotal_R = _totalMeanReward;
	_minmaxReward.minXTotal = fmin(_minmaxReward.minXTotal, _totalMeanReward);
	_minmaxReward.maxXTotal = fmax(_minmaxReward.maxXTotal, _totalMeanReward);
	_minmaxReward.Total_RewardsList.push_back(_totalMeanReward);

	
	rewards.push_back(r_Shuttle);
	rewards.push_back(r_X_TIE_1);
	rewards.push_back(r_X_TIE_2);

	return rewards;
}
