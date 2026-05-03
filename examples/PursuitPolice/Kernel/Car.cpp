#include "stdafx.h"

#include <assert.h>
#include "METIS-Core.h"
#include "Enviroment.h"
#include "UrbanEnviroment.h"
#include "Car.h"

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

Car::Car()
{
	_dHeadingDeg = 90.0;
	_speed = INTERCEPTOR_SHIP_VELOCITY;
	
	_r = _g = _b = 255;

}
Car::~Car()
{

}


void Car::setIniSpeed(double speedini)
{
	_speedIni = speedini;

	_speed = speedini;
}

void Car::setColor(char r, char g, char b)
{
	_r = r;
	_g = g;
	_b = b;
}

void Car::setPosition(double x, double y)
{
	_position.x = x;
	_position.y = y;

}
Metis::Vector2D& Car::getPosition()
{
	return _position;
}

void Car::getSpeed(Metis::Vector2D* vDirection)
{
	double headingRad;

	double speedTmp = 10;
	assert(_dHeadingDeg <= 360);

	headingRad = wxDegToRad(_dHeadingDeg);
	double vx = _speed * sin(headingRad);
	double vy = _speed * cos(headingRad);

	vDirection->x = vx;
	vDirection->y = vy;


}
void Car::getDirection(Metis::Vector2D* vDirection)
{
	double headingRad;

	double speedTmp = 10;
	assert(_dHeadingDeg <= 360);

	headingRad = wxDegToRad(_dHeadingDeg);
	double vx = speedTmp * sin(headingRad);
	double vy = speedTmp * cos(headingRad);

	vDirection->x = vx / speedTmp;
	vDirection->y = vy / speedTmp;

	assert(fabs(vDirection->x) <= 1.0);
	assert(fabs(vDirection->y) <= 1.0);

}

void Car::setHeading(double headingDeg)
{
	_dHeadingDeg = headingDeg;
}
void Car::setSpeed(double speed)
{
	_speed = speed;
}
double Car::getHeading()
{
	return _dHeadingDeg;
}


void Car::TurnLeft()
{
	_speed = _speedIni;
	_dHeadingDeg -= INC_ANGLE_HEADING;
	_dHeadingDeg = normalizeAngleDeg(_dHeadingDeg);
}
void Car::GoHead()
{
	_speed = _speedIni;
}
void Car::TurnRight()
{
	_speed = _speedIni;
	_dHeadingDeg += INC_ANGLE_HEADING;
	_dHeadingDeg = normalizeAngleDeg(_dHeadingDeg);
}
void Car::Stop()
{
	_speed = 0.0;
}

double Car::CalculateCrossTrackError(double baseHeadingDeg, Metis::Vector2D relativePos)
{
	// 1. Convertir el rumbo base (grados) a un vector unitario de dirección
	// En náutica: 0º es Norte (Y+), 90º es Este (X+)
	double angleRad = wxDegToRad(baseHeadingDeg);
	double ux = sin(angleRad);
	double uy = cos(angleRad);

	// 2. Vector que va desde el origen de la línea hasta el barco
	// En tu caso, usamos la posición relativa del barco
	double px = relativePos.x;
	double py = relativePos.y;

	// 3. Producto Vectorial 2D (Calcula la distancia perpendicular)
	// El signo nos dirá si el barco está a la izquierda o derecha de la línea
	// XTE = (Directriz_x * Distancia_y) - (Directriz_y * Distancia_x)
	double xte = (ux * py) - (uy * px);

	return xte;
}

