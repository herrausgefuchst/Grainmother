#include "Reverberation.h"

// =======================================================================================
// MARK: - Early Reflections
// =======================================================================================

using namespace Reverberation;

void EarlyReflections::setup(const float& sampleRate_, const float& blockSize_)
{
    // error handling
    if (!typeParameters) rt_printf("early reflection type parameters = nullptr");
    
    // setup tap delay (room, predelay, size)
    tapDelay.setup(typeParameters->room, 0.f, 1.f, blockSize_);
    
    // setup lowpass (feedbackgain)
    lowpass.setup(typeParameters->damping);
    
    // setup allpass filters (gain, delay, samplerate)
    allpass.filters[0].setup(typeParameters->diffusion, 2.1043f, sampleRate_);
    allpass.filters[1].setup(typeParameters->diffusion, 3.26291f, sampleRate_);
    allpass.update();
    
    // setup Ramp Parameters (start, samplerate, blocksize, isProcessingBlockwise)
    float initialSize = parameterInitialValue[static_cast<int>(Parameters::SIZE)] * 0.01f;
    float initialPreDelay = parameterInitialValue[static_cast<int>(Parameters::PREDELAY)] * sampleRate_ * 0.001f;
    float initialFeedback = parameterInitialValue[static_cast<int>(Parameters::FEEDBACK)];
    
    parameters.size.setup(initialSize, sampleRate_, RAMP_UPDATE_RATE, true);
    parameters.predelay.setup(initialPreDelay, sampleRate_, RAMP_UPDATE_RATE, true);
    parameters.feedback.setup(initialFeedback, sampleRate_, RAMP_UPDATE_RATE, true);
}


void EarlyReflections::updateRamps()
{
    bool tapRampsProcessed = false;
    
    // size ramp
    if (!parameters.size.rampFinished)
        tapRampsProcessed = parameters.size.processRamp();
    
    // predelay ramp
    if (!parameters.predelay.rampFinished)
        tapRampsProcessed = parameters.predelay.processRamp();
    
    // if any change occured: recalculate the delay values for the tapdelay
    if (tapRampsProcessed && typeParameters)
        tapDelay.recalculateTapDelays(typeParameters->room, parameters.predelay(), parameters.size());
    
    if (!parameters.feedback.rampFinished)
    {
        parameters.feedback.processRamp();
        parameters.feedbackEnabled = parameters.feedback() == 0.f ? false : true;
    }
}


// MARK: processAudioSamples
// ------------------------------------------------------------------------------
float32x2_t EarlyReflections::processAudioSamples(const float32x2_t input_, const unsigned int& sampleIndex_)
{
    // --- update ramps blockwise
    if ((sampleIndex_ & (RAMP_UPDATE_RATE-1)) == 0) updateRamps();
    
    // --- read tap delay
    std::array<std::array<float32x4_t, NUM_TAPS/4>, 2>* taps = &tapDelay.readTaps();
    
    // --- the new input for the tapdelay is:
    float32x2_t delayInput = input_;
    
    // 1. the allpassed input sample
    if (allpass.filters[0].enabled)
        allpass.processAudioSamples(delayInput);
    
    // increment write Pointer of Allpassfilter
    AllpassFilterMono::incrementWritePointer();
    
    // 2. plus a definable amount of feedback times the 4th early reflection in the tapdelay
    if (parameters.feedbackEnabled)
    {
        float32x2_t feedbackTap = { tapDelay.getTapAtIndex(0, 3), tapDelay.getTapAtIndex(1, 3) };
        delayInput = vmla_n_f32(delayInput, feedbackTap, parameters.feedback());
    }

    // 3. lowpassed by a one pole lowpass filter
    if (lowpass.enabled) lowpass.processAudioSamples(delayInput);

    // --- write tap delay
    tapDelay.writeBuffer({ vget_lane_f32(delayInput, 0), vget_lane_f32(delayInput, 1) });

    // --- multiply taps with the respective pan-values, 12 taps in 3 float32x4_t vectors
    float32x2_t output = vdup_n_f32(0.f);
    #pragma unroll
    for (unsigned int n = 0; n < 3; ++n)
    {
        // get taps
        float32x4_t tapsL_v = taps->at(0).at(n);
        float32x4_t tapsR_v = taps->at(1).at(n);
        
        // left output = left taps * left panning + right taps * left panning
        float32x4_t outL_v = vmulq_f32(tapsL_v, typeParameters->panL[0][n]);
        outL_v = vmlaq_f32(outL_v, tapsR_v, typeParameters->panL[1][n]);
        
        // right output = left taps * right panning + right taps * right panning
        float32x4_t outR_v = vmulq_f32(tapsL_v, typeParameters->panR[0][n]);
        outR_v = vmlaq_f32(outR_v, tapsR_v, typeParameters->panR[1][n]);
        
        // sum all values of the vectors
        float32x2_t sumL = vadd_f32(vget_low_f32(outL_v), vget_high_f32(outL_v));
        float32x2_t sumR = vadd_f32(vget_low_f32(outR_v), vget_high_f32(outR_v));
        float32x2_t sum = vpadd_f32(sumL, sumR);

        // add them to the output vector
        output = vadd_f32(output, sum);
    }
        
    // --- scale output, 0.83f is a scale parameter found by experimenting with in and output gain
    output = vmul_n_f32(output, 0.83f);
    
    return output;
}


