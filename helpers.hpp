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

/**
 * @class RampLinear
 * @brief The RampLinear object implements a linear fade between two values.
 *
 * This class is used for parameters that glitch or crackle when changing them in the UI too fast. One can set the time, the ramp needs to process.
 */
class LinearRamp
{
public:
    /** ()-operator returns the momentary value.*/
    const float& operator() () const
    {
        return value;
    }
    
    /** !=-operator compres the two momentary values of both objects */
    bool operator!= (const LinearRamp& otherRamp) const
    {
        if (value != otherRamp() || !rampFinished) return true;
        else return false;
    }
    
    /** =-operator sets a new value directly, without ramping it */
    void operator= (const float& newValue_)
    {
        setValueWithoutRamping(newValue_);
    }
    
    /**
     * @brief sets up the ramp
     * @param initialValue_ the start value
     * @param sampleRate_ sample rate
     * @param blocksize_ rates how often the ramp should be processed
     * @param blockwiseProcessing_ if yes, the increments will be calculated accordingly
     *
     * blockSize_ can be audio blocksize, but can also be something else. Just be sure to call the processRamp() function in the same rate.
     */
    void setup (const float& initialValue_, const float& sampleRate_, const unsigned int& blocksize_, const bool& blockwiseProcessing_ = true)
    {
        value = target = initialValue_;
        fs = sampleRate_;
        blocksize_inv = 1.f / (float)blocksize_;
        blockwiseProcessing = blockwiseProcessing_;
    }
    
    /** increments value, deincrements counter and sets finished-flag if counter is off */
    bool processRamp()
    {
        value += incr;
        
        if (--counter <= 0)
        {
            rampFinished = true;
            value = target;
        }
        
        return true;
    }
    
    /** sets value and target to the same value, no ramping needed */
    void setValueWithoutRamping (const float& newValue_)
    {
        value = target = newValue_;
        
        rampFinished = true;
        
        incr = 0.f;
    }
    
    /** sets a new target value for the ramp with a certain time */
    void setRampTo (const float& target_, const float& time_sec)
    {
        target = target_;
        
        if (target != value)
        {
            // calculate the num of steps that the ramp takes
            // if process will be called blockwise, the counter has to be set accordingly
            counter = (int)(time_sec * fs);
            if (blockwiseProcessing) counter *= blocksize_inv;
            
            // calculate the increment that's added every call of process
            if (counter != 0) incr = (target - value) / (float)counter;
            // counter == 0 would mean that the value will be set immediatly without ramping
            else setValueWithoutRamping(target);
        }
        else incr = 0.f;
        
        rampFinished = false;
    }
    
    /** returns the current value,  ()-operator does the same */
    const float& getValue() const { return value; }
    
    const float& getTarget() const { return target; }

private:
    float incr = 0.f; ///< the increment step of the ramp
    float value = 0.f; ///< the current value
    float target = 0.f; ///< the target value of the ramp
    unsigned int counter = 0; ///< counts if ramp has finished
    float fs, blocksize_inv;
    bool blockwiseProcessing = false;
public:
    bool rampFinished = true;
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
