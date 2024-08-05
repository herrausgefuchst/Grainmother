#include "effects.hpp"

// MARK: - BEATREPEAT
// ********************************************************************************

Effect::Effect (AudioParameterGroup* _engineparameters, const String _name)
    : parameters(_name, AudioParameterGroup::Type::EFFECT)
    , engineparameters(_engineparameters)
{}

void Effect::setup (const float _fs, const int _blocksize)
{
    fs = _fs;
    blocksize = _blocksize;
}

// MARK: - BEATREPEAT
// ********************************************************************************

void Beatrepeat::setup (const float _fs, const int _blocksize)
{
    fs = _fs;
    blocksize = _blocksize;
    
    initializeParameters();
    initializeListeners();
    
    calcLengthInSamples();
    calcPitchIncrement();
    
    slice_samples = slice_samples_catch;
    ctr_slice = 0;
    ctr_gate = gate_samples;
    ctr_trigger = trigger_samples;
}

FloatPair Beatrepeat::process (const FloatPair _input)
{
    // process ramps
    parameters.getParameter(MIX)->process();
    
    // get button parameter freeze
    //bool freeze = parameters.getParameter(FREEZE)->getValueI();
    
    // new trigger detected?
    //if (--ctr_trigger <= 0 && !freeze)
    if (--ctr_trigger <= 0)
    {
        // check for chance (100 % = all triggers are valid, 0% no trigger)
        float chance = parameters.getParameter(CHANCE)->getValueF() * 0.01f;
        triggerIsValid = rand() <= chance * RAND_MAX ? true : false;
        
        // reset counters
        ctr_trigger = trigger_samples;
        ctr_gate = gate_samples;
        ctr_slice = 0;
        
        // randomize the slice length = variation parameter
        float variation = parameters.getParameter(VARIATION)->getValueF() * 0.01f;
        if (variation > 0.f) calcLengthInSamples(SLICELENGTH);
        
        // update slice length with catch value
        slice_samples = slice_samples_catch;
        
        // if slice length is very short, also shorten the fade length
        if (slice_samples <= 240) fade = (slice_samples-2) / 2;
        else fade = 120;
        
        // reset reading pointer for the buffer and the increment modifier for the pitch decay parameter
        readptr = 0.f;
        pitchdecay_modifier = 0.f;
        
        // flag that this is the first run through, so we have to write the input in the buffer
        isFirstSlice = true;
    }
    
    // write new slice in buffer, if its the first time it's rollin through
    if (isFirstSlice) writeSliceBuffer(_input);
    
    // declare parameters to be filled
    FloatPair effect(_input.first, _input.second);
        
    // if gate is open & the chance for triggering was positive:
//    if ((ctr_gate-- > 0 && triggerIsValid) || freeze)
    if ((ctr_gate-- > 0 && triggerIsValid))
    {
        // read out new samples from buffer
        effect = readSliceBuffer();
        
        // add a fade in or fade out if at beginning or ending of slice
        // simultaniously fade out/in the dry input signal
        if (ctr_slice < fade)
        {
            float fadeIn = (float)ctr_slice / (float)fade;
            effect.first *= fadeIn;
            effect.second *= fadeIn;
            
            float fadeOut = 1.f - fadeIn;
            effect.first += _input.first * fadeOut;
            effect.second += _input.second * fadeOut;
        }
        else if (ctr_slice >= (slice_samples - fade))
        {
            float fadeOut = (float)(slice_samples - ctr_slice) / (float)fade;
            effect.first *= fadeOut;
            effect.second *= fadeOut;
            
            float fadeIn = 1.f - fadeOut;
            effect.first += _input.first * fadeIn;
            effect.second += _input.second * fadeIn;
        }
        
        // reset slice counter if it reaches size of slice
        if (++ctr_slice >= slice_samples)
        {
            readptr = 0.f;
            ctr_slice = 0;
            
//            if (!freeze)
//            {
                float decay = parameters.getParameter(PITCHDECAY)->getValueF() * 0.01f;
                pitchdecay_modifier += decay * 0.6f * slicelength[slicelength_idx]; // 0.6f is a taste modifier
                if (pitchdecay_modifier > increment) pitchdecay_modifier = increment;
//            }
            
            isFirstSlice = false;
        }
        
        // add fade Out if trigger or gate is just before closing
        // fade In is always defined via the slice start
        if ((ctr_trigger <= 120 && ctr_trigger >= 0) || (ctr_gate <= 120 && ctr_gate >= 0))
        {
            float fadeOut = 1.f;
            if (ctr_trigger <= 120) fadeOut *= (float)ctr_trigger / 120.f;
            if (ctr_gate <= fade) fadeOut *= (float)ctr_gate / 120.f;
            effect.first *= fadeOut;
            effect.second *= fadeOut;
            
            float fadeIn = 1.f - fadeOut;
            effect.first += _input.first * fadeIn;
            effect.second += _input.second * fadeIn;
        }
        
        // wet/dry
        float wet = 0.01f * parameters.getParameter(MIX)->getValueF();
        float dry = 1.f - wet;
        effect.first = wet * effect.first + dry * _input.first;
        effect.second = wet * effect.second + dry * _input.second;
    }
    
    return effect;
}

