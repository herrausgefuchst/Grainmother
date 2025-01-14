#include "SampleRateConverter.h"


// =======================================================================================
// MARK: - HELPER FUNCTIONS
// =======================================================================================

inline float** decomposeFilter(const float* filterCoefficients_, const uint filterLength_, const uint ratio_)
{
    float** polyFilterSet = new float*[ratio_];

    // calc the length of each subband
    uint subBandLength = filterLength_ / ratio_;
    
    // create a new set of polyfilters
    for (uint i = 0; i < ratio_; i++)
    {
        float* polyFilter = new float[subBandLength];
        polyFilterSet[i] = polyFilter;
    }

    // step through each filter
    for (uint m = 0; m < ratio_; ++m)
    {
        float* polyFilter = polyFilterSet[m];
        
        // step through each value of the filter
        for (uint n = 0; n < subBandLength; ++n)
        {
            // assign the next i.e. 2nd, 4th, or 8th value from the filter coefficient array
            // to the poly filter
            polyFilter[n] = filterCoefficients_[n * ratio_ + m];
        }
    }

    return polyFilterSet;
}


inline const float* getFilterCoefficients(const float sampleRate_, const uint filterLength_, const uint ratio_)
{
    // for now no other samplerate than 44100 or 48000 is accepted
    if (sampleRate_ != 44100.f && sampleRate_ != 48000.f) return nullptr;
    
    if (sampleRate_ == 44100.f)
    {
        if (filterLength_ == 64)
        {
            if (ratio_ == 2) return LPF_64_882;
            else if (ratio_ == 4) return LPF_64_1764;
            else if (ratio_ == 8) return LPF_64_3528;
            else return nullptr;
        }
        
        else if (filterLength_ == 128)
        {
            if (ratio_ == 2) return LPF_128_882;
            else if (ratio_ == 4) return LPF_128_1764;
            else if (ratio_ == 8) return LPF_128_3528;
            else return nullptr;
        }
        
        else if (filterLength_ == 256)
        {
            if (ratio_ == 2) return LPF_256_882;
            else if (ratio_ == 4) return LPF_256_1764;
            else if (ratio_ == 8) return LPF_256_3528;
            else return nullptr;
        }
        else return nullptr;
    }
    
    else if (sampleRate_ == 48000.f)
    {
        if (filterLength_ == 64)
        {
            if (ratio_ == 2) return LPF_64_96;
            else if (ratio_ == 4) return LPF_64_192;
            else if (ratio_ == 8) return LPF_64_384;
            else return nullptr;
        }
        
        else if (filterLength_ == 128)
        {
            if (ratio_ == 2) return LPF_128_96;
            else if (ratio_ == 4) return LPF_128_192;
            else if (ratio_ == 8) return LPF_128_384;
            else return nullptr;
        }
        
        else if (filterLength_ == 256)
        {
            if (ratio_ == 2) return LPF_256_96;
            else if (ratio_ == 4) return LPF_256_192;
            else if (ratio_ == 8) return LPF_256_384;
            else return nullptr;
        }
        else return nullptr;
    }
    
    else return nullptr;
}


// =======================================================================================
// MARK: - CONVOLVER
// =======================================================================================

void Convolver::setup(const uint filterLength_, const float *filterCoeffs_)
{
    if (filterLength_ % 4 != 0)
        engine_rt_error("Convolver Length needs to be a multiple of 4", __FILE__, __LINE__, true);
    
    filterLength = filterLength_;
    writePointer = 0;
    
    // copy the coefficient array to the internal vector
    for (uint n = 0; n < MAX_FILTER_LENGTH; ++n)
    {
        if (n < filterLength_) filterCoefficients[n] = filterCoeffs_[n];
        else filterCoefficients[n] = 0.f;
    }

    // fill buffer with 0s
    std::fill(buffer.begin(), buffer.end(), 0.f);
}


float Convolver::processAudioSample(const float input_)
{
    float output = 0.f;
    int readPointer = writePointer;
    float32x4_t sumVector = vdupq_n_f32(0.f);
    
    // write new sample to buffer
    buffer.at(writePointer) = input_;
    
    // increase the write Pointer
    if (++writePointer >= filterLength) writePointer -= filterLength;
    
    for (uint n = 0; n <= filterLength - 4; n += 4)
    {
        // read out the next 4 samples from the samplebuffer
        float copyArray[4];
        for (uint i = 0; i < 4; ++i)
        {
            copyArray[i] = buffer[readPointer--];
            if (readPointer < 0) readPointer += filterLength;
        }
        
        // store the values in simd vectors
        float32x4_t vector1 = vld1q_f32(filterCoefficients.data() + n);
        float32x4_t vector2 = vld1q_f32(copyArray);
        
        // process the sum of products
        sumVector = vmlaq_f32(sumVector, vector1, vector2);
    }
    
    // accumulate the 4 values of the simd vector horizontally to form the output value
    float32x2_t sumPair = vadd_f32(vget_low_f32(sumVector), vget_high_f32(sumVector));
    output = vget_lane_f32(vpadd_f32(sumPair, sumPair), 0);
    
    return output;
}


