// TODO: Modulation Rate doesnt initalize correctly
// TODO: resonance of highcut filter is too strong
// TODO: documentation renaming

// =======================================================================================
//
// BelaReverb.h
/**
 * @file BelaReverb.h
 * @author Julian Fuchs
 * @date 04-August-2024
 * @version 1.0.0
 *
 * @brief This file implements a reverb architecture that is more or less in the style of Schroeder and Moorer.
 *
 * The BelaReverb simulates 24 early reflections using a tap delay, then sends this signal into a late reverberation algorithm similar to those from Schroeder and Moorer. The late reverberation consists of a series of all-pass filters, a set of parallel comb filters, and another set of series all-pass filters. To combine these two elements correctly, the decay has to be delayed to align right after the latest early reflection. In addition, some equalizers can shape the sound. A parametric EQ is implemented pre-FX and used as a “multiplier” of the input signal. A low-cut and high-cut filter can be set upon request; these are implemented post-FX.
 *
 * The user can change the reverb type, which involves many internal parameter changes (referred to as typeParameters). The EarlyReflections class includes three different reflection simulations: Church, Foyer, and Bathroom. This set of tap delays changes according to the type, as well as a diffusion factor (the all-pass filter gain in the early reflections) and a damping factor (low-pass gain in the early reflections). The decay primarily adjusts the composition of all-pass and comb filters and their corresponding delays. Additionally, damping, diffusion, and the rate and depth of modulating the all-pass filter delay times are type parameters.
 *
 * Four different types have been implemented:
 * •    CHURCH
 * •    DIGITAL VINTAGE
 * •    SEASICK
 * •    ROOM
 *
 * You can look up the corresponding parameter sets in the BelaReverb::setReverbType() function.
 */
// =======================================================================================
#pragma once

#include "ReverbModules.h"

/**
 * @defgroup BelaReverbParameters
 * @brief all static variables concerning the reverb UI parameters
 * @{
 */

namespace Reverberation
{

/** @brief number of reverb types */
static const unsigned int NUM_TYPES = 4;

/** @brief the reverb type-enum */
enum class ReverbTypes {
    CHURCH,
    DIGITALVINTAGE,
    SEASICK,
    ROOM
};

static const std::string reverbTypeNames[NUM_TYPES] = {
    "Church",
    "Digital Vintage",
    "Seasick",
    "Room"
};

/** @brief the number of user definable parameters */
static const unsigned int NUM_PARAMETERS = 12;

/** @brief an enum to save the parameter Indexes */
enum class Parameters
{
    DECAY,
    PREDELAY,
    MODRATE,
    MODDEPTH,
    SIZE,
    FEEDBACK,
    HIGHCUT,
    MIX,
    TYPE,
    LOWCUT,
    MULTFREQ,
    MULTGAIN
};

/** @brief names of parameters */
static const std::string parameterID[NUM_PARAMETERS] = {
    "reverb_decay",
    "reverb_predelay",
    "reverb_modrate",
    "reverb_moddepth",
    "reverb_size",
    "reverb_feedback",
    "reverb_highcut",
    "reverb_mix",
    "reverb_type",
    "reverb_lowcut",
    "reverb_multfreq",
    "reverb_multgain"
};

/** @brief names of parameters */
static const std::string parameterName[NUM_PARAMETERS] = {
    "Decay",
    "Predelay",
    "Modulation Rate",
    "Modulation Depth",
    "Size",
    "Feedback",
    "Highcut",
    "Reverb Mix",
    "Reverb Type",
    "Lowcut",
    "Multiplier Freq",
    "Multiplier Gain"
};

/** @brief minimum values of parameters */
static const float parameterMin[NUM_PARAMETERS] = {
    0.3f,
    0.f,
    0.01f,
    0.f,
    10.f,
    0.f,
    200.f,
    0.f,
    0.f,
    20.f,
    80.f,
    -12.f
};

/** @brief maximum values of parameters */
static const float parameterMax[NUM_PARAMETERS] = {
    20.f,
    150.f,
    30.f,
    100.f,
    300.f,
    0.99f,
    20000.f,
    100.f,
    (float)(NUM_TYPES-1),
    1500.f,
    3000.f,
    12.f
};

/** @brief step values of parameters */
static const float parameterStep[NUM_PARAMETERS] = {
    0.1f,
    1.f,
    0.5f,
    0.5f,
    1.f,
    0.01f,
    10.f,
    1.f,
    1.f,
    10.f,
    10.f,
    0.5f
};

/** @brief units of parameters */
static const std::string parameterSuffix[NUM_PARAMETERS] = {
    " sec",
    " msec",
    " hertz",
    " %",
    " %",
    "",
    " hertz",
    " %",
    "",
    " hertz",
    " hertz",
    " dB"
};

/** @brief initial values of parameters */
static const float parameterInitialValue[NUM_PARAMETERS] = {
    1.7f,
    25.f,
    5.f,
    0.f,
    100.f,
    0.f,
    20000.f,
    100.f,
    3.f,
    20.f,
    120.f,
    0.f
};

/**
 * @brief determines the number of samples after which the ramp process function is called
 * @attention has to be a power of 2!
 */
static const unsigned int RAMP_UPDATE_RATE = 2;

static const unsigned int LFO_UPDATE_RATE = 8;

/** @brief compensates for gain loss in effect chain */
static const float32_t GAIN_COMPENSATION = 1.1f;

/** @} */

// =======================================================================================
// MARK: - Early Reflector
// =======================================================================================

/**
 * @struct EarlyReflectionsTypeParameters
 * @brief parameter structrue of fixed constant parameters of the EarlyReflections object
 *
 * these parameters will be initialized when a new reverb type is set,  they define the type of reverb
 */
struct EarlyReflectionsTypeParameters
{
    enum Room { CHURCH, FOYER, SMALLROOM };