void EarlyReflections::setParameters(const EarlyReflectionsParameters& parameters_)
{
    // predelay: ramp to new target
    if (parameters.predelay != parameters_.predelay)
        parameters.predelay.setRampTo(parameters_.predelay(), 0.03f);
    
    // size: ramp to new target
    if (parameters.size != parameters_.size)
        parameters.size.setRampTo(parameters_.size(), 0.03f);
    
    if (parameters.feedback != parameters_.feedback)
        parameters.feedback.setRampTo(parameters_.feedback(), 0.02f);
    
    parameters = parameters_;
}


void EarlyReflections::setTypeParameters(const EarlyReflectionsTypeParameters& typeParameters_)
{
    // reset type parameters
    typeParameters = nullptr;
    
    // alligned allocation of type parameters
    void* rawPointer = aligned_alloc(alignof(float32x4_t), sizeof(EarlyReflectionsTypeParameters));
    EarlyReflectionsTypeParameters* instancePointer = new (rawPointer) EarlyReflectionsTypeParameters(typeParameters_);
    typeParameters = EarlyReflectionsTypeParametersPtr(instancePointer);

    // update tap delay
    tapDelay.recalculateTapDelays(typeParameters->room, parameters.predelay(), parameters.size());
    
    // update lowpass
    lowpass.setFeedbackGain(typeParameters->damping);
    
    // update allpass filters
    allpass.filters[0].setFeedbackGain(typeParameters->diffusion);
    allpass.filters[1].setFeedbackGain(typeParameters->diffusion);
    allpass.update();
}


// =======================================================================================
// MARK: - Decay
// =======================================================================================

void Decay::setup(const DecayParameters& params_, const float& sampleRate_, const unsigned int& blocksize_)
{
    // --- setup samplerate parameters
    fs_inv = 1.f / sampleRate_;
    samplesPerMs_inv = 1.f / (sampleRate_ * 0.001f);
    
    // --- initialize arrays of filters
    if (typeParameters.numPreAllpassFilters > 0) allpassFiltersPre.reset(new AllpassFilterStereo[typeParameters.numPreAllpassFilters]);
    if (typeParameters.numPostAllpassFilters > 0) allpassFiltersPost.reset(new AllpassFilterStereo[typeParameters.numPostAllpassFilters]);
    if (typeParameters.numCombFilters > 0) combFilters = createAlignedCombFilters(typeParameters.halfNumCombFilters);
    
    // --- setup combfilters
    for (unsigned int n = 0; n < typeParameters.numCombFilters; ++n)
        combFilters[n/2].filters[n%2].setup(typeParameters.combDelaySamples[n], typeParameters.damping, sampleRate_, false);
    calcAndSetCombFilterGains(params_.decayTimeMs);
    
    // --- setup allpassfilters
    if (allpassFiltersPre)
        for (unsigned int n = 0; n < typeParameters.numPreAllpassFilters; ++n)
            allpassFiltersPre[n].setup(typeParameters.diffusion, typeParameters.allpassPreDelaySamples[n], sampleRate_);
    
    if (allpassFiltersPost)
        for (unsigned int n = 0; n < typeParameters.numPostAllpassFilters; ++n)
            allpassFiltersPost[n].setup(typeParameters.diffusion, typeParameters.allpassPostDelaySamples[n], sampleRate_);
 
    parameters.modulationDepth.setup(parameterInitialValue[static_cast<int>(Parameters::MODDEPTH)] * 0.5f, sampleRate_, RAMP_UPDATE_RATE, true);

    // --- copy set of Parameters
    setParameters(params_);
}

