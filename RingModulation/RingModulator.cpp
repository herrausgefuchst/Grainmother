#include "RingModulator.h"

using namespace RingModulation;

// =======================================================================================
// MARK: - LFO
// =======================================================================================

void LFO::setup(const float freq_, const float sampleRate_)
{
    phase = 0.f;
    sampleRate = sampleRate_;
    invSampleRate = 1.f / sampleRate_;
    setFrequency(freq_);
}


float LFO::getNextValue()
{
    float output = (this->*getValue)() * amplitude;
    
    phase += increment;
    if (phase >= TWOPI) 
    {
        phase -= TWOPI;
        phaseWrapped = true;
    }
    
    return output;
}


float LFO::getSine()
{
    return approximateSine(phase);
}


float LFO::getTriangle()
{
    if (phase < PI) return TWOoPI * phase - 1.f;
    else return -TWOoPI * phase + 3.f;
}


float LFO::getSaw()
{
    float value = phase * PI_INV;
    if (phase > PI) value -= 2.f;
    return value;
}


float LFO::getPulse()
{
    return (phase < PI) ? 1.f : -1.f;
}


float LFO::getRandom()
{
    if (phaseWrapped)
    {
        nextValue = rand() * TWO_RAND_MAX_INVERSED - 1.f;
        phaseWrapped = false;
    }
    return nextValue;
}


void LFO::setSampleRate(const float sampleRate_)
{
    sampleRate = sampleRate_;
    invSampleRate = 1.f / sampleRate_;
    setFrequency(frequency);
}


void LFO::setFrequency(const float freq_)
{
    frequency = freq_;
    increment = TWOPI * freq_ * invSampleRate;
}


void LFO::setAmplitude(const float amp_)
{
    amplitude = amp_;
}


void LFO::setWaveform(Waveform waveform_)
{
    waveform = waveform_;
    if (waveform_ == SINE) getValue = &LFO::getSine;
    else if (waveform_ == TRIANGLE) getValue = &LFO::getTriangle;
    else if (waveform_ == SAW) getValue = &LFO::getSaw;
    else if (waveform_ == PULSE) getValue = &LFO::getPulse;
    else if (waveform_ == RANDOM) getValue = &LFO::getRandom;
    else getValue = &LFO::getSine;
}


// =======================================================================================
// MARK: - OSCILLATOR
// =======================================================================================

void Oscillator::setup(const float freq_, const float sampleRate_)
{
    phase = 0.f;
    sampleRate = sampleRate_;
    invSampleRate = 1.f / sampleRate_;
    
    setFrequency(freq_);
    
    modulator.setup(1.f, sampleRate_);
}


float32x2_t Oscillator::getNextValues()
{
    float32x2_t output;
    
    output[0] = approximateSine(phase);
    
    if (phaseIsShifted)
    {
        float shiftedPhase = phase + phaseShift;
        if (shiftedPhase >= TWOPI) shiftedPhase -= TWOPI;
        output[1] = approximateSine(shiftedPhase);
    }
    else
    {
        output[1] = output[0];
    }
    
    // modulation increment multipliere should never be 0!
    float modulation = mapValue(modulator.getNextValue() + 1.f, 0.f, 2.f, 0.00001f, 2.f);
    phase += increment * (modulation);
    if (phase >= TWOPI) phase -= TWOPI;
    
    return output;
}


void Oscillator::setSampleRate(const float sampleRate_)
{
    sampleRate = sampleRate_;
    invSampleRate = 1.f / sampleRate_;
    
    setFrequency(frequency);
    
    modulator.setSampleRate(sampleRate_);
}


void Oscillator::setFrequency(const float freq_)
{
    frequency = freq_;
    increment = TWOPI * freq_ * invSampleRate;
}


void Oscillator::setPhaseShift(const float shift_)
{
    phaseShift = shift_;
    phaseIsShifted = phaseShift > 0.f ? true : false;
}


// =======================================================================================
// MARK: - RING MODULATOR
// =======================================================================================

