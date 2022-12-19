#ifndef SIMPLE_MOVEMENTS_H__
#define SIMPLE_MOVEMENTS_H__

class SimpleMovements
{
public:
    SimpleMovements(Leg* legs, int legCount)
        : _legs(legs)
        , _legCount(legCount)
    {
        //_defaultPositions = new Point[_legCount];
        //_defaultMandiblePositions = new Point[_mandibleCount];

        _speedFactor = 0.025;
    }

    ~SimpleMovements()
    {
        //delete _defaultPositions;
        //delete _defaultMandiblePositions;
    }
    float rotate(int steps, float clockwise, float startProgress = 0, bool (*pContinue)() = NULL)
    {
        float p = startProgress;
        for (int i = 0; i < steps; i++)
        {
            for(; p <= 2; /*p += 0.025*/)
            {
                float progress;
                float h[2]; // height
                float s[2]; // sine
                float c[2]; // cosine
                const float angle = clockwise * (PI / 500);

                if (p < 1)
                {
                    progress = p;
                    h[0] = 0;
                    h[1] = 50 * (0.5 - fabs(0.5 - p));
                    s[0] = sin(angle);
                    c[0] = cos(angle);
                    s[1] = sin(- angle);
                    c[1] = cos(- angle);
                }
                else
                {
                    progress = 1 - (p - 1);
                    h[0] = 50 * (0.5 - fabs(1.5 - p));
                    h[1] = 0;
                    s[0] = sin(- angle);
                    c[0] = cos(- angle);
                    s[1] = sin(angle);
                    c[1] = cos(angle);
                }

                for (int li = 0; li < _legCount; ++li)
                {
                    // li - leg index, gi - group index
                    int gi = li % 2;
                    Point pNew;
                    Point pCurr = _legs[li].getCurrentPos();

                    pNew.x = pCurr.x * c[gi] - pCurr.y * s[gi];
                    pNew.y = pCurr.x * s[gi] + pCurr.y * c[gi];

                    // Calc default pos
                    pNew.x -= _legs[li].getDefaultPos().x;
                    pNew.y -= _legs[li].getDefaultPos().y;
                    pNew.z = h[gi];

                    _legs[li].reachRelativeToDefault(pNew);
                    //_legs[li].reachRelativeToCurrent(pNew);
                }

                delay(1);
                if (pContinue != NULL && ! pContinue())
                    return p;

                p += _speedFactor /*+ 0.00 * (0.5 - fabs(0.5 - progress))*/;
            }

            if (p > 2)
                p = 0;
        }

        return p;
    }
private:

    Leg* _legs;
    const int _legCount;
    float _speedFactor;
};

#endif