void Decay::updateRamps()
{
    if (!parameters.modulationDepth.rampFinished)
        parameters.modulationDepth.processRamp();
}

// MARK: processAudioSamples
// ------------------------------------------------------------------------------
float32x2_t Decay::processAudioSamples(const float32x2_t input_, const unsigned int& sampleIndex_)
{
    
    
    if ((sampleIndex_ & (RAMP_UPDATE_RATE-1)) == 0)
    {
        updateRamps();
    }
    
    // --- modulation
    // processes every 8th sample only (if rate is changed, need to change the increment calculations as well)
    if ((sampleIndex_ & (LFO_UPDATE_RATE-1)) == 0)
    {
        if (typeParameters.allpassModulationEnabled)
        {
            if (typeParameters.allpassPreEnabled)
                for (unsigned int n = 0; n < typeParameters.numPreAllpassFilters; ++n)
                    allpassFiltersPre[n].updateLFO(typeParameters.allpassModulationIncr, typeParameters.allpassModulationDepth);
            
            if (typeParameters.allpassPostEnabled)
                for (unsigned int n = 0; n < typeParameters.numPostAllpassFilters; ++n)
                    allpassFiltersPost[n].updateLFO(typeParameters.allpassModulationIncr, typeParameters.allpassModulationDepth);
        }
        
        if (modulationEnabled)
            for (unsigned int n = 0; n < typeParameters.numCombFilters; ++n)
                combFilters[n/2].filters[n&1].updateLFO(modulationIncr, parameters.modulationDepth());
    }
    
    // --- Load samples into NEON registers
    float32x2_t input = input_;
    float32x2_t output = vdup_n_f32(0.f);
    
    // --- process allpass filters PRE
    if (typeParameters.allpassPreEnabled)
        for (unsigned int n = 0; n < typeParameters.numPreAllpassFilters; ++n)
            allpassFiltersPre[n].processAudioSamples(input);
    
    // --- process comb filters in parallel
    for (unsigned int n = 0; n < typeParameters.halfNumCombFilters; ++n)
        output = vadd_f32(output, combFilters[n].processAudioSampleInParallel(input));
    
    // scale the sum of combfilters
    output = vmul_n_f32(output, combFilterScaler);
    
    // --- process allpass filters POST
    if (typeParameters.allpassPostEnabled)
        for (unsigned int n = 0; n < typeParameters.numPostAllpassFilters; ++n)
            allpassFiltersPost[n].processAudioSamples(output);

    // increment static write pointers
    CombFilterStereo::incrementWritePointer();
    AllpassFilterStereo::incrementWritePointer();
    
    return output;
}


void Decay::setParameters(const DecayParameters& parameters_)
{
    // decayTime: recalculation of all feedback gains of combfilters
    if (parameters.decayTimeMs != parameters_.decayTimeMs)
    {
        calcAndSetCombFilterGains(parameters_.decayTimeMs);
    }
    
    // modulation rate
    if (parameters.modulationRate != parameters_.modulationRate)
    {
        // calculate increment: 2 * PI * f / fs * rate
        // rate is the time in samples that the lfo value stays the same, a good choice for efficiency and acceptable audio quality is rate = 8
        modulationIncr = TWOPI * parameters_.modulationRate * fs_inv * 8.f;
    }
    
    // modulation depth
    if (parameters.modulationDepth != parameters_.modulationDepth)
    {
        // set flag, doesn't need to process if values are 0
        if (parameters_.modulationDepth() == 0.f) modulationEnabled = false;
        else modulationEnabled = true;
        
        // if modulation just turned off: need to reset the readpointer of the combfilters
        if (!modulationEnabled)
            for (unsigned int n = 0; n < typeParameters.numCombFilters; ++n)
                combFilters[n/2].filters[n%2].stopModulating();
        
        parameters.modulationDepth.setRampTo(parameters_.modulationDepth(), 0.03f);
    }
    
    parameters = parameters_;
}


