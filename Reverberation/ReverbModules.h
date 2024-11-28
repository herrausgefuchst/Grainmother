// =======================================================================================
//
// ReverbModules.h
/**
 * @file ReverbModules.h
 * @author Julian Fuchs
 * @date 04-August-2024
 * @version 1.0.0
 *
 * @brief implements all necessary sub classes for the BelaReverb
 * @details includes classes for: delay line, equalizing (low- & and highcut, parametric eq), tap delay, comb- and allpass filters
 */
// =======================================================================================

#pragma once

#include "../Helpers.hpp"

namespace Reverberation
{

/**
 * @defgroup EarlyReflectionStaticVariables
 * @brief all static variables concerning the Early Reflections
 * @{
 */

/** @brief the number of stereo taps for the calculation of early reflections */
static const unsigned int NUM_TAPS = 12;

/** @brief the delay in samples for the corresponding taps, array combines thre different room types each one having 2 sets of delay times for left and right channel processing */
static const std::array<std::array<float, NUM_TAPS>, 6> earliesDelaySamples = {{
    {{ 0.f, 52.10119629f, 529.6405029f, 886.6993408f, 1025.965698f, 1075.857056f, 1361.420288f, 2133.624512f, 2174.510254f, 3374.469238f, 4000.f, 5040.838379f }}, // Church Left
    {{ 0.f, 52.46435547f, 446.0803223f, 890.791626f, 1009.140503f, 1157.683228f, 1420.080688f, 2090.175781f, 2210.470703f, 3449.902344f, 4010.f, 5009.54834f }}, // Church Right
    {{ 0.f, 121.1566162f, 363.0744629f, 485.3317261f, 553.0261841f, 554.6968384f, 747.2711792f, 1040.465332f, 1644.89917f, 1730.990234f, 1840.f, 2313.053467f }}, // Foyer Left
    {{ 0.f, 120.4128418f, 361.3810425f, 483.3132935f, 552.5234985f, 624.8609009f, 663.7677612f, 1037.56543f, 1597.299072f, 1769.40918f, 1920.f, 2309.386475f }}, // Foyer Right
    {{ 0.f, 64.39950562f, 101.0967712f, 164.3284302f, 176.6047058f, 181.081604f, 210.6923523f, 312.2133484f, 351.9727478f, 467.f, 621.296875f, 836.8084717f }}, // Small Room Left
    {{ 0.f, 67.71453857f, 116.2631226f, 125.995697f, 139.8855591f, 218.2733765f, 252.7958679f, 282.3094788f, 454.9807434f, 492.f, 675.1395264f, 791.8198242f }} // Small Room Right
}};

/** @brief the left panning scalers for the corresponding taps, array combines thre different room types each one having 2 sets of delay times for left and right channel processing */
static const std::array<std::array<float, NUM_TAPS>, 6> earliesPanL = {{
    {{ 0.4109698534f, -0.4083514512f, 0.286886394f, -0.3686112463f, 0.2110758424f, -0.3990424871f, 0.67167449f, -0.1910956353f, 0.6272038817f, -0.1227137819f, 0.6214978695f, -0.2657240331f }},
    {{ -0.4115462899f, 0.4089060128f, -0.3076239228f, 0.3689430058f, -0.1895676255f, 0.403762579f, -0.6873666048f, 0.6214978695f, -0.1816269755f, 0.1296361983f, -0.1910956353f, 0.2738249302f }},
    {{ 0.4210068882f, -0.4074808061f, 0.3817590475f, -0.3952649832f, 0.5790299773f, -0.35500139f, 0.08921554685f, -0.3376656771f, 0.03652659059f, -0.4705567062f, 0.5209314823f, -0.2177925855f }},
    {{ -0.4204753637f, 0.4070479274f, -0.3814511299f, 0.3176043928f, -0.3772295713f, 0.1505738199f, -0.620565474f, 0.2788486481f, -0.5209314823f, 0.06757049263f, -0.03652659059f, 0.2413483709f }},
    {{ 0.4144642353f, -0.3979062736f, 0.7565126419f, -0.1478334069f, 0.4253483117f, -0.2656958401f, 0.3627090156f, -0.6439499855f, 0.1540976316f, -0.074416399f, 0.1527850181f, -0.3124459982f }},
    {{ -0.4191459417f, 0.4015572965f, -0.3354171216f, 0.6622888446f, -0.03078949079f, 0.3650498688f, -0.5860134959f, 0.4618220627f, -0.074416399f, 0.1540976316f, -0.1880542338f, 0.3658294678f }}
}};

/** @brief the right panning scalers for the corresponding taps, array combines thre different room types each one having 2 sets of delay times for left and right channel processing */
static const std::array<std::array<float, NUM_TAPS>, 6> earliesPanR = {{
    {{ 0.4109698534f, -0.4083514512f, 0.4833461642f, -0.3686112463f, 0.5136854053f, -0.321269542f, 0.02384052612f, -0.4335467815f, 0.00283287908f, -0.4206062555f, 0.0004129825684f, -0.1771449447f }},
    {{ -0.4115462899f, 0.4089060128f, -0.4716632962f, 0.3689430058f, -0.5376827121f, 0.3103525937f, -0.0004129825684f, 0.001542856102f, -0.4459724724f, 0.4094339013f, -0.02384052612f, 0.17136693f }},
    {{ 0.4210068882f, -0.4074808061f, 0.3817590475f, -0.3435036242f, 0.1463815123f, -0.3700834811f, 0.5991941094f, -0.2984366119f, 0.5039471984f, -0.05752024055f, 0.3569473028f, -0.2336093932f }},
    {{ -0.4204753637f, 0.4070479274f, -0.3814511299f, 0.420633018f, -0.347364217f, 0.5600293279f, -0.08262476325f, 0.3569473028f, -0.02582899109f, 0.4544064999f, -0.2984366119f, 0.2099298835f }},
    {{ 0.4144642353f, -0.3979062736f, 0.0210243687f, -0.5991928577f, 0.3158946633f, -0.4734492302f, 0.3627090156f, -0.03631126508f, 0.5092544556f, -0.2392712533f, 0.4065795243f, -0.1755780727f }},
    {{ -0.4191459417f, 0.4015572965f, -0.4433889985f, 0.111733377f, -0.736456275f, 0.3650498688f, -0.1283026189f, 0.2392712533f, -0.5540771484f, 0.03631126508f, -0.3586713374f, 0.1419659406f }},
}};

/** @brief the latest tap delay of each room type, used for calculation of delay of decay */
static const std::array<unsigned int, 3> earliesLatestDelaySamples = {{ 5000u, 2213u, 787u }};

/** @} */

// =======================================================================================
// MARK: - Simple Delay
// =======================================================================================

/**
 * @class SimpleDelayStereo
 * @brief The SimpleDelayStero object implements a basic delay line. It processes two channels synchronous.
 */
class SimpleDelayStereo
{
public:
    /**
     * @brief sets up the delay line
     *
     * @param delaySamples_ the initial delay in samples
     * @param maxDelaySamples_ the maximum value of delaySamples that can be set, used to initialize the bufferLength
     * @param sampleRate_ sample rate
     */
    void setup(const float& delaySamples_, const int& maxDelaySamples_, const float& sampleRate_)
    {
        // adjust length of buffer to the next fitting power of 2
        bufferLength = (unsigned int)powf_neon(2.f, 1.f + (ceilf_neon(logf_neon(maxDelaySamples_) / log_2)));
        
        // used for wrapping the pointers
        bufferWrap = bufferLength - 1;
        
        // initialize the buffer
        buffer.reset(new float32x2_t[bufferLength]);
        
        // set all values in buffer to 0.f
        std::fill(buffer.get(), buffer.get()+bufferLength, vdup_n_f32(0.f));
        
        // setup readPointer
        setDelay(delaySamples_);
    }
    