bool RingModulator::setup(const float sampleRate_, const uint blockSize_)
{
    using namespace RingModulation;
    
    sampleRate = sampleRate_;
    blockSize = blockSize_;
    
    modulator.setup(5.f, sampleRate_);
    
    bitCrusher.setSmoothing(30.f);
    
    // setup oversampling objects
    interpolator.setup(sampleRate_, 2, OVERSAMPLING_FILTER_LENGTH);
    decimator.setup(sampleRate_, 2, OVERSAMPLING_FILTER_LENGTH);
    
    // setup ramps
    gainCompensation.setup(1.f, sampleRate_, RAMP_UPDATE_RATE);
    phaseShift.setup(0.f, sampleRate_, RAMP_UPDATE_RATE);
    diodeSaturation.setup(0.00001f, sampleRate_, RAMP_UPDATE_RATE);
    transistorSaturation.setup(0.00001f, sampleRate_, RAMP_UPDATE_RATE);
    typeBlendingWet.setup(1.f, sampleRate_, RAMP_UPDATE_RATE);
    
    // initialize all variables with defualt parameter values
    for (uint n = 0; n < NUM_PARAMETERS; ++n)
        parameterChanged(parameterID[n], parameterInitialValue[n]);
    
    return true;
}


void RingModulator::updateAudioBlock()
{
    bitCrusher.updateAudioBlock();
}


void RingModulator::updateRamps()
{
    if (!gainCompensation.rampFinished)
    {
        gainCompensation.processRamp();
    }
    
    if (!phaseShift.rampFinished)
    {
        phaseShift.processRamp();
        modulator.setPhaseShift(phaseShift());
    }
    
    if (!diodeSaturation.rampFinished)
    {
        diodeSaturation.processRamp();
        calculateSaturationVariables();
    }
    
    if (!transistorSaturation.rampFinished)
    {
        transistorSaturation.processRamp();
        calculateSaturationVariables();
    }
    
    if (!typeBlendingWet.rampFinished)
    {
        typeBlendingWet.processRamp();
        typeBlendingDry = 1.f - typeBlendingWet();
    }
}


StereoFloat RingModulator::processAudioSamples(const StereoFloat input_, const uint sampleIndex_)
{
    // update ramps in a predefined rate
    if ((sampleIndex_ & (RingModulation::RAMP_UPDATE_RATE-1)) == 0) updateRamps();
    
    // convert StereoFloat to arm_neon vector
    float32x2_t input_vec = { input_[0], input_[1] };
    
    InterpolatorStereoOutput interpolatedOutput;
    DecimatorStereoInput decimationInput;
    
    // Upsample the incoming audio sample
    interpolatedOutput = interpolator.interpolateAudio(input_vec);
    
    // Process each upsampled audio sample (oversample ratio times)
    for (uint n = 0; n < oversampleRatio; ++n)
    {
        // Retrieve the input signal and modulator signal for ring modulation
        // process the input signal with bitcrushing first
        float32x2_t signal1 = bitCrusher.processAudioSample(interpolatedOutput.audioData[n]);
        float32x2_t signal2 = modulator.getNextValues();
        
        // Choose the ring modulation type based on the `type` parameter:
        // - TRANSISTOR: Only transistor ring modulation is applied
        // - TRANSISTOR_DIODE: Blends transistor and diode ring modulation
        // - DIODE: Only diode ring modulation is applied
        decimationInput.audioData[n] = (this->*processRingModulation)(signal1, signal2);

        // If the noise parameter is enabled, apply post noise ring modulation and blend it with the current signal
        if (noiseWet > 0.f)
        {
            float32x2_t noise = { getNoise(), getNoise() };
            float32x2_t noiseRing = vmul_f32(decimationInput.audioData[n], noise);
            noiseRing = vmul_n_f32(noiseRing, noiseWet);
            decimationInput.audioData[n] = vmla_n_f32(noiseRing, decimationInput.audioData[n], noiseDry);
        }
    }
        
    // Downsample the processed audio to the original sample rate
    float32x2_t output_vec = vmul_n_f32(decimator.decimateAudio(decimationInput), gainCompensation());
    
    // apply dry and wet
    output_vec = vmul_n_f32(output_vec, wet);
    output_vec = vmla_n_f32(output_vec, input_vec, dry);
    
    return { vget_lane_f32(output_vec, 0), vget_lane_f32(output_vec, 1) };
}


