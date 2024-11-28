#pragma once

#include "../Functions.h"

static const uint MAX_RATE_CONVERSION_RATIO = 8;
static const uint MAX_FILTER_LENGTH = 256;

// =======================================================================================
// MARK: - HELPER FUNCTIONS
// =======================================================================================

/**
 * @brief Decomposes a single filter coefficient array into a set of polyphase filter banks.
 *
 * This function splits a single FIR filter coefficient array into multiple sub-filters
 * based on the specified ratio. Each sub-filter is stored in its own array, which are
 * then grouped into a dynamically allocated array of pointers.
 *
 * @param filterCoefficients_ A pointer to the original FIR filter coefficient array.
 * @param filterLength_ The length of the filter coefficient array.
 * @param ratio_ The decomposition ratio, determining the number of sub-filters.
 * @return A dynamically allocated 2D array (`float**`) containing the polyphase filter banks.
 *         Each sub-array has a length of `filterLength_ / ratio_`.
 *         The caller is responsible for freeing the memory of each sub-array and the pointer array.
 */
inline float** decomposeFilter(const float* filterCoefficients_, const uint filterLength_, const uint ratio_);

/**
 * @brief Retrieves predefined low-pass filter coefficients for specific sample rates and configurations.
 *
 * This function returns a pointer to a predefined FIR filter coefficient array based on
 * the given sample rate, filter length, and up/downsampling ratio. Supported sample rates
 * are 44.1 kHz and 48 kHz, with specific filter lengths and ratios.
 *
 * @param sampleRate_ The sample rate in Hz (supported values: 44100.0 or 48000.0).
 * @param filterLength_ The length of the desired filter (supported values: 64, 128, or 256).
 * @param ratio_ The upsampling or downsampling ratio (supported values: 2, 4, or 8).
 * @return A pointer to a predefined filter coefficient array, or `nullptr` if the configuration is unsupported.
 */
inline const float* getFilterCoefficients(const float sampleRate_, const uint filterLength_, const uint ratio_);


// =======================================================================================
// MARK: - CONVOLVER
// =======================================================================================

/**
 * @class Convolver
 * @brief Implements a mono audio convolution engine using SIMD optimization for performance.
 *
 * The Convolver class applies an FIR filter to audio samples by convolving the input signal
 * with a set of filter coefficients. It uses SIMD operations for optimized performance.
 */
class Convolver
{
public:
    /**
     * @brief Sets up the convolver with the specified filter length and coefficients.
     * @param filterLength_ The length of the filter, must be a multiple of 4.
     * @param filterCoeffs_ A pointer to the array of filter coefficients.
     * @throws runtime_error If the filter length is not a multiple of 4.
     */
    void setup(const uint filterLength_, const float* filterCoeffs_);
    
    /**
     * @brief Processes a single audio sample through the convolver.
     * @param input_ The input audio sample to process.
     * @return The convolved output sample.
     */
    float processAudioSample(const float input_);

private:
    uint filterLength; ///< The length of the FIR filter.
    std::array<float, MAX_FILTER_LENGTH> buffer; ///< Circular buffer for storing input samples.
    std::array<float, MAX_FILTER_LENGTH> filterCoefficients; ///< Array of filter coefficients.
    uint writePointer; ///< Pointer to the current position in the circular buffer.
};

/**
 * @class ConvolverStereo
 * @brief Implements a stereo audio convolution engine with support for SIMD optimization.
 *
 * The ConvolverStereo class extends the functionality of the Convolver class to handle
 * stereo audio signals, processing left and right channels simultaneously.
 */
class ConvolverStereo
{
public:
    /**
     * @brief Sets up the stereo convolver with the specified filter length and coefficients.
     * @param filterLength_ The length of the filter, must be a multiple of 4.
     * @param filterCoeffs_ A pointer to the array of filter coefficients.
     * @throws runtime_error If the filter length is not a multiple of 4.
     */
    void setup(const uint filterLength_, const float* filterCoeffs_);
    
    /**
     * @brief Processes a stereo audio sample through the convolver.
     * @param input_ The input stereo audio sample to process (float32x2_t format).
     * @return The convolved output stereo sample (float32x2_t format).
     */
    float32x2_t processAudioSamples(const float32x2_t input_);

private:
    uint filterLength; ///< The length of the FIR filter.
    std::array<float32x2_t, MAX_FILTER_LENGTH> buffer; ///< Circular buffer for storing input stereo samples.
    std::array<float32_t, MAX_FILTER_LENGTH> filterCoefficients; ///< Array of filter coefficients.
    uint writePointer; ///< Pointer to the current position in the circular buffer.
};


// =======================================================================================
// MARK: - INTERPOLATOR
// =======================================================================================


/**
 * @struct InterpolatorOutput
 * @brief Stores the interpolated audio data for mono signals.
 *
 * This struct contains an array of upsampled audio data, with one value
 * for each step in the interpolation ratio.
 */
struct InterpolatorOutput
{
    float audioData[MAX_RATE_CONVERSION_RATIO]; ///< Array of interpolated audio samples.
};


/**
 * @class Interpolator
 * @brief Implements a mono audio interpolator for upsampling using polyphase FIR filters.
 *
 * The Interpolator class performs upsampling by inserting additional samples between
 * the input samples and applying a set of polyphase FIR filters. It ensures high-quality
 * interpolation for audio signals.
 */
class Interpolator
{
public:
    /**
     * @brief Configures the interpolator with the given sample rate, ratio, and filter length.
     *
     * Initializes the interpolator with the specified sample rate, interpolation ratio,
     * and FIR filter length. Sets up the polyphase filters.
     *
     * @param sampleRate_ The sample rate of the input signal in Hz.
     * @param ratio_ The interpolation ratio (e.g., 2, 4, 8).
     * @param filterLength_ The length of the FIR filter.
     */
    void setup(const float sampleRate_, const uint ratio_, const uint filterLength_);
    
    /**
     * @brief Interpolates a single input audio sample and generates multiple output samples.
     *
     * Takes a single input sample and computes `ratio` output samples using the configured
     * polyphase FIR filters and interpolation settings.
     *
     * @param input_ The input audio sample to interpolate.
     * @return An `InterpolatorOutput` struct containing the interpolated audio samples.
     */
    InterpolatorOutput interpolateAudio(const float input_);
    
    /**
     * @brief Updates the interpolation ratio and reconfigures the polyphase filters.
     *
     * Adjusts the interpolation ratio and reinitializes the polyphase filter coefficients.
     *
     * @param ratio_ The new interpolation ratio.
     */
    void setInterpolationRatio(const uint ratio_);

private:
    float sampleRate; ///< The sample rate of the input audio signal in Hz.
    uint ratio; ///< The current interpolation ratio.
    uint filterLength; ///< The length of the FIR filter.
    Convolver polyPhaseConvolver[MAX_RATE_CONVERSION_RATIO]; ///< Array of polyphase convolvers for processing.
    float gainCompensation; ///< Gain compensation factor for interpolated samples.
};


/**
 * @struct InterpolatorStereoOutput
 * @brief Stores the interpolated audio data for stereo signals.
 *
 * This struct contains an array of upsampled stereo audio data. Each entry
 * corresponds to one step in the interpolation ratio, containing both
 * left and right channel values.
 */
struct InterpolatorStereoOutput
{
    float32x2_t audioData[MAX_RATE_CONVERSION_RATIO]; ///< Array of interpolated stereo audio samples.
};


/**
 * @class InterpolatorStereo
 * @brief Implements a stereo audio interpolator for upsampling using polyphase FIR filters.
 *
 * The InterpolatorStereo class extends the functionality of the Interpolator class to
 * handle stereo audio signals, processing left and right channels simultaneously.
 */
class InterpolatorStereo
{
public:
    /**
     * @brief Configures the stereo interpolator with the given sample rate, ratio, and filter length.
     *
     * Initializes the stereo interpolator with the specified sample rate, interpolation ratio,
     * and FIR filter length. Sets up the polyphase filters for stereo processing.
     *
     * @param sampleRate_ The sample rate of the input signal in Hz.
     * @param ratio_ The interpolation ratio (e.g., 2, 4, 8).
     * @param filterLength_ The length of the FIR filter.
     */
    void setup(const float sampleRate_, const uint ratio_, const uint filterLength_);
    
    /**
     * @brief Interpolates a stereo audio sample and generates multiple output samples.
     *
     * Takes a single stereo input sample and computes `ratio` output samples for each channel
     * using the configured polyphase FIR filters and interpolation settings.
     *
     * @param input_ The stereo input audio sample to interpolate.
     * @return An `InterpolatorStereoOutput` struct containing the interpolated stereo audio samples.
     */
    InterpolatorStereoOutput interpolateAudio(const float32x2_t input_);
    
    /**
     * @brief Updates the interpolation ratio and reconfigures the polyphase filters.
     *
     * Adjusts the interpolation ratio and reinitializes the polyphase filter coefficients.
     *
     * @param ratio_ The new interpolation ratio.
     */
    void setInterpolationRatio(const uint ratio_);

private:
    float sampleRate; ///< The sample rate of the input audio signal in Hz.
    uint ratio; ///< The current interpolation ratio.
    uint filterLength; ///< The length of the FIR filter.
    ConvolverStereo polyPhaseConvolver[MAX_RATE_CONVERSION_RATIO]; ///< Array of polyphase convolvers for stereo processing.
    float32_t gainCompensation; ///< Gain compensation factor for interpolated samples.
};


// =======================================================================================
// MARK: - DECIMATOR
// =======================================================================================


/**
 * @struct DecimatorInput
 * @brief Stores the audio data for mono decimation.
 *
 * Contains an array of audio data corresponding to the input samples for each polyphase filter.
 */
struct DecimatorInput
{
    float audioData[MAX_RATE_CONVERSION_RATIO]; ///< Array of input audio samples for decimation.
};


/**
 * @class Decimator
 * @brief Implements a mono audio decimator for downsampling using polyphase FIR filters.
 *
 * The Decimator class performs downsampling by reducing the sample rate of the input signal.
 * It uses polyphase FIR filters to ensure high-quality decimation with minimal aliasing.
 */
class Decimator
{
public:
    /**
     * @brief Configures the decimator with the given sample rate, ratio, and filter length.
     *
     * Initializes the decimator with the specified sample rate, decimation ratio,
     * and FIR filter length. Sets up the polyphase filters.
     *
     * @param sampleRate_ The sample rate of the input signal in Hz.
     * @param ratio_ The decimation ratio (e.g., 2, 4, 8).
     * @param filterLength_ The length of the FIR filter.
     */
    void setup(const float sampleRate_, const uint ratio_, const uint filterLength_);
    
    /**
     * @brief Decimates a mono audio signal based on the configured ratio and filter settings.
     *
     * This function takes an input audio buffer and applies the configured polyphase FIR filters
     * to produce a single downsampled output sample.
     *
     * @param input_ A `DecimatorInput` struct containing the audio data to decimate.
     * @return The downsampled mono audio sample.
     */
    float decimateAudio(const DecimatorInput input_);
    
    /**
     * @brief Updates the decimation ratio and reconfigures the polyphase filters.
     *
     * Adjusts the decimation ratio and reinitializes the polyphase filter coefficients.
     *
     * @param ratio_ The new decimation ratio.
     */
    void setDecimationRatio(const uint ratio_);

private:
    float sampleRate; ///< The sample rate of the input audio signal in Hz.
    uint ratio; ///< The current decimation ratio.
    uint filterLength; ///< The length of the FIR filter.
    Convolver polyPhaseConvolver[MAX_RATE_CONVERSION_RATIO]; ///< Array of polyphase convolvers for processing.
};


/**
 * @struct DecimatorStereoInput
 * @brief Stores the stereo audio data for decimation.
 *
 * Contains an array of stereo audio data (one vector per decimation ratio).
 */
struct DecimatorStereoInput
{
    float32x2_t audioData[MAX_RATE_CONVERSION_RATIO]; ///< Array of stereo input audio samples for decimation.
};


