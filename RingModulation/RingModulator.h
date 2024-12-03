#pragma once

#include "../Helpers.hpp"
#include "SampleRateConverter.h"
#include "BitCrusher.h"


/**
 * @defgroup RingModulationParameters
 * @brief all static variables concerning the Ring Modulators UI parameters
 * @{
 */

namespace RingModulation
{

/**
 * @brief determines the number of samples after which the ramp process function is called
 * @attention has to be a power of 2!
 */
static const uint RAMP_UPDATE_RATE = 8;

/**
 * @brief determines the number of taps of the FIR Oversampling Filters
 * @attention has to be 64, 128 or 256 for now
 */
static const uint OVERSAMPLING_FILTER_LENGTH = 64;

/** @brief the number of user definable parameters */
static const unsigned int NUM_PARAMETERS = 9;

static const uint NUM_WAVEFORMS = 5;
static const std::string waveformNames[NUM_WAVEFORMS] {
    "Sine",
    "Triangle",
    "Saw",
    "Pulse",
    "Random"
};

/** @brief an enum to save the parameter Indexes */
enum class Parameters
{
    TUNE,
    RATE,
    DEPTH,
    SATURATION,
    SPREAD,
    NOISE,
    BITCRUSH,
    MIX,
    WAVEFORM
};

/** @brief names of parameters */
static const std::string parameterID[NUM_PARAMETERS] = {
    "ringmod_tune",
    "ringmod_rate",
    "ringmod_depth",
    "ringmod_saturation",
    "ringmod_spread",
    "ringmod_noise",
    "ringmod_bitcrush",
    "ringmod_mix",
    "ringmod_waveform"
};

/** @brief names of parameters */
static const std::string parameterName[NUM_PARAMETERS] = {
    "Tune",
    "Rate",
    "Depth",
    "Saturation",
    "Spread",
    "Noise",
    "Bitcrush",
    "Ringmod Mix",
    "Waveform"
};

/** @brief units of parameters */
static const std::string parameterSuffix[NUM_PARAMETERS] = {
    " hertz",
    " hertz",
    " %",
    " %",
    " %",
    " %",
    " %",
    " %",
    ""
};

/** @brief minimum values of parameters */
static const float parameterMin[NUM_PARAMETERS] = {
    0.1f,
    0.1f,
    0.f,
    0.f,
    0.f,
    0.f,
    0.f,
    0.f,
    0.f
};

/** @brief maximum values of parameters */
static const float parameterMax[NUM_PARAMETERS] = {
    100.f,
    10.f,
    100.f,
    100.f,
    100.f,
    100.f,
    100.f,
    100.f,
    4.f
};

/** @brief step values of parameters */
static const float parameterStep[NUM_PARAMETERS] = {
    0.5f,
    0.1f,
    0.5f,
    0.5f,
    0.5f,
    0.5f,
    0.5f,
    0.5f,
    1.f
};

/** @brief initial values of parameters */
static const float parameterInitialValue[NUM_PARAMETERS] = {
    10.f,
    0.4f,
    0.f,
    0.f,
    0.f,
    0.f,
    0.f,
    70.f,
    0.f
};

/** @} */


// =======================================================================================
// MARK: - LFO
// =======================================================================================

/**
 * @class LFO
 * @brief A class representing a Low Frequency Oscillator (LFO) for audio signal processing.
 *
 * The LFO class generates various low-frequency waveforms for use in audio synthesis.
 * Supported waveforms include sine, triangle, saw, pulse, and random
 */
class LFO
{
public:
    /**
     * @enum Waveform
     * @brief Enum representing the waveform types supported by the LFO.
     */
    enum Waveform { SINE, TRIANGLE, SAW, PULSE, RANDOM };

    /**
     * @brief Initializes the LFO with a specified frequency and sample rate.
     * @param freq_ The initial frequency of the LFO in Hz.
     * @param sampleRate_ The sample rate of the audio system in Hz.
     */
    void setup(const float freq_, const float sampleRate_);
        