float32x2_t RingModulator::getDiodeRingModulation(const float32x2_t carrier_, const float32x2_t modulator_)
{
    // Calculate the diode input signals based on the carrier and modulator
    float32x2_t halfModulator = vmul_n_f32(modulator_, 0.5f);
    float32x2_t diodeOne = vabs_f32(vadd_f32(carrier_, halfModulator));
    float32x2_t diodeTwo = vabs_f32(vsub_f32(carrier_, halfModulator));
                
    // Process the diode saturations using the following logic:
    // For x >= 0: y(x) = tanh(saturation * x) / tanh(saturation)
    // For x <  0: y(x) = tanh((saturation / asymmetry) * x) / tanh(saturation / asymmetry)
    // no asymmetrical processing of asymmetry = 1
    diodeOne[0] = (diodeOne[0] >= 0.f)
        ? approximateTanh(diodeSaturation() * diodeOne[0]) * tanhDiodeSaturation_inversed
        : approximateTanh(diodeSatuaration_o_Asymmetry[0] * diodeOne[0]) * tanhDiodeSaturationAsym_inversed[0];
    
    diodeOne[1] = (diodeOne[1] >= 0.f)
        ? approximateTanh(diodeSaturation() * diodeOne[1]) * tanhDiodeSaturation_inversed
        : approximateTanh(diodeSatuaration_o_Asymmetry[0] * diodeOne[1]) * tanhDiodeSaturationAsym_inversed[0];
    
    diodeTwo[0] = (diodeTwo[0] >= 0.f)
        ? approximateTanh(diodeSaturation() * diodeTwo[0]) * tanhDiodeSaturation_inversed
        : approximateTanh(diodeSatuaration_o_Asymmetry[1] * diodeTwo[0]) * tanhDiodeSaturationAsym_inversed[1];
    
    diodeTwo[1] = (diodeTwo[1] >= 0.f)
        ? approximateTanh(diodeSaturation() * diodeTwo[1]) * tanhDiodeSaturation_inversed
        : approximateTanh(diodeSatuaration_o_Asymmetry[1] * diodeTwo[1]) * tanhDiodeSaturationAsym_inversed[1];
    
    // Return the difference between the two processed diode signals
    return vsub_f32(diodeOne, diodeTwo);
}


float32x2_t RingModulator::getTransistorRingModulation(const float32x2_t carrier_, const float32x2_t modulator_)
{
    // Precalculate the saturated component using the carrier and modulator inputs
    float32x2_t saturated = vmla_n_f32(carrier_, modulator_, a2);

    // Apply saturation processing based on the following conditions:
    // For x >= 0: y(x) = tanh(saturation * x) / tanh(saturation)
    // For x <  0: y(x) = tanh((saturation / asymmetry) * x) / tanh(saturation / asymmetry)
    // Note: Asymmetry processing is bypassed if asymmetry = 1
    saturated[0] = (saturated[0] >= 0.f)
        ? approximateTanh(transistorSaturation() * saturated[0]) * tanhTransistorSaturation_inversed
        : approximateTanh(transistorSaturation_o_Asymmetry * saturated[0]) * tanhTransistorSaturationAsym_inversed;
    
    saturated[1] = (saturated[1] >= 0.f)
        ? approximateTanh(transistorSaturation() * saturated[1]) * tanhTransistorSaturation_inversed
        : approximateTanh(transistorSaturation_o_Asymmetry * saturated[1]) * tanhTransistorSaturationAsym_inversed;

    // Final computation of the output based on the formula:
    // f(x, y) = (x + a1 * y) * sat(y + a2 * x) + a3 * y + a4 * x
    float32x2_t output = vmul_f32(vmla_n_f32(modulator_, carrier_, a1), saturated);
    output = vmla_n_f32(output, carrier_, a3);
    output = vmla_n_f32(output, modulator_, a4);
    return output;
}