/**
 * @class DecimatorStereo
 * @brief Implements a stereo audio decimator for downsampling using polyphase FIR filters.
 *
 * The DecimatorStereo class extends the functionality of the Decimator class to
 * handle stereo audio signals, processing left and right channels simultaneously.
 */
class DecimatorStereo
{
public:
    /**
     * @brief Configures the stereo decimator with the given sample rate, ratio, and filter length.
     *
     * Initializes the stereo decimator with the specified sample rate, decimation ratio,
     * and FIR filter length. Sets up the polyphase filters for stereo processing.
     *
     * @param sampleRate_ The sample rate of the input signal in Hz.
     * @param ratio_ The decimation ratio (e.g., 2, 4, 8).
     * @param filterLength_ The length of the FIR filter.
     */
    void setup(const float sampleRate_, const uint ratio_, const uint filterLength_);
    
    /**
     * @brief Decimates a stereo audio signal based on the configured ratio and filter settings.
     *
     * This function takes an input stereo audio buffer and applies the configured polyphase FIR filters
     * to produce a single downsampled stereo sample.
     *
     * @param input_ A `DecimatorStereoInput` struct containing the stereo audio data to decimate.
     * @return The downsampled stereo audio sample.
     */
    float32x2_t decimateAudio(const DecimatorStereoInput input_);
    
    /**
     * @brief Updates the decimation ratio and reconfigures the polyphase filters.
     *
     * Adjusts the decimation ratio and reinitializes the polyphase filter coefficients.
     *
     * @param ratio_ The new decimation ratio.
     */
    void setDecimationRatio(const uint ratio_);

private:
    float sampleRate; ///< The sample rate of the input audio signal in Hz.
    uint ratio; ///< The current decimation ratio.
    uint filterLength; ///< The length of the FIR filter.
    ConvolverStereo polyPhaseConvolver[MAX_RATE_CONVERSION_RATIO]; ///< Array of polyphase convolvers for stereo processing.
};


// =======================================================================================
// MARK: - FIR LOWPASS FILTERS for UP- & DOWNSAMPLING
// =======================================================================================


/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 88200 Hz

* 0 Hz - 17000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.06792570818435614 dB

* 22000 Hz - 44100 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -80.87865996242823 dB

*/

