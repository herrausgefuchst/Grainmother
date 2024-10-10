// =======================================================================================
//
// Granulator.h
/**
 * @file Granulator.h
 * @author Julian Fuchs
 * @date 09-Oktober-2024
 * @version 1.0.0
 *
 * @brief This file implements a real time granular synthesis
 *
 */
// =======================================================================================

#pragma once

#include "../helpers.hpp"

/**
 * @defgroup GranulatorParameters
 * @brief all static variables concerning the granulators UI parameters
 * @{
 */

namespace Granulation
{

static const float MIN_GRAINLENGTH_MS = 7.f;
static const float MAX_GRAINLENGTH_MS = 70.f;

static const float MIN_DENSITY = 1.f;
static const float MAX_DENSITY = 85.f;

static const float MIN_CUTOFF = 120.f;
static const float MAX_CUTOFF = 20000.f;

static const int BUFFERSIZE = 32768;

static const int MAX_NUM_GRAINS = 100;

static const size_t numDelaySpeedRatios = 4;
static const std::string delaySpeedRatios[numDelaySpeedRatios] {
    "1 : 1",
    "1 : 2",
    "1 : 3",
    "1 : 4"
};

/** @brief the number of user definable parameters */
static const unsigned int NUM_PARAMETERS = 12;

/** @brief an enum to save the parameter Indexes */
enum class Parameters
{
    GRAINLENGTH,
    DENSITY,
    VARIATION,
    PITCH,
    GLIDE,
    DELAY,
    HIGHCUT,
    WETNESS,
    REVERSE,
    DELAY_SPEED_RATIO,
    FILTER_RESONANCE,
    FILTER_MODEL
};

/** @brief ids of parameters */
static const std::string parameterID[NUM_PARAMETERS] = {
    "granulator_grainlength",
    "granulator_density",
    "granulator_variation",
    "granulator_pitch",
    "granulator_glide",
    "granulator_delay",
    "granulator_highcut",
    "granulator_wetness",
    "granulator_reverse",
    "granulator_delayspeedratio",
    "granulator_filterresonance",
    "granulator_filtermodel"
};

/** @brief names of parameters */
static const std::string parameterName[NUM_PARAMETERS] = {
    "Grainlength",
    "Density",
    "Variation",
    "Pitch",
    "Glide",
    "Delay",
    "Highcut",
    "Wetness",
    "Reverse",
    "Delay Speed Ratio",
    "Filter Resonance",
    "Filter Model"
};

/** @brief minimum values of parameters */
static const float parameterMin[NUM_PARAMETERS] = {
    MIN_GRAINLENGTH_MS,
    MIN_DENSITY,
    0.f,
    -12.f,
    -1.f,
    0.f,
    0.f,
    0.f,
    0.f,
    0,
    0.f,
    0.f
};

/** @brief maximum values of parameters */
static const float parameterMax[NUM_PARAMETERS] = {
    MAX_GRAINLENGTH_MS,
    MAX_DENSITY,
    100.f,
    12.f,
    1.f,
    100.f,
    100.f,
    100.f,
    1.f,
    3,
    100.f,
    1.f
};

/** @brief step values of parameters */
static const float parameterStep[NUM_PARAMETERS] = {
    0.5f,
    0.5f,
    0.5f,
    0.25f,
    0.02f,
    0.5f,
    0.5f,
    0.5f,
    1.f,
    1.f,
    0.5f,
    1.f
};

/** @brief units of parameters */
static const std::string parameterSuffix[NUM_PARAMETERS] = {
    " ms",
    " grains/sec",
    " %",
    " semitones",
    " down/up",
    " %",
    " %",
    " %",
    "",
    "",
    " %",
    ""
};

/** @brief initial values of parameters */
static const float parameterInitialValue[NUM_PARAMETERS] = {
    40.f,
    20.f,
    0.f,
    0.f,
    0.f,
    0.f,
    0.f,
    100.f,
    0.f,
    1,
    70.f,
    0.f
};

/** @} */

// =======================================================================================
// MARK: - MOVING AVERAGER
// =======================================================================================


class MovingAveragerStereo
{
public:
    MovingAveragerStereo()
    {
        std::fill(buffer.begin(), buffer.end(), vdup_n_f32(0.f));
    }
    