float32x2_t RingModulator::getTransistorDiodeRingModulation(const float32x2_t carrier_, const float32x2_t modulator_)
{
    float32x2_t diode = getDiodeRingModulation(carrier_, modulator_);
    float32x2_t transistor = getTransistorRingModulation(carrier_, modulator_);
    return vmla_n_f32(vmul_n_f32(diode, typeBlendingWet()), transistor, typeBlendingDry);
}


float RingModulator::getNoise()
{
    // returns a random value in the range -1...1
    return (rand() * TWO_RAND_MAX_INVERSED) - 1.f;
}


void RingModulator::saturate(float& signal_, const float& saturation_, const float asymmetry_)
{
    // deprecated simple saturation function, stays here for clarity
    if (signal_ >= 0.f || asymmetry_ == 1.f)
        signal_ = tanhf(saturation_ * signal_) / tanhf(saturation_);
    
    else
        signal_ = tanhf((saturation_ / asymmetry_) * signal_) / tanhf(saturation_ / asymmetry_);
}


void RingModulator::setWaveform(const LFO::Waveform waveform_)
{
    modulator.getLFO().setWaveform(waveform_);
}


void RingModulator::setTune(const float freq_)
{
    modulator.setFrequency(freq_);
}


void RingModulator::setRate(const float rate_)
{
    modulator.getLFO().setFrequency(rate_);
}


void RingModulator::setDepth(const float depth_)
{
    modulator.getLFO().setAmplitude(depth_);
}


void RingModulator::setSaturation(const float sat_)
{
    // Minimum applied saturation level
    static const float MIN_SATURATION = 0.00001f;
    
    // Maximum saturation levels for diode and transistor ring modulators
    static const float MAX_TRANSISTOR_SATURATION = 8.5f;
    static const float MAX_DIODE_SATURATION = 6.f;
    
    // Parameter thresholds for transitioning between modulation types:
    // TRANSISTOR -> TRANSISTOR_DIODE -> DIODE
    static const float TYPE_TRANSITION_BORDER_1 = 0.5f;
    static const float TYPE_TRANSITION_BORDER_2 = 0.85f;
    
    // Determine the current type of ring modulation based on the saturation parameter (0 to 1 range)
    if (sat_ < TYPE_TRANSITION_BORDER_1)
        type = TRANSISTOR;
    else if (sat_ < TYPE_TRANSITION_BORDER_2)
        type = TRANSISTOR_DIODE;
    else
        type = DIODE;
    
    // caclulate transistor and diode saturation based on the parameter value and the current modulation type
    float transistorSat = (type == TRANSISTOR)
        ? mapValue(sat_, 0.f, TYPE_TRANSITION_BORDER_1, MIN_SATURATION, MAX_TRANSISTOR_SATURATION)
        : MAX_TRANSISTOR_SATURATION;
    
    float diodeSat = (type == DIODE || type == TRANSISTOR_DIODE)
        ? mapValue(sat_, TYPE_TRANSITION_BORDER_1, 1.f, MIN_SATURATION, MAX_DIODE_SATURATION)
        : MIN_SATURATION;
    
    // set the ramps of saturation accordingly
    transistorSaturation.setRampTo(transistorSat, 0.01f);
    diodeSaturation.setRampTo(diodeSat, 0.01f);
    
    // Precompute variables for efficiency, significantly improving the performance of saturation processing
    calculateSaturationVariables();
    
    // Calculate the blend (mix) between transistor and diode ring modulation for TRANSISTOR_DIODE type
    float typeBlending = (type == TRANSISTOR_DIODE)
        ? mapValue(sat_, TYPE_TRANSITION_BORDER_1, 1.f, 0.f, 1.f)
        : 0.f;
    typeBlendingWet.setRampTo(typeBlending, 0.01f);
        
    // Calculate the gain compensation variable (more saturation = less output gain)
    float gainAttenutation = (sat_ < 0.4f) ? mapValue(sat_, 0.f, 0.4f, 0.f, 0.64) : 0.64f;
    gainCompensation.setRampTo(1.f - gainAttenutation, 0.01f);
    
    // set the ringmod-processing function
    if (type == TRANSISTOR) processRingModulation = &RingModulator::getTransistorRingModulation;
    else if (type == DIODE) processRingModulation = &RingModulator::getDiodeRingModulation;
    else if (type == TRANSISTOR_DIODE) processRingModulation = &RingModulator::getTransistorDiodeRingModulation;
}


