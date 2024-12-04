#pragma once

#include "../Functions.h"

/**
 * @class BitCrusher
 * @brief A class for implementing a bitcrusher effect with dynamic smoothing.
 *
 * The BitCrusher class reduces the bit depth of an audio signal, creating
 * a lo-fi, distorted effect. It supports dynamic smoothing, which adjusts
 * the amount of bit crushing based on the amplitude of the input signal.
 * This feature allows for a more musical and responsive bitcrusher effect.
 */
class BitCrusher
{
public:
    /**
     * @brief Processes a single stereo audio sample using the bitcrusher effect.
     *
     * This function applies bit depth reduction to the input stereo sample.
     * If the bit resolution is set to 16 bits, the input is returned unchanged.
     *
     * @param input_ The stereo input sample to be processed.
     * @return The bitcrushed stereo output sample.
     */
    float32x2_t processAudioSample(const float32x2_t input_);
    
    /**
     * @brief Updates the internal state of the bitcrusher for the current audio block.
     *
     * This method dynamically calculates the quantization steps and levels
     * based on the absolute value of the input signal. It ensures smoother
     * transitions between bit depths during processing.
     */
    void updateAudioBlock();
    
    /**
     * @brief Sets the bit resolution for the bitcrusher effect.
     *
     * The bit resolution determines the number of quantization levels applied
     * to the input signal.
     *
     * @param bitResolution_ The desired bit resolution (e.g., 8, 12, 16).
     */
    void setBitResolution(const float bitResolution_);
    
    /**
     * @brief Sets the smoothing factor for dynamic bit crushing.
     *
     * The smoothing factor determines how much the bitcrusher adjusts the
     * quantization steps based on the input signal's amplitude.
     *
     * @param smoothing_ The smoothing factor (0...100)
     */
    void setSmoothing(const float smoothing_);
    
private:
    float32x2_t input = vdup_n_f32(0.f); ///< Current stereo input sample.

    float quantizationLevel_16Bit = (2.f / (powf_neon(2.f, 16.f) - 1.f)); ///< Quantization level for 16-bit resolution.
    float quantizationSteps_16Bit = 1.f / quantizationLevel_16Bit; ///< Quantization steps for 16-bit resolution.

    float bitResolution; ///< Current bit resolution.
    float quantizationLevel; ///< Current quantization level.
    float quantizationSteps; ///< Current quantization steps.
    float quantizationSmoothing; ///< Smoothing factor for dynamic quantization.
    float quantizationSmoothingSlope; ///< Slope of smoothing based on input amplitude.
    float smoothedQuantizationSteps = quantizationSteps_16Bit; ///< Smoothed quantization steps.
    float smoothedQuantizationLevel = quantizationLevel_16Bit; ///< Smoothed quantization level.
};
