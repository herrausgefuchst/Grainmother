#ifndef helpers_hpp
#define helpers_hpp

#include "functions.h"

// MARK: - RAMP
// ********************************************************************************

class Ramp
{
public:
    Ramp() = delete;
    Ramp (const float _value, const float _fs = 44100.f);
        
    bool process();

    void setRampTo (const float _goal, const float _time_ms = 100.f);
    
    void setValue (const float _value);
    
    float getCurrent() const { return current; }
    float getGoal() const { return goal; }

private:
    float current, goal, step;
    int countsteps;
    const float fs;
};


// MARK: - TempoTapper
// ********************************************************************************

class TempoTapper
{
public:
    TempoTapper() {}
    TempoTapper (const float _minBPM, const float _maxBPM, const float _fs = 44100.f)
        { setup(_minBPM, _maxBPM, _fs); }
    
    void setup (const float _minBPM, const float _maxBPM, const float _fs = 44100.f);
    bool process();
    void tapTempo();
    float getBPM();

private:
    void startCounting();
    void stopCounting();
    void calculateNewTempo();
    
private:
    float fs;
    int mincounter, maxcounter;

    float bpm = 120.f;
    int counter = 0;
    
    bool isCounting = false;
    bool bpmChanged = false;
};

// MARK: - ChaosGenerator
// ********************************************************************************

class ChaosGenerator
{
public:
    float process();
    
    void setStartValue (const float _x);
    void setCoefficient (const float _coef);
    
private:
    float y = 0.1f;
    float coef = 2.7f; // 0...4
};


// MARK: - RandomGenerator
// ********************************************************************************

class RandomGenerator
{
public:
    RandomGenerator() { srand((unsigned int)time(NULL)); }
    
    float getRandomFloat(const float _min, const float _max)
    {
        if (_max <= _min) return 0.f;
        
        float dist = _max - _min;
        return rand() * dist / RAND_MAX + _min;
    }
};


// MARK: - MovingAverager
// ********************************************************************************

class MovingAverager
{
public:
    MovingAverager();
    float process (const float _x);
    float getZD1() const { return zd1; }
    
private:
    static const int buffersize = 1024;
    int pointer = 0;
    float delayline[buffersize];
    float integrator, zd1;
};
    

// MARK: - Dc Offset Filter
// ********************************************************************************

class DCOffsetFilter
{
public:
    float process (const float _x) 
    {
        return ma1.getZD1() - ma2.process(ma1.process(_x));
    }

private:
    MovingAverager ma1, ma2;
};


// MARK: - Debouncer
// ********************************************************************************

class Debouncer
{
public:
    Debouncer() = delete;
    Debouncer (const int _debounceunits, const int _defaultstate = OPENED)
        : debounceunits(_debounceunits)
        , state(INT2ENUM(_defaultstate, State))
        , counter(_debounceunits)
    {}

    bool update (const bool _rawvalue);
    
private:
    enum State { OPENED, CLOSED, JUSTOPENED, JUSTCLOSED };
    enum Input { CLOSE, OPEN };
    State state;
    int counter;
    const int debounceunits;
};

#endif /* helpers_hpp */