void Decay::calcAndSetCombFilterGains(float decayTimeMs_)
{
    // helper
    float decayTimeMs_inv = 1.f / decayTimeMs_;
    
    // reset scaler
    combFilterScaler = 0.f;
        
    // do this for every combfilter
    for (unsigned int n = 0; n < typeParameters.numCombFilters; ++n)
    {
        // samples to ms
        float delayMs = typeParameters.combDelaySamples[n] * samplesPerMs_inv;
        
        // gains will be set according to the delay times in order to obtain rt60
        // g = 10 ^ (-3 * delayMs / rt60mS)
        float feedbackGain = powf_neon(10.f, -3.f * delayMs * decayTimeMs_inv);

        // comb scaler is a scale parameter found by experimenting with in and out gain
        // add the weigthed feedback gain to scaler
        combFilterScaler += typeParameters.combScaler * feedbackGain;
        
        // send gains to combfilers
        combFilters[n/2].filters[n%2].setFeedbackGain(feedbackGain);
        
        combFilters[n/2].update();
    }
    
    // inverse and bound scaler value
    combFilterScaler = 1.f / combFilterScaler;
    if (combFilterScaler > 1.f) combFilterScaler = 1.f;
}


Decay::CombFilterDualStereoPtr Decay::createAlignedCombFilters(size_t count)
{
    void* rawPointer = nullptr;
    
    // Allocate aligned memory
    posix_memalign(&rawPointer, alignof(float32x4_t), sizeof(CombFilterDualStereo) * count);

    // cast the raw pointer to a pointer of CombFilterDualStereo
    CombFilterDualStereo* filters = reinterpret_cast<CombFilterDualStereo*>(rawPointer);
    
    // Use placement new to construct the objects
    for (size_t i = 0; i < count; ++i)
        new (&filters[i]) CombFilterDualStereo();

    // Return the unique_ptr with a custom deleter
    return std::unique_ptr<CombFilterDualStereo[], AlignedDeleterArray<CombFilterDualStereo>>(filters, AlignedDeleterArray<CombFilterDualStereo>{count});
}


// =======================================================================================
// MARK: - BELA REVERB
// =======================================================================================

void Reverb::setup(const float& sampleRate_, const unsigned int& blocksize_)
{
    // generel variables
    sampleRate = sampleRate_;
    blocksize = blocksize_;
    samplesPerMs = sampleRate * 0.001f;
    
    unsigned int maxDelayOfDecay = *std::max_element(earliesLatestDelaySamples.begin(), earliesLatestDelaySamples.end()) * 3.f + 7000.f;
    delayedDecay.setup(0, maxDelayOfDecay, sampleRate);

    // sets the default reverb type and the corresponding parameters
    // for earlies and decay, creates a new decay object
    ReverbTypes initialType = static_cast<ReverbTypes>(parameterInitialValue[static_cast<int>(Parameters::TYPE)]);
    setReverbType(initialType);
    
    // setup eraly reflections
    earlyReflections.setup(sampleRate, blocksize);
    
    // setup delayline for decay
    int delayOfDecay = earlyReflections.getLatestTapDelay() - decay->getEarliestCombDelay();
    if (delayOfDecay < 0) delayOfDecay = 0;
    decayDelaySamples.setup(delayOfDecay, sampleRate, RAMP_UPDATE_RATE);
    delayedDecay.setDelay(decayDelaySamples());
    
    // setup equalizers
    float initialMultFreq = parameterInitialValue[static_cast<int>(Parameters::MULTFREQ)];
    float initialMultGain = parameterInitialValue[static_cast<int>(Parameters::MULTGAIN)];
    float initialLowcutFreq = parameterInitialValue[static_cast<int>(Parameters::LOWCUT)];
    float initialHighcutFreq = parameterInitialValue[static_cast<int>(Parameters::HIGHCUT)];
    inputMultiplier.setup(initialMultFreq, initialMultGain, 1.5f, sampleRate);
    lowcut.setup(initialLowcutFreq, sampleRate);
    highcut.setup(initialHighcutFreq, sampleRate);
}


void Reverb::updateRamps()
{
    // delay of decay ramp
    if (!decayDelaySamples.rampFinished)
    {
        decayDelaySamples.processRamp();
        delayedDecay.setDelay(decayDelaySamples());
    }
}

