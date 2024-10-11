#include "effects.hpp"

// =======================================================================================
// MARK: - EFFECT PROCESSOR
// =======================================================================================

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
    inputGainCache = inputGain();
    
    wet.setup(1.f, sampleRate, RAMP_BLOCKSIZE);
}


void EffectProcessor::parameterChanged(AudioParameter *param_)
{
    // TODO: no safety guards here
    engage(param_->getValueAsInt());
}


void EffectProcessor::engage(bool engaged_)
{
    if (engaged_) inputGain.setRampTo(inputGainCache, 0.35f);
    
    else 
    {
        inputGainCache = inputGain();
        inputGain.setRampTo(0.f, 0.1f);
    }
}


void EffectProcessor::setInputGain(const float inGain_)
{
    inputGain.setRampTo(inGain_, 0.05f);
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


// =======================================================================================
// MARK: - REVERB
// =======================================================================================

void ReverbProcessor::setup()
{
    reverb.setup(sampleRate, blockSize);
    
    initializeParameters();
    initializeListeners();
}


StereoFloat ReverbProcessor::processAudioSamples(const StereoFloat input_, const uint sampleIndex_)
{
    if ((sampleIndex_ & RAMP_BLOCKSIZE_WRAP) == 0) updateRamps();
    
//    StereoFloat drySignal = input_ * dry;
//    
//    StereoFloat wetSignal = reverb.processAudioSamples(input_ * inputGain(), sampleIndex_) * wet();
//        
//    return wetSignal + drySignal;
    
    return reverb.processAudioSamples(input_ * inputGain(), sampleIndex_);
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
        
        if (param->getID() != "reverb_mix")
        {
            param->onChange.push_back([this, param] {
                reverb.parameterChanged(param->getID(), param->getValueAsFloat());
            });
        }
    }
    
    parameters.getParameter("reverb_mix")->addListener(this);
}


void ReverbProcessor::parameterChanged(AudioParameter *param_)
{
    if (param_->getID() == "effect1_engaged")
    {
        engage(param_->getValueAsInt());
    }
    
    else if (param_->getID() == "reverb_mix")
    {
        setInputGain(param_->getValueAsFloat() * 0.01f);
//        wet.setRampTo(0.01f * param_->getValueAsFloat(), 0.05f);
//        dry = 1.f - wet();
    }
}


// =======================================================================================
// MARK: - GRANULATOR
// =======================================================================================


void GranulatorProcessor::setup()
{
    granulator.setup(sampleRate, blockSize);
    
    initializeParameters();
    initializeListeners();
}


StereoFloat GranulatorProcessor::processAudioSamples(const StereoFloat input_, const uint sampleIndex_)
{
    if ((sampleIndex_ & RAMP_BLOCKSIZE_WRAP) == 0) updateRamps();
    
//    StereoFloat drySignal = input_ * dry;
//    
//    StereoFloat wetSignal = granulator.processAudioSamples(input_ * inputGain(), sampleIndex_) * wet();
//    
//    return wetSignal + drySignal;
    
    return granulator.processAudioSamples(input_ * inputGain(), sampleIndex_);
}


void GranulatorProcessor::updateAudioBlock()
{
    granulator.update();
}


void GranulatorProcessor::initializeParameters()
{
    using namespace Granulation;
    
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
                                             std::initializer_list<String>{"OFF", "ON"});
    
    // parameters controlled by menu (index 9...)
    uint n = 9;
    parameters.addParameter<ChoiceParameter>(n, parameterID[n], parameterName[n], delaySpeedRatios, numDelaySpeedRatios);
    
    n = 10;
    parameters.addParameter<SlideParameter>(n, parameterID[n],
                                            parameterName[n],
                                            parameterSuffix[n],
                                            parameterMin[n],
                                            parameterMax[n],
                                            parameterStep[n],
                                            parameterInitialValue[n],
                                            sampleRate);
    
    n = 11;
    parameters.addParameter<ChoiceParameter>(n, parameterID[n], parameterName[n], std::initializer_list<String>{ "Moog Ladder -24db", "Moog Half Ladder -12dB" });
    
    // special cases: scaling and ramps:
    static_cast<SlideParameter*>(parameters.getParameter("granulator_density"))->setScaling(SlideParameter::Scaling::FREQ);
}

void GranulatorProcessor::initializeListeners()
{
    for (unsigned int n = 0; n < Granulation::NUM_PARAMETERS; ++n)
    {
        auto param = parameters.getParameter(n);
        
        if (param->getID() != "granulator_mix")
        {
            param->onChange.push_back([this, param] {
                granulator.parameterChanged(param->getID(), param->getValueAsFloat());
            });
        }
    }
    
    parameters.getParameter("granulator_mix")->addListener(this);
}


void GranulatorProcessor::parameterChanged(AudioParameter *param_)
{
    if (param_->getID() == "effect2_engaged")
    {
        engage(param_->getValueAsInt());
    }
    
    else if (param_->getID() == "granulator_mix")
    {
        setInputGain(param_->getValueAsFloat() * 0.01f);
        
//        wet.setRampTo(0.01f * param_->getValueAsFloat(), 0.05f);
//        dry = 1.f - wet();
    }
}


// =======================================================================================
// MARK: - RESONATOR
// =======================================================================================

StereoFloat ResonatorProcessor::processAudioSamples(const StereoFloat input_, const uint sampleIndex_)
{
    // process ramps
    // ...
    
//    rt_printf("processing effect %s\n", id.c_str());

    StereoFloat effect = { 0.f, 0.f };
    
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