void Beatrepeat::processBlock()
{
    
}

void Beatrepeat::calcLengthInSamples (int _param)
{
    float bpm = engineparameters->getParameter("tempo")->getValueF();
    int bpm_samples = (60.f / bpm) * fs; // one beat in samples

    if (_param == SLICELENGTH || _param == -1)
    {
        int range = parameters.getParameter(VARIATION)->getValueF() * 0.01f * 17;
        int halfrange = range * 0.5f;
        int randomSwitch = range != 0 ? rand() % range - halfrange : 0;
        
        slicelength_idx = parameters.getParameter(SLICELENGTH)->getValueI() + randomSwitch;
        if (slicelength_idx < 0) slicelength_idx = -slicelength_idx;
        if (slicelength_idx >= 16) slicelength_idx -= (slicelength_idx - 15);
        if (slicelength_idx < 0 || slicelength_idx >= 16) engine_rt_error("randomizer broken", __FILE__, __LINE__, true);
        
        float slice = slicelength[slicelength_idx];
        slice_samples_catch = bpm_samples * slice * 4.f; // times 4 because 1/4th is one beat
    }
    
    if (_param == GATE || _param == -1)
    {
        float gate = gatelength[parameters.getParameter(GATE)->getValueI()];
        gate_samples = bpm_samples * gate * 4.f; // times 4 because 1/4th is one beat
    }
    
    if (_param == TRIGGER || _param == -1)
    {
        float trigger = triggerlength[parameters.getParameter(TRIGGER)->getValueI()];
        trigger_samples = bpm_samples * trigger * 4.f; // times 4 because 1/4th is one beat
    }
}

inline void Beatrepeat::calcPitchIncrement()
{
    increment = powf_neon(2.f, -(parameters.getParameter(PITCH)->getValueF() / 12.f));
    
    if (increment > 1.f)
        engine_rt_error("buffer step > 1, shouldn't be up pitching, current increment = " + TOSTRING(increment),
                        __FILE__, __LINE__, true);
    
    if (pitchdecay_modifier > increment) pitchdecay_modifier = increment;
}

inline void Beatrepeat::writeSliceBuffer (const FloatPair _sample)
{
    buffer[0][ctr_slice] = _sample.first;
    buffer[1][ctr_slice] = _sample.second;
}

FloatPair Beatrepeat::readSliceBuffer()
{
    FloatPair output(0.f, 0.f);
    
    // no interpolation needed
    if (readptr - (int)readptr == 0)
    {
        output.first = buffer[0][(int)readptr];
        output.second = buffer[1][(int)readptr];
    }
    
    // interpolation needed
    else
    {
        #ifdef BELA_CONNECTED
        int readbottom = floorf_neon(readptr);
        #else
        int readbottom = floorf(readptr);
        #endif
        int readtop = readbottom + 1;
        
        if (readtop >= sizeof(buffer))
            engine_rt_error("error interpolating slice buffer of Beatrepeat effect",
                            __FILE__, __LINE__, true);
        
        float frec =  readptr - readbottom;
        
        output.first = buffer[0][readbottom];
        output.first += frec * (buffer[0][readtop] - output.first);
        output.second = buffer[1][readbottom];
        output.second += frec * (buffer[1][readtop] - output.second);
    }
    
    readptr += (increment - pitchdecay_modifier);
    
    return output;
}