    float32x2_t processAudioSamples(float32x2_t x)
    {
        buffer[pointer] = x;
        
        uint zDPointer = pointer + 1;
        if (zDPointer >= bufferLength) zDPointer -= bufferLength;
        
        uint zD1Pointer = pointer + 2;
        if (zD1Pointer >= bufferLength) zD1Pointer -= bufferLength;
        
        ZD1 = buffer[zD1Pointer];
        
        // output = x - buffer[zDPointer];
        float32x2_t output = vsub_f32(x, buffer[zDPointer]);
        
        //o utput += integrator;
        output = vadd_f32(output, integrator);
        
        integrator = output;
        
        // output *= scalar;
        output = vmul_n_f32(output, scalar);
        
        if (++pointer >= bufferLength) pointer = 0;
        
        return output;
    }
    
    const float32x2_t getZD1() const { return ZD1; }
    
private:
    static const uint bufferLength = 1024;
    static constexpr float32_t scalar = 1.f / (float)bufferLength;
    uint pointer = 0;
    std::array<float32x2_t, bufferLength> buffer;
    float32x2_t integrator;
    float32x2_t ZD1;
};


// =======================================================================================
// MARK: - DC OFFSET FILTER
// =======================================================================================


class DCOffsetFilterStereo
{
public:
    float32x2_t processAudioSamples(float32x2_t x)
    {
        return mMA1.getZD1() - mMA2.processAudioSamples(mMA1.processAudioSamples(x));
    }
    
private:
    MovingAveragerStereo mMA1;
    MovingAveragerStereo mMA2;
};


// =======================================================================================
// MARK: - FILTER
// =======================================================================================


/**
 * @class TPT1stOrderFilterStereo
 * @brief A transposed direct form 1st-order filter for stereo audio processing.
 *
 * The `TPT1stOrderFilterStereo` class implements a 1st-order lowpass (LPF) or allpass (APF) filter
 * for processing stereo audio samples. It uses a transposed direct form to provide efficient filtering.
 */
class TPT1stOrderFilterStereo
{
public:
    /**
     * @enum FilterType
     * @brief Specifies the type of filter: Lowpass (LPF) or Allpass (APF).
     */
    enum FilterType { LPF, APF };
    
    /**
     * @brief Default constructor for `TPT1stOrderFilterStereo`.
     */
    TPT1stOrderFilterStereo() {}
    
    /**
     * @brief Constructor that sets the filter type.
     *
     * @param type_ The type of the filter (LPF or APF).
     */
    TPT1stOrderFilterStereo (FilterType type_) { type = type_; }
    
    /**
     * @brief Destructor for `TPT1stOrderFilterStereo`.
     */
    ~TPT1stOrderFilterStereo() {}
    
    /**
     * @brief Sets up the filter with the specified alpha coefficient and filter type.
     *
     * @param alpha_ Reference to the alpha coefficient for the filter.
     * @param type_ The type of filter to use (default is LPF).
     */
    void setup(float& alpha_, FilterType type_ = FilterType::LPF)
    {
        alpha = &alpha_;
        type = type_;
    }
    
    /**
     * @brief Processes a block of stereo audio samples through the filter.
     *
     * Filters the input stereo samples based on the filter type (LPF or APF) and the alpha coefficient.
     *
     * @param input_ The input stereo samples in a SIMD vector.
     * @return The filtered stereo samples.
     */
    float32x2_t processAudioSamples(const float32x2_t input_)
    {
        float32x2_t v = vsub_f32(input_, s);
        v = vmul_n_f32(v, *alpha);
        
        float32x2_t lpf = vadd_f32(v, s);
        s = vadd_f32(v, lpf);
        
        if (type == LPF) return lpf;
        else return vsub_f32(vadd_f32(lpf, lpf), input_);
    }
    
    /**
     * @brief Gets the feedback value for the filter.
     *
     * Returns the current state value scaled by the beta coefficient.
     *
     * @return The feedback value as a SIMD vector.
     */
    const float32x2_t getFeedbackValue() const { return vmul_n_f32(s, beta); }
    
    /**
     * @brief Resets the filter state.
     *
     * Sets the internal state to zero.
     */
    void reset() { s = vdup_n_f32(0.f); }
    
    /**
     * @brief Sets the beta coefficient for feedback.
     *
     * @param beta_ The beta coefficient value.
     */
    void setBeta(float beta_) { beta = beta_; }
    