    /**
     * @brief Writes a new value into the buffer, increments the pointers, and reads out delayed samples
     *
     * @param input_ a vector of two floats
     * @return the delayed samples
     */
    float32x2_t processAudioSamples(const float32x2_t& input_)
    {
        // write buffer
        buffer[writePointer] = input_;
        
        // increase pointers
        writePointer = (writePointer + 1) & bufferWrap;
        readPointerLo = (readPointerLo + 1) & bufferWrap;
        readPointerHi = (readPointerHi + 1) & bufferWrap;
        
        // read buffer
        float32x2_t returnValue = buffer[readPointerLo];
        if (interpolationNeeded) {
            float32x2_t diff = vsub_f32(buffer[readPointerHi], returnValue);
            returnValue = vmla_n_f32(returnValue, diff, frac);
        }
        return returnValue;
    }
    
    /**
     * @brief sets a new delay
     *
     * @param delaySamples_ new delay in samples
     */
    void setDelay(const float& delaySamples_)
    {
        // set and bound delay, delay in samples should not ecxeed bufferlength
        delaySamples = delaySamples_ + 1.f;
        boundValue(delaySamples, 0.f, (float)bufferWrap);
        
        // interpolation stuff
        readPointerLo = writePointer - (int)delaySamples;
        readPointerHi = readPointerLo - 1;
        if (readPointerLo < 0) readPointerLo += bufferLength;
        if (readPointerHi < 0) readPointerHi += bufferLength;
        frac = delaySamples - (int)delaySamples;
        interpolationNeeded = frac == 0.f ? false : true;
    }
    