void RingModulator::calculateSaturationVariables()
{
    // Precompute variables for efficiency, significantly improving the performance of saturation processing
    tanhDiodeSaturation_inversed = 1.f / approximateTanh(diodeSaturation());
    tanhDiodeSaturationAsym_inversed[0] = 1.f / approximateTanh(diodeSaturation() * diodeAsymmetry[0]);
    tanhDiodeSaturationAsym_inversed[1] = 1.f / approximateTanh(diodeSaturation() * diodeAsymmetry[1]);
    tanhTransistorSaturation_inversed = 1.f / approximateTanh(transistorSaturation());
    tanhTransistorSaturationAsym_inversed = 1.f / approximateTanh(transistorSaturation() * transistorAsymmetry);
    
    diodeSatuaration_o_Asymmetry[0] = diodeSaturation() / diodeAsymmetry[0];
    diodeSatuaration_o_Asymmetry[1] = diodeSaturation() / diodeAsymmetry[1];
    transistorSaturation_o_Asymmetry = transistorSaturation() / transistorAsymmetry;
}


void RingModulator::setSpread(const float spread_)
{
    phaseShift.setRampTo(PI * spread_, 0.01f);
}


void RingModulator::setNoise(const float noise_)
{
    noiseWet = noise_;
    noiseDry = 1.f - noise_;
}


void RingModulator::setOversamplingRatio(const uint ratio_)
{
    oversampleRatio = ratio_;

    // Update the oversampling objects (interpolator and decimator) with the new ratio
    interpolator.setInterpolationRatio(ratio_);
    decimator.setDecimationRatio(ratio_);
    
    // Update the oscillators' sample rate to match the oversampled rate (ratio * program sample rate)
    modulator.setSampleRate(ratio_ * sampleRate);
}


void RingModulator::parameterChanged(const String &parameterID, float newValue)
{
    if (parameterID == "ringmod_tune")
    {
        setTune(newValue);
    }
    
    else if (parameterID == "ringmod_rate")
    {
        setRate(newValue);
    }
    
    else if (parameterID == "ringmod_depth")
    {
        setDepth(newValue * 0.01f);
    }
    
    else if (parameterID == "ringmod_saturation")
    {
        setSaturation(newValue * 0.01f);
    }
    
    else if (parameterID == "ringmod_spread")
    {
        setSpread(newValue * 0.01f);
    }
    
    else if (parameterID == "ringmod_noise")
    {
        setNoise(newValue * 0.01f);
    }
    
    else if (parameterID == "ringmod_bitcrush")
    {
        bitCrusher.setBitResolution(newValue);        
    }
    
    else if (parameterID == "ringmod_mix")
    {
        wet = 0.01f * newValue;
        dry = 1.f - wet;
    }
    
    else if (parameterID == "ringmod_waveform")
    {
        LFO::Waveform waveform = INT2ENUM((int)newValue, LFO::Waveform);
        setWaveform(waveform);
    }
    
    else if (parameterID == "ringmod_oversampling")
    {
        uint ratio;
        if (newValue <= 2) ratio = newValue;
        else if (newValue == 3) ratio = 4;
        else if (newValue == 4) ratio = 8;
        else ratio = 2;
        
        setOversamplingRatio(ratio);
    }
    
    else
    {
        engine_rt_error("Couldnt find Parameter with ID:" + parameterID, __FILE__, __LINE__, false);
    }
}




