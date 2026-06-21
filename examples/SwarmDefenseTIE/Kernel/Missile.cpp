#include "Missile.h"
#include "Spacecraft.h"
#include "Vector2D.h"
#include "HoloDecoy.h"

#include <math.h>
#include <conio.h>
#include <stdio.h>


int Missile::_countIDS=0;
int Missile::_countsDeletes = 0;
int Missile::_countsMissilesCreated = 0;



double normalizeMissilAngleDeg(double angulo)
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
double degreesToRadians(double degrees) 
{
    return (double) degrees * (M_PI / 180.0f);
}
double radiansToDegrees(double radians) 
{
    return radians * (180.0f / M_PI);
}

Missile::Missile()
{
    _speed = 0;

    _id = Missile::_countIDS++;

    Missile::_countsMissilesCreated++;
}
Missile::Missile(const Missile& aMissil)
{
    _id = Missile::_countIDS++;
    _speed = aMissil._speed;
    _timeInFlight = aMissil._timeInFlight;

    _iState = aMissil._iState;

    _position = aMissil._position;
    _dheadingDeg = aMissil._dheadingDeg;
    assert(_dheadingDeg <= 360);

    _lastDirection = aMissil._lastDirection;
    _TimeToLock = aMissil._TimeToLock;

    _enemyAngleAtLocked = aMissil._enemyAngleAtLocked;

    Missile::_countsMissilesCreated++;

    
}
Missile::Missile(Spacecraft* pJet)
{
    _id = Missile::_countIDS++;
    _speed = SPEED_MISSILE;
    _timeInFlight = TIME_IN_FLIGHT;

    _iState = MissilState::InFlight;

    _pPlayer = pJet;

    _position = _pPlayer->getPosition();
    _dheadingDeg = _pPlayer->getHeading();

    _lastDirection = _dheadingDeg;
    Missile::_countsMissilesCreated++;

    assert(_dheadingDeg <= 360);

//   CODE FOR TESTING    
    /*
    char szLine[500];
    sprintf(szLine, "idmisil:%d  idPlayer:%d\n",_id, pJet->_ID);
    OutputDebugStringA(szLine);
    */
    //_position.x = 100;
    //_position.y = -100;
    //_dheadingDeg = 90+45;
//----------------------------------------------------------------
}

Missile::~Missile()
{
    Missile::_countsMissilesCreated--;
    Missile::_countsDeletes++;

    if (Missile::_countsMissilesCreated > 0)
    {
        int x = 0;
    }
}

bool Missile::WasFire()
{
    double diff;
    diff = TIME_IN_FLIGHT - _timeInFlight;
    if (diff <= METIS_INC_TIME)
    {
        return true;
    }
    else return false;
}

bool Missile::IsOnAir()
{
    return !WasFire();
}
bool Missile::lockTheTarget(Spacecraft* pJet)
{
   
   bool b_blockTheTarget = false;
   double enemyHeading = pJet->getHeading();
    
    
    if (_TimeToLock <= 0)
    {
        if (_iState == InFlight)
        {
            _enemyAngleAtLocked = enemyHeading;
            _iState = Locked;
            b_blockTheTarget = true;
        }
        else
        {

            float diffAngle;
            diffAngle = fabs(enemyHeading - _enemyAngleAtLocked);
            b_blockTheTarget = true;
        }
    }
        
    return b_blockTheTarget;
}

void Missile::GetVelocity(Metis::Vector2D* pVeloc)
{
    double headingRad;
    assert(_dheadingDeg <= 360);

    headingRad = wxDegToRad(_dheadingDeg);
    double vx = _speed * sin(headingRad);
    double vy = _speed * cos(headingRad);

    pVeloc->x = vx / _speed;
    pVeloc->y = vy / _speed;

}

// Return 1 if it is an airplane, 2 if it is a countermeasure
int Missile::GetTargetPosition(Metis::Vector2D* pTargetPos)
{
    int jetState;
    int typeTarget = 0;

    bool lockedTheTarget = false;

    if ((_pPlayer->getTarget() != NULL) && (_pPlayer->getTarget()->ThereIsHoloDecoyFlying()))
    {
        HoloDecoy* pHoloDecoy = _pPlayer->getTarget()->GetHoloDecoyLaunched();
        *pTargetPos = pHoloDecoy->GetPosition();
        typeTarget = 2;
    }
    else
    {
        if (_pPlayer->getTarget() != NULL)
        {
            lockedTheTarget = lockTheTarget(_pPlayer->getTarget());
            if (lockedTheTarget)
            {
                typeTarget = 1;
                *pTargetPos = _pPlayer->getTarget()->getPosition();
            }
            else
            {
                TryLockTarget();
                typeTarget = 0;
            }
        }
    }

    return typeTarget;
}
   

