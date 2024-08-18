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


void Reverb::setup()
{
    initializeParameters();
    initializeListeners();
}


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
    using namespace GrainmotherReverb;
    
    rt_printf("Reverb Parameters initializing!\n");
    
    // parameters controlled by potentiometers/sliders (index 0...7)
    for (unsigned int n = 0; n < NUM_POTENTIOMETERS; ++n)
        parameters.addParameter(parameterID[n],
                                parameterName[n],
                                parameterSuffix[n],
                                parameterMin[n],
                                parameterMax[n],
                                parameterStep[n],
                                parameterInitialValue[n],
                                sampleRate);
    
    // parameter controlled by the Action-Button (index 8)
    parameters.addParameter(parameterID[NUM_POTENTIOMETERS],
                            parameterName[NUM_POTENTIOMETERS],
                            reverbTypeNames, NUM_TYPES);
    
    // parameters controlled by menu (index 9...11)
    for (unsigned int n = NUM_POTENTIOMETERS+1; n < NUM_PARAMETERS; ++n)
        parameters.addParameter(parameterID[n],
                                parameterName[n],
                                parameterSuffix[n],
                                parameterMin[n],
                                parameterMax[n],
                                parameterStep[n],
                                parameterInitialValue[n],
                                sampleRate);
    
    // special cases: scaling and ramps:
    static_cast<SlideParameter*>(parameters.getParameter("reverb_highcut"))->setScaling(SlideParameter::Scaling::FREQ);
    static_cast<SlideParameter*>(parameters.getParameter("reverb_lowcut"))->setScaling(SlideParameter::Scaling::FREQ);
    static_cast<SlideParameter*>(parameters.getParameter("reverb_multfreq"))->setScaling(SlideParameter::Scaling::FREQ);
    static_cast<SlideParameter*>(parameters.getParameter("reverb_modrate"))->setScaling(SlideParameter::Scaling::FREQ);
    static_cast<SlideParameter*>(parameters.getParameter("reverb_decay"))->setScaling(SlideParameter::Scaling::FREQ);
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