    float getDelay() const { return delaySamples; }
    
private:
    std::unique_ptr<float32x2_t[]> buffer = nullptr; ///< buffer of stereo float pairs holding previous samples
    unsigned int bufferLength = 1024; ///< the length of the buffer
    unsigned int bufferWrap = 1023; ///< bufferLength-1, used for wrapping the pointers
    unsigned int writePointer = 0; ///< write pointer for the buffer
    int readPointerLo = 0; ///< read pointer for the buffer
    int readPointerHi = 0;
    float frac = 0.f;
    bool interpolationNeeded = false;
    float delaySamples = 0;
};


// =======================================================================================
// MARK: - One Pole Lowpass Filter Stereo
// =======================================================================================

/**
 * @class OnePoleLowpassStereo
 * @brief a simple class processing a one pole lowpass filter, processes two channels synchronous
 */
class OnePoleLowpassStereo
{
public:
    /** sets up the filter */
    void setup(const float& feedbackGain_)
    {
        // initialize state variable
        state = vdup_n_f32(0.f);
        
        // setup feedback gain
        setFeedbackGain(feedbackGain_);
    }
    
    /** y(n) = (1-a) * x(n) + a * y(n-1) */
    void processAudioSamples(float32x2_t& x_)
    {
        // y(n) = (1-a) * x(n) + a * y(n-1)
        state = vmla_n_f32(vmul_n_f32(x_, g_1), state, g);
        
        // overwrite the input
        x_ = state;
    }
    
    /** sets a new feedback gain value */
    void setFeedbackGain(const float& feedbackGain_)
    {
        // set and bound new value
        g = feedbackGain_;
        boundValue(g, 0.f, 0.99999f);
        
        // helper variable
        g_1 = 1.f - g;
        
        // on/off flag
        enabled = g > 0.f ? true : false;
    }
    
    /** returns the momentary feedback gain */
    float32_t getFeedbackGain() const { return g; }
    
private:
    float32x2_t state; ///< the last state of y(n)
    float32_t g; ///< feedback gain
    float32_t g_1; ///< 1.0 - feedbackgain
    
public:
    bool enabled = false; ///< on/off flag
};


// =======================================================================================
// MARK: - Butterworth Highcut Stereo
// =======================================================================================

/**
 * @class ButterworthHighcutStereo
 * @brief a simple class processing a Butterworth Highcut Filter, processes two channels synchronous
 */
class ButterworthHighcutStereo
{
public:
    /** sets up the filter */
    void setup(const float& cutoffFreq_, const float& sampleRate_)
    {
        // calc inversed nyquist frequency
        nyquist_inv = 1.f / (sampleRate_ * 0.5f);
        
        // setup cutoff frequency
        setCutoffFrequency(cutoffFreq_);
    }
    
    /** sets new cutoff frequency */
    void setCutoffFrequency(const float& cutoffFreq_)
    {
        // set and bound value
        cutoffFrequency = cutoffFreq_;
        boundValue(cutoffFrequency, 100.f, 20000.f);
        
        // calculate filter coefficients
        calculateCoefficients();
        
        // set on/off flag
        enabled = cutoffFrequency < 20000 ? true : false;
    }
    
    /** y(n) = b0 * x(n) + b1 * x(n-1) + b2 * x(n-2) - a1 * y(n-1) - a2 * y(n-2) */
    void processAudioSamples(float32x2_t& x_)
    {
        // y(n) = b0 * x(n) + b1 * x(n-1) + b2 * x(n-2) - a1 * y(n-1) - a2 * y(n-2)
        float32x2_t y = vadd_f32(vmul_n_f32(x_, b0), vmul_n_f32(x1, b1));
        y = vmla_n_f32(y, x2, b2);
        y = vmls_n_f32(y, y1, a1);
        y = vmls_n_f32(y, y2, a2);
        
        // delay the state variables
        x2 = x1;
        x1 = x_;
        y2 = y1;
        y1 = y;
        
        // overwrite the input
        x_ = y;
    }
    
    /** returns the momentary cutoff frequency */
    const float& getCutoffFrequency() const { return cutoffFrequency; }
    
private:
    /** helper function caulculates the filter coefficients */
    void calculateCoefficients()
    {
        float normal_cutoff = cutoffFrequency * nyquist_inv;
        float omega_c = PI * normal_cutoff;
        float tan_half_wc = tanf_neon(omega_c * 0.5f);
        
        float b0_coef = tan_half_wc * tan_half_wc;
        
        // sets resonance, lower cutoff frequencys = more resonance
        float Q = 1.0f + (40.f / (cutoffFrequency + 1.0f));
        
        float sqrt2tanhalfwc = sqrt_2 * tan_half_wc / Q;
        float denom_inv = 1.f / (1.0f + sqrt2tanhalfwc + b0_coef);
        
        b0 = b0_coef * denom_inv;
        b1 = 2.0f * b0;
        b2 = b0;
        a1 = 2.0f * (b0_coef - 1.0f) * denom_inv;
        a2 = (1.0f - sqrt2tanhalfwc + b0_coef) * denom_inv;
    }
    
private:
    float cutoffFrequency; ///< cutoff frequency
    float nyquist_inv; ///< inversed nyquist frequency ( 1.0 / nyquist )
    
