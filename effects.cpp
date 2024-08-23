#include "effects.hpp"

// MARK: - BEATREPEAT
// ********************************************************************************

Effect::Effect(AudioParameterGroup* engineParameters_,
                const unsigned int numParameters_, const String& name_,
                const float sampleRate_, const unsigned int blockSize_)
    : id(name_)
    , parameters(name_, numParameters_)
    , engineParameters(engineParameters_)
    , sampleRate(sampleRate_)
    , blockSize(blockSize_)
{}


void Effect::parameterChanged(AudioParameter *param_)
{
    // TODO: no safety guards here
    engage(param_->getValueAsInt());
}


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
    
//    rt_printf("processing effect %s\n", id.c_str());
    
    return effect;
}


void Reverb::updateAudioBlock()
{
    
}


void Reverb::initializeParameters()
{
    using namespace GrainmotherReverb;
    
    // parameters controlled by potentiometers/sliders (index 0...7)
    for (unsigned int n = 0; n < NUM_POTENTIOMETERS; ++n)
        parameters.addParameter<SlideParameter>(n, parameterID[n],
                                                parameterName[n],
                                                parameterSuffix[n],
                                                parameterMin[n],
                                                parameterMax[n],
                                                parameterStep[n],
                                                parameterInitialValue[n],
                                                sampleRate);
    
    // parameter controlled by the Action-Button (index 8)
    parameters.addParameter<ChoiceParameter>(NUM_POTENTIOMETERS,
                                             parameterID[NUM_POTENTIOMETERS],
                                             parameterName[NUM_POTENTIOMETERS],
                                             reverbTypeNames, NUM_TYPES);
    
    // parameters controlled by menu (index 9...11)
    for (unsigned int n = NUM_POTENTIOMETERS+1; n < NUM_PARAMETERS; ++n)
        parameters.addParameter<SlideParameter>(n, parameterID[n],
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


void Granulator::setup()
{
    initializeParameters();
    initializeListeners();
}


StereoFloat Granulator::processAudioSamples(const StereoFloat input_)
{
    // process ramps
//    parameters.getParameter(GRAN1)->process();
//    parameters.getParameter(GRAN2)->process();

    StereoFloat effect = input_;
    
//    rt_printf("processing effect %s\n", id.c_str());
    
    return effect;
}

void Granulator::updateAudioBlock()
{}

void Granulator::initializeParameters()
{
    using namespace GrainmotherGranulator;
    
    // parameters controlled by potentiometers/sliders (index 0...7)
    for (unsigned int n = 0; n < NUM_POTENTIOMETERS; ++n)
        parameters.addParameter<SlideParameter>(n, parameterID[n],
                                                parameterName[n],
                                                parameterSuffix[n],
                                                parameterMin[n],
                                                parameterMax[n],
                                                parameterStep[n],
                                                parameterInitialValue[n],
                                                sampleRate);
    
    // parameter controlled by the Action-Button (index 8)
    parameters.addParameter<ButtonParameter>(NUM_POTENTIOMETERS,
                                             parameterID[NUM_POTENTIOMETERS],
                                             parameterName[NUM_POTENTIOMETERS],
                                             std::initializer_list<String>{"Off", "On"});
    
    // parameters controlled by menu (index 9...11)
    for (unsigned int n = NUM_POTENTIOMETERS+1; n < NUM_PARAMETERS; ++n)
        parameters.addParameter<SlideParameter>(n, parameterID[n],
                                                parameterName[n],
                                                parameterSuffix[n],
                                                parameterMin[n],
                                                parameterMax[n],
                                                parameterStep[n],
                                                parameterInitialValue[n],
                                                sampleRate);
    
    // special cases: scaling and ramps:
    static_cast<SlideParameter*>(parameters.getParameter("gran_density"))->setScaling(SlideParameter::Scaling::FREQ);
    static_cast<SlideParameter*>(parameters.getParameter("gran_highcut"))->setScaling(SlideParameter::Scaling::FREQ);
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
    
//    rt_printf("processing effect %s\n", id.c_str());

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

