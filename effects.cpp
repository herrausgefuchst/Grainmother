#include "effects.hpp"

// MARK: - BEATREPEAT
// ********************************************************************************

Effect::Effect(AudioParameterGroup* engineParameters_,
                const unsigned int numParameters_, const String parameterGroupName_,
                const float sampleRate_, const unsigned int blockSize_)
    : parameters(parameterGroupName_, AudioParameterGroup::Type::EFFECT, numParameters_)
    , engineParameters(engineParameters_)
    , sampleRate(sampleRate_)
    , blockSize(blockSize_)
{}

// MARK: - BEATREPEAT
// ********************************************************************************

StereoFloat Reverb::processAudioSamples(const StereoFloat input_)
{
    StereoFloat effect = input_;
    
    return effect;
}
void Reverb::updateAudioBlock()
{
    
}

void Reverb::initializeParameters()
{
//    parameters.addParameter("beatrepeat_slicelength", "Slice Length", sizeof(slice_choices) / sizeof(String), slice_choices);
//    parameters.addParameter("beatrepeat_gate", "Gate", sizeof(gate_choices) / sizeof(String), gate_choices);
//    parameters.addParameter("beatrepeat_trigger", "Trigger", sizeof(trigger_choices) / sizeof(String), trigger_choices);
//    parameters.addParameter("beatrepeat_chance", "Chance", "%", 0.f, 100.f, 100.f, 0.f);
//    parameters.addParameter("beatrepeat_variation", "Variation", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("beatrepeat_pitch", "Down Pitch", "semitones", 0.f, 24.f, 1.f, 0.f);
//    parameters.addParameter("beatrepeat_pitchdecay", "Pitch Decay", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("beatrepeat_mix", "Mix", "%", 0.f, 100.f, 0.f, 50.f, SlideParameter::LIN, 1.f);
//    parameters.addParameter("beatrepeat_freeze", "Freeze", ButtonParameter::COUPLED);
}

void Reverb::initializeListeners()
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

StereoFloat Granulator::processAudioSamples(const StereoFloat input_)
{
    // process ramps
//    parameters.getParameter(GRAN1)->process();
//    parameters.getParameter(GRAN2)->process();

    StereoFloat effect = input_;
    
    return effect;
}

void Granulator::updateAudioBlock()
{}

void Granulator::initializeParameters()
{
//    parameters.addParameter("granulator_param1", "Gran1", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("granulator_param2", "Gran2", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("granulator_param3", "Gran3", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("granulator_param4", "Gran4", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("granulator_param5", "Gran5", "semitones", 0.f, 24.f, 1.f, 0.f);
//    parameters.addParameter("granulator_param6", "Gran6", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("granulator_param7", "Gran7", "seconds", 0.f, 2.f, 0.f, 0.f);
//    parameters.addParameter("granulator_param8", "Gran8", "%", 0.f, 100.f, 0.f, 50.f);
//    parameters.addParameter("granulator_param9", "Gran9", ButtonParameter::COUPLED);
}

void Granulator::initializeListeners()
{
    
}

// MARK: - GRANULATOR
// ********************************************************************************

StereoFloat Resonator::processAudioSamples(const StereoFloat input_)
{
    // process ramps
    // ...

    StereoFloat effect = input_;
    
    return effect;
}

void Resonator::updateAudioBlock()
{}

void Resonator::initializeParameters()
{
//    parameters.addParameter("delay1", "Delay1", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("delay2", "Delay2", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("delay3", "Delay3", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("delay4", "Delay4", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("delay5", "Delay5", "semitones", 0.f, 24.f, 1.f, 0.f);
//    parameters.addParameter("delay6", "Delay6", "%", 0.f, 100.f, 0.f, 0.f);
//    parameters.addParameter("delay7", "Delay7", "seconds", 0.f, 2.f, 0.f, 0.f);
//    parameters.addParameter("delay8", "Delay8", "%", 0.f, 100.f, 0.f, 50.f);
//    parameters.addParameter("delay9", "Delay9", ButtonParameter::COUPLED);
}

void Resonator::initializeListeners()
{
    
}

