#include "stdafx.h"

#include <cmath>
#include <algorithm>

#include "Car.h"
#include "UrbanEnviroment.h"

using namespace Metis;
double calculateInterceptionHeading(TURBANSTATE& state, double agentSpeed);

double generateRndValue(double minX, double maxX) 
{
	double escala = (double)rand() / RAND_MAX; // Genera un número entre 0 y 1
	return minX + escala * (maxX - minX);
}


UrbanEnviroment::UrbanEnviroment()
{
	//_numActions = (int) STOP+1;
}
UrbanEnviroment::~UrbanEnviroment()
{

}

void UrbanEnviroment::reset()
{

	Car* pAgentShip = (Car *) getAgentFromID(0);
	Car* pEnemyShip = (Car *) getAgentFromID(1);

	int xAgent;
	int yAgent;
	xAgent = +100;
	//yAgent = -( (WINDOW_HEIGHT/2)-10);
	yAgent = -100;
	xAgent = generateRndValue(300,350);
	yAgent = generateRndValue(-100,+100);
	pAgentShip->setPosition(xAgent, yAgent);

	double angleAgent = generateRndValue(180+45.0,360.0-45.0);
	pAgentShip->setHeading(angleAgent);

	/// reseteaer el enemigo
	int xEnemy;
	int yEnemy;
	//xEnemy = -( (WINDOW_WIDTH/2) - 10);
	xEnemy = -100;
	yEnemy = 0;

	xEnemy = generateRndValue(-200, -100);
	yEnemy = generateRndValue(50, -50);
	
	pEnemyShip->setPosition(xEnemy, yEnemy);
	//double angleEnemy = generateRndValue(-45, +45);

	pEnemyShip->_zigZagTimer = 0;


}


void UrbanEnviroment::getState(Metis::State *pState)
{
	Car* pAgentShip = (Car * )getAgentFromID(0);
	Car* pEnemyShip = (Car*)getAgentFromID(1);

	Vector2D posFriend = pAgentShip->getPosition();
	Vector2D posEnemy = pEnemyShip->getPosition();
	Vector2D relaPos = posEnemy - posFriend;

	Vector2D speedFriend;
	pAgentShip->getSpeed(&speedFriend);

	Metis::Vector2D enemySpeed;
	pEnemyShip->getSpeed(&enemySpeed);
	


	double halfWindow = WINDOW_WIDTH / 2.0;
	Vector2D enemyTarget(halfWindow, 0.0);
	Vector2D r = posEnemy - enemyTarget;

	double distanceEnemytoTarget = r.magnitude();


	TURBANSTATE oceantState;
	int sizeState = sizeof(TURBANSTATE);


	oceantState.policeSpeed = speedFriend;

	oceantState.relativeThiefPos = relaPos;
	oceantState.thiefDistanceToTarget = distanceEnemytoTarget;
	oceantState.thiefSpeed = enemySpeed;

	pState->copyState(&oceantState, sizeof(TURBANSTATE));
	




}
// retorna [0 - 360 grados]
double normalizeAngleDeg2(double angulo)
{
	double resultado = fmod(angulo, 360.0);
	if (resultado < 0) resultado += 360.0;
	return resultado;
}

void UrbanEnviroment::serizalizeState(void* state, std::vector<float>* stateVector)
{
	Metis::State* pState = (Metis::State *)state;
	TURBANSTATE* pOceanState = (TURBANSTATE*)pState->getUserState();

	
	Metis::Vector2D friendSpeed;
	double vFriendSpeedNormaX = pOceanState->policeSpeed.x / (double)INTERCEPTOR_SHIP_VELOCITY;
	stateVector->push_back(vFriendSpeedNormaX);
	double vFriendSpeedNormaY = pOceanState->policeSpeed.y / (double)INTERCEPTOR_SHIP_VELOCITY;
	stateVector->push_back(vFriendSpeedNormaY);


	float value = pOceanState->relativeThiefPos.x / (float)WINDOW_WIDTH;
	stateVector->push_back(value);
	value = pOceanState->relativeThiefPos.y / (float)WINDOW_WIDTH;
	stateVector->push_back(value);





	Metis::Vector2D enemySpeed;
	double vSpeedNormaX = pOceanState->thiefSpeed.x / (double) INTERCEPTOR_SHIP_VELOCITY;
	stateVector->push_back(vSpeedNormaX);
	double vSpeedNormaY = pOceanState->thiefSpeed.y / (double)INTERCEPTOR_SHIP_VELOCITY;
	stateVector->push_back(vSpeedNormaY);


	float distanceEnemyToTarget = pOceanState->thiefDistanceToTarget / (float)WINDOW_WIDTH;
	stateVector->push_back(distanceEnemyToTarget);

	int size = stateVector->size();
	assert(size == NUM_INPUTS);
	
}
void UrbanEnviroment::applyAction(Metis::IAgent *pAgent, int actionId)
{
	Car* pShip = (Car*)pAgent;
	switch (actionId)
	{
		case LEFT:
		{
			pShip->TurnLeft();
			break;
		}
		case AHEAD:
		{
			pShip->GoHead();
			break;
		}
		case RIGHT:
		{
			pShip->TurnRight();
			break;
		}
		case STOP:
		{
			pShip->Stop();
			break;
		}
		default:
		{
			assert(false);
			break;
		}
	}
}
bool UrbanEnviroment::isEpisodeDone()
{
	return false;
}