    /**
     * @brief Generates the next value of the LFO's waveform.
     * @return The next value of the LFO, scaled by its amplitude.
     */
    float getNextValue();
    
    /**
     * @brief Sets the sample rate for the LFO.
     * @param sampleRate_ The new sample rate in Hz.
     */
    void setSampleRate(const float sampleRate_);

    /**
     * @brief Sets the frequency of the LFO.
     * @param freq_ The new frequency of the LFO in Hz.
     */
    void setFrequency(const float freq_);

    /**
     * @brief Sets the amplitude of the LFO.
     * @param amp_ The new amplitude of the LFO. Values are typically in the range [0, 1].
     */
    void setAmplitude(const float amp_);

    /**
     * @brief Sets the waveform type of the LFO.
     * @param waveform_ The waveform to generate, selected from the Waveform enum.
     */
    void setWaveform(Waveform waveform_);
    
private:
    /**
     * @brief Generates the next value of the sine waveform.
     * @return The sine waveform value at the current phase.
     */
    float getSine();

    /**
     * @brief Generates the next value of the triangle waveform.
     * @return The triangle waveform value at the current phase.
     */
    float getTriangle();

    /**
     * @brief Generates the next value of the saw waveform.
     * @return The saw waveform value at the current phase.
     */
    float getSaw();

    /**
     * @brief Generates the next value of the pulse waveform.
     * @return The pulse waveform value at the current phase.
     */
    float getPulse();

    /**
     * @brief Generates the next value of the random waveform.
     * @return A random value updated once per waveform cycle.
     */
    float getRandom();
    
    float (LFO::*getValue)(); ///< Function pointer to the currently selected waveform generator function.

    float sampleRate; ///< The current sample rate of the audio system in Hz.
    float invSampleRate; ///< The reciprocal of the sample rate (1 / sampleRate).

    float phase; ///< The current phase of the waveform, in radians.
    float frequency; ///< The frequency of the LFO, in Hz.
    float increment; ///< The phase increment per sample, determined by frequency and sample rate.
    float amplitude; ///< The amplitude of the LFO.

    Waveform waveform; ///< The currently selected waveform type.
    
    bool phaseWrapped = false; ///< Flag indicating whether the phase has wrapped around in the current cycle.
    float nextValue = 0.f; ///< The next value for random waveform generation.
};


// =======================================================================================
// MARK: - OSCILLATOR
// =======================================================================================

/**
 * @class Oscillator
 * @brief A class representing an audio oscillator capable of generating two sine waves with optional modulation and phase shift.
 */
class Oscillator
{
public:
    /**
     * @brief Initializes the oscillator with a specified frequency and sample rate.
     * @param freq_ The initial frequency of the oscillator in Hz.
     * @param sampleRate_ The sample rate of the audio system in Hz.
     */
    void setup(const float freq_, const float sampleRate_);
            
    /**
     * @brief Generates the next values of the oscillator's waveform.
     * @return A float32x2_t array containing the current and (optionally) phase-shifted waveform values.
     */
    float32x2_t getNextValues();
    
    /**
     * @brief Sets the sample rate for the oscillator.
     * @param sampleRate_ The new sample rate in Hz.
     */
    void setSampleRate(const float sampleRate_);
    
    /**
     * @brief Sets the frequency of the oscillator.
     * @param freq_ The new frequency of the oscillator in Hz.
     */
    void setFrequency(const float freq_);
    
    /**
     * @brief Sets the phase shift for the oscillator's secondary output.
     * @param shift_ The phase shift in radians.
     */
    void setPhaseShift(const float shift_);
    
    /**
     * @brief Gets a reference to the LFO modulator used by the oscillator.
     * @return A reference to the LFO instance used for modulation.
     */
    LFO& getLFO() { return modulator; }
    
private:
    float sampleRate; ///< The current sample rate of the audio system in Hz.
    float invSampleRate; ///< The reciprocal of the sample rate (1 / sampleRate).
    