    /** the filter coefficients*/
    float32_t b0, b1, b2, a1, a2;
    
    /** the state variables */
    float32x2_t x1 = vdup_n_f32(0.f), x2 = vdup_n_f32(0.f);
    float32x2_t y1 = vdup_n_f32(0.f), y2 = vdup_n_f32(0.f);
    
public:
    bool enabled = false; ///< on/off flag
};


// =======================================================================================
// MARK: - Butterworth Lowcut Stereo
// =======================================================================================

/**
 * @class ButterworthLowcutStereo
 * @brief a simple class processing a Butterworth Lowcut Filter, processes two channels synchronous
 */
class ButterworthLowcutStereo
{
public:
    /** sets up the filter */
    void setup(const float& cutoffFreq_, const float& sampleRate_)
    {
        // calc inversed nyquist frequency
        nyquist_inv = 1.f / (sampleRate_ * 0.5f);
        
        // setup cutoff frequency
        setCutoffFrequency(cutoffFreq_);
    }
    
    /** sets new cutoff frequency */
    void setCutoffFrequency(const float& cutoffFreq_)
    {
        // set and bound value
        cutoffFrequency = cutoffFreq_;
        boundValue(cutoffFrequency, 20.f, 15000.f);
        
        // recalculate the filter coefficients
        calculateCoefficients();
        
        // set on/off flag
        enabled = cutoffFrequency > 20.f ? true : false;
    }
    
    /** y(n) = b0 * x(n) + b1 * x(n-1) + b2 * x(n-2) - a1 * y(n-1) - a2 * y(n-2) */
    void processAudioSamples(float32x2_t& x_)
    {
        // y(n) = b0 * x(n) + b1 * x(n-1) + b2 * x(n-2) - a1 * y(n-1) - a2 * y(n-2)
        float32x2_t y = vadd_f32(vmul_n_f32(x_, b0), vmul_n_f32(x1, b1));
        y = vmla_n_f32(y, x2, b2);
        y = vmls_n_f32(y, y1, a1);
        y = vmls_n_f32(y, y2, a2);
        
        // delay the state variables
        x2 = x1;
        x1 = x_;
        y2 = y1;
        y1 = y;
        
        // overwrite the input
        x_ = y;
    }
    
    /** returns the momentary cutoff frequency */
    const float& getCutoffFrequency() const { return cutoffFrequency; }
    
private:
    /** helper function caulculates the filter coefficients */
    void calculateCoefficients()
    {
        float normal_cutoff = cutoffFrequency * nyquist_inv;
        float omega_c = PI * normal_cutoff;
        float tan_half_wc = tanf_neon(omega_c * 0.5f);
        
        float sqrt2tanhalfwc = sqrt_2 * tan_half_wc;
        float tanhalfwc_sq = tan_half_wc * tan_half_wc;
        float denom_o1 = 1.f / (1.0f + sqrt2tanhalfwc + tanhalfwc_sq);
        
        b0 = denom_o1;
        b1 = -2.0f * b0;
        b2 = b0;
        a1 = 2.0f * (tanhalfwc_sq - 1.0f) * denom_o1;
        a2 = (1.0f - sqrt2tanhalfwc + tanhalfwc_sq) * denom_o1;
    }
    
private:
    float cutoffFrequency; ///< cutoff frequency
    float nyquist_inv; ///< inversed nyquist frequency ( 1.0 / nyquist )
    
    /** the filter coefficients*/
    float32_t b0, b1, b2, a1, a2;
    
    /** the state variables */
    float32x2_t x1 = vdup_n_f32(0.f), x2 = vdup_n_f32(0.f);
    float32x2_t y1 = vdup_n_f32(0.f), y2 = vdup_n_f32(0.f);
    
public:
    bool enabled = false; ///< on/off flag
};


// =======================================================================================
// MARK: - Parametric EQ Stereo
// =======================================================================================

/**
 * @class ParametricEQStereo
 * @brief a simple class processing a Parametric EQ Filter, processes two channels synchronous
 */
class ParametricEQStereo
{
public:
    /** sets up the filter */
    void setup(const float& centerFreq_, const float& gain_, const float& bandwidth_, const float& sampleRate_)
    {
        // inversed sample rate
        fs_inv = 1.f / sampleRate_;
        
        // bandwidth
        bandwidth = bandwidth_;
        bandwidth2 = 2.f * bandwidth_;
        
        // gain
        gain = gain_;
        A = powf_neon(10.0f, gain * 0.025f);
        A_o1 = 1.f / A;
        
        // center frequency
        setCenterFrequency(centerFreq_);
    }
    
