 #include <Multiservo.h>
#include "Leg.h"
#include <Wire.h>
#include "SimpleMovements.h"

// TODO: Remove this temporary goo
void log(const char* x) { Serial.println(x); }
void log(float x) { Serial.println(x); }
void log(int x) { Serial.println(x); }

static Leg legs[6];
static int N = 6;//sizeof(legs) / sizeof(Leg);

static Leg* rightLegs = legs;
static Leg* leftLegs = legs + N / 2;

static SimpleMovements moveSimple(legs, N);
static Point zero(0,0,0);

static Point stateLinearMovement;

void processState()
{
    // TODO:
    for (int i = 0; i < N; ++i)
        legs[i].reachRelativeToCurrent(stateLinearMovement);
}

void walk(int steps, Point direction)
{
    for (int i = 0; i < steps; i++)
    {
        for(float p = 0; p <= 2; p += 0.025)
        {
            float progress; 
            float height1;
            float height2;
            if (p < 1) 
            {
                progress = p;
                height1 = 0;
                height2 = direction.z * (0.5 - fabs(0.5 - p));
            }
            else 
            {
                progress = 1 - (p - 1);
                height1 = direction.z * (0.5 - fabs(1.5 - p));
                height2 = 0;
            }
          
            Point group1(- direction.x / 2 + (direction.x - progress * direction.x),
                         - direction.y / 2 + (direction.y - progress * direction.y),
                         height1);
            Point group2(- direction.x / 2 + progress * direction.x,
                         - direction.y / 2 + progress * direction.y,
                         height2);


            if (legs[0].getCurrentRelative().maxDistance(group1) > 10)
                smoothTo(group1, 0);
            if (legs[1].getCurrentRelative().maxDistance(group2) > 5)
                smoothTo(group2, 1);
                    
            for (int li = 0; li < N; li+=2)
            {
                    
              
                legs[li].reachRelativeToDefault(group1);                  
                legs[li + 1].reachRelativeToDefault(group2);                  
            }

            delay(1);
        }        
    }
}

void smoothTo(Point& to)
{
    smoothTo(to, 0);
    smoothTo(to, 1);
}

// Relative to default!
void smoothTo(Point& to, int legGroup)
{
    Point relative[N];
    for (int i = legGroup; i < N; i += 2)
        relative[i].assign((legs[i].getDefaultPos() + to) - legs[i].getCurrentPos());
  
    for(float p = 0; p <= 1; p += 0.05)
    {
        for (int li = legGroup; li < N; li += 2)
        {
            Point currSubStep = relative[li] * p;
            Point currStep = legs[li].getCurrentPos() + currSubStep;
            currStep.z = legs[li].getDefaultPos().z + to.z + 50 * (0.5 - fabs(0.5 - p));
            legs[li].reach(currStep);
        }
        
        delay(5);
    }
}