    float frequency; ///< The frequency of the oscillator in Hz.
    float phase; ///< The current phase of the oscillator waveform, in radians.
    float increment; ///< The phase increment per sample, determined by frequency and sample rate.
    float phaseShift; ///< The phase shift applied to the secondary output, in radians.
    bool phaseIsShifted = false; ///< Flag indicating whether a phase shift is applied to the secondary output.
    
    LFO modulator; ///< An instance of the LFO class used for frequency modulation.
};


// =======================================================================================
// MARK: - RING MODULATOR
// =======================================================================================


class RingModulator
{
public:
    /**
     * @brief Initializes the ring modulator with the given sample rate and block size.
     * @param sampleRate_ The sample rate in Hz.
     * @param blockSize_ The number of samples per processing block.
     * @return True if setup was successful.
     */
    bool setup(const float sampleRate_, const uint blockSize_);
    
    /**
     * @brief Processes audio samples using ring modulation.
     * @param input_ The stereo input sample.
     * @param sampleIndex_ The index of the sample being processed.
     * @return The processed stereo sample.
     */
    float32x2_t processAudioSamples(const float32x2_t input_, const uint sampleIndex_);
    
    /**
     * @brief Updates internal parameters for the audio processing block.
     */
    void updateAudioBlock();
    
    /**
     * @brief Handles changes to parameters.
     * @param parameterID The identifier of the changed parameter.
     * @param newValue The new value of the parameter.
     */
    void parameterChanged(const String& parameterID, float newValue);
    
private:
    /**
     * @brief Updates internal ramps for smooth parameter transitions.
     */
    void updateRamps();
    
    // Setters for various parameters
    void setTune(const float freq_);
    void setRate(const float rate_);
    void setDepth(const float depth_);
    void setWaveform(const LFO::Waveform waveform_);
    void setSaturation(const float sat_);
    void setSpread(const float spread_);
    void setNoise(const float noise_);
    void setOversamplingRatio(const uint ratio_);
    
    float32x2_t (RingModulator::*processRingModulation)(const float32x2_t, const float32x2_t); ///< Function pointer to the ring modulation function.
    
    /**
     * @brief Applies diode-based ring modulation to the input signals.
     *
     * The diode ring modulation technique models the behavior of diodes in an analog circuit.
     * It uses a combination of absolute values and saturation functions to simulate the nonlinear characteristics of diodes.
     * @cite Julian Parker: "A Simple Digital Model of the Diode-Based Ring-Modulator"
     *
     * The formula used processes the carrier and modulator signals through two diode-like components:
     * @code
     * - For x >= 0: y(x) = tanh(saturation * x) / tanh(saturation)
     * - For x <  0: y(x) = tanh((saturation / asymmetry) * x) / tanh(saturation / asymmetry)
     * @endcode
     *
     * @param carrier_ The carrier signal, represented as a `float32x2_t` vector (stereo signal).
     * @param modulator_ The modulator signal, represented as a `float32x2_t` vector (stereo signal).
     * @return A `float32x2_t` vector containing the ring-modulated output signal.
     */
    float32x2_t getDiodeRingModulation(const float32x2_t carrier_, const float32x2_t modulator_);

    /**
     * @brief Applies transistor-based ring modulation to the input signals.
     *
     * The transistor ring modulation technique models the nonlinear behavior of transistors in an analog circuit.
     * It combines the carrier and modulator signals with saturation and blending parameters to emulate the transistor response.
     * @cite: Richard Hoffmann-Burchardi: "ASYMMETRIES MAKE THE DIFFERENCE: AN ANALYSIS OF TRANSISTOR-BASED ANALOG RING MODULATORS"
     *
     * The formula used blends the modulator and carrier signals while applying nonlinear saturation:
     * @code
     * - For x >= 0: y(x) = tanh(saturation * x) / tanh(saturation)
     * - For x <  0: y(x) = tanh((saturation / asymmetry) * x) / tanh(saturation / asymmetry)
     * @endcode
     * The resulting output is further shaped using coefficients `a1`, `a2`, `a3`, and `a4` for fine-tuned transistor emulation.
     *
     * @param carrier_ The carrier signal, represented as a `float32x2_t` vector (stereo signal).
     * @param modulator_ The modulator signal, represented as a `float32x2_t` vector (stereo signal).
     * @return A `float32x2_t` vector containing the ring-modulated output signal.
     */
    float32x2_t getTransistorRingModulation(const float32x2_t carrier_, const float32x2_t modulator_);

