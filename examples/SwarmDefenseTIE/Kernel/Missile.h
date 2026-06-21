
#ifndef __MISSILE__
    #define __MISSILE__

#include "METIS-Core.h"
#include <wx/wx.h>

class Spacecraft;

#define FOV_LOCKED 30 // 30 degress
#define RANGE_MISSILE_SENSOR 400

#define SPEED_MISSILE 60
#define TIME_IN_FLIGHT METIS_INC_TIME*60.0

class Missile
{
protected:

    
    Metis::Vector2D _position;
    
    float _speed;  // pixel por seconds
    float _dheadingDeg; // degreess
    

    double _lastDirection;


    virtual int GetTargetPosition(Metis::Vector2D* pTargetPos);
    void GetVelocity(Metis::Vector2D* pVeloc);
    bool lockTheTarget(Spacecraft* pJet);
    

    double _timeInFlight;

    void DrawCone(wxDC* pDC, int r, int g, int b);
    void CalculateArcPoints(const wxPoint& center, double radius, double angleFOV, wxPoint* p1, wxPoint* p2);

    void TryLockTarget();
    int _TimeToLock;
    float _enemyAngleAtLocked;

    FILE *_fp;
    int _countToDelete = 2;
    
public:

    int _id;
    static int _countIDS;
    static int _countsDeletes;
    static int _countsMissilesCreated;
    // Enum definition inside the class
    enum MissilState {
        InFlight=1,
        Destroyed,
        HitTarget,
        NoFuel,
        Locked
    };
    Spacecraft* _pPlayer;
    int _iState; //1 in flight, -1 to delete


    Missile();
    Missile(Spacecraft*pJet);
    Missile(const Missile& aMissil);
    ~Missile();

    bool WasFire();
    bool IsOnAir();

    Metis::Vector2D GetPosition();
    double GetHeading();

    void SetState(MissilState state);
    bool CanBeDelete();

        
    virtual void Update(double time);

    virtual void Draw(wxDC* pDC,int r,int g,int b);

    double GetSpeed() { return _speed; };
    void GetDirection(Metis::Vector2D* vDirection);

};


#endif