void ConvolverStereo::setup(const uint filterLength_, const float *filterCoeffs_)
{
    if (filterLength_ % 4 != 0)
        engine_rt_error("Convolver Length needs to be a multiple of 4", __FILE__, __LINE__, true);
    
    filterLength = filterLength_;
    writePointer = 0;

    // copy the coefficient array to the internal vector
    for (uint n = 0; n < MAX_FILTER_LENGTH; ++n)
    {
        if (n < filterLength_) filterCoefficients[n] = filterCoeffs_[n];
        else filterCoefficients[n] = 0.f;
    }

    // fill buffer with 0s
    std::fill(buffer.begin(), buffer.end(), vdup_n_f32(0.f));
}


float32x2_t ConvolverStereo::processAudioSamples(const float32x2_t input_)
{
    float32x2_t output = vdup_n_f32(0.f);
    int readPointer = writePointer;
    
    // write new sample to buffer
    buffer.at(writePointer) = input_;
    
    // increase the write Pointer
    if (++writePointer >= filterLength) writePointer -= filterLength;
    
    for (uint n = 0; n < filterLength; ++n)
    {
        output = vmla_n_f32(output, buffer[readPointer--], filterCoefficients[n]);
        if (readPointer < 0) readPointer += filterLength;
    }
    
    return output;
}


// =======================================================================================
// MARK: - INTERPOLATOR
// =======================================================================================

void Interpolator::setup(const float sampleRate_, const uint ratio_, const uint filterLength_)
{
    sampleRate = sampleRate_;
    filterLength = filterLength_;

    setInterpolationRatio(ratio_);
}


InterpolatorOutput Interpolator::interpolateAudio(const float input_)
{
    InterpolatorOutput output;
    int m = ratio - 1;
    
    for (uint n = 0; n < ratio; ++n)
        output.audioData[n] = gainCompensation * polyPhaseConvolver[m--].processAudioSample(input_);
    
    return output;
}


void Interpolator::setInterpolationRatio(const uint ratio_)
{
    ratio = ratio_;
    gainCompensation = (float)ratio;
    
    // retrieve the matching set of filter coefficients
    const float* filterCoefficients = getFilterCoefficients(sampleRate, filterLength, ratio);
    
    // check if we found valid values
    if (!filterCoefficients) 
        engine_rt_error("No machting FIR LPF found for these specifications!", __FILE__, __LINE__, true);

    // for polyphase processing:
    // decompose the filter coefficients to a set of new poly phase filter coefficients
    float** polyPhaseFilterCoefficients = decomposeFilter(filterCoefficients, filterLength, ratio);
    
    // check for valid values
    if (!polyPhaseFilterCoefficients)
        engine_rt_error("Decomposing of Oversampling Filter wasn't succesfull!", __FILE__, __LINE__, true);
    
    // calc the length of each poly phase filter (subband)
    uint subBandLength = filterLength / ratio;
    
    // setup each poly phase convolver
    for (uint i = 0; i < ratio; i++)
    {
        polyPhaseConvolver[i].setup(subBandLength, polyPhaseFilterCoefficients[i]);
        delete[] polyPhaseFilterCoefficients[i];
    }

    delete[] polyPhaseFilterCoefficients;
}


void InterpolatorStereo::setup(const float sampleRate_, const uint ratio_, const uint filterLength_)
{
    sampleRate = sampleRate_;
    filterLength = filterLength_;

    setInterpolationRatio(ratio_);
}


InterpolatorStereoOutput InterpolatorStereo::interpolateAudio(const float32x2_t input_)
{
    InterpolatorStereoOutput output;
    int m = ratio - 1;
    
    for (uint n = 0; n < ratio; ++n)
        output.audioData[n] = vmul_n_f32(polyPhaseConvolver[m--].processAudioSamples(input_), gainCompensation);
    
    return output;
}