    /**
     * @brief Sets the alpha coefficient for the filter.
     *
     * @param alpha_ Reference to the alpha coefficient.
     */
    void setAlpha(float& alpha_) { alpha = &alpha_; }
    
private:
    FilterType type = LPF;        ///< The type of filter (LPF or APF).
    float32x2_t s = vdup_n_f32(0.f); ///< The internal filter state.
    float32_t* alpha = nullptr;   ///< Pointer to the alpha coefficient for the filter.
    float32_t beta = 1.0f;        ///< The beta coefficient used for feedback.
};


/**
 * @class FilterStereo
 * @brief A stereo filter class for audio processing, implementing Moog ladder and half-ladder filter models.
 *
 * The `FilterStereo` class processes stereo audio samples through either a Moog ladder or a half-ladder filter model.
 * It allows the user to adjust the cutoff frequency, resonance, and filter model for various audio filtering effects.
 */
class FilterStereo
{
public:
    /**
     * @enum Model
     * @brief The available filter models.
     */
    enum Model { MOOGLADDER, MOOGHALFLADDER };
    
    /**
     * @brief Constructs a `FilterStereo` object and initializes the filter coefficients.
     */
    FilterStereo();
    
    /**
     * @brief Destructor for the `FilterStereo` class.
     */
    ~FilterStereo() {}
    
    /**
     * @brief Sets up the filter with a specified sample rate and optional cutoff frequency.
     *
     * Initializes the filter with the provided sample rate and sets the initial cutoff frequency.
     *
     * @param sampleRate_ The sample rate of the audio system.
     * @param cutoff_ The initial cutoff frequency (default is 18,000 Hz).
     */
    void setup(const float sampleRate_, const float cutoff_ = 18000.f);
    
    /**
     * @brief Processes a block of stereo audio samples through the filter.
     *
     * Filters the input stereo samples based on the current filter model, cutoff frequency, and resonance.
     *
     * @param input_ The input stereo samples in a SIMD vector.
     * @return The filtered stereo samples.
     */
    float32x2_t processAudioSamples(float32x2_t input_);
    
    /**
     * @brief Sets the cutoff frequency of the filter.
     *
     * Adjusts the cutoff frequency and recalculates the filter coefficients.
     *
     * @param freq_ The new cutoff frequency in Hz.
     */
    void setCutoffFrequency(const float freq_);
    
    /**
     * @brief Sets the resonance of the filter.
     *
     * Adjusts the resonance of the filter, affecting the sharpness of the cutoff.
     *
     * @param reso_ The new resonance value.
     */
    void setResonance(const float reso_);
    
    /**
     * @brief Sets the filter model to either Moog ladder or Moog half-ladder.
     *
     * Resets the internal state and recalculates the coefficients for the chosen model.
     *
     * @param model_ The new filter model.
     */
    void setFilterModel(const Model model_);
    
private:
    /**
     * @brief Calculates and updates the resonance based on the cutoff frequency and resonance amount.
     */
    void calcResonance();
    
    Model model = MOOGLADDER;     ///< The current filter model (Moog ladder or half-ladder).
    
    float sampleRate;             ///< The sample rate of the audio system.
    float invSampleRate;          ///< The inverse of the sample rate for calculations.
    
    float cutoff = 18000.0f;      ///< The cutoff frequency of the filter.
    float32_t resonance = 0.f;    ///< The resonance value of the filter.
    float32_t resonanceAmount = 0.f; ///< The amount of resonance applied to the filter.
    
    float32_t alpha0 = 0.f;       ///< Pre-calculated coefficient for filter processing.
    float32_t g = 0.f;            ///< Feedforward coefficient for the lowpass filters.
    float32_t g_apf = 0.f;        ///< Feedforward coefficient for the allpass filter.
    
    static const uint numLowpassFilter = 4; ///< The number of lowpass filters used in the Moog ladder model.
    
    TPT1stOrderFilterStereo LPF[numLowpassFilter]; ///< Array of lowpass filters for processing.
    TPT1stOrderFilterStereo APF; ///< Allpass filter used in the half-ladder model.
};


// =======================================================================================
// MARK: - DELAY
// =======================================================================================


/**
 * @class Delay
 * @brief Implements a delay effect with feedback and linear interpolation.
 *
 * The `Delay` class processes audio samples with a delay effect, including optional
 * feedback and time adjustment. It supports linear interpolation for fractional delay times.
 */
class Delay
{
public:
    /**
     * @brief Constructs the `Delay` object and initializes the buffer.
     *
     * The constructor fills the internal buffer with zeros and initializes the delay state.
     */
    Delay()
    {
        std::fill(buffer.begin(), buffer.end(), vdup_n_f32(0.f));
    }
    
    /**
     * @brief Sets up the delay with the provided sample rate.
     *
     * Initializes the delay with the given sample rate and prepares the delay ramp.
     *
     * @param sampleRate_ The sample rate of the audio system.
     */
    void setup(const float sampleRate_)
    {
        sampleRate = sampleRate_;
        delayMs.setup(100.f, sampleRate, 8);
    }
    