int Car::getActionProcedural(Metis::State& state)
{
	int iAction = AHEAD;
	TURBANSTATE* pOceanState = (TURBANSTATE*)state._pState;
	double distanceAgent = pOceanState->relativeThiefPos.magnitude();

	// Configuración (Podrían ser constantes o variables de clase)
	const double ZIGZAG_AMPLITUDE = 25.0; // Grados
	const int STRAIGHT_STEPS = 3;       // Cuántos frames ir recto (aprox 1-2 seg)

	double targetHeading = 0.0;
	double currentHeading = getHeading();

	if (distanceAgent < 100.0) // Activación
	{
		const double CORRIDOR_WIDTH = 15.0; // Metros/Píxeles que nos desviamos del eje
		const double APPROACH_ANGLE = 25.0; // Ángulo de ataque para volver a la línea

		// 1. Inicialización de la línea de misión
		if (_zigZagTimer == 0) {
			_baseHeading = getHeading();

			// NUEVO: Guardamos dónde empieza nuestro pasillo imaginario
			_startPos = this->_position; // <-- Función hipotética para obtener tu pos actual

			//_sideGoal = 1; // 1: Ir hacia la derecha de la línea, -1: Ir hacia la izquierda
			// Elegir aleatoriamente entre 1 (Derecha) y -1 (Izquierda)
			// rand() % 2 devuelve 0 o 1
			if (rand() % 2 == 0) {
				_sideGoal = 1;
			}
			else {
				_sideGoal = -1;
			}
		}
		_zigZagTimer++;

		// 2. Calcular error lateral (XTE)
		// Calculamos cuánto nos hemos movido desde que empezó la maniobra
		Metis::Vector2D displacement;
		displacement.x = getPosition().x - _startPos.x;
		displacement.y = getPosition().y - _startPos.y;

		// Ahora sí: calculamos la desviación lateral de nuestro propio movimiento
		double xte = CalculateCrossTrackError(_baseHeading, displacement);

		// Si la DERECHA es NEGATIVA en tu simulador:
		if (_sideGoal == 1 && xte < -CORRIDOR_WIDTH) { // '1' es ir a la derecha
			_sideGoal = -1; // Toca pared derecha (negativa), cambia a izquierda
		}
		else if (_sideGoal == -1 && xte > CORRIDOR_WIDTH) { // '-1' es ir a la izquierda
			_sideGoal = 1;  // Toca pared izquierda (positiva), cambia a derecha
		}

		// 4. Calcular rumbo deseado para cruzar la línea
		targetHeading = _baseHeading + (_sideGoal * APPROACH_ANGLE);

	}
	else 
	{
		// algoritmo para ir al target aqui ya que no hay peligro ya
		_zigZagTimer = 0;
		
		_zigZagTimer = 0; // Resetear para la próxima vez que entre en peligro

		// Calculamos el ángulo hacia el enemigo usando su posición relativa
		// Usamos atan2(x, y) para obtener el ángulo desde el Norte (0º) en sentido horario

		Metis::Vector2D targetPos(700, 0);
		Metis::Vector2D relavePos = targetPos - _position;
		double angleToTargetRad = atan2(relavePos.x, relavePos.y);
		targetHeading = angleToTargetRad * (180.0 / 3.14159265358979323846);

	}

	// --- MOTOR DE CONTROL DE TIMÓN (UNIFICADO) ---
	double headingError = targetHeading - currentHeading;

	// Normalizar error (-180 a 180)
	while (headingError > 180.0)  headingError -= 360.0;
	while (headingError < -180.0) headingError += 360.0;

	// Aplicar acción según el error de rumbo
	if (headingError > 2.0)       iAction = RIGHT;
	else if (headingError < -2.0) iAction = LEFT;
	else                          iAction = AHEAD;

	return iAction;
}

int Car::update(double delta_time)
{

	double vx, vy;



	//_time += (unsigned long)incTime;

	double headingRad;

	headingRad = wxDegToRad(_dHeadingDeg);
	vx = _speed * sin(headingRad) * delta_time;
	vy = _speed * cos(headingRad) * delta_time;

	double newX, newY;
	newX = _position.x + vx;
	newY = _position.y + vy;

	_position.x = newX;
	_position.y = newY;


	return 0;
}
void Car::draw(wxDC* pDC)
{
	// 1. Preparar colores y backup de la matriz (como ya tenías)
	wxPen pen(wxColour(_r, _g, _b));
	wxPen oldPen = pDC->GetPen();
	pDC->SetPen(pen);

	wxAffineMatrix2D matrix = pDC->GetTransformMatrix();
	wxAffineMatrix2D matrixBackup = matrix;
		
	// 2. Aplicar transformación (Posición y Rotación)
	matrix.Translate(_position.x, -_position.y);
	matrix.Rotate(wxDegToRad(_dHeadingDeg));
	pDC->SetTransformMatrix(matrix);

	
	// el polígono 
	wxPoint points[4] = { wxPoint(-4, -8), wxPoint(-4, 8), wxPoint(4, 8), wxPoint(4, -8) };
	pDC->DrawPolygon(4, points);
	
	// -------------------------
	// Indicador de dirección (Opcional, si quieres mantener la línea sobre el sprite)
	wxPen penDir(wxColour(_r, _g, _b), 2);
	pDC->SetPen(penDir);
	pDC->DrawLine(0, -8, 0, -13); // Simplificado relativo al centro

	wxColour colorArrow(255, 200, 200);

	//DrawHeadingArrow(pDC, 0, 0, 0.0, colorArrow, 1.0);


	// Restaurar matriz y puntero
	pDC->SetTransformMatrix(matrixBackup);
	pDC->SetPen(oldPen);

}