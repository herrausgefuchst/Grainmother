#include "helpers.hpp"

// MARK: - RAMP
// ********************************************************************************

void Ramp::setup(const float value_, const float fs_)
{
    current = goal = value_;
    fs = fs_;
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
    return false;
}

void Ramp::setRampTo(const float goal_, const float time_ms_)
{
    if (goal_ != goal)
    {
        if (time_ms_ == 0.f)
        {
            setValue(goal_);
        }
        else
        {
            goal = goal_;
            float range = fabsf(goal - current);
            countsteps = fs * 0.001f * time_ms_;
            step = range / static_cast<float>(countsteps);

            if (goal < current)
                step *= -1.f;
        }
    }
}

void Ramp::setValue(const float value_)
{
    current = value_;
    goal = current;
    countsteps = 0;
    step = 0.f;
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
