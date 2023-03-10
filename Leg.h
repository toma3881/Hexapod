#ifndef LEG_H__
#define LEG_H__
#include <Multiservo.h>
#include <Wire.h>
#include <math.h>

// #include "WProgram.h"

#define sqr(x) ((x)*(x))
//#define PI 3.141592653589793238
#define rad2deg(x) ( (180.0 / PI) * (x) )
#define deg2rad(x) ( (PI / 180.0) * (x) )
#define fabs(x) ((x) >= 0 ? (x) : - (x))
//#define max(x, y) ((x) > (y) ? (x) : (y))

#define DONT_MOVE 123456.123456

void log(const char* x);
/*void log(const char* x, int a);
void log(const char* x, float a);
void log(const char* x, float a, float b);
void log(const char* x, float a, float b, float c);*/
void log(int x);
void log(float x);

float polarAngle(float x, float y, bool thirdQuarterFix)
{
    if (x > 0)
        return atan(y / x);
    
    if (x < 0 && y >= 0)
    {
        if (thirdQuarterFix)
            return atan(y / x) - PI;
        return atan(y / x) + PI;
    }
    
    if (x < 0 && y < 0)
        return atan(y / x) - PI;
        
    // x = 0
    if (y > 0)
        return PI / 2;
    
    if (y < 0)
        return PI / -2;
    
    // y = 0
    return 0;
}

struct Point
{
    Point()
    {}

    Point(float _x, float _y, float _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }

    float x, y, z;
    
    Point operator+(Point& that)
    {
        return Point(x + that.x, 
                     y + that.y, 
                     z + that.z);
    }

    Point operator-(Point& that)
    {
        return Point(x - that.x, 
                     y - that.y, 
                     z - that.z);
    }

    Point operator*(float m)
    {
        return Point(x * m, 
                     y * m, 
                     z * m);
    }
    
    void operator=(Point& that)
    {
        x = that.x;
        y = that.y;
        z = that.z;
    }

    // Workaround for an gccavr issue with reference args
    void assign(Point that)
    {
        x = that.x;
        y = that.y;
        z = that.z;
    }
    
    void assign(float _x, float _y, float _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }

    float maxDistance(Point& that)
    {
        float dx = fabs(x - that.x);
        float dy = fabs(y - that.y);
        float dz = fabs(z - that.z);
        
        return max(dz, max(dx, dy));
    }    
};

class Leg
{
public:
    
    Leg()
        : _debug(false)
        , _attached(false)
        , _cServoRestAngle(PI / 2)
        , _fServoRestAngle(PI / 2)
        , _tServoRestAngle(PI / 2)
        , _cServoDirection(0.0)
        , _fServoDirection(0.0)
        , _tServoDirection(0.0)
    {}
    
    void debug(bool on)
    {
        _debug = on;
    }
    
    void attach(int coxaPin, int femurPin, int tibiaPin)
    {   
        _attached = true;
      
        if (_debug)
            return;
        
        _coxaPin = coxaPin;
        _femurPin = femurPin;
        _tibiaPin = tibiaPin;
        
        attach();
    }

    void attach()
    {
        _cServo.attach(_coxaPin);
        _fServo.attach(_femurPin);
        _tServo.attach(_tibiaPin);
    }
    
    void detach()
    {
        _cServo.detach();
        _fServo.detach();
        _tServo.detach();
    }
    
    void configureCoxa(float cStartX, 
                       float cStartY,
                       float cStartAngle,
                       float cFemurOffset)
    {
        _cStart.x = cStartX;
        _cStart.y = cStartY;
        _cStartAngle = cStartAngle;
        _cFemurOffset = cFemurOffset;
    }
        
    void configureFemur(float fStartZOffset,
                        float fStartFarOffset,
                        float fLength,
                        float fStartAngle)
    {
        _fStartZOffset = fStartZOffset;
        _fStartFarOffset = fStartFarOffset;
        _fLength = fLength;
        _fStartAngle = fStartAngle;
    }
        
    void configureTibia(float tLenght,
                        float tStartAngle)
    {
        _tLenght = tLenght;
        _tStartAngle = tStartAngle;
    }
    
    void configureServoDirections(float cServoDirection,
                                  float fServoDirection,
                                  float tServoDirection,
                                  bool thirdQuarterFix)
    {
        _cServoDirection = cServoDirection;
        _fServoDirection = fServoDirection;
        _tServoDirection = tServoDirection;
        _thirdQuarterFix = thirdQuarterFix;
    }
    
    void tuneRestAngles(float cServoRestAngle,
                        float fServoRestAngle,
                        float tServoRestAngle)
    {
        _cServoRestAngle = cServoRestAngle;
        _fServoRestAngle = fServoRestAngle;
        _tServoRestAngle = tServoRestAngle;
    }
    
    bool reset()
    {
        move(0, 0, 0);
    }
    
