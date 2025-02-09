#include "EffectProcessor.hpp"

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
    wetGain.setup(1.f, sampleRate, RAMP_BLOCKSIZE);
    dryGain = 0.f;

    muteGain.setup(1.f, sampleRate, RAMP_BLOCKSIZE);
}


void EffectProcessor::parameterChanged(AudioParameter *param_)
{
    // Check if "engage" is in the string
    std::string paramID = param_->getID();
    std::string check = "engage";
    if (paramID.find(check) == std::string::npos)
        engine_rt_error("The Parameter with ID: " + paramID + " is not allowed to change the engagement of an effect.",
                        __FILE__, __LINE__, true);
        
    engage(param_->getValueAsInt());
}


void EffectProcessor::engage(bool engaged_)
{
    if (engaged_) muteGain.setRampTo(1.f, 0.05f);
    
    else muteGain.setRampTo(0.f, 0.05f);
}


void EffectProcessor::setMix(const float mixGain_)
{
    wetGain.setRampTo(mixGain_, 0.05f);
}


void EffectProcessor::setExecutionFlow(const ExecutionFlow flow_)
{
    isProcessedIn = flow_;
}


void EffectProcessor::updateRamps()
{
    if (!muteGain.rampFinished) muteGain.processRamp();
    
    if (!wetGain.rampFinished)
    {
        wetGain.processRamp();
        
        dryGain = getDryAmount(wetGain());
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


float32x2_t ReverbProcessor::processAudioSamples(const float32x2_t input_, const uint sampleIndex_)
{
    // process ramps in a predefined rate
    if ((sampleIndex_ & RAMP_BLOCKSIZE_WRAP) == 0) updateRamps();
    
    if (isProcessedIn == PARALLEL)
    {
        if (muteGain() <= 0.f || wetGain() <= 0.f)
            if (averager.isNearZero())
                return vdup_n_f32(0.f);
        
        // input = input * muteGain * wetGain
        float32x2_t input = vmul_n_f32(input_, muteGain());
        input = vmul_n_f32(input, wetGain());
        
        // output = process(input)
        float32x2_t output = reverb.processAudioSamples(input, sampleIndex_);
        
        // averager
        averager.processAudioSamples(output);
        
        return output;
    }
    else // if (isProcessedIN == SERIES)
    {
        if (muteGain() <= 0.f || wetGain() <= 0.f)
            if (averager.isNearZero())
                return vmul_n_f32(input_, dryGain);
        
        // input = input * muteGain
        float32x2_t input = vmul_n_f32(input_, muteGain());
        
        // output = process(input) * wetgain + input_ * dryGain;
        float32x2_t output = reverb.processAudioSamples(input, sampleIndex_);
        
        averager.processAudioSamples(output);
        
        output = vmul_n_f32(output, wetGain());
        return vmla_n_f32(output, input_, dryGain);
    }
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
    
    // special cases: scaling:
    static_cast<SlideParameter*>(parameters.getParameter("reverb_highcut"))->setScaling(SlideParameter::Scaling::FREQ);
    static_cast<SlideParameter*>(parameters.getParameter("reverb_lowcut"))->setScaling(SlideParameter::Scaling::FREQ);
    static_cast<SlideParameter*>(parameters.getParameter("reverb_multfreq"))->setScaling(SlideParameter::Scaling::FREQ);
    static_cast<SlideParameter*>(parameters.getParameter("reverb_modrate"))->setScaling(SlideParameter::Scaling::FREQ);
    static_cast<SlideParameter*>(parameters.getParameter("reverb_decay"))->setScaling(SlideParameter::Scaling::FREQ);
    
    // set MIDI CC Indexi
    for (uint n = 0; n < NUM_PARAMETERS; ++n)
        parameters.getParameter(n)->setupMIDI(41 + n);
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
    if (param_->getID() == "effect3_engaged")
    {
        engage(param_->getValueAsInt());
    }
    
    else if (param_->getID() == "reverb_mix")
    {
        float raw = param_->getValueAsFloat() * 0.01f;
        float wet = sinf_neon(raw * PIo2);
        
        setMix(wet);
    }
    
    else
    {
        engine_rt_error("Effect Processor with ID '" + this->getId() + "' couldn't set parameter with ID '" + param_->getID() + "'",
                        __FILE__, __LINE__, false);
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


float32x2_t GranulatorProcessor::processAudioSamples(const float32x2_t input_, const uint sampleIndex_)
{
    // process ramps in a predefined rate
    if ((sampleIndex_ & RAMP_BLOCKSIZE_WRAP) == 0) updateRamps();
    
    if (isProcessedIn == PARALLEL)
    {
        if (muteGain() <= 0.f || wetGain() <= 0.f)
            if (averager.isNearZero())
                return vdup_n_f32(0.f);
        
        // input = input * muteGain * wetGain
        float32x2_t input = vmul_n_f32(input_, muteGain());
        input = vmul_n_f32(input, wetGain());
        
        // output = process(input)
        float32x2_t output = granulator.processAudioSamples(input, sampleIndex_);
        
        // averager
        averager.processAudioSamples(output);
        
        return output;
    }
    else // if (isProcessedIN == SERIES)
    {
        if (muteGain() <= 0.f || wetGain() <= 0.f)
            if (averager.isNearZero())
                return vmul_n_f32(input_, dryGain);
        
        // input = input * muteGain
        float32x2_t input = vmul_n_f32(input_, muteGain());
        
        // output = process(input) * wetgain + input_ * dryGain;
        float32x2_t output = granulator.processAudioSamples(input, sampleIndex_);
        
        averager.processAudioSamples(output);
        
        output = vmul_n_f32(output, wetGain());
        return vmla_n_f32(output, input_, dryGain);
    }
}

void GranulatorProcessor::updateAudioBlock()
{
    granulator.update();
}


void GranulatorProcessor::synchronize()
{
    granulator.resetPhase();
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
    parameters.addParameter<SlideParameter>(n, parameterID[n],
                                            parameterName[n],
                                            parameterSuffix[n],
                                            parameterMin[n],
                                            parameterMax[n],
                                            parameterStep[n],
                                            parameterInitialValue[n],
                                            sampleRate);
    
    n = 12;
    parameters.addParameter<ChoiceParameter>(n, parameterID[n], parameterName[n], std::initializer_list<String>{ "-24db/oct", "-12dB/oct" });
    
    n = 13;
    parameters.addParameter<ChoiceParameter>(n, parameterID[n], parameterName[n], envelopeTypeNames, numEnvelopeTypes);
    
    // special cases: scaling and ramps:
    static_cast<SlideParameter*>(parameters.getParameter("granulator_density"))->setScaling(SlideParameter::Scaling::FREQ);
    
    // set MIDI CC Indexi
    for (uint n = 0; n < NUM_PARAMETERS; ++n)
        parameters.getParameter(n)->setupMIDI(21 + n);
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
        float raw = param_->getValueAsFloat() * 0.01f;
        float wet = sinf_neon(raw * PIo2);
        
        setMix(wet);
    }
    
    else
    {
        engine_rt_error("Effect Processor with ID '" + this->getId() + "' couldn't set parameter with ID '" + param_->getID() + "'",
                        __FILE__, __LINE__, false);
    }
}


// =======================================================================================
// MARK: - RINGMODULATOR
// =======================================================================================

void RingModulatorProcessor::setup()
{
    ringModulator.setup(sampleRate, blockSize);
    
    initializeParameters();
    initializeListeners();
}


float32x2_t RingModulatorProcessor::processAudioSamples(const float32x2_t input_, const uint sampleIndex_)
{
    // process ramps in a predefined rate
    if ((sampleIndex_ & RAMP_BLOCKSIZE_WRAP) == 0) updateRamps();
    
    if (isProcessedIn == PARALLEL)
    {
        // since this effect doesnt have any feedbacks or delays, we can skip the process function if
        // mute is enabled or wet == 0
        if (muteGain() <= 0.f || wetGain() <= 0.f) return vdup_n_f32(0.f);
        
        // input = input * muteGain * wetGain
        float32x2_t input = vmul_n_f32(input_, muteGain());
        input = vmul_n_f32(input, wetGain());
        
        // output = process(input)
        return ringModulator.processAudioSamples(input, sampleIndex_);
    }
    
    else // if (isProcessedIN == SERIES)
    {
        // since this effect doesnt have any feedbacks or delays, we can skip the process function if
        // mute is enabled or wet == 0
        if (muteGain() <= 0.f || wetGain() <= 0.f) return vmul_n_f32(input_, dryGain);
        
        // input = input * muteGain
        float32x2_t input = vmul_n_f32(input_, muteGain());
        
        // output = process(input) * wetgain + input_ * dryGain;
        float32x2_t output = vmul_n_f32(ringModulator.processAudioSamples(input, sampleIndex_), wetGain());
        return vmla_n_f32(output, input_, dryGain);
    }
}


void RingModulatorProcessor::updateAudioBlock()
{
    ringModulator.updateAudioBlock();
}


void RingModulatorProcessor::synchronize()
{
    ringModulator.resetPhases();
}


void RingModulatorProcessor::initializeParameters()
{
    using namespace RingModulation;
    
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
                                             waveformNames, NUM_WAVEFORMS);
    
    // special cases: scaling
    static_cast<SlideParameter*>(parameters.getParameter("ringmod_tune"))->setScaling(SlideParameter::Scaling::FREQ);
    static_cast<SlideParameter*>(parameters.getParameter("ringmod_rate"))->setScaling(SlideParameter::Scaling::FREQ);
    
    // set MIDI CC Indexi
    for (uint n = 0; n < NUM_PARAMETERS; ++n)
        parameters.getParameter(n)->setupMIDI(1 + n);
}


void RingModulatorProcessor::initializeListeners()
{
    for (unsigned int n = 0; n < RingModulation::NUM_PARAMETERS; ++n)
    {
        auto param = parameters.getParameter(n);
        
        if (param->getID() != "ringmod_mix")
        {
            param->onChange.push_back([this, param] {
                ringModulator.parameterChanged(param->getID(), param->getValueAsFloat());
            });
        }
    }
    
    parameters.getParameter("ringmod_mix")->addListener(this);
}


void RingModulatorProcessor::parameterChanged(AudioParameter *param_)
{
    if (param_->getID() == "effect1_engaged")
    {
        engage(param_->getValueAsInt());
    }
    
    else if (param_->getID() == "ringmod_mix")
    {
        float raw = param_->getValueAsFloat() * 0.01f;
        float wet = sinf_neon(raw * PIo2);
        
        setMix(wet);
    }
    
    else
    {
        engine_rt_error("Effect Processor with ID '" + this->getId() + "' couldn't set parameter with ID '" + param_->getID() + "'",
                        __FILE__, __LINE__, false);
    }
}