    /**
     * @brief Applies a combination of diode and transistor-based ring modulation.
     *
     * This method blends the output of diode and transistor ring modulation techniques based on a blending factor.
     * The blending factor (`typeBlendingWet`) determines the weight of each technique in the final output.
     *
     * The combined modulation creates a hybrid effect, simulating both diode and transistor nonlinearities.
     * It is particularly useful for achieving a unique, blended tone not achievable with pure diode or transistor modulation alone.
     *
     * @param carrier_ The carrier signal, represented as a `float32x2_t` vector (stereo signal).
     * @param modulator_ The modulator signal, represented as a `float32x2_t` vector (stereo signal).
     * @return A `float32x2_t` vector containing the blended ring-modulated output signal.
     */
    float32x2_t getTransistorDiodeRingModulation(const float32x2_t carrier_, const float32x2_t modulator_);
    
    inline float getNoise(); ///< Generates a random noise value.
    inline void saturate(float& signal_, const float& saturation, const float asymmetry = 1.f); ///< Applies saturation to the signal.
    void calculateSaturationVariables(); ///< Precomputes variables for efficient saturation processing.
    
private:
    float sampleRate; ///< The sample rate of the audio system in Hz.
    uint blockSize; ///< The size of the audio block being processed.

    float32_t dry = 0.3f; ///< The dry (unprocessed) signal mix ratio.
    float32_t wet = 0.7f; ///< The wet (processed) signal mix ratio.
    LinearRamp gainCompensation; ///< Ramp for gain compensation.
    LinearRamp phaseShift; ///< Ramp for phase shifting.

    enum Type { TRANSISTOR, TRANSISTOR_DIODE, DIODE }; ///< Enum for modulation type.
    Type type = TRANSISTOR; ///< The current ring modulation type.
    LinearRamp typeBlendingWet; ///< Ramp for wet blending between modulation types.
    float32_t typeBlendingDry; ///< Dry blending value for modulation types.

    Oscillator modulator; ///< Modulator oscillator instance.
    BitCrusher bitCrusher; ///< Bitcrusher instance for sample rate and resolution reduction.

    LinearRamp diodeSaturation; ///< Ramp for diode saturation.
    LinearRamp transistorSaturation; ///< Ramp for transistor saturation.
    float tanhDiodeSaturation_inversed; ///< Inverse of the diode saturation tanh value.
    float tanhTransistorSaturation_inversed; ///< Inverse of the transistor saturation tanh value.
    float tanhDiodeSaturationAsym_inversed[2]; ///< Inverse tanh values for asymmetric diode saturation.
    float tanhTransistorSaturationAsym_inversed; ///< Inverse tanh value for asymmetric transistor saturation.
    float diodeSatuaration_o_Asymmetry[2]; ///< Saturation divided by asymmetry for diodes.
    float transistorSaturation_o_Asymmetry; ///< Saturation divided by asymmetry for transistors.
    const float transistorAsymmetry = 0.99f; ///< Default transistor asymmetry value.
    const float diodeAsymmetry[2] = { 0.96f, 0.87f }; ///< Default diode asymmetry values.
    const float32_t a1 = 0.1f; ///< Parameter for modulation formula.
    const float32_t a2 = 0.0001f; ///< Parameter for modulation formula.
    const float32_t a3 = 0.1f; ///< Parameter for modulation formula.
    const float32_t a4 = 0.0001f; ///< Parameter for modulation formula.

    float32_t noiseWet, noiseDry; ///< Wet and dry levels for noise modulation.

    InterpolatorStereo interpolator; ///< Interpolator for upsampling.
    DecimatorStereo decimator; ///< Decimator for downsampling.
    uint oversampleRatio = 2; ///< Oversampling ratio for the audio processing.
};

} // namspace RingModulation