    /** sets new center frequency */
    void setCenterFrequency(const float& centerFreq_)
    {
        // set and bound value
        centerFreq = centerFreq_;
        boundValue(centerFreq, 20.f, 20000.f);
        
        // calculate helper variables
        omega0 = TWOPI * centerFreq * fs_inv;
        cosOmega0 = cosf_neon(omega0);
        sinOmega0 = sinf_neon(omega0);
        alpha = sinOmega0 * sinhf_neon(log_2 / bandwidth2 * omega0 / sinOmega0);
        
        // recalculate filter coefficients
        calculateCoefficients();
    }
    
    /** sets new filter gain */
    void setGain(const float& gain_)
    {
        // set and bound value
        gain = gain_;
        boundValue(gain, -12.f, 12.f);
        
        // calculate helper variables
        A = powf_neon(10.0f, gain * 0.025f);
        A_o1 = 1.f / A;
        
        // recalculate filter coefficients
        calculateCoefficients();
        
        // set on/off flag
        enabled = gain != 0.f ? true : false;
    }
    
    /** sets new bandwidth */
    void setBandwidth(const float& bandwidth_)
    {
        // set
        bandwidth = bandwidth_;
        
        // calculate helper variables
        bandwidth2 = 2.0f * bandwidth;
        alpha = sinOmega0 * sinhf_neon(log_2 / bandwidth2 * omega0 / sinOmega0);
        
        // recalculate filter coefficients
        calculateCoefficients();
    }
    
    /** y(n) = b0 * x(n) + b1 * x(n-1) + b2 * x(n-2) - a1 * y(n-1) - a2 * y(n-2) */
    void processAudioSamples(float32x2_t& x_)
    {
        // y(n) = b0 * x(n) + b1 * x(n-1) + b2 * x(n-2) - a1 * y(n-1) - a2 * y(n-2)
        float32x2_t y = vadd_f32(vmul_n_f32(x_, b0), vmul_n_f32(x1, b1));
        y = vmla_n_f32(y, x2, b2);
        y = vmls_n_f32(y, y1, a1);
        y = vmls_n_f32(y, y2, a2);
        
        // delay the state variables
        x2 = x1;
        x1 = x_;
        y2 = y1;
        y1 = y;
        
        // overwrite the input
        x_ = y;
    }
    
    const float& getCenterFrequency() const { return centerFreq; }
    const float& getGain() const { return gain; }
    const float& getBandwidth() const { return bandwidth; }
    
private:
    /** helper function caulculates the filter coefficients */
    void calculateCoefficients()
    {
        float denominator_inv = 1.f / (1.0f + alpha * A_o1);
        
        b0 = (1.0f + alpha * A) * denominator_inv;
        b1 = (-2.0f * cosOmega0) * denominator_inv;
        b2 = (1.0f - alpha * A) * denominator_inv;
        a1 = (-2.0f * cosOmega0) * denominator_inv;
        a2 = (1.0f - alpha * A_o1) * denominator_inv;
    }
    
private:
    /** user definable filter parameters */
    float centerFreq, gain, bandwidth;
    
    float fs_inv; ///< inversed sample rate
    
    /** helper variables */
    float omega0, A, A_o1, alpha, cosOmega0, sinOmega0, bandwidth2;
    
    /** filter coefficients */
    float32_t b0, b1, b2, a1, a2;
    
    /** filter states */
    float32x2_t x1 = vdup_n_f32(0.f), x2 = vdup_n_f32(0.f);
    float32x2_t y1 = vdup_n_f32(0.f), y2 = vdup_n_f32(0.f);
    
public:
    bool enabled = false; ///< on/off flag
};


// =======================================================================================
// MARK: - Tap Delay Stereo
// =======================================================================================

/**
 * @class TapDelayStereo
 * @brief A helper class for EarlyReflections.
 *
 * Owns a two dimensional buffer saving the past stereo states and reads out the corresponding delays with a set of read pointers.
 */
class TapDelayStereo
{
public:
    using TapArray = std::array<std::array<float32x4_t, NUM_TAPS/4>, 2>;
    
    /**
     * @brief sets up the TapDelayStereo object
     *
     * @param room_ the room index controls where to read the delay times from
     * @param predelaySamples_ the predelay in samples
     * @param size_ the size mulitplier
     * @param blockSize_ the audio block size
     */
    void setup(const unsigned int& room_, const unsigned int& predelaySamples_, const float& size_, const unsigned int& blockSize_);
    