    bool reach(Point& dest)
    {
        float hDist = sqrt( sqr(dest.x - _cStart.x) +  sqr(dest.y - _cStart.y) );
        float additionalCoxaAngle = hDist == 0.0 ? DONT_MOVE 
                                                 : asin( _cFemurOffset / hDist );
        
        float primaryCoxaAngle = polarAngle(dest.x - _cStart.x, dest.y - _cStart.y, _thirdQuarterFix);
        
        float cAngle = hDist == 0.0 ? DONT_MOVE 
                                    : primaryCoxaAngle - additionalCoxaAngle - _cStartAngle;

        // Moving to local Coxa-Femur-target coordinate system
        // Note the case when hDist <= _cFemurOffset. This is for the blind zone.
        // We never can't reach the point that is nearer to the _cStart then
        // femur offset (_fStartFarOffset)
        float localDestX = hDist <= _cFemurOffset 
            ? - _fStartFarOffset
            : sqrt(sqr(hDist) - sqr(_cFemurOffset)) - _fStartFarOffset;

        float localDestY = dest.z - _fStartZOffset;

        // Check reachability
        float localDistSqr = sqr(localDestX) + sqr(localDestY);
        if (localDistSqr > sqr(_fLength + _tLenght))
        {
            log("Can't reach!");
            return false;
        }
        
        // Find joint as circle intersect ( equations from http://e-maxx.ru/algo/circles_intersection & http://e-maxx.ru/algo/circle_line_intersection )
        float A = -2 * localDestX;
        float B = -2 * localDestY;
        float C = sqr(localDestX) + sqr(localDestY) + sqr(_fLength) - sqr(_tLenght);
        float X0 = -A * C / (sqr(A) + sqr(B));
        float Y0 = -B * C / (sqr(A) + sqr(B));
        float D = sqrt( sqr(_fLength) - (sqr(C) / (sqr(A) + sqr(B))) );
        float mult = sqrt ( sqr(D) / (sqr(A) + sqr(B)));
        float ax, ay, bx, by;
        ax = X0 + B * mult;
        bx = X0 - B * mult;
        ay = Y0 - A * mult;
        by = Y0 + A * mult;
        // Select solution on top as joint
        float jointLocalX = (ax > bx) ? ax : bx;
        float jointLocalY = (ax > bx) ? ay : by;
        
        float primaryFemurAngle = polarAngle(jointLocalX, jointLocalY, false);
        float fAngle = primaryFemurAngle - _fStartAngle;
        
        float primaryTibiaAngle = polarAngle(localDestX - jointLocalX, localDestY - jointLocalY, false);
        float tAngle = (primaryTibiaAngle - fAngle) - _tStartAngle;

        move(cAngle, fAngle, tAngle);
    }

    // Can't use reference argument here due to limitations in gcc avr
    void configureDefault(Point def, bool move)
    {
        _defaultPos = def;
        _currentPos = def;
        
        if (move)
            reach(def);
    }
    
    void reachRelativeToDefault(Point& dest)
    {
        _currentPos.assign(_defaultPos + dest);
        reach(_currentPos);
    }
    
    void reachRelativeToCurrent(Point& dest)
    {
        _currentPos.assign(_currentPos + dest);
        reach(_currentPos);
    }
    
    Point getCurrentRelative()
    {
        return _currentPos - _defaultPos;
    }
    
    Point& getCurrentPos()
    {
        return _currentPos;
    }
    
    Point& getDefaultPos()
    {
        return _defaultPos;
    }
    

private:
    
    void move(float cAngle, float fAngle, float tAngle)
    {
        if (_cServoDirection == 0.0 || _fServoDirection == 0.0 || _tServoDirection == 0.0)
            log("ERROR: Null servo directions detected");
      
        if (! _attached)
            return;
      
        int mc = rad2deg(_cServoDirection * cAngle + _cServoRestAngle);
        int mf = rad2deg(_fServoDirection * fAngle + _fServoRestAngle);
        int mt = rad2deg(_tServoDirection * tAngle + _tServoRestAngle);
        
        if (! _debug)
        {
            if (cAngle != DONT_MOVE)
                _cServo.write(coxaLimit(mc));
            if (fAngle != DONT_MOVE)    
                _fServo.write(femurLimit(mf));
            if (tAngle != DONT_MOVE)
                _tServo.write(tibiaLimit(mt));
        }
        else
        {
            log("Angles:");
            log(mc);
            log(mf);
            log(mt);
        }
    }
    
    int coxaLimit(int x)
    {
        const int cMin = 90 - 45;
        const int cMax = 90 + 45;
      
        if (x < cMin) return cMin;
        if (x > cMax) return cMax;
        
        return x;
    }

    int femurLimit(int x)
    {
        const int fMin = 90 - 45;
        const int fMax = 90 + 45;
      
        if (x < fMin) return fMin;
        if (x > fMax) return fMax;
        
        return x;
    }

    int tibiaLimit(int x)
    {
        const int tMin = 90 - 45;
        const int tMax = 90 + 45;
      
        if (x < tMin) return tMin;
        if (x > tMax) return tMax;
        
        return x;
    }

    
private:
    bool _debug;
    
    // Config
    Point _cStart;             // Coordinates of the Coxa start point
    float _cServoRestAngle;    // Angle of Coxa servo relax point (usually pi/2)
    float _cServoDirection;
    float _cStartAngle;        // Angle where Coxa is pointed to when relaxed
    float _cFemurOffset;       // Offset of Femur and Tibia axis from the Coxa axis
    float _fStartZOffset;      // Botton offset of Femur start from the z0 plane
    float _fStartFarOffset;
    float _fLength;            // Femur length
    float _fServoRestAngle;
    float _fServoDirection;
    float _fStartAngle;
    float _tLenght;
    float _tServoRestAngle;
    float _tServoDirection;
    float _tStartAngle;
  
    Point _defaultPos;  
    Point _currentPos;
    bool _attached;
    bool _thirdQuarterFix;
  
    int _coxaPin;
    int _femurPin;
    int _tibiaPin;
  
    // Servos
    Multiservo _cServo;
    Multiservo _fServo;
    Multiservo _tServo;
};

#endif