static const float LPF_64_882[64] = {
  0.00012741976628362868,
  0.0007771240504406757,
  0.0013617015519883596,
  0.0007587722463109201,
  -0.0009855597684638154,
  -0.0015996173248545823,
  0.00047332944375004564,
  0.0026320371278935966,
  0.0008619796961159972,
  -0.0032936643769441425,
  -0.0030896189434833025,
  0.002910130316051008,
  0.005872973751257355,
  -0.0008491993430610934,
  -0.00844229683962603,
  -0.003258836543508074,
  0.009647102646158784,
  0.009288676452138574,
  -0.00812205944479744,
  -0.016446452050533026,
  0.0025251566890351863,
  0.0231986201819361,
  0.008259931919374863,
  -0.02726476154868444,
  -0.025163167116507536,
  0.02542896820278027,
  0.049740525676643485,
  -0.012094913172220691,
  -0.08831837143457202,
  -0.03095239416372958,
  0.1887948840349981,
  0.39917662739294574,
  0.39917662739294574,
  0.1887948840349981,
  -0.03095239416372958,
  -0.08831837143457202,
  -0.012094913172220691,
  0.049740525676643485,
  0.02542896820278027,
  -0.025163167116507536,
  -0.02726476154868444,
  0.008259931919374863,
  0.0231986201819361,
  0.0025251566890351863,
  -0.016446452050533026,
  -0.00812205944479744,
  0.009288676452138574,
  0.009647102646158784,
  -0.003258836543508074,
  -0.00844229683962603,
  -0.0008491993430610934,
  0.005872973751257355,
  0.002910130316051008,
  -0.0030896189434833025,
  -0.0032936643769441425,
  0.0008619796961159972,
  0.0026320371278935966,
  0.00047332944375004564,
  -0.0015996173248545823,
  -0.0009855597684638154,
  0.0007587722463109201,
  0.0013617015519883596,
  0.0007771240504406757,
  0.00012741976628362868
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 176400 Hz

* 0 Hz - 16000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.761679744283678 dB

* 22000 Hz - 88200 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -59.8894146518267 dB

*/

static float LPF_64_1764[64] = {
  -0.0004900496572475682,
  -0.0023678156720725527,
  -0.0039272504606054515,
  -0.005950668393774824,
  -0.007113688510339651,
  -0.006821364234361367,
  -0.00459459473969128,
  -0.0007062898947712826,
  0.0038075056152239036,
  0.007342685412353525,
  0.008295753728964905,
  0.005774294293498351,
  0.00017401157016459553,
  -0.006681811999808946,
  -0.012020097585390044,
  -0.013124567776566155,
  -0.008557256346331798,
  0.0009190462595487236,
  0.012180129196311132,
  0.02057926556449076,
  0.02162684256946161,
  0.012973613965238766,
  -0.00407726698651423,
  -0.024219980404588004,
  -0.03930269360699877,
  -0.04078614198738441,
  -0.022752006470576458,
  0.015580021290581234,
  0.06879894301013266,
  0.12624147594929686,
  0.17476571750847733,
  0.20252537128870715,
  0.20252537128870715,
  0.17476571750847733,
  0.12624147594929686,
  0.06879894301013266,
  0.015580021290581234,
  -0.022752006470576458,
  -0.04078614198738441,
  -0.03930269360699877,
  -0.024219980404588004,
  -0.00407726698651423,
  0.012973613965238766,
  0.02162684256946161,
  0.02057926556449076,
  0.012180129196311132,
  0.0009190462595487236,
  -0.008557256346331798,
  -0.013124567776566155,
  -0.012020097585390044,
  -0.006681811999808946,
  0.00017401157016459553,
  0.005774294293498351,
  0.008295753728964905,
  0.007342685412353525,
  0.0038075056152239036,
  -0.0007062898947712826,
  -0.00459459473969128,
  -0.006821364234361367,
  -0.007113688510339651,
  -0.005950668393774824,
  -0.0039272504606054515,
  -0.0023678156720725527,
  -0.0004900496572475682
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 352800 Hz

* 0 Hz - 14000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 3.279644002616435 dB

* 22000 Hz - 176400 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -47.305110199700756 dB

*/

static float LPF_64_3528[64] = {
  0.0031304603246164846,
  0.002226757852445247,
  0.002735783004064701,
  0.0030604322790254803,
  0.0030980245239449226,
  0.0027274991075246224,
  0.001861507555573215,
  0.00043027904510620115,
  -0.0015930844219034235,
  -0.004180324170804289,
  -0.007237152125333012,
  -0.010608453531278985,
  -0.014074226186941074,
  -0.017356567683644714,
  -0.020135025084561366,
  -0.02207198755590919,
  -0.02283425146866204,
  -0.022119211957557297,
  -0.019682930500589283,
  -0.015368682667487305,
  -0.009127237059323865,
  -0.001031705455017688,
  0.008716447709807841,
  0.019790627965944815,
  0.03175350913086422,
  0.0440787260120275,
  0.056181130813247285,
  0.06745401580861181,
  0.07731023274776205,
  0.08522139797586918,
  0.09075395678291649,
  0.09360003866920351,
  0.09360003866920351,
  0.09075395678291649,
  0.08522139797586918,
  0.07731023274776205,
  0.06745401580861181,
  0.056181130813247285,
  0.0440787260120275,
  0.03175350913086422,
  0.019790627965944815,
  0.008716447709807841,
  -0.001031705455017688,
  -0.009127237059323865,
  -0.015368682667487305,
  -0.019682930500589283,
  -0.022119211957557297,
  -0.02283425146866204,
  -0.02207198755590919,
  -0.020135025084561366,
  -0.017356567683644714,
  -0.014074226186941074,
  -0.010608453531278985,
  -0.007237152125333012,
  -0.004180324170804289,
  -0.0015930844219034235,
  0.00043027904510620115,
  0.001861507555573215,
  0.0027274991075246224,
  0.0030980245239449226,
  0.0030604322790254803,
  0.002735783004064701,
  0.002226757852445247,
  0.0031304603246164846
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 88200 Hz

* 0 Hz - 18000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.00510822896213176 dB

* 22000 Hz - 44100 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -90.93746884012756 dB

*/

static float LPF_128_882[128] = {
  -0.000001326248297307848,
  -0.000006153320597536581,
  -0.000009967954689700856,
  -0.000001127776186349388,
  0.00001550152811137445,
  0.000007608621302513701,
  -0.00003229252416893721,
  -0.000041161738287925066,
  0.000026991121885574214,
  0.00007805755156376652,
  -0.000011940862383658682,
  -0.00013863411645916742,
  -0.000054004591439959345,
  0.00018019481984014602,
  0.0001537371791304442,
  -0.00020559135602201943,
  -0.00031661502167200297,
  0.00015392063141370966,
  0.0005001487336468455,
  -0.00001936439062741374,
  -0.0006995812604199312,
  -0.0002543079201927842,
  0.000832726950080611,
  0.0006489815949638099,
  -0.0008579532375235741,
  -0.0011762124062939742,
  0.0006679712973637118,
  0.0017534233377638072,
  -0.00021878349640193378,
  -0.0023146341528559773,
  -0.0005675446208513834,
  0.0026975607237250706,
  0.0016677841524093925,
  -0.0027698569450945215,
  -0.003055001757303726,
  0.0023326844156057883,
  0.004569628210368333,
  -0.0012633070903426268,
  -0.006030531694869041,
  -0.0005688426158492296,
  0.007126169992732099,
  0.0031549021368398205,
  -0.007547629914838522,
  -0.006433300851491731,
  0.0069019535452797005,
  0.010155603315853112,
  -0.0048590970064612245,
  -0.013990333683150077,
  0.0010693967034724429,
  0.017408967046398317,
  0.004707418687465372,
  -0.0197973846138246,
  -0.012733853133037534,
  0.020316071046870485,
  0.0232780387578783,
  -0.017891013242669132,
  -0.037019157026280315,
  0.010655988607603117,
  0.05587480189876355,
  0.0057135549016135035,
  -0.08721654940957257,
  -0.04867723260483846,
  0.18034135388087297,
  0.41356534969946274,
  0.41356534969946274,
  0.18034135388087297,
  -0.04867723260483846,
  -0.08721654940957257,
  0.0057135549016135035,
  0.05587480189876355,
  0.010655988607603117,
  -0.037019157026280315,
  -0.017891013242669132,
  0.0232780387578783,
  0.020316071046870485,
  -0.012733853133037534,
  -0.0197973846138246,
  0.004707418687465372,
  0.017408967046398317,
  0.0010693967034724429,
  -0.013990333683150077,
  -0.0048590970064612245,
  0.010155603315853112,
  0.0069019535452797005,
  -0.006433300851491731,
  -0.007547629914838522,
  0.0031549021368398205,
  0.007126169992732099,
  -0.0005688426158492296,
  -0.006030531694869041,
  -0.0012633070903426268,
  0.004569628210368333,
  0.0023326844156057883,
  -0.003055001757303726,
  -0.0027698569450945215,
  0.0016677841524093925,
  0.0026975607237250706,
  -0.0005675446208513834,
  -0.0023146341528559773,
  -0.00021878349640193378,
  0.0017534233377638072,
  0.0006679712973637118,
  -0.0011762124062939742,
  -0.0008579532375235741,
  0.0006489815949638099,
  0.000832726950080611,
  -0.0002543079201927842,
  -0.0006995812604199312,
  -0.00001936439062741374,
  0.0005001487336468455,
  0.00015392063141370966,
  -0.00031661502167200297,
  -0.00020559135602201943,
  0.0001537371791304442,
  0.00018019481984014602,
  -0.000054004591439959345,
  -0.00013863411645916742,
  -0.000011940862383658682,
  0.00007805755156376652,
  0.000026991121885574214,
  -0.000041161738287925066,
  -0.00003229252416893721,
  0.000007608621302513701,
  0.00001550152811137445,
  -0.000001127776186349388,
  -0.000009967954689700856,
  -0.000006153320597536581,
  -0.000001326248297307848
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 176400 Hz

* 0 Hz - 19000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.704870406409916 dB

* 22000 Hz - 88200 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -60.56187857628239 dB

*/

static float LPF_128_1764[128] = {
  -0.0010687846168826445,
  -0.0015089698736386263,
  -0.0019179602349633,
  -0.0017344402010129542,
  -0.000727916820110155,
  0.0010464571864235508,
  0.003182175604470357,
  0.005012440234825428,
  0.005849247260798823,
  0.005285734688805946,
  0.0034260686505743184,
  0.0009162331912489467,
  -0.0012748196425396135,
  -0.002255188158578787,
  -0.0016243745896260349,
  0.0002970190325223054,
  0.0025593482012904118,
  0.0039890803852088135,
  0.0037435582771333064,
  0.001757961583132426,
  -0.0011526265829431245,
  -0.0036218915555995363,
  -0.0043706821336806,
  -0.0028672457567143757,
  0.00032767650512886956,
  0.003734378899725659,
  0.005617368962314341,
  0.004845415670748653,
  0.0015335342499025262,
  -0.002887022011237451,
  -0.006264592635379451,
  -0.006756164258458046,
  -0.003805508502926733,
  0.0014335156964754355,
  0.006506795489387376,
  0.008779777735115247,
  0.006770595882180115,
  0.0010379719398742376,
  -0.005879115993660105,
  -0.010542135488107502,
  -0.010276072594578684,
  -0.004631999463249569,
  0.004119469155497545,
  0.011825030695582097,
  0.014346994678687004,
  0.009687070763846142,
  -0.0007119490176874496,
  -0.012242005891115184,
  -0.019045261366432528,
  -0.016822990080468486,
  -0.005260466879821671,
  0.01118654466895618,
  0.024752056309916,
  0.027688405720618793,
  0.016127451116752158,
  -0.007238158982163502,
  -0.03307377522123706,
  -0.04828352155983569,
  -0.04097387773061619,
  -0.005603902839226014,
  0.053830181618794125,
  0.12405764239249202,
  0.18641011904389648,
  0.2230167738395066,
  0.2230167738395066,
  0.18641011904389648,
  0.12405764239249202,
  0.053830181618794125,
  -0.005603902839226014,
  -0.04097387773061619,
  -0.04828352155983569,
  -0.03307377522123706,
  -0.007238158982163502,
  0.016127451116752158,
  0.027688405720618793,
  0.024752056309916,
  0.01118654466895618,
  -0.005260466879821671,
  -0.016822990080468486,
  -0.019045261366432528,
  -0.012242005891115184,
  -0.0007119490176874496,
  0.009687070763846142,
  0.014346994678687004,
  0.011825030695582097,
  0.004119469155497545,
  -0.004631999463249569,
  -0.010276072594578684,
  -0.010542135488107502,
  -0.005879115993660105,
  0.0010379719398742376,
  0.006770595882180115,
  0.008779777735115247,
  0.006506795489387376,
  0.0014335156964754355,
  -0.003805508502926733,
  -0.006756164258458046,
  -0.006264592635379451,
  -0.002887022011237451,
  0.0015335342499025262,
  0.004845415670748653,
  0.005617368962314341,
  0.003734378899725659,
  0.00032767650512886956,
  -0.0028672457567143757,
  -0.0043706821336806,
  -0.0036218915555995363,
  -0.0011526265829431245,
  0.001757961583132426,
  0.0037435582771333064,
  0.0039890803852088135,
  0.0025593482012904118,
  0.0002970190325223054,
  -0.0016243745896260349,
  -0.002255188158578787,
  -0.0012748196425396135,
  0.0009162331912489467,
  0.0034260686505743184,
  0.005285734688805946,
  0.005849247260798823,
  0.005012440234825428,
  0.003182175604470357,
  0.0010464571864235508,
  -0.000727916820110155,
  -0.0017344402010129542,
  -0.0019179602349633,
  -0.0015089698736386263,
  -0.0010687846168826445
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 352800 Hz

* 0 Hz - 16000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.7573387020639423 dB

* 22000 Hz - 176400 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -59.93899655002301 dB

*/

static float LPF_128_3528[128] = {
  0.00011218749687924419,
  -0.0007464361605560429,
  -0.0008764982886901148,
  -0.0012679216858710518,
  -0.001740251634545812,
  -0.0022421684456002877,
  -0.0027298343355908247,
  -0.003153820638443718,
  -0.0034615967210696592,
  -0.0036023885310534887,
  -0.0035333384417309886,
  -0.0032257588647719424,
  -0.0026706187449276942,
  -0.0018828494205983517,
  -0.0009033615931072947,
  0.00020145614738555412,
  0.001343271972510473,
  0.002418561034369306,
  0.003317463869113217,
  0.003935112959947904,
  0.004183856045289097,
  0.004002569402468273,
  0.0033723448124858574,
  0.0023142677656185504,
  0.0009004599996940042,
  -0.0007519864714193713,
  -0.002489088197211072,
  -0.004131479532896524,
  -0.005490766244774002,
  -0.006389038945005218,
  -0.006679938162903181,
  -0.006267715256134589,
  -0.005122436155293736,
  -0.003290244729894237,
  -0.0008966967592950602,
  0.0018579591620135542,
  0.004712323743306164,
  0.007365518748829755,
  0.009504727882607308,
  0.010837021420235729,
  0.011122193357170573,
  0.010203156786375896,
  0.00803278751687552,
  0.00468929561451393,
  0.00038476096483587205,
  -0.004542141082559332,
  -0.009644889558536929,
  -0.014398825316740036,
  -0.018241907310132248,
  -0.0206214943864586,
  -0.021044375646660233,
  -0.019124654764767445,
  -0.014625648668243542,
  -0.007491801094760709,
  0.002133592539466603,
  0.013906699032171984,
  0.027298299224415203,
  0.0416250255903625,
  0.05609450050202795,
  0.06986170562649806,
  0.08209134944619914,
  0.09202080095037085,
  0.09901911213551508,
  0.10263542924370965,
  0.10263542924370965,
  0.09901911213551508,
  0.09202080095037085,
  0.08209134944619914,
  0.06986170562649806,
  0.05609450050202795,
  0.0416250255903625,
  0.027298299224415203,
  0.013906699032171984,
  0.002133592539466603,
  -0.007491801094760709,
  -0.014625648668243542,
  -0.019124654764767445,
  -0.021044375646660233,
  -0.0206214943864586,
  -0.018241907310132248,
  -0.014398825316740036,
  -0.009644889558536929,
  -0.004542141082559332,
  0.00038476096483587205,
  0.00468929561451393,
  0.00803278751687552,
  0.010203156786375896,
  0.011122193357170573,
  0.010837021420235729,
  0.009504727882607308,
  0.007365518748829755,
  0.004712323743306164,
  0.0018579591620135542,
  -0.0008966967592950602,
  -0.003290244729894237,
  -0.005122436155293736,
  -0.006267715256134589,
  -0.006679938162903181,
  -0.006389038945005218,
  -0.005490766244774002,
  -0.004131479532896524,
  -0.002489088197211072,
  -0.0007519864714193713,
  0.0009004599996940042,
  0.0023142677656185504,
  0.0033723448124858574,
  0.004002569402468273,
  0.004183856045289097,
  0.003935112959947904,
  0.003317463869113217,
  0.002418561034369306,
  0.001343271972510473,
  0.00020145614738555412,
  -0.0009033615931072947,
  -0.0018828494205983517,
  -0.0026706187449276942,
  -0.0032257588647719424,
  -0.0035333384417309886,
  -0.0036023885310534887,
  -0.0034615967210696592,
  -0.003153820638443718,
  -0.0027298343355908247,
  -0.0022421684456002877,
  -0.001740251634545812,
  -0.0012679216858710518,
  -0.0008764982886901148,
  -0.0007464361605560429,
  0.00011218749687924419
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 88200 Hz

* 0 Hz - 20000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.004103581634489705 dB

* 22000 Hz - 44100 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -87.39858293002382 dB

*/

static float LPF_256_882[256] = {
  -1.233874226414999e-7,
  -4.572405451593453e-7,
  -4.561307723873045e-7,
  4.1651726936373105e-7,
  8.668335150970118e-7,
  -8.364044705258299e-7,
  -0.0000024451304666430503,
  1.472719883278116e-7,
  0.000003859674240005345,
  5.82649902004218e-7,
  -0.000006480064921830934,
  -0.00000324656607540795,
  0.000008522111305189946,
  0.000006666728806253622,
  -0.000011154025415874738,
  -0.00001278470442689314,
  0.000012143365583727823,
  0.000020165018923654894,
  -0.000012377912994008165,
  -0.000030506955730396854,
  0.000009258771433153023,
  0.00004189291782164469,
  -0.0000034765698445852614,
  -0.00005547972379051587,
  -0.000007678036007735468,
  0.00006868157707424679,
  0.000023433994126330156,
  -0.00008195154572906459,
  -0.000046245927874971093,
  0.00009197210903165128,
  0.00007486583376850753,
  -0.00009850717253030551,
  -0.00011105314663570879,
  0.00009763810842787279,
  0.00015263844527407924,
  -0.0000886838926305579,
  -0.00020024922576395454,
  0.00006751028439242685,
  0.00025042493941048404,
  -0.00003348977533786711,
  -0.00030237540924826154,
  -0.000017122256926406192,
  0.0003511765746380564,
  0.00008420509304807701,
  -0.00039458899947662844,
  -0.00017035203882705303,
  0.0004263666132372138,
  0.0002739130571304973,
  -0.00044312916181558936,
  -0.0003955381628259023,
  0.0004378193595686736,
  0.0005312976853953152,
  -0.0004066422800012117,
  -0.0006792618029103274,
  0.00034263209196798634,
  0.000832716696886661,
  -0.00024265764982757182,
  -0.0009868290789895003,
  0.00010107604775862525,
  0.0011320056175493293,
  0.00008322744070676078,
  -0.0012607039568197957,
  -0.000313189587445996,
  0.0013609102907284592,
  0.0005865005479915861,
  -0.0014231499261012765,
  -0.0009020356388477029,
  0.001434091509579234,
  0.0012528330053680416,
  -0.0013837461830167688,
  -0.00163268134380199,
  0.001259163870917044,
  0.0020292040727269566,
  -0.0010517792722827576,
  -0.002430651936553873,
  0.0007511618873668505,
  0.0028191610807396825,
  -0.00035242031519730336,
  -0.0031777587446098063,
  -0.00015003481157548277,
  0.00348385572755843,
  0.0007551537793834873,
  -0.003716429010794755,
  -0.0014615347809435067,
  0.003849740390862121,
  0.0022602494077657398,
  -0.0038606375753593124,
  -0.003141246912678169,
  0.0037224309284720793,
  0.0040864101949137045,
  -0.003412205369989498,
  -0.005076116428165842,
  0.0029047127339649394,
  0.006082513082831736,
  -0.002179549654506315,
  -0.007076224871119868,
  0.0012148243643460496,
  0.008019707616926356,
  0.000006006639985686466,
  -0.008873968856629966,
  -0.001501065327124657,
  0.009591680864667567,
  0.00328400833765102,
  -0.010123414411457598,
  -0.005372192712896252,
  0.010409700105032902,
  0.007782400439418641,
  -0.010385491025471831,
  -0.010541974053772827,
  0.009968845929852387,
  0.013689407940545783,
  -0.009059234071890193,
  -0.017294281263689572,
  0.007514333915370681,
  0.02147502919642247,
  -0.005123745496954874,
  -0.02645546750056862,
  0.0015302699627557062,
  0.032669708869143,
  0.003937879748477611,
  -0.041057964070763134,
  -0.012771671151265405,
  0.05397745825094055,
  0.029101100793058922,
  -0.07926921436553577,
  -0.07029651945216626,
  0.16678345582784435,
  0.4318258051668274,
  0.4318258051668274,
  0.16678345582784435,
  -0.07029651945216626,
  -0.07926921436553577,
  0.029101100793058922,
  0.05397745825094055,
  -0.012771671151265405,
  -0.041057964070763134,
  0.003937879748477611,
  0.032669708869143,
  0.0015302699627557062,
  -0.02645546750056862,
  -0.005123745496954874,
  0.02147502919642247,
  0.007514333915370681,
  -0.017294281263689572,
  -0.009059234071890193,
  0.013689407940545783,
  0.009968845929852387,
  -0.010541974053772827,
  -0.010385491025471831,
  0.007782400439418641,
  0.010409700105032902,
  -0.005372192712896252,
  -0.010123414411457598,
  0.00328400833765102,
  0.009591680864667567,
  -0.001501065327124657,
  -0.008873968856629966,
  0.000006006639985686466,
  0.008019707616926356,
  0.0012148243643460496,
  -0.007076224871119868,
  -0.002179549654506315,
  0.006082513082831736,
  0.0029047127339649394,
  -0.005076116428165842,
  -0.003412205369989498,
  0.0040864101949137045,
  0.0037224309284720793,
  -0.003141246912678169,
  -0.0038606375753593124,
  0.0022602494077657398,
  0.003849740390862121,
  -0.0014615347809435067,
  -0.003716429010794755,
  0.0007551537793834873,
  0.00348385572755843,
  -0.00015003481157548277,
  -0.0031777587446098063,
  -0.00035242031519730336,
  0.0028191610807396825,
  0.0007511618873668505,
  -0.002430651936553873,
  -0.0010517792722827576,
  0.0020292040727269566,
  0.001259163870917044,
  -0.00163268134380199,
  -0.0013837461830167688,
  0.0012528330053680416,
  0.001434091509579234,
  -0.0009020356388477029,
  -0.0014231499261012765,
  0.0005865005479915861,
  0.0013609102907284592,
  -0.000313189587445996,
  -0.0012607039568197957,
  0.00008322744070676078,
  0.0011320056175493293,
  0.00010107604775862525,
  -0.0009868290789895003,
  -0.00024265764982757182,
  0.000832716696886661,
  0.00034263209196798634,
  -0.0006792618029103274,
  -0.0004066422800012117,
  0.0005312976853953152,
  0.0004378193595686736,
  -0.0003955381628259023,
  -0.00044312916181558936,
  0.0002739130571304973,
  0.0004263666132372138,
  -0.00017035203882705303,
  -0.00039458899947662844,
  0.00008420509304807701,
  0.0003511765746380564,
  -0.000017122256926406192,
  -0.00030237540924826154,
  -0.00003348977533786711,
  0.00025042493941048404,
  0.00006751028439242685,
  -0.00020024922576395454,
  -0.0000886838926305579,
  0.00015263844527407924,
  0.00009763810842787279,
  -0.00011105314663570879,
  -0.00009850717253030551,
  0.00007486583376850753,
  0.00009197210903165128,
  -0.000046245927874971093,
  -0.00008195154572906459,
  0.000023433994126330156,
  0.00006868157707424679,
  -0.000007678036007735468,
  -0.00005547972379051587,
  -0.0000034765698445852614,
  0.00004189291782164469,
  0.000009258771433153023,
  -0.000030506955730396854,
  -0.000012377912994008165,
  0.000020165018923654894,
  0.000012143365583727823,
  -0.00001278470442689314,
  -0.000011154025415874738,
  0.000006666728806253622,
  0.000008522111305189946,
  -0.00000324656607540795,
  -0.000006480064921830934,
  5.82649902004218e-7,
  0.000003859674240005345,
  1.472719883278116e-7,
  -0.0000024451304666430503,
  -8.364044705258299e-7,
  8.668335150970118e-7,
  4.1651726936373105e-7,
  -4.561307723873045e-7,
  -4.572405451593453e-7,
  -1.233874226414999e-7
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 176400 Hz

* 0 Hz - 20000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.19958476672254752 dB

* 22000 Hz - 88200 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -71.51713354197533 dB

*/

static float LPF_256_1764[256] = {
  -0.000012279512544045723,
  0.00034590969664414687,
  0.0006438387460850933,
  0.0010280243722069515,
  0.0013353445356691705,
  0.0014328925446629754,
  0.0012386548360903706,
  0.0007689491896709781,
  0.00015156041039469286,
  -0.0004118384967495127,
  -0.0007237274599743983,
  -0.000677857227374268,
  -0.0003146005476676816,
  0.00018731893027430015,
  0.0005851116178237884,
  0.0006827920554820505,
  0.00042745672188398395,
  -0.00005886794533728311,
  -0.0005324197828624307,
  -0.0007454848582445817,
  -0.0005735989415024044,
  -0.00008699360102788667,
  0.0004745664555367106,
  0.000817175501852402,
  0.0007443948167078732,
  0.0002677827328514454,
  -0.00038512170549733594,
  -0.0008771236934951349,
  -0.0009335729969465061,
  -0.0004934820428108571,
  0.00024253279894850363,
  0.0009009501441794788,
  0.001123180810271669,
  0.0007589341365085485,
  -0.000038154135877725434,
  -0.0008711380938825889,
  -0.0012954984551540915,
  -0.001054576675379074,
  -0.0002308813538672611,
  0.0007742136939848772,
  0.0014336707510882046,
  0.0013692999139047304,
  0.0005655977279275661,
  -0.0005966188865225199,
  -0.0015175630525576204,
  -0.001686028008037316,
  -0.0009604964542727858,
  0.00032947421213681686,
  0.0015285309320077487,
  0.001986122607135542,
  0.0014069409660932817,
  0.00003365879415930311,
  -0.0014478241061850318,
  -0.0022481243708008278,
  -0.0018923573533122166,
  -0.0004964239733939482,
  0.0012565814928425082,
  0.00244697896668934,
  0.002398178133184088,
  0.001056681270887333,
  -0.0009388129991676913,
  -0.00255659959165558,
  -0.002902099321927321,
  -0.0017082712301521531,
  0.000480419495104351,
  0.0025492690850269857,
  0.0033771137299175536,
  0.0024395714809985277,
  0.00012939495257949064,
  -0.0023966209466412887,
  -0.0037919047395279758,
  -0.0032331559360811593,
  -0.0008970784634048236,
  0.0020709921000628515,
  0.004112108345516234,
  0.00406696055699184,
  0.001825989260778272,
  -0.0015441086994088726,
  -0.004299169454655971,
  -0.00491338069698928,
  -0.002915606482712893,
  0.0007877813888569555,
  0.004310786427972423,
  0.005739514175191729,
  0.0041620364194676854,
  0.00022720121873687248,
  -0.0040997701881001465,
  -0.0065072582054849434,
  -0.005560015668898629,
  -0.001534175592085737,
  0.003611408949285914,
  0.007172292156495216,
  0.007104049820131516,
  0.0031738611862527663,
  -0.0027795436771956457,
  -0.007682729766044604,
  -0.008792718972733196,
  -0.005204033849307677,
  0.0015163917600132502,
  0.007975414739616079,
  0.010635972816005201,
  0.007716550940506387,
  0.00030663530502347423,
  -0.007966569420414092,
  -0.012667025498254743,
  -0.01087233570844541,
  -0.0029014361858823385,
  0.007531588535280653,
  0.014971642704400268,
  0.014983468815187726,
  0.00666429588118504,
  -0.0064523183350309495,
  -0.01775666594662768,
  -0.020728185074852704,
  -0.012458704979921853,
  0.004261955747538636,
  0.02156352383222909,
  0.029857011795336178,
  0.022609932774830352,
  0.0003505195548144099,
  -0.02815776133768446,
  -0.04850512357720275,
  -0.04624605255639572,
  -0.013317602408434461,
  0.047510854940369183,
  0.12228264174728166,
  0.19007176879019422,
  0.23029952591504435,
  0.23029952591504435,
  0.19007176879019422,
  0.12228264174728166,
  0.047510854940369183,
  -0.013317602408434461,
  -0.04624605255639572,
  -0.04850512357720275,
  -0.02815776133768446,
  0.0003505195548144099,
  0.022609932774830352,
  0.029857011795336178,
  0.02156352383222909,
  0.004261955747538636,
  -0.012458704979921853,
  -0.020728185074852704,
  -0.01775666594662768,
  -0.0064523183350309495,
  0.00666429588118504,
  0.014983468815187726,
  0.014971642704400268,
  0.007531588535280653,
  -0.0029014361858823385,
  -0.01087233570844541,
  -0.012667025498254743,
  -0.007966569420414092,
  0.00030663530502347423,
  0.007716550940506387,
  0.010635972816005201,
  0.007975414739616079,
  0.0015163917600132502,
  -0.005204033849307677,
  -0.008792718972733196,
  -0.007682729766044604,
  -0.0027795436771956457,
  0.0031738611862527663,
  0.007104049820131516,
  0.007172292156495216,
  0.003611408949285914,
  -0.001534175592085737,
  -0.005560015668898629,
  -0.0065072582054849434,
  -0.0040997701881001465,
  0.00022720121873687248,
  0.0041620364194676854,
  0.005739514175191729,
  0.004310786427972423,
  0.0007877813888569555,
  -0.002915606482712893,
  -0.00491338069698928,
  -0.004299169454655971,
  -0.0015441086994088726,
  0.001825989260778272,
  0.00406696055699184,
  0.004112108345516234,
  0.0020709921000628515,
  -0.0008970784634048236,
  -0.0032331559360811593,
  -0.0037919047395279758,
  -0.0023966209466412887,
  0.00012939495257949064,
  0.0024395714809985277,
  0.0033771137299175536,
  0.0025492690850269857,
  0.000480419495104351,
  -0.0017082712301521531,
  -0.002902099321927321,
  -0.00255659959165558,
  -0.0009388129991676913,
  0.001056681270887333,
  0.002398178133184088,
  0.00244697896668934,
  0.0012565814928425082,
  -0.0004964239733939482,
  -0.0018923573533122166,
  -0.0022481243708008278,
  -0.0014478241061850318,
  0.00003365879415930311,
  0.0014069409660932817,
  0.001986122607135542,
  0.0015285309320077487,
  0.00032947421213681686,
  -0.0009604964542727858,
  -0.001686028008037316,
  -0.0015175630525576204,
  -0.0005966188865225199,
  0.0005655977279275661,
  0.0013692999139047304,
  0.0014336707510882046,
  0.0007742136939848772,
  -0.0002308813538672611,
  -0.001054576675379074,
  -0.0012954984551540915,
  -0.0008711380938825889,
  -0.000038154135877725434,
  0.0007589341365085485,
  0.001123180810271669,
  0.0009009501441794788,
  0.00024253279894850363,
  -0.0004934820428108571,
  -0.0009335729969465061,
  -0.0008771236934951349,
  -0.00038512170549733594,
  0.0002677827328514454,
  0.0007443948167078732,
  0.000817175501852402,
  0.0004745664555367106,
  -0.00008699360102788667,
  -0.0005735989415024044,
  -0.0007454848582445817,
  -0.0005324197828624307,
  -0.00005886794533728311,
  0.00042745672188398395,
  0.0006827920554820505,
  0.0005851116178237884,
  0.00018731893027430015,
  -0.0003146005476676816,
  -0.000677857227374268,
  -0.0007237274599743983,
  -0.0004118384967495127,
  0.00015156041039469286,
  0.0007689491896709781,
  0.0012386548360903706,
  0.0014328925446629754,
  0.0013353445356691705,
  0.0010280243722069515,
  0.0006438387460850933,
  0.00034590969664414687,
  -0.000012279512544045723
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 352800 Hz

* 0 Hz - 19000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.6931757918399839 dB

* 22000 Hz - 176400 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -60.70703963761856 dB

*/

static float LPF_256_3528[256] = {
  -0.000711232136390441,
  -0.0005937211782544453,
  -0.0007800944045261022,
  -0.0009497609175311817,
  -0.0010787929041029885,
  -0.0011427478672225257,
  -0.0011195498260157695,
  -0.0009925164348851398,
  -0.0007532513164160842,
  -0.00040375938654709957,
  0.00004206440177288954,
  0.0005587898092978046,
  0.0011102802718566116,
  0.0016531868435413564,
  0.002140176302175352,
  0.0025253172713963663,
  0.0027690625415784827,
  0.0028429804692429923,
  0.002734134870447168,
  0.002447472449844821,
  0.0020064098109422706,
  0.0014517220098459126,
  0.0008377857209139338,
  0.00022724862425358027,
  -0.00031565138443135813,
  -0.0007322498280108687,
  -0.000976721694697012,
  -0.00102190999325788,
  -0.0008632734066273822,
  -0.0005205797429036134,
  -0.00003648764527603549,
  0.0005277539091200441,
  0.0010989664141594902,
  0.0016006213570673616,
  0.0019621335769403224,
  0.0021280285841574122,
  0.0020655498764572903,
  0.0017697360272584946,
  0.0012652793465478087,
  0.0006048540811021882,
  -0.00013622696467299573,
  -0.0008686719291732307,
  -0.0014998935242724399,
  -0.0019456602431001263,
  -0.0021413963809147254,
  -0.0020514107641668553,
  -0.0016749254197958117,
  -0.001047866582419207,
  -0.00024003728159437108,
  0.000652128689212602,
  0.0015165962818700458,
  0.002239326376752246,
  0.002718934159087228,
  0.002880350014869966,
  0.0026859502243721588,
  0.002142297052961511,
  0.0013013686977244357,
  0.0002562174523722746,
  -0.0008689483812113674,
  -0.0019332393586063224,
  -0.0027962635499739577,
  -0.0033363191530772995,
  -0.0034671622525114065,
  -0.0031511585739689876,
  -0.002406445361954557,
  -0.0013075210640532133,
  0.000021555738630066052,
  0.001420601384462041,
  0.002711696028862533,
  0.0037213390676518513,
  0.004303472282486226,
  0.004359666027680551,
  0.0038544081371112483,
  0.0028223404516722265,
  0.001368235194419607,
  -0.00034230136348110653,
  -0.002101158655665598,
  -0.003683302803968405,
  -0.00487396677577141,
  -0.005495540465714521,
  -0.005434789843522995,
  -0.0046616637528415815,
  -0.0032327894506379677,
  -0.001297065221577159,
  0.0009254869780776884,
  0.0031627859808694347,
  0.00512510927507575,
  0.006540176542738789,
  0.0071894752149712395,
  0.006939457486347169,
  0.005764102607008874,
  0.0037548179216224,
  0.0011169221649870887,
  -0.0018500992251067245,
  -0.004782900137198276,
  -0.007298041170814939,
  -0.009037566128394542,
  -0.009717113583219278,
  -0.009167291688795874,
  -0.0073627790670395075,
  -0.004436866559455261,
  -0.0006770507098926655,
  0.0034993394523675047,
  0.007586314789810554,
  0.011044534025085859,
  0.013365023070733891,
  0.014134591301278357,
  0.013094308872253112,
  0.010185125598849968,
  0.005574163825280597,
  -0.00034290372059790945,
  -0.006964850311002285,
  -0.013532387612862603,
  -0.01919616339657165,
  -0.02309907150464473,
  -0.02446537158248697,
  -0.02268682762915932,
  -0.01739678230573191,
  -0.008523875239958113,
  0.003680862058313484,
  0.01864763740119383,
  0.03552297966719553,
  0.05323370777842715,
  0.07057449249796449,
  0.08631065960445447,
  0.09928644528373323,
  0.10852842388289528,
  0.11333360666650334,
  0.11333360666650334,
  0.10852842388289528,
  0.09928644528373323,
  0.08631065960445447,
  0.07057449249796449,
  0.05323370777842715,
  0.03552297966719553,
  0.01864763740119383,
  0.003680862058313484,
  -0.008523875239958113,
  -0.01739678230573191,
  -0.02268682762915932,
  -0.02446537158248697,
  -0.02309907150464473,
  -0.01919616339657165,
  -0.013532387612862603,
  -0.006964850311002285,
  -0.00034290372059790945,
  0.005574163825280597,
  0.010185125598849968,
  0.013094308872253112,
  0.014134591301278357,
  0.013365023070733891,
  0.011044534025085859,
  0.007586314789810554,
  0.0034993394523675047,
  -0.0006770507098926655,
  -0.004436866559455261,
  -0.0073627790670395075,
  -0.009167291688795874,
  -0.009717113583219278,
  -0.009037566128394542,
  -0.007298041170814939,
  -0.004782900137198276,
  -0.0018500992251067245,
  0.0011169221649870887,
  0.0037548179216224,
  0.005764102607008874,
  0.006939457486347169,
  0.0071894752149712395,
  0.006540176542738789,
  0.00512510927507575,
  0.0031627859808694347,
  0.0009254869780776884,
  -0.001297065221577159,
  -0.0032327894506379677,
  -0.0046616637528415815,
  -0.005434789843522995,
  -0.005495540465714521,
  -0.00487396677577141,
  -0.003683302803968405,
  -0.002101158655665598,
  -0.00034230136348110653,
  0.001368235194419607,
  0.0028223404516722265,
  0.0038544081371112483,
  0.004359666027680551,
  0.004303472282486226,
  0.0037213390676518513,
  0.002711696028862533,
  0.001420601384462041,
  0.000021555738630066052,
  -0.0013075210640532133,
  -0.002406445361954557,
  -0.0031511585739689876,
  -0.0034671622525114065,
  -0.0033363191530772995,
  -0.0027962635499739577,
  -0.0019332393586063224,
  -0.0008689483812113674,
  0.0002562174523722746,
  0.0013013686977244357,
  0.002142297052961511,
  0.0026859502243721588,
  0.002880350014869966,
  0.002718934159087228,
  0.002239326376752246,
  0.0015165962818700458,
  0.000652128689212602,
  -0.00024003728159437108,
  -0.001047866582419207,
  -0.0016749254197958117,
  -0.0020514107641668553,
  -0.0021413963809147254,
  -0.0019456602431001263,
  -0.0014998935242724399,
  -0.0008686719291732307,
  -0.00013622696467299573,
  0.0006048540811021882,
  0.0012652793465478087,
  0.0017697360272584946,
  0.0020655498764572903,
  0.0021280285841574122,
  0.0019621335769403224,
  0.0016006213570673616,
  0.0010989664141594902,
  0.0005277539091200441,
  -0.00003648764527603549,
  -0.0005205797429036134,
  -0.0008632734066273822,
  -0.00102190999325788,
  -0.000976721694697012,
  -0.0007322498280108687,
  -0.00031565138443135813,
  0.00022724862425358027,
  0.0008377857209139338,
  0.0014517220098459126,
  0.0020064098109422706,
  0.002447472449844821,
  0.002734134870447168,
  0.0028429804692429923,
  0.0027690625415784827,
  0.0025253172713963663,
  0.002140176302175352,
  0.0016531868435413564,
  0.0011102802718566116,
  0.0005587898092978046,
  0.00004206440177288954,
  -0.00040375938654709957,
  -0.0007532513164160842,
  -0.0009925164348851398,
  -0.0011195498260157695,
  -0.0011427478672225257,
  -0.0010787929041029885,
  -0.0009497609175311817,
  -0.0007800944045261022,
  -0.0005937211782544453,
  -0.000711232136390441
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 96000 Hz

* 0 Hz - 18000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.03485792898100224 dB

* 24000 Hz - 48000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -86.67327923923628 dB

*/

static float LPF_64_96[64] = {
  0.00013923492080123261,
  0.0005756531585841415,
  0.0007884170130830842,
  0.00012128933554432831,
  -0.0010057707195473837,
  -0.0008713990585965952,
  0.0009911789331157974,
  0.0020185679308250062,
  -0.00020984886169145343,
  -0.003162053825019493,
  -0.0016321246988159324,
  0.003608826636965099,
  0.00445821079173836,
  -0.0025170156149653187,
  -0.007658009429042673,
  -0.0008262014350395203,
  0.01004070510847133,
  0.006654326005294189,
  -0.009972469540732741,
  -0.014404790496206927,
  0.005674749727991049,
  0.0225322299278879,
  0.004436099949506035,
  -0.028463525311576067,
  -0.021617650229181273,
  0.028476506473844494,
  0.04747478371848614,
  -0.016412933828541653,
  -0.08805770736502652,
  -0.02636590020505139,
  0.19073502443579335,
  0.39545488705011467,
  0.39545488705011467,
  0.19073502443579335,
  -0.02636590020505139,
  -0.08805770736502652,
  -0.016412933828541653,
  0.04747478371848614,
  0.028476506473844494,
  -0.021617650229181273,
  -0.028463525311576067,
  0.004436099949506035,
  0.0225322299278879,
  0.005674749727991049,
  -0.014404790496206927,
  -0.009972469540732741,
  0.006654326005294189,
  0.01004070510847133,
  -0.0008262014350395203,
  -0.007658009429042673,
  -0.0025170156149653187,
  0.00445821079173836,
  0.003608826636965099,
  -0.0016321246988159324,
  -0.003162053825019493,
  -0.00020984886169145343,
  0.0020185679308250062,
  0.0009911789331157974,
  -0.0008713990585965952,
  -0.0010057707195473837,
  0.00012128933554432831,
  0.0007884170130830842,
  0.0005756531585841415,
  0.00013923492080123261
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 96000 Hz

* 0 Hz - 20000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.01730932443539188 dB

* 24000 Hz - 48000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -92.2939341050753 dB

*/

static float LPF_128_96[128] = {
  -0.000003388466219184029,
  -0.000015601498381853597,
  -0.000026643804348832594,
  -0.000010189627915433858,
  0.000026430524898847965,
  0.00001790763609109062,
  -0.00005867473647887538,
  -0.0000899994596970013,
  0.00002337845869276301,
  0.00013415722030182212,
  0.00000834232069273254,
  -0.0002241508297734047,
  -0.00014234802460659924,
  0.00023731627232215102,
  0.00028449299817390086,
  -0.00023535551550612168,
  -0.0005178806420629609,
  0.00008381046323670572,
  0.0007091827609278725,
  0.00014577315215508245,
  -0.0009047147407161551,
  -0.000563761562686935,
  0.0009355827423291159,
  0.0010520102853884589,
  -0.0008303190994329271,
  -0.0016635124202559256,
  0.0004195023726883794,
  0.0022110685535025316,
  0.00023931277431765827,
  -0.002681725258985498,
  -0.001258573288595998,
  0.0028332826976217532,
  0.0024944953350030004,
  -0.0026217922105095,
  -0.003949429550429041,
  0.0018103568193073311,
  0.005358140968556435,
  -0.0004009196153743552,
  -0.006598450437525926,
  -0.0017468054759101714,
  0.0072997957291961076,
  0.004491698954050074,
  -0.007263206747318094,
  -0.007794370819872529,
  0.006086542953875714,
  0.011309478744301263,
  -0.0035868181300292134,
  -0.01478094253340135,
  -0.0005791793316389216,
  0.017644588569773224,
  0.006512862283581355,
  -0.019423582460265813,
  -0.014506479984010303,
  0.019289940645616975,
  0.024744670053917672,
  -0.016333951297962532,
  -0.03800533879305329,
  0.008699792002685667,
  0.05619259517600354,
  0.007802552463959967,
  -0.08683599701516058,
  -0.05067654284968773,
  0.17925782929687725,
  0.4151899518036932,
  0.4151899518036932,
  0.17925782929687725,
  -0.05067654284968773,
  -0.08683599701516058,
  0.007802552463959967,
  0.05619259517600354,
  0.008699792002685667,
  -0.03800533879305329,
  -0.016333951297962532,
  0.024744670053917672,
  0.019289940645616975,
  -0.014506479984010303,
  -0.019423582460265813,
  0.006512862283581355,
  0.017644588569773224,
  -0.0005791793316389216,
  -0.01478094253340135,
  -0.0035868181300292134,
  0.011309478744301263,
  0.006086542953875714,
  -0.007794370819872529,
  -0.007263206747318094,
  0.004491698954050074,
  0.0072997957291961076,
  -0.0017468054759101714,
  -0.006598450437525926,
  -0.0004009196153743552,
  0.005358140968556435,
  0.0018103568193073311,
  -0.003949429550429041,
  -0.0026217922105095,
  0.0024944953350030004,
  0.0028332826976217532,
  -0.001258573288595998,
  -0.002681725258985498,
  0.00023931277431765827,
  0.0022110685535025316,
  0.0004195023726883794,
  -0.0016635124202559256,
  -0.0008303190994329271,
  0.0010520102853884589,
  0.0009355827423291159,
  -0.000563761562686935,
  -0.0009047147407161551,
  0.00014577315215508245,
  0.0007091827609278725,
  0.00008381046323670572,
  -0.0005178806420629609,
  -0.00023535551550612168,
  0.00028449299817390086,
  0.00023731627232215102,
  -0.00014234802460659924,
  -0.0002241508297734047,
  0.00000834232069273254,
  0.00013415722030182212,
  0.00002337845869276301,
  -0.0000899994596970013,
  -0.00005867473647887538,
  0.00001790763609109062,
  0.000026430524898847965,
  -0.000010189627915433858,
  -0.000026643804348832594,
  -0.000015601498381853597,
  -0.000003388466219184029
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 96000 Hz

* 0 Hz - 20000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.0034994409696737887 dB

* 22000 Hz - 48000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -106.63932587374329 dB

*/

static float LPF_256_96[256] = {
  0.000008409397344030037,
  0.00000792911448044749,
  -0.000014129977003659003,
  -0.00004767565136694943,
  -0.00005415463829089486,
  -0.00001554747066814107,
  0.00002742270606055546,
  0.00001925241764547503,
  -0.000027713632857395964,
  -0.0000389350372205654,
  0.000012924216618372403,
  0.000053044530224265294,
  0.00001040294664772026,
  -0.00005941088180341343,
  -0.00004211487096659811,
  0.00005102752498089464,
  0.00007575464341006262,
  -0.000024385018707681357,
  -0.0001025358294697428,
  -0.0000203205950663632,
  0.00011224788267105089,
  0.00007764443602556857,
  -0.00009585822719161727,
  -0.00013681250625308691,
  0.00004832119474102229,
  0.0001829712579322793,
  0.00002884771926932802,
  -0.00019988596805816904,
  -0.00012601985291919973,
  0.0001737543968471982,
  0.00022564710878990478,
  -0.00009729007201422634,
  -0.00030436425348643405,
  -0.000026679930705659368,
  0.00033707334945003224,
  0.00018313769849936617,
  -0.00030247228474917603,
  -0.0003450953401255247,
  0.00018928245183283857,
  0.00047700035246397353,
  -0.000001388279276248225,
  -0.0005408469663146555,
  -0.00023933623914144884,
  0.0005041868581217324,
  0.0004935230086509104,
  -0.0003488485510061683,
  -0.000708901190367084,
  0.00007856990223019568,
  0.000829101537972102,
  0.00027685434326352355,
  -0.00080515731203907,
  -0.000662324088981865,
  0.0006076151623862377,
  0.0010034755520085242,
  -0.000237469603138008,
  -0.001218648650844641,
  -0.0002675225485334583,
  0.0012348026580578002,
  0.0008332472823949221,
  -0.0010045001004943064,
  -0.0013570022898255238,
  0.0005213036074703745,
  0.0017231717600245814,
  0.00017060857677291295,
  -0.0018246127601166757,
  -0.0009756628988053416,
  0.0015859924662962458,
  0.001755998879210677,
  -0.000985617960175226,
  -0.0023515566514350638,
  0.00006980561082554887,
  0.002607851573426203,
  0.0010435390995795384,
  -0.0024077106296381888,
  -0.002174828437407867,
  0.0017016157993918013,
  0.0031071906228965734,
  -0.0005293523127551942,
  -0.0036216454026095886,
  -0.0009711393067181381,
  0.0035396741119529777,
  0.002573369905515296,
  -0.002764239301876103,
  -0.0039894874204905026,
  0.0013131172161114674,
  0.004914498452767665,
  0.0006646133190681525,
  -0.005080402649358242,
  -0.0028933452169882736,
  0.004312735792245397,
  0.005000132973120942,
  -0.00257888390507092,
  -0.006565988452484033,
  0.000018790790883055608,
  0.007193812707240999,
  0.003050182449273472,
  -0.00658306221236475,
  -0.006158534199267109,
  0.0045993693836768575,
  0.008739573234100406,
  -0.0013268606114257794,
  -0.010208798567578007,
  -0.0029075383600204985,
  0.010057518656181416,
  0.007544497214855383,
  -0.007947563379438416,
  -0.011838999430000247,
  0.0037925353781480954,
  0.014938300560466227,
  0.0021869172617886823,
  -0.015980367402096574,
  -0.009442765815393186,
  0.014195277316465483,
  0.017114700065312682,
  -0.008987601974619619,
  -0.02407073839972876,
  -0.00003103137994087173,
  0.0289425482562979,
  0.013109581480538928,
  -0.030077806691948875,
  -0.030649396330558427,
  0.025171309791875162,
  0.05417328183199872,
  -0.009550585905104981,
  -0.09047996728068318,
  -0.034340022616914954,
  0.1883014533876371,
  0.4017703985311941,
  0.4017703985311941,
  0.1883014533876371,
  -0.034340022616914954,
  -0.09047996728068318,
  -0.009550585905104981,
  0.05417328183199872,
  0.025171309791875162,
  -0.030649396330558427,
  -0.030077806691948875,
  0.013109581480538928,
  0.0289425482562979,
  -0.00003103137994087173,
  -0.02407073839972876,
  -0.008987601974619619,
  0.017114700065312682,
  0.014195277316465483,
  -0.009442765815393186,
  -0.015980367402096574,
  0.0021869172617886823,
  0.014938300560466227,
  0.0037925353781480954,
  -0.011838999430000247,
  -0.007947563379438416,
  0.007544497214855383,
  0.010057518656181416,
  -0.0029075383600204985,
  -0.010208798567578007,
  -0.0013268606114257794,
  0.008739573234100406,
  0.0045993693836768575,
  -0.006158534199267109,
  -0.00658306221236475,
  0.003050182449273472,
  0.007193812707240999,
  0.000018790790883055608,
  -0.006565988452484033,
  -0.00257888390507092,
  0.005000132973120942,
  0.004312735792245397,
  -0.0028933452169882736,
  -0.005080402649358242,
  0.0006646133190681525,
  0.004914498452767665,
  0.0013131172161114674,
  -0.0039894874204905026,
  -0.002764239301876103,
  0.002573369905515296,
  0.0035396741119529777,
  -0.0009711393067181381,
  -0.0036216454026095886,
  -0.0005293523127551942,
  0.0031071906228965734,
  0.0017016157993918013,
  -0.002174828437407867,
  -0.0024077106296381888,
  0.0010435390995795384,
  0.002607851573426203,
  0.00006980561082554887,
  -0.0023515566514350638,
  -0.000985617960175226,
  0.001755998879210677,
  0.0015859924662962458,
  -0.0009756628988053416,
  -0.0018246127601166757,
  0.00017060857677291295,
  0.0017231717600245814,
  0.0005213036074703745,
  -0.0013570022898255238,
  -0.0010045001004943064,
  0.0008332472823949221,
  0.0012348026580578002,
  -0.0002675225485334583,
  -0.001218648650844641,
  -0.000237469603138008,
  0.0010034755520085242,
  0.0006076151623862377,
  -0.000662324088981865,
  -0.00080515731203907,
  0.00027685434326352355,
  0.000829101537972102,
  0.00007856990223019568,
  -0.000708901190367084,
  -0.0003488485510061683,
  0.0004935230086509104,
  0.0005041868581217324,
  -0.00023933623914144884,
  -0.0005408469663146555,
  -0.000001388279276248225,
  0.00047700035246397353,
  0.00018928245183283857,
  -0.0003450953401255247,
  -0.00030247228474917603,
  0.00018313769849936617,
  0.00033707334945003224,
  -0.000026679930705659368,
  -0.00030436425348643405,
  -0.00009729007201422634,
  0.00022564710878990478,
  0.0001737543968471982,
  -0.00012601985291919973,
  -0.00019988596805816904,
  0.00002884771926932802,
  0.0001829712579322793,
  0.00004832119474102229,
  -0.00013681250625308691,
  -0.00009585822719161727,
  0.00007764443602556857,
  0.00011224788267105089,
  -0.0000203205950663632,
  -0.0001025358294697428,
  -0.000024385018707681357,
  0.00007575464341006262,
  0.00005102752498089464,
  -0.00004211487096659811,
  -0.00005941088180341343,
  0.00001040294664772026,
  0.000053044530224265294,
  0.000012924216618372403,
  -0.0000389350372205654,
  -0.000027713632857395964,
  0.00001925241764547503,
  0.00002742270606055546,
  -0.00001554747066814107,
  -0.00005415463829089486,
  -0.00004767565136694943,
  -0.000014129977003659003,
  0.00000792911448044749,
  0.000008409397344030037
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 192000 Hz

* 0 Hz - 17000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.5734513435758413 dB

* 24000 Hz - 96000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -62.35252188949316 dB

*/

static float LPF_64_192[64] = {
  -0.0006125591091897512,
  -0.002132181537823223,
  -0.0034207494681011315,
  -0.005077969963373183,
  -0.005788931170325741,
  -0.005256155068773428,
  -0.0030221940730850235,
  0.0004976563917948214,
  0.004344306662587212,
  0.0070519780612529675,
  0.007272074052623634,
  0.004354657875624558,
  -0.0011429900170132484,
  -0.007406257433995558,
  -0.011847557579346258,
  -0.012069496082054569,
  -0.006968614111406536,
  0.002466225905376266,
  0.013092608396697736,
  0.020475539315484518,
  0.02049863639218361,
  0.011207950782064078,
  -0.005828780653332538,
  -0.025283424081018274,
  -0.039247064716339486,
  -0.039599125588827395,
  -0.020858809792046042,
  0.01746937035329495,
  0.0699615630273449,
  0.12622312837423091,
  0.17356162310839102,
  0.20058627792021644,
  0.20058627792021644,
  0.17356162310839102,
  0.12622312837423091,
  0.0699615630273449,
  0.01746937035329495,
  -0.020858809792046042,
  -0.039599125588827395,
  -0.039247064716339486,
  -0.025283424081018274,
  -0.005828780653332538,
  0.011207950782064078,
  0.02049863639218361,
  0.020475539315484518,
  0.013092608396697736,
  0.002466225905376266,
  -0.006968614111406536,
  -0.012069496082054569,
  -0.011847557579346258,
  -0.007406257433995558,
  -0.0011429900170132484,
  0.004354657875624558,
  0.007272074052623634,
  0.0070519780612529675,
  0.004344306662587212,
  0.0004976563917948214,
  -0.0030221940730850235,
  -0.005256155068773428,
  -0.005788931170325741,
  -0.005077969963373183,
  -0.0034207494681011315,
  -0.002132181537823223,
  -0.0006125591091897512
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 192000 Hz

* 0 Hz - 20000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.32431512738382107 dB

* 24000 Hz - 96000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -67.30096409695778 dB

*/

static float LPF_128_192[128] = {
  -0.0004168279464196247,
  -0.00043328218743436263,
  -0.0003538291854635148,
  0.00005491508949681246,
  0.0008052384065771798,
  0.0017456476344537286,
  0.002575394122069929,
  0.0029422679899913096,
  0.002596600707844533,
  0.0015361050530435405,
  0.0000647646315739159,
  -0.0012846878307797727,
  -0.0019558552735542078,
  -0.0016240621369847078,
  -0.00038438032790543856,
  0.0012333223718018118,
  0.002459641725772306,
  0.002627742759792068,
  0.0015140233926682661,
  -0.0004932992168130596,
  -0.002507533962319746,
  -0.0035219729826872,
  -0.002899576160135281,
  -0.0007422718603853722,
  0.0020632593939278326,
  0.004189985723305679,
  0.004482175313137186,
  0.0025602523207696997,
  -0.0008884475849643903,
  -0.004320585032578146,
  -0.006006555257617614,
  -0.004879153797620689,
  -0.0011554930775272334,
  0.0036125955216041593,
  0.007151714753444691,
  0.007524634092782191,
  0.004145399397271755,
  -0.0017537571667507682,
  -0.007514263606694116,
  -0.010208844169216108,
  -0.008078085191586903,
  -0.0015660473992653008,
  0.006614925484667335,
  0.0125467885682134,
  0.012909301608139802,
  0.006725760464854965,
  -0.003819444667801155,
  -0.01401893057119795,
  -0.01862001115868469,
  -0.014358529393071785,
  -0.001894846270746489,
  0.01386669645236032,
  0.02546318964072758,
  0.02606494036656201,
  0.01291211904442809,
  -0.01053757278415136,
  -0.034892396176049396,
  -0.04774598994334475,
  -0.03831800166671098,
  -0.002074014887347417,
  0.05656762256284204,
  0.12470370760416337,
  0.18465431720498549,
  0.21968695487373988,
  0.21968695487373988,
  0.18465431720498549,
  0.12470370760416337,
  0.05656762256284204,
  -0.002074014887347417,
  -0.03831800166671098,
  -0.04774598994334475,
  -0.034892396176049396,
  -0.01053757278415136,
  0.01291211904442809,
  0.02606494036656201,
  0.02546318964072758,
  0.01386669645236032,
  -0.001894846270746489,
  -0.014358529393071785,
  -0.01862001115868469,
  -0.01401893057119795,
  -0.003819444667801155,
  0.006725760464854965,
  0.012909301608139802,
  0.0125467885682134,
  0.006614925484667335,
  -0.0015660473992653008,
  -0.008078085191586903,
  -0.010208844169216108,
  -0.007514263606694116,
  -0.0017537571667507682,
  0.004145399397271755,
  0.007524634092782191,
  0.007151714753444691,
  0.0036125955216041593,
  -0.0011554930775272334,
  -0.004879153797620689,
  -0.006006555257617614,
  -0.004320585032578146,
  -0.0008884475849643903,
  0.0025602523207696997,
  0.004482175313137186,
  0.004189985723305679,
  0.0020632593939278326,
  -0.0007422718603853722,
  -0.002899576160135281,
  -0.0035219729826872,
  -0.002507533962319746,
  -0.0004932992168130596,
  0.0015140233926682661,
  0.002627742759792068,
  0.002459641725772306,
  0.0012333223718018118,
  -0.00038438032790543856,
  -0.0016240621369847078,
  -0.0019558552735542078,
  -0.0012846878307797727,
  0.0000647646315739159,
  0.0015361050530435405,
  0.002596600707844533,
  0.0029422679899913096,
  0.002575394122069929,
  0.0017456476344537286,
  0.0008052384065771798,
  0.00005491508949681246,
  -0.0003538291854635148,
  -0.00043328218743436263,
  -0.0004168279464196247
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 192000 Hz

* 0 Hz - 20000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.2960445845547247 dB

* 22000 Hz - 96000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -68.0929975681415 dB

*/

static float LPF_256_192[256] = {
  0.0003895530929486431,
  0.0004599199538157325,
  0.0005424406409098877,
  0.0004480235964582806,
  0.00012283393440307418,
  -0.00041722684577440513,
  -0.0010728164748845656,
  -0.0016789710409604987,
  -0.002053826111715261,
  -0.0020635974228653724,
  -0.0016793469703438545,
  -0.0010009022767412203,
  -0.0002333601754285527,
  0.0003805126578396617,
  0.0006490335406356264,
  0.0005072243301200239,
  0.000047133985158496214,
  -0.000513709617679563,
  -0.0009170750487312066,
  -0.000969940012687841,
  -0.0006283789750409286,
  -0.00002166462807701789,
  0.0005970972335177095,
  0.0009566312649767858,
  0.0008870648737260908,
  0.000400784226242249,
  -0.00030276672364246745,
  -0.0009159101588058498,
  -0.001154477773438763,
  -0.0008871593771805827,
  -0.00020380359245253987,
  0.0006116981653223742,
  0.001198000120060689,
  0.0012752030242213707,
  0.000776850279581786,
  -0.00010645870815146598,
  -0.000997353908891487,
  -0.0014912726436853876,
  -0.001338593380556061,
  -0.0005680070655031433,
  0.0005089086783429051,
  0.0014216019902398708,
  0.00174465543166131,
  0.0012943525760290052,
  0.00022746233909625574,
  -0.0010087688728137392,
  -0.001862182345700517,
  -0.0019206392791159032,
  -0.001108508205674253,
  0.0002579973301146177,
  0.0015905967351757908,
  0.0022802567542615157,
  0.0019735755127219833,
  0.000749568009126515,
  -0.0008898180776919695,
  -0.00222280263574845,
  -0.0026234426202543897,
  -0.0018528326013301556,
  -0.00019153758676683096,
  0.0016568895685043048,
  0.002861113534298231,
  0.0028334154196715017,
  0.001512508440937585,
  -0.0005768013609455142,
  -0.0025277773769119233,
  -0.003442414037757975,
  -0.0028416168301655456,
  -0.0009087546358221812,
  0.0015540300695857756,
  0.0034546612942783893,
  0.003891598218580495,
  0.002577326504310224,
  0.000006126866112363057,
  -0.002723680429912773,
  -0.0043748433065254385,
  -0.004127174917584241,
  -0.001977212643675138,
  0.0012096030705589312,
  0.004040558377492658,
  0.0051989983903645886,
  0.004050262254521983,
  0.0009742383866819471,
  -0.00274423749730762,
  -0.005444461510157206,
  -0.005824154734719537,
  -0.0035579957977535273,
  0.0004900988815077832,
  0.004583985635236193,
  0.006850059473687851,
  0.006123934391869783,
  0.0025319581309859386,
  -0.002478244102381314,
  -0.006707460515239372,
  -0.008155291454339271,
  -0.005951814467022548,
  -0.0008355798243428299,
  0.005061906629299169,
  0.009086476125316696,
  0.00923417736785398,
  0.005119815443376935,
  -0.0017157734347381624,
  -0.00835063548005438,
  -0.011700851248060166,
  -0.00991717252209631,
  -0.0033420576981112835,
  0.005443627252343513,
  0.012586427016176098,
  0.014595653793055885,
  0.009975483039446889,
  0.00012100807470299453,
  -0.010996738623476198,
  -0.0183413162298717,
  -0.017972960416215156,
  -0.00897791761721718,
  0.005710311238668767,
  0.020086777469000908,
  0.027306879445805607,
  0.02260294429616013,
  0.005746114992698684,
  -0.01801712359101611,
  -0.03911539197831309,
  -0.04667256883659577,
  -0.03242717709536991,
  0.005849539403180411,
  0.0627543121822654,
  0.12619879876986836,
  0.1807339165922969,
  0.21220981174131515,
  0.21220981174131515,
  0.1807339165922969,
  0.12619879876986836,
  0.0627543121822654,
  0.005849539403180411,
  -0.03242717709536991,
  -0.04667256883659577,
  -0.03911539197831309,
  -0.01801712359101611,
  0.005746114992698684,
  0.02260294429616013,
  0.027306879445805607,
  0.020086777469000908,
  0.005710311238668767,
  -0.00897791761721718,
  -0.017972960416215156,
  -0.0183413162298717,
  -0.010996738623476198,
  0.00012100807470299453,
  0.009975483039446889,
  0.014595653793055885,
  0.012586427016176098,
  0.005443627252343513,
  -0.0033420576981112835,
  -0.00991717252209631,
  -0.011700851248060166,
  -0.00835063548005438,
  -0.0017157734347381624,
  0.005119815443376935,
  0.00923417736785398,
  0.009086476125316696,
  0.005061906629299169,
  -0.0008355798243428299,
  -0.005951814467022548,
  -0.008155291454339271,
  -0.006707460515239372,
  -0.002478244102381314,
  0.0025319581309859386,
  0.006123934391869783,
  0.006850059473687851,
  0.004583985635236193,
  0.0004900988815077832,
  -0.0035579957977535273,
  -0.005824154734719537,
  -0.005444461510157206,
  -0.00274423749730762,
  0.0009742383866819471,
  0.004050262254521983,
  0.0051989983903645886,
  0.004040558377492658,
  0.0012096030705589312,
  -0.001977212643675138,
  -0.004127174917584241,
  -0.0043748433065254385,
  -0.002723680429912773,
  0.000006126866112363057,
  0.002577326504310224,
  0.003891598218580495,
  0.0034546612942783893,
  0.0015540300695857756,
  -0.0009087546358221812,
  -0.0028416168301655456,
  -0.003442414037757975,
  -0.0025277773769119233,
  -0.0005768013609455142,
  0.001512508440937585,
  0.0028334154196715017,
  0.002861113534298231,
  0.0016568895685043048,
  -0.00019153758676683096,
  -0.0018528326013301556,
  -0.0026234426202543897,
  -0.00222280263574845,
  -0.0008898180776919695,
  0.000749568009126515,
  0.0019735755127219833,
  0.0022802567542615157,
  0.0015905967351757908,
  0.0002579973301146177,
  -0.001108508205674253,
  -0.0019206392791159032,
  -0.001862182345700517,
  -0.0010087688728137392,
  0.00022746233909625574,
  0.0012943525760290052,
  0.00174465543166131,
  0.0014216019902398708,
  0.0005089086783429051,
  -0.0005680070655031433,
  -0.001338593380556061,
  -0.0014912726436853876,
  -0.000997353908891487,
  -0.00010645870815146598,
  0.000776850279581786,
  0.0012752030242213707,
  0.001198000120060689,
  0.0006116981653223742,
  -0.00020380359245253987,
  -0.0008871593771805827,
  -0.001154477773438763,
  -0.0009159101588058498,
  -0.00030276672364246745,
  0.000400784226242249,
  0.0008870648737260908,
  0.0009566312649767858,
  0.0005970972335177095,
  -0.00002166462807701789,
  -0.0006283789750409286,
  -0.000969940012687841,
  -0.0009170750487312066,
  -0.000513709617679563,
  0.000047133985158496214,
  0.0005072243301200239,
  0.0006490335406356264,
  0.0003805126578396617,
  -0.0002333601754285527,
  -0.0010009022767412203,
  -0.0016793469703438545,
  -0.0020635974228653724,
  -0.002053826111715261,
  -0.0016789710409604987,
  -0.0010728164748845656,
  -0.00041722684577440513,
  0.00012283393440307418,
  0.0004480235964582806,
  0.0005424406409098877,
  0.0004599199538157325,
  0.0003895530929486431
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 384000 Hz

* 0 Hz - 15000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 3.058816035046404 dB

* 24000 Hz - 192000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -47.89735410291313 dB

*/

static float LPF_64_384[64] = {
  0.002927882684001967,
  0.002085362752258263,
  0.002561811072345439,
  0.002856982937005518,
  0.0028745695686630364,
  0.0025031014276775446,
  0.0016500528380861386,
  0.00025261788242966434,
  -0.0017115376101456909,
  -0.004215641823187509,
  -0.007170809987259646,
  -0.01042241121492704,
  -0.01375316123309944,
  -0.016893603472073512,
  -0.019534567547513364,
  -0.021345842582006946,
  -0.022000428860012224,
  -0.021203605090915133,
  -0.018720394396653607,
  -0.014399862274437496,
  -0.008194814011234319,
  -0.00017705364758225625,
  0.00945552104783621,
  0.02038197589734559,
  0.032172293825251747,
  0.04430983881609776,
  0.056221201875193895,
  0.0673118813810134,
  0.0770058525463164,
  0.08478491128781818,
  0.09022434076827515,
  0.09302245148328328,
  0.09302245148328328,
  0.09022434076827515,
  0.08478491128781818,
  0.0770058525463164,
  0.0673118813810134,
  0.056221201875193895,
  0.04430983881609776,
  0.032172293825251747,
  0.02038197589734559,
  0.00945552104783621,
  -0.00017705364758225625,
  -0.008194814011234319,
  -0.014399862274437496,
  -0.018720394396653607,
  -0.021203605090915133,
  -0.022000428860012224,
  -0.021345842582006946,
  -0.019534567547513364,
  -0.016893603472073512,
  -0.01375316123309944,
  -0.01042241121492704,
  -0.007170809987259646,
  -0.004215641823187509,
  -0.0017115376101456909,
  0.00025261788242966434,
  0.0016500528380861386,
  0.0025031014276775446,
  0.0028745695686630364,
  0.002856982937005518,
  0.002561811072345439,
  0.002085362752258263,
  0.002927882684001967
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 384000 Hz

* 0 Hz - 17000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.5721619363506063 dB

* 24000 Hz - 192000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -62.37205996583444 dB

*/

static float LPF_128_384[128] = {
  -0.00004289853771121794,
  -0.0007213137371070805,
  -0.0007768611229821008,
  -0.0011501783273056663,
  -0.001546647862298532,
  -0.0019563428830992546,
  -0.002339728521267254,
  -0.0026558629938977065,
  -0.0028589694784207065,
  -0.0029079324428894966,
  -0.0027697062714865063,
  -0.002424067472112675,
  -0.0018693454562378113,
  -0.0011260407213205636,
  -0.0002362252407730758,
  0.0007361246403435034,
  0.0017086118750246182,
  0.0025878384479185446,
  0.003278047540843394,
  0.003689894365460355,
  0.003751792701633047,
  0.003420456989566836,
  0.002688707067198203,
  0.0015903622413836365,
  0.00020205656858473647,
  -0.0013599265482999246,
  -0.0029473661108016754,
  -0.004392494906348255,
  -0.00552409922423523,
  -0.0061854977240184995,
  -0.006252515725295101,
  -0.00565078033015888,
  -0.004369649671416899,
  -0.0024703290900271423,
  -0.0000871139271444027,
  0.002578550574107198,
  0.005271939984382884,
  0.007705943726906496,
  0.00958750905943804,
  0.010647655595283944,
  0.010672334476640542,
  0.009530681445819917,
  0.007197956341739426,
  0.0037703512659174048,
  -0.0005305734102971746,
  -0.005364948244365862,
  -0.010293314752374591,
  -0.014806639647231253,
  -0.018365396966613264,
  -0.020444386236165835,
  -0.020579710486939375,
  -0.018414010507899654,
  -0.013735517806233605,
  -0.00650712412481655,
  0.0031176856491159704,
  0.014794201842206636,
  0.02800179549535362,
  0.04207435456746542,
  0.05624409368440713,
  0.06969537798427448,
  0.08162399236445074,
  0.09129706959947512,
  0.09810881493344374,
  0.10162712345675438,
  0.10162712345675438,
  0.09810881493344374,
  0.09129706959947512,
  0.08162399236445074,
  0.06969537798427448,
  0.05624409368440713,
  0.04207435456746542,
  0.02800179549535362,
  0.014794201842206636,
  0.0031176856491159704,
  -0.00650712412481655,
  -0.013735517806233605,
  -0.018414010507899654,
  -0.020579710486939375,
  -0.020444386236165835,
  -0.018365396966613264,
  -0.014806639647231253,
  -0.010293314752374591,
  -0.005364948244365862,
  -0.0005305734102971746,
  0.0037703512659174048,
  0.007197956341739426,
  0.009530681445819917,
  0.010672334476640542,
  0.010647655595283944,
  0.00958750905943804,
  0.007705943726906496,
  0.005271939984382884,
  0.002578550574107198,
  -0.0000871139271444027,
  -0.0024703290900271423,
  -0.004369649671416899,
  -0.00565078033015888,
  -0.006252515725295101,
  -0.0061854977240184995,
  -0.00552409922423523,
  -0.004392494906348255,
  -0.0029473661108016754,
  -0.0013599265482999246,
  0.00020205656858473647,
  0.0015903622413836365,
  0.002688707067198203,
  0.003420456989566836,
  0.003751792701633047,
  0.003689894365460355,
  0.003278047540843394,
  0.0025878384479185446,
  0.0017086118750246182,
  0.0007361246403435034,
  -0.0002362252407730758,
  -0.0011260407213205636,
  -0.0018693454562378113,
  -0.002424067472112675,
  -0.0027697062714865063,
  -0.0029079324428894966,
  -0.0028589694784207065,
  -0.0026558629938977065,
  -0.002339728521267254,
  -0.0019563428830992546,
  -0.001546647862298532,
  -0.0011501783273056663,
  -0.0007768611229821008,
  -0.0007213137371070805,
  -0.00004289853771121794
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 384000 Hz

* 0 Hz - 20000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.31834261044067436 dB

* 24000 Hz - 192000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -67.46237594199393 dB

*/

static float LPF_256_384[256] = {
  -0.0003028971817199662,
  -0.00020555801988433382,
  -0.0002452203090790241,
  -0.00026327360166700995,
  -0.000249699948929114,
  -0.00019594264284566245,
  -0.00009633932481120885,
  0.00005053491643358132,
  0.0002408852448482736,
  0.00046522662944123433,
  0.0007085557972226111,
  0.0009512454315017002,
  0.0011706502732783811,
  0.0013432981754322132,
  0.0014475410063521366,
  0.001466266379078221,
  0.0013894642711656158,
  0.00121608093306262,
  0.0009552307118196105,
  0.0006261114539335104,
  0.0002568607951537778,
  -0.00011788064249887931,
  -0.0004604484309872351,
  -0.0007341509947457277,
  -0.0009076528992662767,
  -0.0009590627085821301,
  -0.000879158380295847,
  -0.000673402871466806,
  -0.00036229402705512906,
  0.0000200280343931034,
  0.0004289928291725036,
  0.0008143444069374276,
  0.0011258824086395587,
  0.0013195709387566665,
  0.001363395359623526,
  0.0012421143240098824,
  0.0009601311390357797,
  0.0005422493808396902,
  0.00003194697160880381,
  -0.0005130105729659689,
  -0.0010267714186298507,
  -0.0014431221033866172,
  -0.0017039751027979773,
  -0.0017668632629682358,
  -0.0016116359670230343,
  -0.001244156881894752,
  -0.0006973984993918269,
  -0.0000289175288921703,
  0.000684788850705755,
  0.001356759986875993,
  0.0018998831194359748,
  0.002237874630225739,
  0.0023155656133239753,
  0.0021071620477858687,
  0.001621337034799425,
  0.0009022293138781065,
  0.000026166073972288994,
  -0.0009062250494084473,
  -0.001780739531563792,
  -0.002483910584201641,
  -0.0029165362704376163,
  -0.0030078100152049615,
  -0.002725641760473421,
  -0.0020829404891073654,
  -0.0011390361381104666,
  0.000005176783940116584,
  0.0012175418764740213,
  0.0023492338401935506,
  0.0032525144297898454,
  0.0037995404889011744,
  0.0038996663782179977,
  0.003513072889772702,
  0.002658897472241464,
  0.001416479104319737,
  -0.00008102997874650616,
  -0.0016601797184165456,
  -0.0031266083757365416,
  -0.004288070210646929,
  -0.004978740898067277,
  -0.005081786097521481,
  -0.004547095121975206,
  -0.003401811563280599,
  -0.0017517071151958606,
  0.00022717598878383647,
  0.0023060461362887914,
  0.004228644782761341,
  0.005741423210500968,
  0.006625504972841762,
  0.006726417049775001,
  0.0059777357854568745,
  0.004415150676447778,
  0.0021788350163281034,
  -0.0004966682103826998,
  -0.0033047190498417893,
  -0.005899615661607898,
  -0.007937094156427038,
  -0.009116855604637613,
  -0.009222038113100677,
  -0.008152937166736584,
  -0.005949300222485785,
  -0.0027933469133305305,
  0.0009973017470928476,
  0.005000258461483804,
  0.008728981341455742,
  0.01168672031287444,
  0.013424899005360244,
  0.013600919663776384,
  0.01202843872836453,
  0.008714016823819067,
  0.003875114344102797,
  -0.002063752476230455,
  -0.008497639065431994,
  -0.014689177123081244,
  -0.0198340439832851,
  -0.023138112304936186,
  -0.02389857912001871,
  -0.02158130640671657,
  -0.015885661949237677,
  -0.00679036702131726,
  0.005425806549646582,
  0.0201911418416205,
  0.03667626560952992,
  0.05385537936602855,
  0.07058791161470955,
  0.08571339597904032,
  0.09815075327377187,
  0.10699241367736285,
  0.1115843784341314,
  0.1115843784341314,
  0.10699241367736285,
  0.09815075327377187,
  0.08571339597904032,
  0.07058791161470955,
  0.05385537936602855,
  0.03667626560952992,
  0.0201911418416205,
  0.005425806549646582,
  -0.00679036702131726,
  -0.015885661949237677,
  -0.02158130640671657,
  -0.02389857912001871,
  -0.023138112304936186,
  -0.0198340439832851,
  -0.014689177123081244,
  -0.008497639065431994,
  -0.002063752476230455,
  0.003875114344102797,
  0.008714016823819067,
  0.01202843872836453,
  0.013600919663776384,
  0.013424899005360244,
  0.01168672031287444,
  0.008728981341455742,
  0.005000258461483804,
  0.0009973017470928476,
  -0.0027933469133305305,
  -0.005949300222485785,
  -0.008152937166736584,
  -0.009222038113100677,
  -0.009116855604637613,
  -0.007937094156427038,
  -0.005899615661607898,
  -0.0033047190498417893,
  -0.0004966682103826998,
  0.0021788350163281034,
  0.004415150676447778,
  0.0059777357854568745,
  0.006726417049775001,
  0.006625504972841762,
  0.005741423210500968,
  0.004228644782761341,
  0.0023060461362887914,
  0.00022717598878383647,
  -0.0017517071151958606,
  -0.003401811563280599,
  -0.004547095121975206,
  -0.005081786097521481,
  -0.004978740898067277,
  -0.004288070210646929,
  -0.0031266083757365416,
  -0.0016601797184165456,
  -0.00008102997874650616,
  0.001416479104319737,
  0.002658897472241464,
  0.003513072889772702,
  0.0038996663782179977,
  0.0037995404889011744,
  0.0032525144297898454,
  0.0023492338401935506,
  0.0012175418764740213,
  0.000005176783940116584,
  -0.0011390361381104666,
  -0.0020829404891073654,
  -0.002725641760473421,
  -0.0030078100152049615,
  -0.0029165362704376163,
  -0.002483910584201641,
  -0.001780739531563792,
  -0.0009062250494084473,
  0.000026166073972288994,
  0.0009022293138781065,
  0.001621337034799425,
  0.0021071620477858687,
  0.0023155656133239753,
  0.002237874630225739,
  0.0018998831194359748,
  0.001356759986875993,
  0.000684788850705755,
  -0.0000289175288921703,
  -0.0006973984993918269,
  -0.001244156881894752,
  -0.0016116359670230343,
  -0.0017668632629682358,
  -0.0017039751027979773,
  -0.0014431221033866172,
  -0.0010267714186298507,
  -0.0005130105729659689,
  0.00003194697160880381,
  0.0005422493808396902,
  0.0009601311390357797,
  0.0012421143240098824,
  0.001363395359623526,
  0.0013195709387566665,
  0.0011258824086395587,
  0.0008143444069374276,
  0.0004289928291725036,
  0.0000200280343931034,
  -0.00036229402705512906,
  -0.000673402871466806,
  -0.000879158380295847,
  -0.0009590627085821301,
  -0.0009076528992662767,
  -0.0007341509947457277,
  -0.0004604484309872351,
  -0.00011788064249887931,
  0.0002568607951537778,
  0.0006261114539335104,
  0.0009552307118196105,
  0.00121608093306262,
  0.0013894642711656158,
  0.001466266379078221,
  0.0014475410063521366,
  0.0013432981754322132,
  0.0011706502732783811,
  0.0009512454315017002,
  0.0007085557972226111,
  0.00046522662944123433,
  0.0002408852448482736,
  0.00005053491643358132,
  -0.00009633932481120885,
  -0.00019594264284566245,
  -0.000249699948929114,
  -0.00026327360166700995,
  -0.0002452203090790241,
  -0.00020555801988433382,
  -0.0003028971817199662
};