    /**
     * @brief reads out all taps by using linear interpolation, combines them in an array
     * @return a custom array of neon-vectors holding all the taps
     */
    TapArray& readTaps();
    
    /**
     * @brief returns a tap at a certain index
     *
     * @param channel_ the channel number 0 or 1
     * @param tap_ the tap index 0...NUM_TAPS
     *
     * @return the corresponding tap
     */
    float getTapAtIndex(const unsigned int& channel_, const unsigned int& tap_) const;
    
    /**
     * @brief writes new values into buffer, increments all the pointers
     * @param input_ a custom struct of two float input samples
     */
    void writeBuffer(const StereoFloat& input_);
    
    /**
     * @brief recalculates the tap delays
     *
     * each tap delay is the given early delay * the size parameter + predelay <br>
     * according to the new delay times it also precalculates the framents for linear interpolation and
     * resets the readpointers.
     *
     * @param room_ the room index controls where to read the delay times from
     * @param predelaySamples_ the predelay in samples
     * @param size_ the size mulitplier
     */
    void recalculateTapDelays(const unsigned int& room_, const float& predelaySamples_, const float& size_);
    
private:
    static const unsigned int bufferSize = 32768; ///< length of the buffer
    static const unsigned int bufferSizeWrap = 32767; ///< bufferlength-1, used for wrapping pointers
    
    unsigned int blockSize = 128; ///< audio block size
    
    std::array<std::array<float, bufferSize>, 2> buffer; ///< two dimensional buffer, holding the past stereo values
    
    unsigned int writePointer = 0; ///< write pointer for the buffer
    std::array<std::array<int, NUM_TAPS>, 2> readPointer; ///< the set of read pointers, each delay has one
    std::array<std::array<float32x4_t, NUM_TAPS/4>, 2> frac; ///< precalculated fracments for linear interpolation
    
    TapArray taps; ///< a custom array holding the momentary set of taps
};


// =======================================================================================
// MARK: - Allpass Filter Mono
// =======================================================================================

/**
 * @class AllpassFilterMono
 * @brief A simple one channel Allpass Filter module, no interpolation (delay is always rounded to int)
 */
class AllpassFilterMono
{
public:
    /** sets up the filter */
    bool setup(const float& feedbackGain_, const float& delayMs_, const float& sampleRate_)
    {
        // set feedback gain
        setFeedbackGain(feedbackGain_);
        
        // delay from ms to samples
        unsigned int delaySamples = delayMs_ * sampleRate_ * 0.001f;
        
        // set all values in buffer to 0.f
        std::fill(buffer.begin(), buffer.end(), 0.f);
        
        // setup readPointer (-1 because read before write!)
        readPointer = bufferLength - 1 - delaySamples;
        
        return true;
    }
    
    /**
     * @brief processes the allpass filter
     * @attention process function of AllpassFilterDualMono  is used instead
     *
     * y(n) = v(n) - g * w(n) where v(n) is the delay output and w(n) = x(n) + g * v(n) is the delay input
     */
    void processAudioSample(float& xn_)
    {
        // delay output vn
        float vn = readBuffer();
        
        // delay input wn = xn + g * wn
        float wn = xn_ + feedbackGain * vn;
        writeBuffer(wn);
        
        // output yn = vn - g * wn
        xn_ = vn - feedbackGain * wn;
        
        // check for underflow
        checkFloatUnderflow(xn_);
    }
    
    /** sets a new feedback gain */
    void setFeedbackGain(const float& feedbackGain_)
    {
        // set and bound feedback gain
        feedbackGain = feedbackGain_;
        boundValue(feedbackGain, -0.99999f, 0.99999f);
        
        // on/off flag
        enabled = feedbackGain != 0.f ? true : false;
    }
    
    /** reads the intenal buffer at read pointer index */
    float readBuffer() { return buffer[readPointer]; }
    
    /** writes ithe internal buffer at write pointer index, increments the read pointer */
    void writeBuffer(float input_)
    {
        // write new value into buffer
        buffer[writePointer] = input_;
        
        // increment read pointer
        readPointer = (readPointer + 1) & bufferWrap;
    }
    
    /** returns the momentary feedback gain */
    const float& getFeedbackGain() const { return feedbackGain; }
    
    /** increments the static member variable writePointer */
    static void incrementWritePointer() { writePointer = (writePointer + 1) & bufferWrap; }
    
private:
    static const unsigned int bufferLength = 1024; ///< fixed buffer length for this module
    static const unsigned int bufferWrap = 1023; ///< bufferLength - 1, used for wrapping pointers
    
    static unsigned int writePointer; ///< write position for the internal buffer, since bufferLength stays the same for all objects of this class, this variable can be static
    
    unsigned int readPointer = 0; ///< individual read pointer
    std::array<float, bufferLength> buffer; ///< the internal buffer holding past samples
    