Metis::Vector2D calculateInterceptionPoint(const TURBANSTATE& state, double agentSpeedFriend, double& outTime)
{
	outTime = -1.0;
	Metis::Vector2D pTarget = state.relativeThiefPos; // Fallback a posición actual
	const double EPSILON = 1e-6;
	const double PI = 3.14159265358979323846;

	// 1. Obtener vector de velocidad del enemigo (Norte=0, Este=90)
	double enemySpeedMag = state.thiefSpeed.magnitude();

	double radEnemy = atan2(state.thiefSpeed.x, state.thiefSpeed.y);
	//double radEnemy = state.headingEnemy * (PI / 180.0);
	double vex = enemySpeedMag * sin(radEnemy);
	double vey = enemySpeedMag * cos(radEnemy);

	// 2. Coordenadas relativas actuales del enemigo (P_e)
	double pex = state.relativeThiefPos.x;
	double pey = state.relativeThiefPos.y;

	// 3. Coeficientes: at^2 + bt + c = 0
	// a = (Vex^2 + Vey^2) - Sa^2  -> Esto es lo mismo que: Ve^2 - Sa^2
	double a = (enemySpeedMag * enemySpeedMag) - (agentSpeedFriend * agentSpeedFriend);
	double b = 2.0 * (pex * vex + pey * vey);
	double c = (pex * pex + pey * pey);

	// 4. Resolución de la ecuación
	if (fabs(a) < EPSILON)
	{
		// CASO LINEAL: Velocidades iguales. La ecuación es bt + c = 0 -> t = -c/b
		if (fabs(b) > EPSILON) {
			double t = -c / b;
			if (t > 0) outTime = t;
		}
	}
	else
	{
		// CASO CUADRÁTICO
		double discriminant = b * b - 4.0 * a * c;

		if (discriminant >= 0)
		{
			double sqrtDisc = sqrt(discriminant);
			double t1 = (-b + sqrtDisc) / (2.0 * a);
			double t2 = (-b - sqrtDisc) / (2.0 * a);

			// Buscamos el tiempo positivo más pequeño
			if (t1 > 0 && t2 > 0) outTime = (t1 < t2) ? t1 : t2;
			else if (t1 > 0)      outTime = t1;
			else if (t2 > 0)      outTime = t2;
		}
	}

	// 5. Cálculo del punto final si encontramos un tiempo válido
	if (outTime > 0)
	{
		pTarget.x = pex + vex * outTime;
		pTarget.y = pey + vey * outTime;

		// Verificación de seguridad (NaN/Inf)
		if (pTarget.x != pTarget.x || pTarget.y != pTarget.y) { // Verificación C++98 para NaN
			outTime = -1.0;
			return state.relativeThiefPos;
		}
	}

	return pTarget;
	
}
double calculatePureInterceptionHeading(TURBANSTATE& state, double agentSpeedFriend)
{
	const double PI = 3.14159265358979323846;

	// Ángulo LOS hacia el enemigo (convenio: Norte=+Y, atan2(x,y))
	double losAngleRad = atan2(state.relativeThiefPos.x, state.relativeThiefPos.y);

	double deg = losAngleRad * (180.0 / PI);
	deg = fmod(deg, 360.0);
	if (deg < 0) deg += 360.0;
	return deg;
	

}
double calculateInterceptionHeading(TURBANSTATE& state, double agentSpeedFriend)
{
	const double PI = 3.14159265358979323846;

	// Ángulo LOS hacia el enemigo (convenio: Norte=+Y, atan2(x,y))
	double losAngleRad = atan2(state.relativeThiefPos.x, state.relativeThiefPos.y);

	// Fallback si el agente está parado
	if (agentSpeedFriend < 1e-6)
	{
		double deg = losAngleRad * (180.0 / PI);
		deg = fmod(deg, 360.0);
		if (deg < 0) deg += 360.0;
		return deg;
	}

	double enemySpeedModule = state.thiefSpeed.magnitude();

	// Si el enemigo está parado, apuntar directo (pure pursuit = interception perfecta)
	if (enemySpeedModule < 1e-6)
	{
		double deg = losAngleRad * (180.0 / PI);
		deg = fmod(deg, 360.0);
		if (deg < 0) deg += 360.0;
		return deg;
	}

	// Ángulo de la velocidad del enemigo (mismo convenio que LOS)
	double enemyVelAngleRad = atan2(state.thiefSpeed.x, state.thiefSpeed.y);

	// Alpha: ángulo entre la dirección del enemigo y la LOS
	double alpha = enemyVelAngleRad - losAngleRad;

	// Ley de los senos: sin(beta) / Ve = sin(alpha) / Va  =>  sin(beta) = (Ve/Va)*sin(alpha)
	double sinBeta = (enemySpeedModule / agentSpeedFriend) * sin(alpha);

	double leadAngleRad = 0.0;
	if (sinBeta >= -1.0 && sinBeta <= 1.0)
	{
		leadAngleRad = asin(sinBeta);
	}
	// else: intercepción imposible, pure pursuit (leadAngleRad = 0)

	// Rumbo objetivo = LOS + ángulo de adelanto
	double targetHeadingRad = losAngleRad + leadAngleRad;
	double targetHeadingDeg = targetHeadingRad * (180.0 / PI);

	// Normalizar a [0, 360)
	targetHeadingDeg = fmod(targetHeadingDeg, 360.0);
	if (targetHeadingDeg < 0) targetHeadingDeg += 360.0;

	return targetHeadingDeg;
}
std::vector<Metis::TMULTIHEAD> UrbanEnviroment::calculateRewards(Metis::State& state, int* pDone)
{
	std::vector<Metis::TMULTIHEAD> r;

	return r;
}
float UrbanEnviroment::calculateReward(Metis::State& state, int* piDone)
{
	float reward = 0.0f;
	const double maxDistanceToTarget = 700.0;
	*piDone = 0;

	const double PI = 3.14159265358979323846;

	TURBANSTATE* pOceanState = (TURBANSTATE*)state.getUserState();
	


	// --- 1. CÁLCULO DEL ERROR DE ALINEACIÓN REAL ---
	double speedAgent = pOceanState->policeSpeed.magnitude();
	double outtime;
	_interceptPoint = calculateInterceptionPoint(*pOceanState, speedAgent, outtime);

	// El rumbo que tiene que tomar el agente es el ángulo hacia ese PUNTO
	double targetHeading = calculateInterceptionHeading(*pOceanState, speedAgent);    /////////  FUNCIONA
	//double targetHeading = calculatePureInterceptionHeading(*pOceanState, speedAgent);
	

	double currentHeading = atan2(pOceanState->policeSpeed.x, pOceanState->policeSpeed.y) * (180.0 / PI);
	
	// normalizar el angulo
	double headingError = fmod(fabs(targetHeading - currentHeading), 360.0);
	if (headingError > 180.0) headingError = 360.0 - headingError;


	float rewardAlignment = 0.0f;
	if (headingError <= 15.0) {
		rewardAlignment = (float)(1.0 - (headingError / 15.0));
	}
	else {
		// Penalización suave hasta 180 grados
		rewardAlignment = (float)(-(headingError - 15.0) / 165.0);
	}

	// Recompensa por distancia
	double distanceToTarget = pOceanState->relativeThiefPos.magnitude();
	// Normalizada de 0.0 a 1.0
	float rewardDistance = (float)(1.0 - (distanceToTarget / maxDistanceToTarget));
	
	// --- 4. CÁLCULO FINAL PESADO ---
	// Damos peso 70% a estar bien orientado y 30% a la distancia absoluta
	//reward = (rewardAlignment * 0.7f) + (rewardDistance * 0.3f);
	reward = rewardAlignment;
	
	if (pOceanState->policeSpeed.magnitude() <= 0.0)
	{
		reward -= 0.5;
		reward = fmax(reward, -1.0);
	}
	
	// --- 5. CONDICIONES TERMINALES (Sparsity) ---
	// Éxito: Interceptación
	if (distanceToTarget < 10.0) 
	{
		*piDone = 1;
		reward += 10.0f;
	}


	Car* pAgentShip = (Car*)getAgentFromID(0);
	Car* pEnemyShip = (Car*)getAgentFromID(1);

	Metis::Vector2D posFriend = pAgentShip->getPosition();
	Metis::Vector2D posEnemy = pEnemyShip->getPosition();

	// Fracaso: El enemigo escapa o nosotros nos salimos
	Metis::Vector2D pos = pOceanState->relativeThiefPos;
	if ( (fabs(posFriend.x) > 350 || fabs(posFriend.y) > 250) || (pOceanState->thiefDistanceToTarget < 10) ||
		 (fabs(posEnemy.x) > 350 || fabs(posEnemy.y) > 250) )
	{
		*piDone = -1;
		reward -= 10.0f;
	}

	return reward;
}