Metis::Vector2D Missile::GetPosition()
{
    return _position;
}
double Missile::GetHeading()
{
    assert(_dheadingDeg <= 360);
    return _dheadingDeg;
}
void Missile::TryLockTarget()
{
    _TimeToLock--;

}

void Missile::SetState(MissilState state)
{
    if (_iState == InFlight)
    {
        if (state == Missile::MissilState::Destroyed)
        {
            _countToDelete = (1 * 4) - 1;  //  4 are the multiples
        }
        if (state == Missile::MissilState::NoFuel)
        {
            _countToDelete = 1;
        }
    }

    _iState = state;
}
bool Missile::CanBeDelete()
{
    if (_countToDelete <= 0)
    {
        return true;
    }
    else return false;
}
void Missile::Update(double time)
{
    Metis::Vector2D targetPos;
    int bTarget;

    if (_iState == Missile::MissilState::Destroyed)
    {
        _countToDelete--;
        return;
    }
    if (_iState == Missile::NoFuel)
    {
        _countToDelete--;
        return;
    }


    bTarget = GetTargetPosition(&targetPos);
    double maxTurnRateRad = 0.34; //20.0 degress
    double direction=0;
    double dheadingRad = 0;


    float dx;
    float dy;
    if (bTarget)
    {
        // Calculate the direction vector to the target
        dx = targetPos.x - _position.x;
        dy = targetPos.y - _position.y;
        float length = sqrt(dx * dx + dy * dy);

        if (length < 18) // simenos a 10 pixel -> da en el blanco
        {
            int x = 0;
        }
//#ifdef  _DEBUG
//        //char szLine[500];
//        //sprintf(szLine, "time:%ld typeTarget:%d distance impact:%f\n", time, bTarget, length);
//        //OutputDebugStringA(szLine);
//#endif //  _DEBUG

        
        //si bTarget es 1 el target es un avion, y no una contramediada
        if ( (bTarget == 1) && (length < 10) ) // simenos a 10 pixel -> da en el blanco
        {
            _iState = MissilState::HitTarget;
            return;
        }

        float desiredDirectionRad = std::atan2(dy, dx); // Angle in radians

        desiredDirectionRad = (M_PI/2.0) - desiredDirectionRad;
                
        double headingTmpDesiere = radiansToDegrees(desiredDirectionRad);
        // Calculate the difference between the desired and current direction
        dheadingRad = degreesToRadians(_dheadingDeg);
        float angleDifferenceRad = desiredDirectionRad - dheadingRad;
        // Normalize the angle to the range [-π, π]
        angleDifferenceRad = std::atan2(std::sin(angleDifferenceRad), std::cos(angleDifferenceRad));

        double angleDiffDeg = radiansToDegrees(angleDifferenceRad);

        // Constrain the angle difference to the maximum turn rate
        if (std::fabs(angleDifferenceRad) > maxTurnRateRad) {
            if (angleDifferenceRad < 0) 
            {
                angleDifferenceRad = -maxTurnRateRad;
            }
            else 
            {
                angleDifferenceRad = maxTurnRateRad;
            }
        }
        else
        {
            int x = 0;
        }

        // Update the missile's direction
        dheadingRad += angleDifferenceRad;

        _lastDirection = dheadingRad;
    }
    else
    {
        dheadingRad = _lastDirection;
    }

    
    _dheadingDeg = radiansToDegrees(dheadingRad);
    if (_dheadingDeg > 360)
    {
        int x = 0;
    }
    _dheadingDeg = normalizeMissilAngleDeg(_dheadingDeg);
    assert(_dheadingDeg <= 360);

    // Update the missile's position based on the new direction
    double vx = _speed * sin(dheadingRad)* time;
    double vy = _speed * cos(dheadingRad)* time;

    _position.x = _position.x + vx;
    _position.y = _position.y + vy;

    _timeInFlight -= time;

    if (_timeInFlight < 0)
    {
        SetState(MissilState::NoFuel);
    }
}

