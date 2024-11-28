#pragma once

#include "ConstantVariables.h"

class BitCrusher
{
public:
    float32x2_t processAudioSample(const float32x2_t input_)
    {
        if (bitResolution == 16) return input_;

        // Compute the absolute value of the current audio sample
        input = input_;
        
        int32x2_t roundedInt = vcvt_s32_f32(vmul_n_f32(input_, smoothedQuantizationSteps));
        float32x2_t output = vcvt_f32_s32(roundedInt);
        output = vmul_n_f32(output, smoothedQuantizationLevel);
            
        // Apply bit crushing with smoothing to the audio
        return output;
    }
    
    
    void updateAudioBlock()
    {
        input = vabs_f32(input);
        float absoluteInput = vget_lane_f32(input, 0);
        
        // Calculate the dynamic quantization steps with smoothing
        // Lower values receive less bit crushing due to the smoothing slope
        smoothedQuantizationSteps = -quantizationSmoothingSlope * absoluteInput
                                          + quantizationSteps
                                          + quantizationSmoothingSlope;
            
        // Compute the quantization level for each step
        smoothedQuantizationLevel = 1.f / smoothedQuantizationSteps;
    }
    
    
    void setBitResolution(const float bitResolution_)
    {
        bitResolution = bitResolution_;
        quantizationLevel = (2.f / (powf_neon(2.f, bitResolution) - 1.f));
        quantizationSteps = 1.f / quantizationLevel;
    }
    
    void setSmoothing(const float smoothing_)
    {
        quantizationSmoothing = 0.00001f * smoothing_;
        quantizationSmoothingSlope = quantizationSmoothing * (quantizationSteps_16Bit - quantizationSteps);
    }
    
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
