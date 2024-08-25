#include "ReverbModules.h"

// initialize static member variables
unsigned int CombFilterStereo::writePointer = 0;
unsigned int AllpassFilterStereo::writePointer = 0;
unsigned int AllpassFilterMono::writePointer = 0;


// =======================================================================================
// MARK: - Tap Delay
// =======================================================================================


void TapDelayStereo::setup(const unsigned int& room_, const unsigned int& predelaySamples_, const float& size_, const unsigned int& blockSize_)
{
    blockSize = blockSize_;
    
    // set all buffer values to 0.f
    std::fill(buffer[0].begin(), buffer[0].end(), 0.f);
    std::fill(buffer[1].begin(), buffer[1].end(), 0.f);
    
    // calculate the tap delays
    recalculateTapDelays(room_, predelaySamples_, size_);
}


std::array<std::array<float32x4_t, NUM_TAPS/4>, 2>& TapDelayStereo::readTaps()
{
    // read out all taps by using linear interpolation, combine them in an array
    for (unsigned int ch = 0; ch < 2; ++ch)
    {
        // tap4 = the index of tap, n = the index of neon-vectors
        for (unsigned int tap4 = 0, n = 0; n < 3; tap4 += 4, ++n)
        {
            // calc second readpointers for interpolation
            int readHi0 = readPointer[ch][tap4] - 1;
            if (readHi0 < 0) readHi0 += bufferSize;

            int readHi1 = readPointer[ch][tap4 + 1] - 1;
            if (readHi1 < 0) readHi1 += bufferSize;

            int readHi2 = readPointer[ch][tap4 + 2] - 1;
            if (readHi2 < 0) readHi2 += bufferSize;

            int readHi3 = readPointer[ch][tap4 + 3] - 1;
            if (readHi3 < 0) readHi3 += bufferSize;
            
            // get the corresponding tap delay values out of the buffer
            float32x4_t tapsLo_v = {
                buffer[ch][readPointer[ch][tap4]],
                buffer[ch][readPointer[ch][tap4+1]],
                buffer[ch][readPointer[ch][tap4+2]],
                buffer[ch][readPointer[ch][tap4+3]]
            };
            
            float32x4_t tapsHi_v = {
                buffer[ch][readHi0],
                buffer[ch][readHi1],
                buffer[ch][readHi2],
                buffer[ch][readHi3]
            };
            
            // linear interpolation: difference between the two neighboured points
            float32x4_t diff = vsubq_f32(tapsHi_v, tapsLo_v);
            
            // linear interpolation: low value + fracement * difference
            taps[ch][n] = vmlaq_f32(tapsLo_v, frac[ch][n], diff);
        }
    }
    return taps;
}


float TapDelayStereo::getTapAtIndex(const unsigned int& channel_, const unsigned int& tap_) const
{
    switch (tap_ & 3)
    {
        case 0: return vgetq_lane_f32(taps[channel_][tap_ / 4], 0);
        case 1: return vgetq_lane_f32(taps[channel_][tap_ / 4], 1);
        case 2: return vgetq_lane_f32(taps[channel_][tap_ / 4], 2);
        case 3: return vgetq_lane_f32(taps[channel_][tap_ / 4], 3);
        default: return 0.0f; // Should never reach here
    }
}


void TapDelayStereo::writeBuffer(const StereoFloat& input_)
{
    // write new input
    buffer[0][writePointer] = input_.leftSample;
    buffer[1][writePointer] = input_.rightSample;

    // Increment and wrap write pointer
    writePointer = (writePointer + 1) & bufferSizeWrap;

    // Increment and wrap read pointers
    #pragma unroll
    for (unsigned int ch = 0; ch < 2; ++ch)
        #pragma unroll
        for (unsigned int tap = 0; tap < NUM_TAPS; ++tap)
            readPointer[ch][tap] = (readPointer[ch][tap] + 1) & bufferSizeWrap;
}


void TapDelayStereo::recalculateTapDelays(const unsigned int& room_, const float& predelaySamples_, const float& size_)
{
    // room input: 0, 1, 2,...
    // needs to be: 0, 2, 4,... for array-reading
    const unsigned int room = room_ * 2;
    
    // an array of interpolation fracments
    std::array<std::array<float, NUM_TAPS>, 2> fracments;
    
    for (unsigned int ch = 0; ch < 2; ++ch)
    {
        for (unsigned int tap = 0; tap < NUM_TAPS; ++tap)
        {
            // each tap delay is the given early delay * the size parameter + predelay
            float delaySamples = earliesDelaySamples[room+ch][tap] * size_ + predelaySamples_;
            
            // calculate floor value of the delay for interpolation
            unsigned int lo = floorf_neon(delaySamples);
            
            // calculate and store the fracment for interpolation
            fracments[ch][tap] = delaySamples - lo;
            
            // calculate and wrap read pointer (-1 because we read before we write!)
            readPointer[ch][tap] = writePointer - 1 - lo;
            if (readPointer[ch][tap] < 0) readPointer[ch][tap] += bufferSize;
        }
        
        // load array values into neon-vectors
        for (unsigned int tap4 = 0, n = 0; n < 3; tap4 += 4, ++n)
        {
            frac[ch][n] = vld1q_f32(fracments[ch].data() + tap4);
        }
    }
    
}


