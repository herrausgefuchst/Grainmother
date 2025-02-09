#ifndef helpers_hpp
#define helpers_hpp

#include "Functions.h"


// =======================================================================================
// MARK: - LINEAR RAMP
// =======================================================================================

/**
 * @class LinearRamp
 * @brief The RampLinear object implements a linear fade between two values.
 *
 * This class is used for parameters that glitch or crackle when changing them in the UI too fast. One can set the time, the ramp needs to process.
 */
class LinearRamp
{
public:
    /** ()-operator returns the momentary value.*/
    const float& operator() () const
    {
        return value;
    }
    
    /** !=-operator compres the two momentary values of both objects */
    bool operator!= (const LinearRamp& otherRamp) const
    {
        if (value != otherRamp() || !rampFinished) return true;
        else return false;
    }
    
    /** =-operator sets a new value directly, without ramping it */
    void operator= (const float& newValue_)
    {
        setValueWithoutRamping(newValue_);
    }
    
    /**
     * @brief sets up the ramp
     * @param initialValue_ the start value
     * @param sampleRate_ sample rate
     * @param blocksize_ rates how often the ramp should be processed
     * @param blockwiseProcessing_ if yes, the increments will be calculated accordingly
     *
     * blockSize_ can be audio blocksize, but can also be something else. Just be sure to call the processRamp() function in the same rate.
     */
    void setup(const float& initialValue_, const float& sampleRate_, const unsigned int& blocksize_,
               const bool& blockwiseProcessing_ = true);
    
    void setID(const String& id_) { id = id_; }
    
    /** increments value, deincrements counter and sets finished-flag if counter is off */
    bool processRamp();
    
    /** sets value and target to the same value, no ramping needed */
    void setValueWithoutRamping(const float& newValue_);
    
    /** sets a new target value for the ramp with a certain time */
    void setRampTo(const float& target_, const float& time_sec);
    
    /** returns the current value,  ()-operator does the same */
    const float& getValue() const { return value; }
    
    const float& getTarget() const { return target; }

private:
    String id = "";
    float incr = 0.f; ///< the increment step of the ramp
    float value = 0.f; ///< the current value
    float target = 0.f; ///< the target value of the ramp
    int counter = 0; ///< counts if ramp has finished
    float fs, blocksize_inv;
    bool blockwiseProcessing = false;
public:
    bool rampFinished = true;
};


// =======================================================================================
// MARK: - DEBOUNCER
// =======================================================================================

/**
 * @class Debouncer
 * @brief A class for debouncing digital signals.
 *
 * The Debouncer class is designed to filter out noise in digital input signals by
 * implementing a time-based debounce mechanism. It transitions between stable states
 * (OPENED and CLOSED) and intermediate states (JUSTOPENED and JUSTCLOSED) based on a
 * configurable debounce time.
 */
class Debouncer
{
public:
    /**
     * @brief Deleted default constructor.
     *
     * The Debouncer must be constructed with a specific debounce time and default state.
     */
    Debouncer() = delete;

    /**
     * @brief Constructs a Debouncer instance with a specified debounce time and default state.
     * @param _debounceunits The number of units to wait for signal stabilization.
     * @param _defaultstate The default initial state (OPENED by default).
     */
    Debouncer(const int _debounceunits, const int _defaultstate = OPENED)
        : state(INT2ENUM(_defaultstate, State))
        , counter(_debounceunits)
        , debounceunits(_debounceunits)
    {}

    /**
     * @brief Updates the debouncer state based on the raw input value.
     *
     * This function processes a raw input signal, transitioning between stable
     * and intermediate states based on the debounce logic and timing. It returns
     * the stable state of the signal after debouncing.
     *
     * @param _rawvalue The raw input signal (true for CLOSE, false for OPEN).
     * @return The stable state of the signal (true for CLOSE, false for OPEN).
     */
    bool update(const bool _rawvalue);

private:
    /**
     * @enum State
     * @brief Represents the internal states of the debouncer.
     *
     * - **OPENED:** Stable open state.
     * - **CLOSED:** Stable closed state.
     * - **JUSTOPENED:** Transitioning to open state.
     * - **JUSTCLOSED:** Transitioning to closed state.
     */
    enum State { OPENED, CLOSED, JUSTOPENED, JUSTCLOSED };

    /**
     * @enum Input
     * @brief Represents the possible raw input states.
     *
     * - **CLOSE:** Input signal indicates a closed state.
     * - **OPEN:** Input signal indicates an open state.
     */
    enum Input { CLOSE, OPEN };

    State state; ///< Current state of the debouncer.
    int counter; ///< Countdown timer for transitioning between states.
    const int debounceunits; ///< Number of units to wait for signal stabilization.
};


// =======================================================================================
// MARK: - EFFECT AVERAGER
// =======================================================================================


/**
 * @class EffectAverager
 * @brief Implements a small moving averager of stereo  input samples over a large buffer.
 *
 * The `EffectAverager` class provides functionality to
 * maintain a running average, and determine if the average is near zero.
 * It uses a fixed-size buffer and NEON SIMD operations for efficiency. The class can be used
 * to determine whether an effect could be turned off or not.
 */
class EffectAverager
{
public:
    EffectAverager();
    ~EffectAverager() {}

    /**
     * @brief updates the running average.
     *
     * This method subtracts the oldest sample in the buffer from the average,
     * stores the new input sample (scaled by the buffer length), and adds the
     * scaled input to the average. The buffer is updated in a circular manner.
     *
     * @param input_ The new input audio sample to be processed (as a 2-channel vector).
     */
    void processAudioSamples(float32x2_t input_);

    /**
     * @brief Checks if the running average is near zero.
     *
     * Compares each channel of the running average with a small epsilon value
     * to determine if they are both close to zero.
     *
     * @return `true` if both channels of the average are near zero, `false` otherwise.
     */
    bool isNearZero();

private:
    static constexpr uint bufferLength = 65536; /**< Length of the circular buffer. */
    static constexpr float32_t fracment = 1.f / static_cast<float>(bufferLength); /**< Scaling factor for averaging. */
    uint writePointer = 0; /**< Index to the current write position in the buffer. */
    float32x2_t buffer[65536]; /**< Circular buffer to store audio samples. */
    float32x2_t average; /**< Current running average of the audio samples. */
};

#endif /* helpers_hpp */