    float feedbackGain = 0.f; ///< feedback gain of the filter
    
public:
    bool enabled = false; ///< on/off flag
};

// MARK: - Allpass Filter Dual Mono
// --------------------------------------------------------------------------------

/**
 * @class AllpassFilterDualMono
 * @brief a kind of wrapper for two individual AllpassFilterMono objects, used to be able to calculate with neon-instructions
 * @see AllpassFilterMono
 */
class AllpassFilterDualMono
{
public:
    std::array<AllpassFilterMono, 2> filters; ///< two AllpassFilterMono objects
    
    /**
     * @brief updates the internal feedback-neon-vector by catching the individual values from its AllpassFilterMono member objects
     * @attention be sure to call this every time one of the individual feedback gains has changed
     */
    void update()
    {
        feedbackGain = { filters[0].getFeedbackGain(), filters[1].getFeedbackGain() };
    }
    
    /**
     * @brief processes the allpass filter with neon-intrinsics
     * @param xn_ the input float samples
     * @see AllpassFilterMono
     */
    void processAudioSamples(float32x2_t& xn_)
    {
        // delay output vn
        float32x2_t vn = { filters[0].readBuffer(), filters[1].readBuffer() };
        
        // delay input wn = xn + g * vn;
        float32x2_t wn = vmla_f32(xn_, feedbackGain, vn);
        
        // write wn to buffer
        filters[0].writeBuffer(vget_lane_f32(wn,0));
        filters[1].writeBuffer(vget_lane_f32(wn,1));
        
        // output yn = vn - g * wn;
        xn_ = vmls_f32(vn, feedbackGain, wn);
    }
    
private:
    alignas(alignof(float32x2_t)) float32x2_t feedbackGain = vdup_n_f32(0.f); ///< a vector of the two inidividual feedbackgains
};


// =======================================================================================
// MARK: - Allpass Filter Stereo
// =======================================================================================

/**
 * @class AllpassFilterStereo
 * @brief The AllpassFilterStereo object implements a two channel Allpass Filter with LFO
 *
 * neon-intrinsics are used to efficiently process each sample-pair, channels get processed synchronous
 */
class AllpassFilterStereo
{
public:
    /**
     * @brief sets up the filter
     * @param feedbackGain_ the filters feedback gain
     * @param delaySamples_ the filters delay in samples
     * @param sampleRate_ sample rate
     * @return true if successful
     */
    bool setup(const float& feedbackGain_, const unsigned int& delaySamples_, const float& sampleRate_);
    
    /**
     * @brief updates the lfo phase and calculates new buffer read pointers
     * @attention this gets called in the process function, the rate of calls is determined beforehand
     * @param lfoIncrement_ step of change of the lfo phase, corresponds to the modulation rate
     * @param lfoDepth_ depth of modulation in samples
     */
    void updateLFO(const float& lfoIncrement_, const float& lfoDepth_);
    
    /**
     * @brief processes a stereo pair of samples
     * @see AllpassFilterMono
     * @param xn_ two stereo input samples
     */
    void processAudioSamples(float32x2_t& xn_);
    
    /** sets new feedback gain */
    void setFeedbackGain(const float& feedbackGain_);
    
    /** increments the write pointer */
    static void incrementWritePointer() { writePointer = (writePointer + 1) & bufferWrap; }
    
    /** reads out the buffer, uses linear interpolation if necessary */
    float32x2_t readBuffer();
    
    /** writes new samples to the buffer, increments read pointers */
    void writeBuffer(float32x2_t input_);
    
private:
    static const unsigned int bufferLength = 1024; ///< fixed buffer length for this module
    static const unsigned int bufferWrap = 1023; ///< bufferLength - 1, used for wrapping pointers
    
    static unsigned int writePointer; ///< write pointer for the internal buffer, since bufferLength stays the same for all objects of this class, this variable can be static
    
    std::array<float32x2_t, bufferLength> buffer; ///< the internal buffer holding past samples
    
    int readPointerLo = 0; ///< integer read pointers next to the float read position
    int readPointerHi = 0; ///< integer read pointers next to the float read position
    float32_t readPointerFrac = 0.f; ///< fracment for linear interpolation
    bool interpolationNeeded = false; ///< fllag for efficiency purposes
    
    unsigned int delaySamples = 0; ///< the fixed filters delay in samples
    
