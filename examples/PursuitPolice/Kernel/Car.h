#pragma once



#include "METIS-Core.h"
#include "UrbanEnviroment.h"

#define INTERCEPTOR_SHIP_VELOCITY 15.0   // 3 pixel por segundo
#define ENEMY_SHIP_VELOCITY 10.0   // 3 pixel por segundo
#define INC_ANGLE_HEADING 10 // 15 grados de incremento de giro



class Car :public Metis::Agent
{
private:
	int _id;
	Metis::Vector2D _position;
	float _dHeadingDeg;
	float 	_speed;
	float 	_speedIni;
	char _r, _g, _b;

	double _baseHeading;
	int _sideGoal;
	int _waitCounter;
	Metis::Vector2D _startPos;
	
	double CalculateCrossTrackError(double baseHeadingDeg, Metis::Vector2D relativePos);

public:

	int _zigZagTimer = 0;

	Car();
	~Car();
	
	//dibujo
	void draw(wxDC* pDC);

	// Accions
	void TurnLeft();
	void GoHead();
	void TurnRight();
	void Stop();

	//Funciones movimiento
	void setPosition(double x, double y);
	Metis::Vector2D& getPosition();
	void getDirection(Metis::Vector2D* vDirection);
	void getSpeed(Metis::Vector2D* vDirection);
	double getHeading();
	void setHeading(double headingDeg);
	void setSpeed(double speed);
	void setIniSpeed(double speedini);
	double getSpeed();
	void setColor(char r, char g, char b);


	// override from Metis-Core
	virtual int getActionProcedural(Metis::State& state);
	virtual int update(double delta_time);
	virtual bool isValidAction(int iAction) { return true; };

};

