#include "effects.hpp"

// MARK: - BEATREPEAT
// ********************************************************************************

const uint EffectProcessor::RAMP_BLOCKSIZE = 1;
const uint EffectProcessor::RAMP_BLOCKSIZE_WRAP = RAMP_BLOCKSIZE - 1;

EffectProcessor::EffectProcessor(AudioParameterGroup* engineParameters_,
                const unsigned int numParameters_, const String& name_,
                const float sampleRate_, const unsigned int blockSize_)
    : id(name_)
    , sampleRate(sampleRate_)
    , blockSize(blockSize_)
    , parameters(name_, numParameters_)
    , engineParameters(engineParameters_)
{
    inputGain.setup(1.f, sampleRate, RAMP_BLOCKSIZE);
    
    wet.setup(0.f, sampleRate, RAMP_BLOCKSIZE);
}


void EffectProcessor::parameterChanged(AudioParameter *param_)
{
    // TODO: no safety guards here
    engage(param_->getValueAsInt());
}


void EffectProcessor::engage(bool engaged_)
{
    if (engaged_) inputGain.setRampTo(1.f, 0.35f);
    
    else inputGain.setRampTo(0.f, 0.1f);
}


void EffectProcessor::updateRamps()
{
    if (!inputGain.rampFinished) inputGain.processRamp();
    
    if (!wet.rampFinished)
    {
        wet.processRamp();
        
        dry = 1.f - wet();
    }
}


// MARK: - BEATREPEAT
// ********************************************************************************

void ReverbProcessor::setup()
{
    reverb.setup(sampleRate, blockSize);
    
    initializeParameters();
    initializeListeners();
}


StereoFloat ReverbProcessor::processAudioSamples(const StereoFloat input_, const uint sampleIndex_)
{
    if ((sampleIndex_ & RAMP_BLOCKSIZE_WRAP) == 0) updateRamps();
    
    StereoFloat drySignal = input_ * dry;
    
    StereoFloat wetSignal = reverb.processAudioSamples(input_ * inputGain(), sampleIndex_) * wet();
        
    return wetSignal + drySignal;
}


void ReverbProcessor::updateAudioBlock()
{
    
}


void ReverbProcessor::initializeParameters()
{
    using namespace Reverberation;
    
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

void ReverbProcessor::initializeListeners()
{
    for (unsigned int n = 0; n < Reverberation::NUM_PARAMETERS; ++n)
    {
        auto param = parameters.getParameter(n);
        
        if (param->getID() != "reverb_wetness")
        {
            param->onChange.push_back([this, param] {
                reverb.parameterChanged(param->getID(), param->getValueAsFloat());
            });
        }
    }
    
    parameters.getParameter("reverb_wetness")->addListener(this);
}


void ReverbProcessor::parameterChanged(AudioParameter *param_)
{
    if (param_->getID() == "effect1_engaged")
    {
        engage(param_->getValueAsInt());
    }
    
    else if (param_->getID() == "reverb_wetness")
    {
        wet.setRampTo(0.01f * param_->getValueAsFloat(), 0.05f);
        dry = 1.f - wet();
    }
}


// MARK: - GRANULATOR
// ********************************************************************************


void GranulatorProcessor::setup()
{
    initializeParameters();
    initializeListeners();
}


StereoFloat GranulatorProcessor::processAudioSamples(const StereoFloat input_, const uint sampleIndex_)
{
    // process ramps
//    parameters.getParameter(GRAN1)->process();
//    parameters.getParameter(GRAN2)->process();

    StereoFloat effect = input_;
    
//    rt_printf("processing effect %s\n", id.c_str());
    
    return effect;
}

void GranulatorProcessor::updateAudioBlock()
{}

void GranulatorProcessor::initializeParameters()
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

void GranulatorProcessor::initializeListeners()
{
    
}

// MARK: - GRANULATOR
// ********************************************************************************

StereoFloat ResonatorProcessor::processAudioSamples(const StereoFloat input_, const uint sampleIndex_)
{
    // process ramps
    // ...
    
//    rt_printf("processing effect %s\n", id.c_str());

    StereoFloat effect = input_;
    
    return effect;
}

void ResonatorProcessor::updateAudioBlock()
{}

void ResonatorProcessor::initializeParameters()
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

void ResonatorProcessor::initializeListeners()
{
    
}