void Missile::Draw(wxDC* pDC,int r, int g, int b)
{
    wxPen pen(wxColour(255, 50, 200), 1);

    pDC->SetPen(pen);

    wxAffineMatrix2D matrixBackup, matrix;
    matrix = pDC->GetTransformMatrix();
    matrixBackup = matrix;

    Metis::Vector2D misilPos = GetPosition();
    int x = misilPos.x;
    int y = misilPos.y;
    double dHeadingDeg = 0;

    dHeadingDeg = GetHeading();

    


    matrix.Translate(x, -y);
    pDC->SetTransformMatrix(matrix);

    matrix.Rotate(wxDegToRad(dHeadingDeg));
    pDC->SetTransformMatrix(matrix);

    // Define the points of the missile shape
    wxPoint missilePoints[3] = {
        wxPoint(0, -17),  // Tip of the missile
        wxPoint(-4, 4),  // Right fin
        wxPoint(4, 4),  // Bottom right
    };

    // Set a pen and brush
    pDC->SetPen(wxPen(wxColour(0, 0, 0), 2)); // Black outline
    pDC->SetBrush(wxBrush(wxColour(255, 50, 200))); // Red fill

    // Draw the missile as a polygon
    pDC->DrawPolygon(3, missilePoints);

    // Optionally, draw the outline of the polygon
    pDC->DrawLines(3, missilePoints);
    //DrawCone(pDC,0,255,0);

    pDC->SetTransformMatrix(matrixBackup);
}
void Missile::CalculateArcPoints(const wxPoint& center, double radius, double angleFOV, wxPoint* p1, wxPoint* p2)
{

    // Define the starting and ending angles based on the heading and FOV
    double angle1 = _dheadingDeg - (angleFOV / 2.0);  // Starting angle
    double angle2 = _dheadingDeg + (angleFOV / 2.0);  // Ending angle

    // Convert angles to radians
    double radians1 = angle1 * M_PI / 180.0;
    double radians2 = angle2 * M_PI / 180.0;

    // Calculate the points on the circle relative to the center
    int x1 = static_cast<int>(radius * std::cos(radians1));
    int y1 = static_cast<int>(radius * std::sin(radians1));

    int x2 = static_cast<int>(radius * std::cos(radians2));
    int y2 = static_cast<int>(radius * std::sin(radians2));

    // Set the points to the calculated coordinates
    p1->x = x1;
    p1->y = y1;

    p2->x = x2;
    p2->y = y2;

}


void Missile::DrawCone(wxDC* pDC, int r, int g, int b)
{
    wxAffineMatrix2D matrixBackup, matrix;

    matrix = pDC->GetTransformMatrix();
    matrixBackup = matrix;

    double rangeSensor = RANGE_MISSILE_SENSOR;
    double angleFOV = FOV_LOCKED;
    wxPoint start1, start2;

    pDC->SetPen(wxPen(wxColour(r, g, b), 1)); // Black outline

    Metis::Vector2D posMissil = GetPosition();
    CalculateArcPoints(wxPoint(posMissil.x, posMissil.y), rangeSensor, angleFOV, &start1, &start2);

    
    double angleDegToRotate = 270 - _dheadingDeg;
    matrix.Rotate(wxDegToRad(angleDegToRotate));
    pDC->SetTransformMatrix(matrix);
    pDC->DrawLine(wxPoint(0, 0), start1);
    pDC->DrawLine(wxPoint(0, 0), start2);

    // Use the brush to paint a rectangle
    pDC->SetBrush(*wxTRANSPARENT_BRUSH);

    // Draw the arc (pie slice) using DrawArc
    pDC->DrawArc(start2, start1, wxPoint(0, -12));

    pDC->SetTransformMatrix(matrixBackup);
}

void Missile::GetDirection(Metis::Vector2D* vDirection)
{
    double headingRad;


    headingRad = wxDegToRad(_dheadingDeg);
    double vx = _speed * sin(headingRad);
    double vy = _speed * cos(headingRad);

    vDirection->x = vx / _speed;
    vDirection->y = vy / _speed;
}