    /**
     * @brief Processes a block of stereo audio samples through the delay effect.
     *
     * Processes the input samples, applying the delay effect with optional linear interpolation
     * and feedback. The delay time can be adjusted in real-time.
     *
     * @param input_ The input stereo samples in a SIMD vector.
     * @param sampleIndex_ The current sample index within the block.
     * @return The processed stereo samples with the delay effect applied.
     */
    float32x2_t processAudioSamples(float32x2_t input_, const uint sampleIndex_)
    {
        if ((sampleIndex_ & 7) == 0)
        {
            if (!delayMs.rampFinished) delayMs.processRamp();
            setDelayTimeInMs(delayMs());
        }
        
        float32x2_t output = buffer.at(readPointerLo);
        
        // Linear interpolation
        if (interpolationNeeded)
        {
            float32x2_t interpolated = vmul_n_f32(vsub_f32(buffer.at(readPointerHi), output), frac);
            output = vadd_f32(output, interpolated);
        }
        
        buffer.at(writePointer) = vmla_n_f32(vrev64_f32(input_), output, feedback);
        
        if (++writePointer >= bufferLength) writePointer = 0;
        if (++readPointerLo >= bufferLength) readPointerLo = 0;
        if (++readPointerHi >= bufferLength) readPointerHi = 0;
        
        return output;
    }
    
    /**
     * @brief Sets the feedback amount for the delay effect.
     *
     * @param feedback_ The feedback level (0.0 to 1.0).
     */
    void setFeedback(const float32_t feedback_) { feedback = feedback_; }
    
    /**
     * @brief Sets the delay time with a ramp, in milliseconds.
     *
     * This method adjusts the delay time over a specified duration using a ramp.
     *
     * @param delayMs_ The target delay time in milliseconds.
     */
    void setDelayTimeRampInMs(const float delayMs_)
    {
        delayMs.setRampTo(delayMs_, 0.1f);
    }
    
    /**
     * @brief Sets the delay time in samples.
     *
     * This method directly sets the delay time in samples, without interpolation.
     *
     * @param delaySamples_ The delay time in samples.
     */
    void setDelayTimeInSamples(const uint delaySamples_)
    {
        if (delaySamples_ >= bufferLength)
            engine_rt_error("delay exceeds buffer length of delay object", __FILE__, __LINE__, true);
        
        readPointerLo = writePointer - delaySamples_;
        if (readPointerLo < 0) readPointerLo += bufferLength;
        
        interpolationNeeded = false;
    }
    
    /**
     * @brief Sets the delay time in milliseconds, with linear interpolation if needed.
     *
     * This method calculates the delay time in samples based on the sample rate and adjusts
     * the read pointers. If the delay time requires fractional samples, linear interpolation is applied.
     *
     * @param delayMs_ The delay time in milliseconds.
     */
    void setDelayTimeInMs(const float delayMs_)
    {
        float delaySamples = delayMs_ * 0.001f * sampleRate;
        
        readPointerLo = writePointer - floorf_neon(delaySamples);
        readPointerHi = readPointerLo + 1;
        
        if (readPointerLo < 0) readPointerLo += bufferLength;
        if (readPointerHi < 0) readPointerHi += bufferLength;
        
        frac = delaySamples - floorf_neon(delaySamples);
        interpolationNeeded = (frac == 0.f) ? false : true;
    }
    
private:
    float sampleRate = 44100.f;               ///< The sample rate of the audio system.
    
    LinearRamp delayMs;                       ///< Ramp handler for smooth delay time transitions.
    
    static const uint bufferLength = 65536;   ///< Length of the delay buffer in samples.
    std::array<float32x2_t, bufferLength> buffer; ///< Buffer for storing delayed samples.
    
    uint writePointer = 0;                    ///< Write pointer for the delay buffer.
    
    int readPointerLo, readPointerHi;         ///< Read pointers for the delay buffer (low and high for interpolation).
    float32_t frac;                           ///< Fractional value for linear interpolation between delay samples.
    bool interpolationNeeded = false;         ///< Flag indicating whether interpolation is needed.
    