    EarlyReflectionsTypeParameters(const Room& room_,
                                   const float& diffusion_,
                                   const float& damping_,
                                   const unsigned int& latestDelaySamples_)
        : room(room_)
        , diffusion(diffusion_)
        , damping(damping_)
        , latestDelaySamples(latestDelaySamples_)
    {
        for (unsigned int n = 0, idx = 0; n < 3; ++n, idx+=4)
        {
            panL[0][n] = vld1q_f32(earliesPanL[room*2].data() + idx);
            panL[1][n] = vld1q_f32(earliesPanL[room*2+1].data() + idx);
            panR[0][n] = vld1q_f32(earliesPanR[room*2].data() + idx);
            panR[1][n] = vld1q_f32(earliesPanR[room*2+1].data() + idx);
        }
    }

    
    EarlyReflectionsTypeParameters (const EarlyReflectionsTypeParameters& other)
        : room(other.room)
        , diffusion(other.diffusion)
        , damping(other.damping)
        , latestDelaySamples(other.latestDelaySamples)
    {
        for (unsigned int n = 0, idx = 0; n < 3; ++n, idx+=4)
        {
            panL[0][n] = vld1q_f32(earliesPanL[room*2].data() + idx);
            panL[1][n] = vld1q_f32(earliesPanL[room*2+1].data() + idx);
            panR[0][n] = vld1q_f32(earliesPanR[room*2].data() + idx);
            panR[1][n] = vld1q_f32(earliesPanR[room*2+1].data() + idx);
        }
    }
    
    const Room room; ///< the room type
    const float diffusion; ///< controls the gain of the allpassfilter
    const float damping; ///< controls the gain of the lowpass filter
    const unsigned int latestDelaySamples; ///< latest tap delay of momentary roomtype
    std::array<float32x4_t, 3> panL[2]; ///< left panning scaler values of momentary roomtype
    std::array<float32x4_t, 3> panR[2]; ///< right panning scaler values of momentary roomtype
};

/**
 * @struct EarlyReflectionsParameters
 * @brief custom parameter structrue for the EarlyReflections object
 */
struct EarlyReflectionsParameters
{
    EarlyReflectionsParameters& operator=(const EarlyReflectionsParameters& params)
    {
        if (this == &params) return *this;

        feedbackEnabled = feedback() > 0.f ? true : false;

        return *this;
    }
    