// MARK: processAudioSamples
// ------------------------------------------------------------------------------
StereoFloat Reverb::processAudioSamples(const StereoFloat input_, const unsigned int& sampleIndex_)
{
    if (!settingType)
    {
        if ((sampleIndex_ & (RAMP_UPDATE_RATE-1)) == 0) updateRamps();
    
        float32x2_t input = { input_.leftSample, input_.rightSample };
        float32x2_t output = input;
        
        // parametric eq shapes the input signal
        if (inputMultiplier.enabled) inputMultiplier.processAudioSamples(output);
       
        // early reflections
        output = earlyReflections.processAudioSamples(output, sampleIndex_);
        
        // decay, mixes the delayed processed decay with the early reflections
        if (decay)
        {
            // get the delayed decay values and input the momentary values simultaniously
            // a richer stereo effect is achieved by swapping the input values
            float32x2_t dcy = delayedDecay.processAudioSamples(decay->processAudioSamples(vrev64_f32(output), sampleIndex_));
            // weighted sum of earlies and decay
            output = vmul_n_f32(vadd_f32(dcy, output), 0.5f);
        }
        
        // process low and highcut
        if (lowcut.enabled) lowcut.processAudioSamples(output);
        if (highcut.enabled) highcut.processAudioSamples(output);
        
        // apply output gain compensation
        output = vmul_n_f32(output, GAIN_COMPENSATION);
        
        // return as StereoFloat
        return { vget_lane_f32(output, 0), vget_lane_f32(output, 1) };
    }
    return { 0.f, 0.f };
}


void Reverb::setReverbType(ReverbTypes type_)
{
    settingType = true;
    
    // helper
    using Room = EarlyReflectionsTypeParameters::Room;
    
    // make a new set of Decay Parameters
    DecayParameters paramsDecay;
    
    // if a decay already exists (only in setup its not the case) we can copy parameters from there
    if (decay) 
    {
        paramsDecay = decay->getParameters();
        paramsDecay.modulationDepth = decay->getParameters().modulationDepth();
    }
    
    // clear decay
    decay = nullptr;
        
    // looks for the new type, and sets the corresponding fixed type parameters
    // creates a new unique pointer to a Decay object
    switch (type_)
    {
        case ReverbTypes::CHURCH: {
            EarlyReflectionsTypeParameters earliesTypeParams
            (Room::CHURCH, // Room Type
            -0.42f, // diffusion
             0.67f, // damping
            earliesLatestDelaySamples[Room::CHURCH]); // latestDelaySamples
            
            DecayTypeParameters decayTypeParams
            ("Church", // name
            -0.83f, // diffusion
            0.27f, // damping
            8, { 3391, 3637, 3881, 4127, 4363, 4603, 4861, 5087 }, // combfilters
            0, {}, // pre-allpassfilters
            4, { 264, 74, 423, 105 }, // post-allpassfilters
            0.68f, // comb scaler
            0.59f, // apf mod rate
            6.12, // apf mod depth
            sampleRate);
            
            earlyReflections.setTypeParameters(earliesTypeParams);
            
            decay = std::make_unique<Decay>(decayTypeParams);
            
            break;
        }
        
        case ReverbTypes::DIGITALVINTAGE: {
            EarlyReflectionsTypeParameters earliesTypeParams
            (Room::SMALLROOM, // Room Type
            -0.74f, // diffusion
            0.51f, // damping
            earliesLatestDelaySamples[Room::SMALLROOM]); // latestDelaySamples
            
            earlyReflections.setTypeParameters(earliesTypeParams);
            
            DecayTypeParameters decayTypeParams
            ("Plate", // name
            -0.68f, // diffusion
            0.13f, // damping
            8, { 1847, 1979, 2111, 2239, 2371, 2503, 2633, 2767 }, // combfilters
            4, { 92, 357, 132, 339 }, // pre-allpassfilters
            4, { 264, 74, 423, 105 }, // post-allpassfilter
            0.92f, // comb scaler
            9.03f, // apf mod rate
            1.46f, // apf mod depth
            sampleRate);
            
            earlyReflections.setTypeParameters(earliesTypeParams);

            decay = std::make_unique<Decay>(decayTypeParams);
            
            break;
        }
            
        case ReverbTypes::SEASICK: {
            EarlyReflectionsTypeParameters earliesTypeParams
            (Room::SMALLROOM, // Room Type
             -0.64f, // diffusion
             0.6f, // damping
             earliesLatestDelaySamples[Room::SMALLROOM]); // latestDelaySamples
            
            earlyReflections.setTypeParameters(earliesTypeParams);
            
            DecayTypeParameters decayTypeParams
            ("Metallic Resonator", // name
            -0.94f, // diffusion
            0.1f, // damping
            4, { 3109, 3631, 4153, 4673 }, // combfilters
            8, { 264, 74, 423, 105, 366, 141, 194, 220 }, // pre-allpassfilters
            8, { 414, 92, 357, 132, 339, 264, 308, 275 }, // post-allpassfilters
            0.85f, // comb scaler
            0.28f, // apf mod rate
            49.f, // apf mod depth
            sampleRate);
            
            earlyReflections.setTypeParameters(earliesTypeParams);

            decay = std::make_unique<Decay>(decayTypeParams);
            
            break;
        }
            
        case ReverbTypes::ROOM: {
            EarlyReflectionsTypeParameters earliesTypeParams
            (Room::FOYER, // Room Type
             -0.68f, // diffusion
             0.46f, // damping
             earliesLatestDelaySamples[Room::FOYER]); // latestDelaySamples
                        
            DecayTypeParameters decayTypeParams
            ("Bathroom", // name
            -0.64f, // diffusion
            0.29f, // damping
            6, { 1759, 1933, 2113, 2293, 2467, 2647 }, // combfilters
            0, {}, // pre-allpassfilters
            3, { 414, 92, 357 }, // post-allpassfilters
            0.87f); // comb scaler
            
            earlyReflections.setTypeParameters(earliesTypeParams);
            
            decay = std::make_unique<Decay>(decayTypeParams);

            break;
        }
    }
    
    // decay setup with the UI parameters of the last decay
    decay->setup(paramsDecay, sampleRate, blocksize);
    
    // setup delayline for decay
    int delayOfDecay = earlyReflections.getLatestTapDelay() - decay->getEarliestCombDelay();
    if (delayOfDecay < 0) delayOfDecay = 0;
    decayDelaySamples = delayOfDecay;
    delayedDecay.setDelay(decayDelaySamples());
    
    settingType = false;
}


