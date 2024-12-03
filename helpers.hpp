#ifndef helpers_hpp
#define helpers_hpp

#include "Functions.h"


// =======================================================================================
// MARK: - LINEAR RAMP
// =======================================================================================

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
    void setup(const float& initialValue_, const float& sampleRate_, const unsigned int& blocksize_,
               const bool& blockwiseProcessing_ = true);
    
    /** increments value, deincrements counter and sets finished-flag if counter is off */
    bool processRamp();
    
    /** sets value and target to the same value, no ramping needed */
    void setValueWithoutRamping(const float& newValue_);
    
    /** sets a new target value for the ramp with a certain time */
    void setRampTo(const float& target_, const float& time_sec);
    
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


// =======================================================================================
// MARK: - DEBOUNCER
// =======================================================================================

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
