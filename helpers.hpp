#ifndef helpers_hpp
#define helpers_hpp

#include "functions.h"

// MARK: - RAMP
// ********************************************************************************

/**
 * @class Ramp
 * @brief A class that handles smooth transitions (ramping) between values over time.
 */
class Ramp
{
public:
    /**
     * @brief Sets up a Ramp with an initial value and sampling rate.
     * @param value_ The initial value of the ramp.
     * @param fs_ The sampling rate (default is 44100 Hz).
     */
    void setup(const float value_, const float fs_ = 44100.f);
    
    /**
     * @brief Processes the ramp, updating the current value towards the goal.
     * @return True if the ramp is still in progress, false if it has reached the goal.
     */
    bool process();

    /**
     * @brief Sets a new goal for the ramp, specifying the time to reach it.
     * @param goal_ The target value to ramp to.
     * @param time_ms_ The time in milliseconds to reach the goal (default is 100 ms).
     */
    void setRampTo(const float goal_, const float time_ms_ = 100.f);
    
    /**
     * @brief Sets the current value of the ramp immediately.
     * @param value_ The new current value.
     */
    void setValue(const float value_);
    
    /**
     * @brief Gets the current value of the ramp.
     * @return The current value.
     */
    float getCurrent() const { return current; }

    /**
     * @brief Gets the goal value of the ramp.
     * @return The goal value.
     */
    float getGoal() const { return goal; }

private:
    float current; /**< The current value of the ramp */
    float goal; /**< The target value of the ramp */
    float step; /**< The amount to change per step */
    int countsteps; /**< The number of steps remaining to reach the goal */
    float fs; /**< The sampling rate */
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
        : state(INT2ENUM(_defaultstate, State))
        , counter(_debounceunits)
        , debounceunits(_debounceunits)
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