inline void Beatrepeat::initializeParameters()
{
    String trigger_choices[] = { "1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "5/4", "3/2", "7/4", "2/1", "3/1", "4/1" };
    String slice_choices[] = { "1/256", "1/128", "1/96", "1/64", "1/48", "1/32", "1/24", "1/16", "1/12", "1/8", "1/6", "1/4", "1/3", "1/2", "3/4", "1/1" };
    String gate_choices[] = { "1/16", "1/8", "3/16", "2/8", "5/16", "3/8", "7/16", "1/2", "9/16", "5/8", "11/16", "3/4", "13/16", "7/8", "15/16", "1/1", "5/4", "3/2", "7/4", "2/1", "3/1", "4/1" };
    
    parameters.addParameter("beatrepeat_slicelength", "Slice Length", sizeof(slice_choices) / sizeof(String), slice_choices);
    parameters.addParameter("beatrepeat_gate", "Gate", sizeof(gate_choices) / sizeof(String), gate_choices);
    parameters.addParameter("beatrepeat_trigger", "Trigger", sizeof(trigger_choices) / sizeof(String), trigger_choices);
    parameters.addParameter("beatrepeat_chance", "Chance", "%", 0.f, 100.f, 100.f, 0.f);
    parameters.addParameter("beatrepeat_variation", "Variation", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("beatrepeat_pitch", "Down Pitch", "semitones", 0.f, 24.f, 1.f, 0.f);
    parameters.addParameter("beatrepeat_pitchdecay", "Pitch Decay", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("beatrepeat_mix", "Mix", "%", 0.f, 100.f, 0.f, 50.f, SlideParameter::LIN, 1.f);
    parameters.addParameter("beatrepeat_freeze", "Freeze", ButtonParameter::COUPLED);
}

inline void Beatrepeat::initializeListeners()
{
    parameters.getParameter(SLICELENGTH)->onChange.push_back([this] { calcLengthInSamples(SLICELENGTH); });
    parameters.getParameter(TRIGGER)->onChange.push_back([this] { calcLengthInSamples(TRIGGER); });
    parameters.getParameter(GATE)->onChange.push_back([this] { calcLengthInSamples(GATE); });
    
    parameters.getParameter(PITCH)->onChange.push_back([this] { calcPitchIncrement(); });
    
    engineparameters->getParameter("tempo")->onChange.push_back([this]
                                                                {
        calcLengthInSamples();
        ctr_trigger = 0;
    });
}


// MARK: - GRANULATOR
// ********************************************************************************

void Granulator::setup (const float _fs, const int _blocksize)
{
    fs = _fs;
    blocksize = _blocksize;
    
    initializeParameters();
    initializeListeners();
}

FloatPair Granulator::process (const FloatPair _input)
{
    // process ramps
    parameters.getParameter(GRAN1)->process();
    parameters.getParameter(GRAN2)->process();

    FloatPair effect = _input;
    
    return effect;
}

void Granulator::processBlock()
{}

inline void Granulator::initializeParameters()
{
    parameters.addParameter("granulator_param1", "Gran1", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("granulator_param2", "Gran2", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("granulator_param3", "Gran3", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("granulator_param4", "Gran4", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("granulator_param5", "Gran5", "semitones", 0.f, 24.f, 1.f, 0.f);
    parameters.addParameter("granulator_param6", "Gran6", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("granulator_param7", "Gran7", "seconds", 0.f, 2.f, 0.f, 0.f);
    parameters.addParameter("granulator_param8", "Gran8", "%", 0.f, 100.f, 0.f, 50.f);
    parameters.addParameter("granulator_param9", "Gran9", ButtonParameter::COUPLED);
}

inline void Granulator::initializeListeners()
{
    
}

// MARK: - GRANULATOR
// ********************************************************************************

void Delay::setup (const float _fs, const int _blocksize)
{
    fs = _fs;
    blocksize = _blocksize;
    
    initializeParameters();
    initializeListeners();
}

FloatPair Delay::process (const FloatPair _input)
{
    // process ramps
    // ...

    FloatPair effect = _input;
    
    return effect;
}

void Delay::processBlock()
{}

inline void Delay::initializeParameters()
{
    parameters.addParameter("delay1", "Delay1", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("delay2", "Delay2", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("delay3", "Delay3", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("delay4", "Delay4", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("delay5", "Delay5", "semitones", 0.f, 24.f, 1.f, 0.f);
    parameters.addParameter("delay6", "Delay6", "%", 0.f, 100.f, 0.f, 0.f);
    parameters.addParameter("delay7", "Delay7", "seconds", 0.f, 2.f, 0.f, 0.f);
    parameters.addParameter("delay8", "Delay8", "%", 0.f, 100.f, 0.f, 50.f);
    parameters.addParameter("delay9", "Delay9", ButtonParameter::COUPLED);
}

inline void Delay::initializeListeners()
{
    
}