    float32_t feedback;                       ///< Feedback level for the delay effect.
};



// =======================================================================================
// MARK: - SOURCE DATA
// =======================================================================================


/**
 * @class SourceData
 * @brief A class for managing a buffer of floating point values with circular writing.
 *
 * The `SourceData` class provides functionality to store floating point values
 * in a fixed-size buffer. It maintains a write pointer that cycles through
 * the buffer as new values are written. Old values in the buffer are overwritten
 * when the buffer capacity is exceeded.
 */
class SourceData
{
public:
    /**
     * @brief Constructor that initializes the buffer and the write pointer.
     *
     * The constructor initializes the buffer by setting all elements to 0.
     * The write pointer is set to 0, indicating that writing starts at the
     * beginning of the buffer.
     */
    SourceData()
    {
        std::fill(buffer.begin(), buffer.end(), 0.f);
        writePointer = 0;
    }
    
    /**
     * @brief Writes a value into the buffer at the current write pointer.
     *
     * This function inserts the provided value at the current position of
     * the write pointer. The write pointer is then incremented and, if it
     * exceeds the buffer size, wraps around to 0 (circular buffer behavior).
     *
     * @param value_ The value to be written into the buffer.
     */
    void writeBuffer(const float value_)
    {
        buffer.at(writePointer) = value_;
        if (++writePointer >= BUFFERSIZE) writePointer = 0;
    }
    
    /**
     * @brief Retrieves a value from the buffer at a given position.
     *
     * This function returns the value stored at the specified position
     * within the buffer.
     *
     * @param pos_ The index position of the value to retrieve.
     * @return The value stored at the given position.
     */
    float get(const uint pos_) const { return buffer.at(pos_); }
    
    /**
     * @brief Gets the current position of the write pointer.
     *
     * This function returns the current index where the next write operation
     * will take place in the buffer.
     *
     * @return The current index of the write pointer.
     */
    int getWritePointer() const { return writePointer; }
    
private:
    std::array<float, BUFFERSIZE> buffer; ///< Fixed-size buffer to store floating point values.
    int writePointer; ///< Current position of the write pointer in the buffer.
};


// =======================================================================================
// MARK: - ENVELOPES
// =======================================================================================


/**
 * @class Envelope
 * @brief A base class representing an amplitude envelope for grains.
 *
 * The `Envelope` class provides the interface and basic structure for an envelope
 * that controls the amplitude of grains over time. Derived classes must implement
 * the `getNextAmplitude()` method to calculate the next amplitude value.
 */
class Envelope
{
public:
    /**
     * @brief Constructor for the Envelope class.
     *
     * Initializes the envelope with the given duration and grain amplitude.
     *
     * @param durationSamples_ The total duration of the envelope in samples.
     * @param grainAmplitude_ The amplitude of the grain.
     */
    Envelope(const uint durationSamples_, const float grainAmplitude_)
    : grainAmplitude(grainAmplitude_)
    , durationSamples(durationSamples_)
    {}
    
    /**
     * @brief Virtual destructor.
     */
    virtual ~Envelope() {}
    
    /**
     * @brief Pure virtual function to get the next amplitude value.
     *
     * This function must be implemented by derived classes to provide
     * the next amplitude value in the envelope's sequence.
     *
     * @return The next amplitude value.
     */
    virtual float getNextAmplitude() = 0;
    
protected:
    float nextAmplitude = 0.f;          ///< The next amplitude value in the envelope.
    const float grainAmplitude;         ///< The amplitude of the grain.
    const uint durationSamples;         ///< The total duration of the envelope in samples.
};

/**
 * @class ParabolicEnvelope
 * @brief A derived class implementing a parabolic amplitude envelope.
 *
 * The `ParabolicEnvelope` class calculates amplitude values based on a parabolic curve.
 */
class ParabolicEnvelope : public Envelope
{
public:
    /**
     * @brief Constructor for the ParabolicEnvelope class.
     *
     * Initializes the parabolic envelope with the given duration and grain amplitude.
     *
     * @param durationSamples_ The total duration of the envelope in samples.
     * @param grainAmplitude_ The amplitude of the grain.
     */
    ParabolicEnvelope(const uint durationSamples_, const float grainAmplitude_);
    
    /**
     * @brief Calculates and returns the next amplitude value based on a parabolic curve.
     *
     * This function overrides the base class method to implement the parabolic
     * amplitude calculation.
     *
     * @return The next amplitude value.
     */
    float getNextAmplitude() override;
    
private:
    float slope;                        ///< The slope value used for the parabolic calculation.
    float curve;                        ///< The curvature value of the parabolic envelope.
};


// =======================================================================================
// MARK: - GRAIN PROPERTIES
// =======================================================================================


/**
 * @struct GrainProperties
 * @brief A structure that defines the properties of a grain
 *
 * The `GrainProperties` struct holds various parameters that control the behavior
 * of an individual grain, such as amplitude, length, pitch, and panning.
 */
struct GrainProperties
{
    /** @brief Amplitude of the envelope (range: 0.0 to 1.0). */
    float envelopeAmplitude = 1.f;
    