    LinearRamp size; ///< a multiplier for the tap delay times
    LinearRamp predelay; ///< time till the first tap
    LinearRamp feedback; ///< feedbackloop gain
    bool feedbackEnabled = false; ///< flag for efficienxy purposes
};

/**
 * @class EarlyReflections
 * @brief handles processing of the early reflections
 *
 * processes all taps of TapDelayStereo with their corresponding pan values, lowpasses allpasses and feedbacks the input
 */
class EarlyReflections
{
public:
    using EarlyReflectionsTypeParametersPtr = std::unique_ptr<EarlyReflectionsTypeParameters, AlignedDeleter<EarlyReflectionsTypeParameters>>;
    
    /**
     * @brief sets up the early reflection members
     *
     * @param sampleRate_ the sample rate
     * @param blockSize_ num samples in one audio block
     */
    void setup(const float& sampleRate_, const float& blockSize_);
    
    /**
     * @brief processes incoming stereo samples
     *
     * creates the new input for the tapdelay-buffer:
     * 1. alpasses the input samples
     * 2. adds a definable amount of feedback times the 4th early reflection in the tapdelay
     * 3. lowpasses the allpassed feedbacked signal
     *
     * reads the taps from TapDelayStereo and multiplies them with respective pan-values,
       then sums up all the taps and returns a stereo float vector
     *
     * @param input_  a vector of a pair of floats
     * @param sampleIndex_ (0...blocksize) the momentary index of the audioblock sample
     *
     * @return the processed audio samples
     */
    float32x2_t processAudioSamples(const float32x2_t input_, const unsigned int& sampleIndex_);
    
    /**
     * @brief updates the ramp parameters size and predelay
     *
     * invokes the ramp's process functions if the ramps are not stable yet <br>
     * if a ramp has processed a new output value, the tapdelay has to be updated accordingly
     *
     */
    void updateRamps();
    
    /**
     * @brief sets a new set of parameters
     *
     * set the following parameters:
     * - predelay
     * - size
     *
     * @param parameters_ the new set of parameter
     */
    void setParameters(const EarlyReflectionsParameters& parameters_);
    
    /**
     * @brief sets a new set of reverb type  parameters
     *
     * set the following parameters:
     * - room, which includes different tap delays and panning scaler values
     * - diffusion (the amount of allpass filtering)
     * - damping (the amount of lowpass filtering)
     *
     * @param typeParameters_ a custom struct of type parameters
     */
    void setTypeParameters(const EarlyReflectionsTypeParameters& typeParameters_);
    
    /**
     * @brief returns momentary set of parameters
     * @return momentary set of parameters
     */
    const EarlyReflectionsParameters& getParameters() const { return parameters; }
    
    /**
     * @brief return the latest tap delay, gets called when calculating the delay of the decay
     * @return the latest tap delay in samples
     */
    unsigned int getLatestTapDelay() const
    {
        if (typeParameters) return typeParameters->latestDelaySamples * parameters.size.getTarget();
        else return 0;
    }

private:
    EarlyReflectionsParameters parameters; ///< a custom struct of user definable parameters
    EarlyReflectionsTypeParametersPtr typeParameters = nullptr; ///< a custom struct of reverb type parameters, set when changing the reverb typepöp