// =======================================================================================
// MARK: - AllpassFilter
// =======================================================================================


bool AllpassFilterStereo::setup(const float& feedbackGain_, const unsigned int& delaySamples_, const float& sampleRate_)
{
    // set feedback gain
    setFeedbackGain(feedbackGain_);

    // delay from ms to samples (+1 because buffer will be written after reading!)
    delaySamples = delaySamples_ + 1;
    
    // set all values in buffer to 0.f
    std::fill(buffer.begin(), buffer.end(), vdup_n_f32(0.f));

    // setup readPointer (-1 because read before write!)
    readPointerLo = bufferLength - delaySamples;
    if (readPointerLo < 0) readPointerLo += bufferLength;
                
    // set random start phase for lfo
    lfoPhase = ((rand() / (float)RAND_MAX) * TWOPI);
            
    return true;
}

void AllpassFilterStereo::updateLFO(const float& lfoIncrement_, const float& lfoDepth_)
{
    // increment lfo Phase
    lfoPhase += lfoIncrement_;
    if (lfoPhase >= TWOPI) lfoPhase -= TWOPI;
    
    // calc new delay, combination of fixed delay + lfo value
    float totalDelay = delaySamples + (lfoDepth_ * approximateSine(lfoPhase));
    
    // floor of total Delay
    int lowerBound = (int)totalDelay;
    
    // fracment for interpolation
    readPointerFrac = totalDelay - lowerBound;
    
    // calc integer readPointers around the read Point
    readPointerLo = writePointer - lowerBound;
    readPointerHi = readPointerLo - 1;
    
    // wrap Pointers if necessary
    if (readPointerLo < 0) readPointerLo += bufferLength;
    if (readPointerHi < 0) readPointerHi += bufferLength;
    
    // flag for efficiency
    interpolationNeeded = readPointerFrac != 0.f ? true : false;
}

void AllpassFilterStereo::processAudioSamples(float32x2_t& xn_)
{
    // delay output vn
    float32x2_t vn = readBuffer();
    
    // delay input wn = xn + vn * g
    float32x2_t wn = vmla_n_f32(xn_, vn, g);
    writeBuffer(wn);
    
    // output yn = vn - wn * g
    xn_ = vmls_n_f32(vn, wn, g);
}

void AllpassFilterStereo::setFeedbackGain(const float& feedbackGain_)
{
    // set and bound feedback gain
    g = feedbackGain_;
    boundValue(g, -0.99999f, 0.99999f);
}

float32x2_t AllpassFilterStereo::readBuffer()
{
    // read buffer with linear interpolation if needed
    float32x2_t vn = buffer[readPointerLo];
    if (interpolationNeeded) {
        float32x2_t diff = vsub_f32(buffer[readPointerHi], vn);
        vn = vmla_n_f32(vn, diff, readPointerFrac);
    }
    return vn;
}

void AllpassFilterStereo::writeBuffer(float32x2_t input_)
{
    // write new values in buffer
    buffer[writePointer] = input_;
    
    // increment buffer-pointers
    if (++readPointerLo >= bufferLength) readPointerLo = 0;
    if (++readPointerHi >= bufferLength) readPointerHi = 0;
}


// =======================================================================================
// MARK: - CombFilter
// =======================================================================================


bool CombFilterStereo::setup(const unsigned int& delaySamples_, const float& damping_, const float& sampleRate_, const bool& phaseShift_)
{
    // set Lowpass feedback gain
    setLowpassFeedbackGain(damping_);

    // delay from ms to samples
    delaySamples = delaySamples_;
    
    // set PhaseShifting flag
    phaseShift = phaseShift_;
    
    // set all values in buffer to 0.f
    std::fill(buffer.begin(), buffer.end(), vdup_n_f32(0.f));

    // setup readPointer (-1 because read before write!)
    readPointerLo = writePointer - 1 - delaySamples;
    if (readPointerLo < 0) readPointerLo += bufferLength;
                
    // set random start phase for lfo
    lfoPhase = ((rand() / (float)RAND_MAX) * TWOPI);
        
    return true;
}