    /** @brief Length of the grain in samples. */
    int length = 2200;
    
    /** @brief Initial delay for the read pointer in samples. */
    int initDelay = 5;
    
    /** @brief Read pointer increment for pitching (range: 0.5 to 2.0, one octave down/up). */
    float pitchIncrement = 1.f;
    
    /** @brief Amount of glide for pitch changes (range: 0.5 to 2.0, one octave down/up, 1.0 = no glide). */
    float glideAmount = 1.f;
    
    /** @brief Flag indicating whether the grain is read in reverse. */
    bool reverse = false;
    
    /** @brief Panning value for the home channel (range: 0.0 to 1.0). */
    float panHomeChannel = 1.f;
    
    /** @brief Panning value for the neighboring channel (range: 0.0 to 1.0). */
    float panNeighbourChannel = 0.f;
};


// =======================================================================================
// MARK: - GRAIN PROPERTIES MANAGER
// =======================================================================================


/**
 * @class GrainPropertiesManager
 * @brief Manages grain properties and handles the configuration of grain synthesis parameters.
 *
 * The `GrainPropertiesManager` class provides methods to configure various properties
 * of grains, such as length, inter-onset intervals, pitch increment, glide amount, and panning.
 */
class GrainPropertiesManager
{
public:
    /**
     * @brief Sets up the grain manager with the provided sample rate.
     *
     * @param sampleRate_ The sample rate for the audio system.
     */
    void setup(float sampleRate_);
    
    /**
     * @brief Gets the next inter-onset interval.
     *
     * will be called each time the interonset counter has reached zero
     * if randomization is active, it randomizes the parameter in the given range
     *
     * @return The next inter-onset interval in samples.
     */
    int getNextInterOnset();
    
    /**
     * @brief Retrieves the properties for the next grain.
     *
     * @return Pointer to the next grain's properties.
     */
    GrainProperties* getNextGrainProperties();
    
    /**
     * @brief Sets the center length for grain duration.
     *
     * will be called when a new grain is born. This function calculates new random
     * values for Initial Delay, Grainlength and Panning, and defines an overall amplitude
     * for the envelope. Then it saves those variables to the `GrainProperties` struct.
     * The `Grain` object will copy those parameters.
     *
     * @param length_ The center length of the grain in samples.
     */
    void setLength(const uint length_) { lengthCenter = length_; }
    
    /**
     * @brief Sets the variation in grain length.
     *
     * @param variation_ The variation factor for grain length (0.0 ... 1.0)
     */
    void setLengthVariation(const float variation_);
    
    /**
     * @brief Sets the center inter-onset interval.
     *
     * @param interOnset_ The center inter-onset interval in samples.
     */
    void setInterOnset(const uint interOnset_) { interOnsetCenter = interOnset_; }
    
    /**
     * @brief Sets the variation in inter-onset intervals.
     *
     * @param variation_ The variation factor for inter-onset intervals. (0.0 ... 1.0)
     */
    void setInterOnsetVariation(const float variation_);
    
    /**
     * @brief Sets the initial delay for the read pointer.
     *
     * @param initDelay_ The center initial delay in samples.
     */
    void setInitDelay(const uint initDelay_) { initDelayCenter = initDelay_; }
    
    /**
     * @brief Sets the variation in the initial delay.
     *
     * @param variation_ The variation factor for initial delay. (0.0 ... 1.0)
     */
    void setInitDelayVariation(const float variation_);
    
    /**
     * @brief Sets the pitch increment value for pitching.
     *
     * @param incr_ The pitch increment (range: 0.5 to 2.0).
     */
    void setPitchIncrement(const float incr_) { props.pitchIncrement = incr_; }
    
    /**
     * @brief Sets the glide amount for pitch changes.
     *
     * @param glide_ The glide amount (range: 0.5 to 2.0).
     */
    void setGlideAmount(const float glide_) { props.glideAmount = glide_; }
    
    /**
     * @brief Sets whether the grain should be reversed.
     *
     * @param reverse_ A boolean value indicating if the grain should be reversed.
     */
    void setReverse(const bool reverse_) { props.reverse = reverse_; }
    
    /**
     * @brief Sets the variation in panning
     *
     * @param variation_ The variation value (range: 0.0 to 1.0).
     */
    void setPanningVariation(const float variation_);
    
