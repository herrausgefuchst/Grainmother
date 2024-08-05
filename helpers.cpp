#include "helpers.hpp"

// MARK: - RAMP
// ********************************************************************************

Ramp::Ramp (const float _value, const float _fs)
: current(_value), goal(_value), fs(_fs)
{
    step = 0.f;
    countsteps = 0;
}

bool Ramp::process()
{
    if (countsteps > 0)
    {
        --countsteps;
        
        current += step;
        
        return true;
    }
    
    else return false;
}

void Ramp::setRampTo (const float _goal, const float _time_ms)
{
    if (_goal != goal)
    {
        if (_time_ms == 0.f)
        {
            setValue(_goal);
        }
        else
        {
            goal = _goal;
            
            float range = fabsf(goal - current);
            
            countsteps = fs * 0.001f * _time_ms;
            
            step = range / (float)countsteps;
            
            if (goal < current) step *= -1.f;
        }
    }
}

inline void Ramp::setValue (const float _value)
{
    current = _value;
    goal = current;
    
    countsteps = 0;
    step = 0.f;
}


// MARK: - TEMPOTAPPER
// ********************************************************************************
  
void TempoTapper::setup(const float _minBPM, const float _maxBPM, const float _fs)
{
    engine_error(_maxBPM == 0 || _minBPM == 0,
          "max or min BPM cannot be 0",
          __FILE__, __LINE__, true);
    
    fs = _fs;
    mincounter = (60.f * fs) / _maxBPM;
    maxcounter = (60.f * fs) / _minBPM;
    // high bpm = low counter!
    //   60 bpm = (60 * fs) / 60
    //    1 bpm = (60 * fs)
    //  120 bpm = (60 * fs) / 120
}

bool TempoTapper::process()
{
    if (isCounting)
        if (++counter > maxcounter)
            stopCounting();
    
    return bpmChanged;
}

inline void TempoTapper::startCounting()
{
    if (counter >= mincounter && counter <= maxcounter) calculateNewTempo();
    
    isCounting = true;
    
    counter = 0;
}

inline void TempoTapper::stopCounting()
{
    isCounting = false;

    counter = -1;
}

void TempoTapper::calculateNewTempo()
{
    //  44100 samples / fs = 1s
    //  60s / 1s = 60 bpm
    //  22050 samples / fs = 0.5s
    //  60s / 0.5s = 120 bpm
    //  88200 samples / fs = 2s
    //  60s / 2s = 30 bpm
    
    float seconds = counter / fs;
    bpm = 60.f / seconds;
    bpm = roundf(10.f*bpm) * 0.1f; //TODO: ???
    
    bpmChanged = true;
}

float TempoTapper::getBPM()
{
    bpmChanged = false;
    
    return bpm;
}

void TempoTapper::tapTempo()
{
    startCounting();
}


// MARK: - CHAOS GENERATOR
// ********************************************************************************
    
inline float ChaosGenerator::process()
{
    y = coef * y * (1.f - y);
    return y;
}

inline void ChaosGenerator::setStartValue (const float _x)
{
    y = _x;
}

inline void ChaosGenerator::setCoefficient(const float _coef)
{
    coef = _coef;
    boundValue(coef, 0.f, 4.f);
}

// MARK: - MOVING AVERAGER
// ********************************************************************************

MovingAverager::MovingAverager()
{
    for (unsigned int n = 0; n < buffersize; n++)
        delayline[n] = 0.f;
}

float MovingAverager::process(const float _x)
{
    delayline[pointer] = _x;
    
    int zDPointer = pointer + 1;
    if (zDPointer >= buffersize) zDPointer -= buffersize;
    
    int zD1Pointer = pointer + 2;
    if (zD1Pointer >= buffersize) zD1Pointer -= buffersize;
    
    zd1 = delayline[zD1Pointer];
    
    float output = _x - delayline[zDPointer];
    output += integrator;
    
    integrator = output;
    
    output *= 0.0009765625f; // /= 1024

    if (++pointer >= buffersize) pointer = 0;
    
    return output;
}


// MARK: - DEBOUNCER
// ********************************************************************************

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