void configureLegs()
{
    rightLegs[0].attach(15, 16, 17);
    rightLegs[1].attach(12, 13, 14);
    rightLegs[2].attach(11, 10, 9);
    
    leftLegs[0].attach(2, 1, 0);
    leftLegs[1].attach(5, 4, 3);
    leftLegs[2].attach(6, 7, 8);


    rightLegs[0].configureServoDirections(-1, -1,  1, false);
    leftLegs [0].configureServoDirections(-1,  1, -1, false);
    
    rightLegs[1].configureServoDirections(-1, -1,  1, false);
    leftLegs [1].configureServoDirections(-1,  1, -1, true);
    
    rightLegs[2].configureServoDirections(-1, -1,  1, false);
    leftLegs [2].configureServoDirections(-1,  1, -1, true);

    for (int i = 0; i < 3; i++)
    {
        rightLegs[i].configureFemur(-26, 12, 46.5, deg2rad(10));
        leftLegs [i].configureFemur(-26, 12, 46.5, deg2rad(10));

        rightLegs[i].configureTibia(58, deg2rad(-70));
        leftLegs [i].configureTibia(58, deg2rad(-70));
    }
    
    rightLegs[0].configureCoxa( 34,  65, deg2rad(      20),  10);                        
    leftLegs [0].configureCoxa(-34,  65, deg2rad(180 - 20), -10);

    rightLegs[1].configureCoxa( 52,   0, deg2rad(-         20),  10);                        
    leftLegs [1].configureCoxa(-52,   0, deg2rad(- (180 - 20)), -10);

    rightLegs[2].configureCoxa( 34, -65, deg2rad(-         63),  10);                        
    leftLegs [2].configureCoxa(-34, -65, deg2rad(- (180 - 63)), -10);
    
    
    rightLegs[0].configureDefault(Point( 94, 120, -70), true);
    leftLegs [0].configureDefault(Point(-94, 120, -70), true);
  
    rightLegs[1].configureDefault(Point( 120, 00, -70), true);
    leftLegs [1].configureDefault(Point(-120, 00, -70), true);

    rightLegs[2].configureDefault(Point( 94, -110, -70), true);
    leftLegs [2].configureDefault(Point(-94, -110, -70), true);
    
    
    // Fine tuning
    rightLegs[0].tuneRestAngles(PI / 2,
                                PI / 2 + deg2rad(10),
                                PI / 2);
    leftLegs[0].tuneRestAngles(PI / 2,
                                PI / 2 - deg2rad(5),
                                PI / 2);

    leftLegs[1].tuneRestAngles(PI / 2- deg2rad(10),
                                PI / 2,
                                PI / 2 - deg2rad(27));

    rightLegs[1].tuneRestAngles(PI / 2+ deg2rad(10),
                                PI / 2,
                                PI / 2 + deg2rad(20));


    leftLegs[2].tuneRestAngles(PI / 2 - deg2rad(20),
                                PI / 2 - deg2rad(10),
                                PI / 2);
    rightLegs[2].tuneRestAngles(PI / 2 + deg2rad(20),
                                PI / 2 + deg2rad(10),
                                PI / 2);
    
    for (Leg* leg = legs; leg < legs + 6; leg++)
        leg->reachRelativeToDefault(zero);
      //leg->reset();
}

void setup() 
{
     Wire.begin();
    Serial.begin(9600);
    
    Serial.println("Starting");
    
    //for (Leg* leg = legs; leg < legs + 6; leg++)
     //   leg->debug(true);
    

    configureLegs();



  smoothTo(zero);
  
  /*for (int i = 0; i < 6; i++)
    legs[i].detach();
*/
} 

int clearens = 75; 
char incoming;
void loop() 
{  
  if (Serial.available())
  // if text arrived in from BT serial...
  {
    incoming=(Serial.read());
    if (incoming=='1')
    {      
      smoothTo(zero);
      Serial.println("Zero");
    }else if (incoming=='2')
    {
      Serial.println("Backward");
      walk(2, Point(0, -50, clearens ));      
      smoothTo(zero);
    }else if (incoming=='3')
    {
      Serial.println("Forward");
      walk(2, Point(0, 50, clearens ));
      smoothTo(zero);
    }else if (incoming=='4')
    {
      Serial.println("Left");      
      walk(2, Point(50, 0, clearens ));
      smoothTo(zero);
    }else if (incoming=='5')
    {
      Serial.println("Right");      
      walk(2, Point(-50, 0, clearens ));
      smoothTo(zero);
    }else if (incoming=='6')
    {
      Serial.println("Rotate clockwise");    
      smoothTo(zero);
      moveSimple.rotate( 2 , 1 );      
      smoothTo(zero);
    }else if (incoming=='7')
    {
      Serial.println("Rotate CounterClockwise");    
      smoothTo(zero);
      moveSimple.rotate( 2 , -1 );      
      smoothTo(zero);
    }    
  }
  
} 
