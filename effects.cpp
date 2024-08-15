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

}

StereoFloat Beatrepeat::process (const StereoFloat _input)
{
    StereoFloat effect = _input;
    
    return effect;
}

void Beatrepeat::processBlock()
{
    
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
//    parameters.getParameter(SLICELENGTH)->onChange.push_back([this] { calcLengthInSamples(SLICELENGTH); });
//    parameters.getParameter(TRIGGER)->onChange.push_back([this] { calcLengthInSamples(TRIGGER); });
//    parameters.getParameter(GATE)->onChange.push_back([this] { calcLengthInSamples(GATE); });
//    
//    parameters.getParameter(PITCH)->onChange.push_back([this] { calcPitchIncrement(); });
//    
//    engineparameters->getParameter("tempo")->onChange.push_back([this]
//                                                                {
//        calcLengthInSamples();
//        ctr_trigger = 0;
//    });
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

StereoFloat Granulator::process (const StereoFloat _input)
{
    // process ramps
    parameters.getParameter(GRAN1)->process();
    parameters.getParameter(GRAN2)->process();

    StereoFloat effect = _input;
    
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

StereoFloat Delay::process (const StereoFloat _input)
{
    // process ramps
    // ...

    StereoFloat effect = _input;
    
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

