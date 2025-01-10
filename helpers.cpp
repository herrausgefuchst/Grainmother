#include "Helpers.hpp"

// =======================================================================================
// MARK: - LINEAR RAMP
// =======================================================================================

void LinearRamp::setup(const float& initialValue_, const float& sampleRate_, const unsigned int& blocksize_, const bool& blockwiseProcessing_)
{
    value = target = initialValue_;
    fs = sampleRate_;
    blocksize_inv = 1.f / (float)blocksize_;
    blockwiseProcessing = blockwiseProcessing_;
}


bool LinearRamp::processRamp()
{
    value += incr;
        
    if (--counter <= 0)
    {
        rampFinished = true;
        value = target;
    }
    
    return true;
}


void LinearRamp::setValueWithoutRamping(const float& newValue_)
{
    value = target = newValue_;
        
    rampFinished = true;
    
    incr = 0.f;
}


void LinearRamp::setRampTo(const float& target_, const float& time_sec)
{
    target = target_;
            
    if (target != value)
    {
        // calculate the num of steps that the ramp takes
        // if process will be called blockwise, the counter has to be set accordingly
        counter = (uint)(time_sec * fs);
        if (blockwiseProcessing) counter *= blocksize_inv;
                                
        // calculate the increment that's added every call of process
        if (counter != 0) incr = (target - value) / (float)counter;
        // counter == 0 would mean that the value will be set immediatly without ramping
        else setValueWithoutRamping(target);
    }
    else incr = 0.f;
            
    rampFinished = false;
}


// =======================================================================================
// MARK: - DEBOUNCER
// =======================================================================================

bool Debouncer::update (const bool _rawvalue)
{
    switch (state)
    {
        case JUSTCLOSED: {
            if (counter <= 0) state = CLOSED;
            --counter;
            return OPEN;
            break;
        }
        case CLOSED: {
            if (_rawvalue == OPEN) {
                state = JUSTOPENED;
                counter = debounceunits;
            }
            return CLOSE;
            break;
        }
        case JUSTOPENED: {
            if (counter <= 0) state = OPENED;
            --counter;
            return CLOSE;
            break;
        }
        case OPENED: {
            if (_rawvalue == CLOSE) {
                state = JUSTCLOSED;
                counter = debounceunits;
            }
            return OPEN;
            break;
        }
    }
}