    float32_t g = 0.f; ///< feedback gain
    float lfoPhase = 0.f; ///< lfo phase 0...2PI
};

// =======================================================================================
// MARK: - Comb Filter Stereo
// =======================================================================================

class CombFilterDualStereo;

/**
 * @class CombFilterStereo
 * @brief implements a Comb Filter with a one pole lowpass in the feedback loop and additional modulation of the delay
 *
 * neon-intrinsics are used to efficiently process each sample-pair, channels get processed synchronous
 */
class CombFilterStereo
{
public:
    /**
     * @brief sets up the comb filter
     * @param delaySamples_ the comb filter delay in samples
     * @param damping_ the lowpass filter gain
     * @param sampleRate_ the sample rate
     * @param phaseShift_ flag, this determines if the output samples get phase shifted after processing
     * @attention the variable @p phaseShift_ is deprecated since we use a CombFilterDualStereo object that handles this issue in its process function
     * @return true if successful
     */
    bool setup(const unsigned int& delaySamples_, const float& damping_, const float& sampleRate_, const bool& phaseShift_);
    
    /**
     * @brief updates the lfo phase and calculates new buffer read pointers
     * @attention this gets called in the process function, the rate of calls is determined beforehand
     * @param lfoIncrement_ step of change of the lfo phase, corresponds to the modulation rate
     * @param lfoDepth_ depth of modulation in samples
     */
    void updateLFO(const float& lfoIncrement_, const float& lfoDepth_);
    
    /** @brief resets the read pointers when user chooses to stop the modulation */
    void stopModulating();
    
    /**
     * @brief processes the comb filter with lowpass in feedback loop
     * @attention process function of CombFilterDualStereo is used instead
     *
     * y(n) = x(n-D) + gComb * [(1-gLP) * y(n-D) + gLP * y(n-D-1)]
     *
     * @param xn_ a pair of input samples
     * @return the processed pair of input samples
     */
    float32x2_t processAudioSample(float32x2_t xn_);
    
    /** sets new feedback gain */
    void setFeedbackGain(const float& feedbackGain_);
    
    /** sets new lowpass feedback gain */
    void setLowpassFeedbackGain(const float& lowpassFeedbackGain_);
    
    /** returns the momentary delay in samples */
    unsigned int getDelaySamples() const { return delaySamples; }
    
    /** increments the write pointer */
    static void incrementWritePointer() { writePointer = (writePointer + 1) & bufferWrap; }
    
    /** reads out the buffer, uses linear interpolation if necessary */
    float32x2_t readBuffer();
    
    /** writes new samples to the buffer, increments read pointers */
    void writeBuffer(float32x2_t input_);
    
    friend class CombFilterDualStereo;
    
private:
    static const unsigned int bufferLength = 8192; ///< fixed buffer length for this module
    static const unsigned int bufferWrap = 8191; ///< bufferLength - 1, used for wrapping pointers
    
    static unsigned int writePointer; ///< write pointer for the internal buffer, since bufferLength stays the same for all objects of this class, this variable can be static
    
    std::array<float32x2_t, bufferLength> buffer; ///< the internal buffer holding past samples
    
    int readPointerLo = 0; ///< integer read pointers next to the float read position
    int readPointerHi = 0; ///< integer read pointers next to the float read position
    float32_t readPointerFrac = 0.f; ///< fracment for linear interpolation
    bool interpolationNeeded = false; ///< fllag for efficiency purposes
    
    unsigned int delaySamples = 0; ///< the fixed filters delay in samples
    
    float gComb = 0.707f; ///< comb filter feedback gain
    float gLP = 0.f; ///< lowpass filter feedback gain
    
    /** filter coefficients */
    float32_t b0, b1;
    float32x2_t lowpassState = vdup_n_f32(0.f); ///< the last state of y(n)
    
    float lfoPhase = 0.f; ///< lfo phase 0...2PI
    
    bool phaseShift; ///< flags if output is being phase shifted, not used
};


// MARK: - Comb Filter Dual Stereo
// --------------------------------------------------------------------------------

/**
 * @class CombFilterDualStereo
 * @brief a kind of wrapper for two individual CombFilterStereo objects, used to be able to calculate with neon-instructions
 * @see CombFilterStereo
 */
class CombFilterDualStereo
{
public:
    std::array<CombFilterStereo, 2> filters; ///< two CombFilterStereo objects
    
    /**
     * @brief updates the internal coefficient-neon-vectors by catching the individual values from its CombFilterStereo member objects
     * @attention be sure to call this every time one of the individual filter parameters has changed
     */
    void update();
    
    /**
     * @brief processes the comb filter with neon-intrinsics
     * @param xn_ the input float samples
     * @see CombFilterStereo
     */
    float32x2_t processAudioSampleInParallel(float32x2_t xn_);
    
private:
    /** vectors of 4 filter coefficents, two each are copied from the corresponding CombFilterStereo objects */
    alignas(alignof(float32x4_t)) float32x4_t b0, b1;
    /** vector of 4 lowpass states, two each are copied from the corresponding CombFilterStereo objects */
    alignas(alignof(float32x4_t)) float32x4_t lowpassState = vdupq_n_f32(0.f);
};

} // namespace Reverberation