void CombFilterStereo::updateLFO(const float& lfoIncrement_, const float& lfoDepth_)
{
    // increment lfo Phase
    lfoPhase += lfoIncrement_;
    if (lfoPhase >= TWOPI) lfoPhase -= TWOPI;
    
    // total delay is the fixed delay + lfo value + 1
    // +1 because reading buffer before writing!
    float totalDelay = 1 + delaySamples + (lfoDepth_ * approximateSine(lfoPhase));
    
    // calc fracment for linear interpolation
    readPointerFrac = totalDelay - floorf_neon(totalDelay);
    
    // calc new read pointer around the total delay
    readPointerLo = writePointer - floorf_neon(totalDelay);
    readPointerHi = readPointerLo - 1;
    if (readPointerLo < 0) readPointerLo += bufferLength;
    if (readPointerHi < 0) readPointerHi += bufferLength;
    
    // flag for efficiency purposes
    interpolationNeeded = readPointerFrac != 0.f ? true : false;
}


void CombFilterStereo::stopModulating()
{
    // recalculate the read pointer lower bound
    // the read pointer higher bound is not needed because we dont have to interpolate in this case
    readPointerLo = writePointer - 1 - delaySamples;
    if (readPointerLo < 0) readPointerLo += bufferLength;
    interpolationNeeded = false;
}


float32x2_t CombFilterStereo::processAudioSample(float32x2_t xn_)
{
    // read buffer
    float32x2_t yn = readBuffer();

    // write delay line inputs with feedback
    float32x2_t feedback = vmla_n_f32(vmul_n_f32(yn, b0), lowpassState, b1);
    writeBuffer(vadd_f32(xn_, feedback));

    // Save state for next sample
    lowpassState = yn;

    // shift phase if needed
    if (phaseShift) yn = vneg_f32(yn);
    
    return yn;
}


void CombFilterStereo::setFeedbackGain(const float& feedbackGain_)
{
    // set and bound value
    gComb = feedbackGain_;
    boundValue(gComb, 0.f, 0.99999f);
    
    // recalculate the filter coefficients
    b1 = gComb * gLP;
    b0 = gComb - b1;
}


void CombFilterStereo::setLowpassFeedbackGain(const float& lowpassFeedbackGain_)
{
    // set and bound value
    gLP = lowpassFeedbackGain_;
    boundValue(gLP, 0.f, 0.707f); // for some reason, lowpass returns to flat frequency response when values get higher than around 0.707

    // recalculate the filter coefficients
    b1 = gComb * gLP;
    b0 = gComb - b1;
}


float32x2_t CombFilterStereo::readBuffer()
{
    // read buffer
    float32x2_t yn = buffer[readPointerLo];
    
    // linear interpolation if needed
    if (interpolationNeeded) {
        float32x2_t diff = vsub_f32(buffer[readPointerHi], yn);
        yn = vmla_n_f32(yn, diff, readPointerFrac);
    }
    
    return yn;
}


void CombFilterStereo::writeBuffer(float32x2_t input_)
{
    // write new input values
    buffer[writePointer] = input_;
    
    // Increment buffer-pointers
    readPointerLo = (readPointerLo + 1) & bufferWrap;
    readPointerHi = (readPointerHi + 1) & bufferWrap;
}


void CombFilterDualStereo::update()
{
    // recatch the filter coefficients from corresponding members
    b0 = { filters[0].b0, filters[0].b0, filters[1].b0, filters[1].b0 };
    b1 = { filters[0].b1, filters[0].b1, filters[1].b1, filters[1].b1 };
}


float32x2_t CombFilterDualStereo::processAudioSampleInParallel(float32x2_t xn_)
{
    // create a input vector
    float32x4_t xn = vcombine_f32(xn_, xn_);
    
    // create the buffer output vector
    float32x4_t yn = vcombine_f32(filters[0].readBuffer(), filters[1].readBuffer());

    // write delay line inputs with feedback
    float32x4_t feedback = vmlaq_f32(vmulq_f32(yn, b0), lowpassState, b1);
    float32x4_t bufferInput = vaddq_f32(xn, feedback);
    
    // write buffers individually
    filters[0].writeBuffer(vget_low_f32(bufferInput));
    filters[1].writeBuffer(vget_high_f32(bufferInput));

    // Save state for next sample
    lowpassState = yn;
    
    // return the sum of both processed pairs, one of the outputs is being phase shifted
    return vadd_f32(vget_low_f32(yn), vneg_f32(vget_high_f32(yn)));
}