    TapDelayStereo tapDelay; ///< a helper class to read the tap delays
    OnePoleLowpassStereo lowpass; ///< a one pole lowpass filter in stereo format, synchronized channel processing
    AllpassFilterDualMono allpass; ///< a simple allpass filter in dual mono format, indepent channel processing
};


// =======================================================================================
// MARK: - Decay
// =======================================================================================

/**
 * @struct DecayTypeParameters
 * @brief parameter structrue of fixed constant parameters for the Decay object
 *
 * these parameters will be initialized when Decay constructor is called, they define the type of reverb
 */
struct DecayTypeParameters
{
    DecayTypeParameters(const std::string& name_,
                        const float diffusion_,
                        const float damping_,
                        const unsigned int numCombFilters_,
                        std::initializer_list<int> combDelays_,
                        const unsigned int numPreAllpassFilters_,
                        std::initializer_list<int> allpassPreDelays_,
                        const unsigned int numPostAllpassFilters_,
                        std::initializer_list<int> allpassPostDelays_,
                        const float combScaler_,
                        const float allpassModulationRate_ = 0.f,
                        const float allpassModulationDepth_ = 0.f,
                        const float sampleRate_ = 444100.f)
        : name(name_)
        , diffusion(diffusion_)
        , damping(damping_)
        , numCombFilters(numCombFilters_)
        , numPreAllpassFilters(numPreAllpassFilters_)
        , numPostAllpassFilters(numPostAllpassFilters_)
        , combDelaySamples(setDelays(combDelays_, numCombFilters))
        , allpassPreDelaySamples(setDelays(allpassPreDelays_, numPreAllpassFilters))
        , allpassPostDelaySamples(setDelays(allpassPostDelays_, numPostAllpassFilters))
        , combScaler(combScaler_)
        , allpassModulationIncr(TWOPI * allpassModulationRate_ * (1.f / sampleRate_) * 8.f)
        , allpassModulationDepth(allpassModulationDepth_)
    {
        allpassModulationEnabled = (allpassModulationIncr == 0.f || allpassModulationDepth == 0.f) ? false : true;
        allpassPreEnabled = numPreAllpassFilters == 0 ? false : true;
        allpassPostEnabled = numPostAllpassFilters == 0 ? false : true;
        halfNumCombFilters = numCombFilters / 2;
    }

    DecayTypeParameters(const DecayTypeParameters& other)
        : name(other.name)
        , diffusion(other.diffusion)
        , damping(other.damping)
        , numCombFilters(other.numCombFilters)
        , numPreAllpassFilters(other.numPreAllpassFilters)
        , numPostAllpassFilters(other.numPostAllpassFilters)
        , combDelaySamples(new int[other.numCombFilters])
        , allpassPreDelaySamples(new int[other.numPreAllpassFilters])
        , allpassPostDelaySamples(new int[other.numPostAllpassFilters])
        , combScaler(other.combScaler)
        , allpassModulationEnabled(other.allpassModulationEnabled)
        , allpassModulationIncr(other.allpassModulationIncr)
        , allpassModulationDepth(other.allpassModulationDepth)
    {
        std::memcpy(combDelaySamples.get(), other.combDelaySamples.get(), numCombFilters * sizeof(int));
        std::memcpy(allpassPreDelaySamples.get(), other.allpassPreDelaySamples.get(), numPreAllpassFilters * sizeof(int));
        std::memcpy(allpassPostDelaySamples.get(), other.allpassPostDelaySamples.get(), numPostAllpassFilters * sizeof(int));
        allpassPreEnabled = numPreAllpassFilters == 0 ? false : true;
        allpassPostEnabled = numPostAllpassFilters == 0 ? false : true;
        halfNumCombFilters = numCombFilters / 2;
    }

    const std::string name; ///< name of the reverb type
    const float diffusion; ///< controls the amount of feedback in the allpass filters
    const float damping; ///< controls the amount of lowpass in the combfilters
    const unsigned int numCombFilters; ///< number of comb filters
    const unsigned int numPreAllpassFilters; ///< number of pre allpass filters
    const unsigned int numPostAllpassFilters; ///< number of post allpass filters
    unsigned int halfNumCombFilters;
    const std::unique_ptr<int[]> combDelaySamples = nullptr; ///< the delay times in samples for the comb filters
    const std::unique_ptr<int[]> allpassPreDelaySamples = nullptr; ///< the delay times in samples for the pre-allpassfilter
    const std::unique_ptr<int[]> allpassPostDelaySamples = nullptr; ///< the delay times in samples fot the post-allpassfilters
    bool allpassPreEnabled, allpassPostEnabled;
    const float combScaler; ///< a scale value for the parallel processing of combfilters
    bool allpassModulationEnabled;
    const float allpassModulationIncr; ///< step of change in phase when modulation is called
    const float allpassModulationDepth; ///< depth of modulation in samples