    /**
     * @brief Gets the center inter-onset interval.
     *
     * @return The center inter-onset interval in samples.
     */
    const uint getInterOnset() const { return interOnsetCenter; }
    
private:
    GrainProperties props;                  ///< The properties of the current grain.
    
    int interOnsetCenter = 4410;            ///< Center value for the inter-onset interval in samples.
    int interOnsetRange = 0;                ///< Range of variation for the inter-onset interval.
    
    int lengthCenter = 2200;                ///< Center value for the grain length in samples.
    int lengthRange = 0;                    ///< Range of variation for the grain length.
    
    int initDelayCenter = 5;                ///< Center value for the initial delay in samples.
    int initDelayRange = 0;                 ///< Range of variation for the initial delay.
    
    float panningRange = 0.f;               ///< Range of variation for the grain's panning value.
    
    static const int MIN_INITDELAY, MAX_INITDELAY; ///< Minimum and maximum allowed initial delay values.
    int MIN_INTERONSET, MAX_INTERONSET;           ///< Minimum and maximum allowed inter-onset values.
    int MIN_GRAINLENGTH_SAMPLES, MAX_GRAINLENGTH_SAMPLES; ///< Minimum and maximum allowed grain length values in samples.
};


// =======================================================================================
// MARK: - GRAIN DATA
// =======================================================================================


/**
 * @class GrainData
 * @brief Manages data and behavior for an individual grain during granular synthesis.
 *
 * The `GrainData` class interacts with a `SourceData` object and modifies its behavior
 * based on grain properties such as pitch increment, glide, and reverse playback.
 */
class GrainData
{
public:
    /**
     * @brief Constructor for the GrainData class.
     *
     * Initializes the `GrainData` object with a source of data and grain properties.
     * copys necessary parameters from the `GrainProperties` object to member variables
     * calculates pitch and glide increment, and sets the read pointer accordingly
     * (higher pitches require the pointer to be further back in the past, since the pitch increment
     * moves faster forward)
     *
     * @param sourceData_ Pointer to the `SourceData` object that provides the source data.
     * @param props_ Pointer to the `GrainProperties` object that defines the properties of the grain.
     */
    GrainData(SourceData* sourceData_, GrainProperties* props_);
    
    /**
     * @brief Retrieves the next data value from the source, modified by the envelope.
     *
     * reads new sample from `SourceData` with linear interpolation
     * increments or decrements the pointer depending on the reverse flag
     * increments the pitch increment with the glide increment
     *
     * @param envelope_ The current envelope value that shapes the grain's amplitude.
     * @return The next data value from the source.
     */
    float getNextData(const float envelope_);
    
private:
    SourceData* sourceData = nullptr;   ///< Pointer to the source data object.
    float incr = 1.f;                   ///< Increment value for reading data, related to pitch.
    float glideIncr = 0.f;              ///< Increment value for pitch glide.
    float readPointer = 0;              ///< Current read position in the source data.
    const bool reverse;                 ///< Flag indicating whether the grain is played in reverse.
};


// =======================================================================================
// MARK: - GRAIN
// =======================================================================================


/**
 * @class Grain
 * @brief Represents an individual grain in granular synthesis.
 *
 * The `Grain` class handles the lifecycle of a grain, including its envelope and data retrieval.
 * Each grain uses a `GrainProperties` object to define its behavior and interacts with
 * a `SourceData` object to produce audio samples.
 */
class Grain
{
public:
    /**
     * @brief Constructs a `Grain` object with the specified properties and source data.
     *
     * This constructor initializes a new grain with the provided properties and creates
     * both the `GrainData` and envelope objects. It also sets the grain's lifetime and marks
     * it as alive.
     *
     * @param props_ Pointer to the `GrainProperties` object that defines the grain's properties.
     * @param sourceData_ Pointer to the `SourceData` object that provides the data for the grain.
     */
    Grain(GrainProperties* props_, SourceData* sourceData_);
    
    /**
     * @brief Deleted default constructor.
     *
     * This constructor is deleted to ensure that grains must be created with properties
     * and source data.
     */
    Grain() = delete;
    
    /**
     * @brief Destructor for the `Grain` class.
     *
     * Cleans up allocated `GrainData` and envelope objects.
     */
    ~Grain();
    
    /**
     * @brief Retrieves the next sample for the grain.
     *
     * This function fetches the next audio sample for the grain by multiplying the
     * data and the envelope values. It also decrements the life counter and marks the
     * grain as no longer alive if its lifetime has expired.
     *
     * @return The next audio sample for the grain.
     */
    float getNextSample();
    