void InterpolatorStereo::setInterpolationRatio(const uint ratio_)
{
    ratio = ratio_;
    gainCompensation = (float32_t)ratio;
    
    // retrieve the matching set of filter coefficients
    const float* filterCoefficients = getFilterCoefficients(sampleRate, filterLength, ratio);
    
    // check if we found valid values
    if (!filterCoefficients) engine_rt_error("No machting FIR LPF found for these specifications!", __FILE__, __LINE__, true);

    // for polyphase processing:
    // decompose the filter coefficients to a set of new poly phase filter coefficients
    float** polyPhaseFilterCoefficients = decomposeFilter(filterCoefficients, filterLength, ratio);
    
    // check for valid values
    if (!polyPhaseFilterCoefficients)
        engine_rt_error("Decomposing of Oversampling Filter wasn't succesfull!", __FILE__, __LINE__, true);
    
    // calc the length of each poly phase filter (subband)
    uint subBandLength = filterLength / ratio;
    
    // setup each poly phase convolver
    for (uint i = 0; i < ratio; i++)
    {
        polyPhaseConvolver[i].setup(subBandLength, polyPhaseFilterCoefficients[i]);
        delete[] polyPhaseFilterCoefficients[i];
    }

    delete[] polyPhaseFilterCoefficients;
}


// =======================================================================================
// MARK: - DECIMATOR
// =======================================================================================


void Decimator::setup(const float sampleRate_, const uint ratio_, const uint filterLength_)
{
    sampleRate = sampleRate_;
    filterLength = filterLength_;
    
    setDecimationRatio(ratio_);
}


float Decimator::decimateAudio(const DecimatorInput input_)
{
    float output = 0.f;
    
    for (uint n = 0; n < ratio; ++n)
        output += polyPhaseConvolver[n].processAudioSample(input_.audioData[n]);

    return output;
}


void Decimator::setDecimationRatio(const uint ratio_)
{
    ratio = ratio_;
    
    // retrieve the matching set of filter coefficients
    const float* filterCoefficients = getFilterCoefficients(sampleRate, filterLength, ratio);
    
    // check if we found valid values
    if (!filterCoefficients) engine_rt_error("No machting FIR LPF found for these specifications!", __FILE__, __LINE__, true);

    // for polyphase processing:
    // decompose the filter coefficients to a set of new poly phase filter coefficients
    float** polyPhaseFilterCoefficients = decomposeFilter(filterCoefficients, filterLength, ratio);
    
    // check for valid values
    if (!polyPhaseFilterCoefficients)
        engine_rt_error("Decomposing of Oversampling Filter wasn't succesfull!", __FILE__, __LINE__, true);
    
    // calc the length of each poly phase filter (subband)
    uint subBandLength = filterLength / ratio;
    
    // setup each poly phase convolver
    for (uint i = 0; i < ratio; i++)
    {
        polyPhaseConvolver[i].setup(subBandLength, polyPhaseFilterCoefficients[i]);
        delete[] polyPhaseFilterCoefficients[i];
    }

    delete[] polyPhaseFilterCoefficients;
}


void DecimatorStereo::setup(const float sampleRate_, const uint ratio_, const uint filterLength_)
{
    sampleRate = sampleRate_;
    filterLength = filterLength_;
    
    setDecimationRatio(ratio_);
}


float32x2_t DecimatorStereo::decimateAudio(const DecimatorStereoInput input_)
{
    float32x2_t output = vdup_n_f32(0.f);
    
    for (uint n = 0; n < ratio; ++n)
        output = vadd_f32(output, polyPhaseConvolver[n].processAudioSamples(input_.audioData[n]));

    return output;
}


void DecimatorStereo::setDecimationRatio(const uint ratio_)
{
    ratio = ratio_;
    
    // retrieve the matching set of filter coefficients
    const float* filterCoefficients = getFilterCoefficients(sampleRate, filterLength, ratio);
    
    // check if we found valid values
    if (!filterCoefficients) engine_rt_error("No machting FIR LPF found for these specifications!", __FILE__, __LINE__, true);

    // for polyphase processing:
    // decompose the filter coefficients to a set of new poly phase filter coefficients
    float** polyPhaseFilterCoefficients = decomposeFilter(filterCoefficients, filterLength, ratio);
    
    // check for valid values
    if (!polyPhaseFilterCoefficients)
        engine_rt_error("Decomposing of Oversampling Filter wasn't succesfull!", __FILE__, __LINE__, true);
    
    // calc the length of each poly phase filter (subband)
    uint subBandLength = filterLength / ratio;
    
    // setup each poly phase convolver
    for (uint i = 0; i < ratio; i++)
    {
        polyPhaseConvolver[i].setup(subBandLength, polyPhaseFilterCoefficients[i]);
        delete[] polyPhaseFilterCoefficients[i];
    }

    delete[] polyPhaseFilterCoefficients;
}