    std::unique_ptr<int[]> setDelays(std::initializer_list<int> delays_, const unsigned int numDelays_)
    {
        std::unique_ptr<int[]> delays = std::make_unique<int[]>(numDelays_);
        int n = 0;
        for (int elem : delays_) delays[n++] = elem;
        return delays;
    }
};

/**
 * @struct DecayParameters
 * @brief custom parameter structrue for the Decay object
 */
struct DecayParameters
{
    DecayParameters& operator=(const DecayParameters& params)
    {
        if (this == &params) return *this;

        decayTimeMs = params.decayTimeMs;
        modulationRate = params.modulationRate;
        
        return *this;
    }
    
    float decayTimeMs = parameterInitialValue[static_cast<int>(Parameters::DECAY)] * 1000.f; ///< the rt60 time in miliseconds
    float modulationRate = parameterInitialValue[static_cast<int>(Parameters::MODRATE)]; ///< the modulation rate of combfilters in hertz
    LinearRamp modulationDepth; ///< the modulation depth of combfilters in samples
};


/**
 * @class Decay
 * @brief handles processing of the late reverberation
 *
 * combines series processing of allpass filters and parallel processing of combfilters
 */
class Decay
{
public:
    using CombFilterDualStereoPtr = std::unique_ptr<CombFilterDualStereo[], AlignedDeleterArray<CombFilterDualStereo>>;
    
    /**
     * @brief constructor
     * @param typeParameters_ a set of constant variables
     */
    Decay(DecayTypeParameters& typeParameters_)
    : typeParameters(typeParameters_) {}
    
    /**
     * @brief sets up the decay
     *
     * @param params_  a set of parameters that can be changed by user
     * @param sampleRate_ the sample rate
     * @param blocksize_ num samples in one audio block
     */
    void setup(const DecayParameters& params_, const float& sampleRate_, const unsigned int& blocksize_);
    
    void updateRamps();
    
    /**
     * @brief processes incoming stereo samples
     *
     * @param input_  a vector of a pair of floats
     * @param sampleIndex_ (0...blocksize) the momentary index of the audioblock sample
     *
     * @return the processed audio samples
     */
    float32x2_t processAudioSamples(const float32x2_t input_, const unsigned int& sampleIndex_);
    
    /**
     * @brief sets a new set of parameters
     *
     * set the following parameters:
     * - decay time
     * - modulation rate
     * - modulation depth
     *
     * @param parameters_ the new set of parameter
     */
    void setParameters(const DecayParameters& parameters_);
    
    /**
     * @brief returns momentary set of parameters
     * @return momentary set of parameters
     */
    const DecayParameters& getParameters() const { return parameters; }
    
    /**
     * @brief return the earliest delay of all combfilters, gets called when calculating the delay of the decay
     * @return the earliest delay of all combfilters in samples
     */
    unsigned int getEarliestCombDelay() const { return combFilters[0].filters[0].getDelaySamples(); }
    
private:
    /**
     * @brief helper, recalculates the new gain values according to the rt60 time
     * @param decayTimeMs_ the rt60 time in miliseconds
     */
    void calcAndSetCombFilterGains(float decayTimeMs_);

    /**
     * @brief helper, makes sure, the array of combfilters is alligned correctly
     * @param count number of combfilters in the array
     * @return the alligned unique pointer to an array of combfilters
     */
    CombFilterDualStereoPtr createAlignedCombFilters(size_t count);

private:
    float fs_inv, samplesPerMs_inv;
    
    DecayParameters parameters; ///< set of parameters changeable by user
    DecayTypeParameters typeParameters; ///< set of constant parameters defined through the reverb type

    CombFilterDualStereoPtr combFilters; ///< a pointer to an array of wrappers of combfilters (each containing 4 combfilters, or 2x2)
    std::unique_ptr<AllpassFilterStereo[]> allpassFiltersPre; ///< a pointer to an array of allpassfilters
    std::unique_ptr<AllpassFilterStereo[]> allpassFiltersPost; ///< a pointer to an array of allpassfilters
    