    /**
     * @brief Retrieves the panning value for the home channel.
     *
     * @return The panning value for the home channel (range: 0.0 to 1.0).
     */
    const float getHomeChannelPanning() const { return panHomeChannel; }
    
    /**
     * @brief Retrieves the panning value for the neighboring channel.
     *
     * @return The panning value for the neighboring channel (range: 0.0 to 1.0).
     */
    const float getNeighbourChannelPanning() const { return panNeighbourChannel; }
    
public:
    bool isAlive = false;   ///< Flag indicating whether the grain is currently active.
    
private:
    Envelope* envelope = nullptr;    ///< Pointer to the envelope object that shapes the grain's amplitude.
    GrainData* data = nullptr;       ///< Pointer to the grain's data, which interacts with the source data.
    unsigned int lifeCounter;        ///< Counter tracking the remaining life of the grain in samples.
    
    const float panHomeChannel;      ///< Panning value for the home channel (range: 0.0 to 1.0).
    const float panNeighbourChannel; ///< Panning value for the neighboring channel (range: 0.0 to 1.0).
};


// =======================================================================================
// MARK: - GRANULATOR
// =======================================================================================


/**
 * @class Granulator
 * @brief A class for granular synthesis processing and parameter management.
 *
 * The `Granulator` class processes audio samples through granular synthesis,
 * applying various effects such as delay, filtering, and spatialization. It also
 * listens for parameter changes and adjusts the synthesis process accordingly.
 */
class Granulator
{
public:
    /**
     * @brief Sets up the granulator with the specified sample rate and block size.
     *
     * Initializes the necessary resources, configures the grain property manager,
     * and prepares the grain clouds and other DSP components like delay and filter.
     *
     * @param sampleRate_ The sample rate of the audio system.
     * @param blockSize_ The size of the audio block to process.
     * @return True if the setup is successful, false otherwise.
     */
    bool setup(const float sampleRate_, const uint blockSize_);
    
    /**
     * @brief Updates the granulator state, potentially adding new grains.
     *
     * This function checks the inter-onset counters and creates new grains if
     * necessary, storing them in the grain cloud for each audio channel.
     */
    void update();
    
    /**
     * @brief Processes a block of stereo audio samples through granular synthesis.
     *
     * Processes the input stereo samples, spatializes grains, applies delay, filtering,
     * and DC offset correction, and blends wet and dry signals.
     *
     * @param input_ The input stereo samples.
     * @param sampleIndex_ The index of the current sample within the block.
     * @return The processed stereo samples.
     */
    StereoFloat processAudioSamples(const StereoFloat input_, const uint sampleIndex_);
    
    /**
     * @brief Responds to changes in audio parameters.
     *
     * This function is called whenever a parameter changes, updating the corresponding
     * property in the granulator, such as grain length, pitch, wetness, and filter cutoff.
     *
     * @param parameterID The ID of the parameter that changed.
     * @param newValue The new value of the parameter.
     */
    void parameterChanged(const String parameterID, float newValue);
    
private:
    /// Enumeration for the audio channels (left and right).
    enum Channel { LEFT, RIGHT };
    
    float sampleRate;             ///< The sample rate of the audio system.
    uint blockSize;               ///< The size of the audio block to process.
    
    float32_t wet = 0.7f * GAIN_COMPENSATION; ///< Wet signal level for the granulator output.
    float dry = 0.3f;             ///< Dry signal level for the granulator output.
    
    float32_t delayWet = 0.f;     ///< Wet signal level for the delay effect.
    float32_t delayDry = 1.f;     ///< Dry signal level for the delay effect.
    float delaySpeedRatio = 1.f;  ///< Speed ratio for delay feedback timing.
    
    SourceData data[2];           ///< Audio source data for each channel.
    GrainPropertiesManager manager; ///< Manager for grain properties.
    
    std::vector<Grain*> grainCloud[2]; ///< The collection of active grains for each channel.
    size_t numActiveGrains[2] = { 0, 0 }; ///< Number of active grains for each channel.
    
    uint onsetCounter[2] = { 1, 1 };  ///< Counter for the time until the next grain onset.
    uint nextInterOnset[2] = { 0, 0 }; ///< Time until the next grain onset for each channel.
    
    FilterStereo filter;          ///< Stereo filter applied to the output.
    Delay delay;                  ///< Delay effect applied to the output.
    DCOffsetFilterStereo dcOffsetFilter; ///< DC offset filter applied to the output.
    
    static const float GAIN_COMPENSATION; ///< Compensation factor for gain adjustment.
};

} // namespace Granulation
