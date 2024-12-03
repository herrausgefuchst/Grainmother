#pragma once

#include "../Functions.h"

class BitCrusher
{
public:
    float32x2_t processAudioSample(const float32x2_t input_);
    
    void updateAudioBlock();
    
    void setBitResolution(const float bitResolution_);
    void setSmoothing(const float smoothing_);
    
private:
    float32x2_t input = vdup_n_f32(0.f);
    
    float quantizationLevel_16Bit = (2.f / (powf_neon(2.f, 16.f) - 1.f));
    float quantizationSteps_16Bit = 1.f / quantizationLevel_16Bit;
    
    float bitResolution;
    float quantizationLevel;
    float quantizationSteps;
    float quantizationSmoothing;
    float quantizationSmoothingSlope;
    float smoothedQuantizationSteps = quantizationSteps_16Bit;
    float smoothedQuantizationLevel = quantizationLevel_16Bit;
};