    float32_t combFilterScaler = 1.f; ///< scale factor for parallel processing of combfilters
    
    float modulationIncr = 0.f; ///< step, the phase moves, when calling modulation, calculated out of the modrate
    bool modulationEnabled = false;
};


// =======================================================================================
// MARK: - Bela Reverb
// =======================================================================================

/**
 * @class Reverb
 * @brief processes the whole Reverb
 *
 */
class Reverb
{
public:
    /**
     * @brief setting up the reverb
     *
     * @param sampleRate_  the samplerate
     * @param blocksize_ the number of samples in one audio block
     */
    void setup(const float& sampleRate_, const unsigned int& blocksize_);
    
    void updateRamps();
    
    /**
     * @brief processes a stereo pair of incoming audio samples
     *
     * @param input_  a struct (@c StereoFloat ) that holds two floating point values (left and right channel - sample)
     * @param sampleIndex_ (0...blocksize-1) the momentary index of the processing loop
     * @return returns the processed samples
     */
    StereoFloat processAudioSamples(StereoFloat input_, const unsigned int& sampleIndex_);
    
    /**
     * @brief handles new incoming values from UI
     *
     * gets called each time a UI parameter has changed, converts UI values into internal values
       (i.e. percent (0...100) to scaler (0...1)), sends these values to corresponding modules
     *
     * @warning call this blockwise only!
     *
     * @param parameterID the name of the changed parameter
     * @param newValue the UI value
     */
    void parameterChanged(const std::string& parameterID, float newValue);
    
    /**
     * @brief sets a new reverb type
     *
     * the fixed parameters for every type of reverb are defined here,
       they are called type parameters different to the normal parameters that can be changed by the user,
       the function clears the type parameters of each EarlyReflections and Decay and sets up new instances of them
     *
     * @param type_  the Reverb Type
     */
    void setReverbType(ReverbTypes type_);
    
private:
    float sampleRate; ///< the sample rate
    unsigned int blocksize; ///< number of samples in one block
    float samplesPerMs; ///< num processed samples per milisecond
    
    EarlyReflections earlyReflections;
    std::unique_ptr<Decay> decay = nullptr;
    SimpleDelayStereo delayedDecay; ///< delay of decay, used to sync decay to earlies
    LinearRamp decayDelaySamples;
    
    ParametricEQStereo inputMultiplier;
    ButterworthLowcutStereo lowcut;
    ButterworthHighcutStereo highcut;
    
    bool settingType = false;
};

} // namespace Reverberation

/**
 * @mainpage BelaReverb
 *
 * @section intro_sec Introduction
 * The BelaReverb simulates 24 early reflections using a tap delay, then sends this signal into a late reverberation algorithm similar to those from Schroeder and Moorer. The late reverberation consists of a series of all-pass filters, a set of parallel comb filters, and another set of series all-pass filters. To combine these two elements correctly, the decay has to be delayed to align right after the latest early reflection. In addition, some equalizers can shape the sound. A parametric EQ is implemented pre-FX and used as a “multiplier” of the input signal. A low-cut and high-cut filter can be set upon request; these are implemented post-FX.
 *
 * @section type_sec The Reverb Types
 * The user can change the reverb type, which involves many internal parameter changes (referred to as typeParameters). The EarlyReflections class includes three different reflection simulations: Church, Foyer, and Bathroom. This set of tap delays changes according to the type, as well as a diffusion factor (the all-pass filter gain in the early reflections) and a damping factor (low-pass gain in the early reflections). The decay primarily adjusts the composition of all-pass and comb filters and their corresponding delays. Additionally, damping, diffusion, and the rate and depth of modulating the all-pass filter delay times are type parameters.
 *
 * Four different types have been implemented:
 * •    CHURCH
 * •    DIGITAL VINTAGE REVERB
 * •    SEASICK
 * •    ROOM
 *
 * You can look up the corresponding parameter sets in the BelaReverb::setReverbType() function.
 */