// MARK: Parameter Changed
// ------------------------------------------------------------------------------
void Reverb::parameterChanged(const std::string& parameterID, float newValue)
{
    if (parameterID == "reverb_decay")
    {
        DecayParameters params = decay->getParameters();
        params.decayTimeMs = newValue * 1000.f; // sec to ms
        decay->setParameters(params);
    }
    else if (parameterID == "reverb_predelay")
    {
        EarlyReflectionsParameters params = earlyReflections.getParameters();
        params.predelay = newValue * samplesPerMs; // ms to samples
        earlyReflections.setParameters(params);
    }
    else if (parameterID == "reverb_modrate")
    {
        DecayParameters params = decay->getParameters();
        params.modulationRate = newValue;
        decay->setParameters(params);
    }
    else if (parameterID == "reverb_moddepth")
    {
        DecayParameters params = decay->getParameters();
        params.modulationDepth = newValue * 0.5f; // % to 0...50 samples
        decay->setParameters(params);
    }
    else if (parameterID == "reverb_size")
    {
        EarlyReflectionsParameters params = earlyReflections.getParameters();
        params.size = newValue * 0.01f; // % to scaler
        earlyReflections.setParameters(params);
        
        int delayOfDecay = earlyReflections.getLatestTapDelay() - decay->getEarliestCombDelay();
        if (delayOfDecay < 0) delayOfDecay = 0;
        decayDelaySamples.setRampTo(delayOfDecay, 0.03f);
    }
    else if (parameterID == "reverb_feedback")
    {
        EarlyReflectionsParameters params = earlyReflections.getParameters();
        params.feedback = newValue;
        earlyReflections.setParameters(params);
    }
    else if (parameterID == "reverb_lowcut")
    {
        lowcut.setCutoffFrequency(newValue);
    }
    else if (parameterID == "reverb_highcut")
    {
        highcut.setCutoffFrequency(newValue);
    }
    else if (parameterID == "reverb_multfreq")
    {
        inputMultiplier.setCenterFrequency(newValue);
    }
    else if (parameterID == "reverb_multgain")
    {
        inputMultiplier.setGain(newValue);
    }
    else if (parameterID == "reverb_type")
    {
        setReverbType(static_cast<ReverbTypes>(newValue));
    }
}
