#ifndef effects_hpp
#define effects_hpp

#include "Functions.h"
#include "Parameters.hpp"
#include "Reverberation/Reverberation.h"
#include "Granulation/Granulation.h"
#include "RingModulation/RingModulator.h"

// =======================================================================================
// MARK: - EFFECT PROCESSOR
// =======================================================================================

/**
 * @class Effect
 * @brief A base class representing an audio effect processor, with setup and processing capabilities.
 *
 * This class wraps the actual effect class, handling dry/wet and mute processing and the parameter layout
 */
class EffectProcessor : public AudioParameter::Listener
{
public:
    /**
     * @enum ExecutionFlow
     * @brief Specifies how the effect is processed in relation to other effects.
     */
    enum ExecutionFlow { PARALLEL, SERIES };
    
    /** @brief default constructor */
    EffectProcessor() {}
    
    /**
     * @brief Constructs an Effect with specified engine parameters and name.
     * @param engineParameters_ Pointer to the AudioParameterGroup containing engine parameters.
     * @param numParameters_ the number of parameters for this effect
     * @param name_ The name of the effect processor = ID
     * @param sampleRate_ The sample rate
     * @param blockSize_ The audio block size
     */
    EffectProcessor(AudioParameterGroup* engineParameters_,
           const unsigned int numParameters_, const String& name_,
           const float sampleRate_, const unsigned int blockSize_);

    /** @brief Virtual destructor for Effect. */
    virtual ~EffectProcessor() {}

    /** @brief Prepares the effect processor for use. */
    virtual void setup() {}
    
    /**
     * @brief Processes audio samples and returns the processed stereo output.
     * @param input_ The stereo input sample to process.
     * @param sampleIndex_ The index of the current sample.
     * @return The processed stereo output sample.
     */
    virtual float32x2_t processAudioSamples(const float32x2_t input_, const uint sampleIndex_) = 0;

    /** @brief Updates the audio block for the effect. */
    virtual void updateAudioBlock() {}
    
    /** @brief Updates the gain ramps used for processing transitions. */
    void updateRamps();

    /**
     * @brief Engages or disengages the effect.
     * @param engaged_ True to engage the effect, false to bypass it.
     */
    void engage(bool engaged_);
    
    /**
     * @brief Sets the execution flow of the effect (parallel or series).
     * @param flow_ The execution flow mode.
     */
    void setExecutionFlow(const ExecutionFlow flow_);
    
    /**
     * @brief Sets the mix ratio between dry and wet signals.
     * @param mixGain_ The mix gain (0.0 for fully dry, 1.0 for fully wet).
     */
    void setMix(const float mixGain_);
    
    /** @brief Synchronizes the effect state, typically used to align with external changes. i.e. phase reset */
    virtual void synchronize() {}
    
    /**
     * @brief Callback for when a parameter is changed.
     * @param param_ The parameter that has been changed.
     */
    void parameterChanged(AudioParameter *param_) override;
    
    /**
     * @brief Gets the parameter group associated with the effect.
     * @return A pointer to the AudioParameterGroup for the effect.
     */
    AudioParameterGroup* getEffectParameterGroup() { return &parameters; }

    /**
     * @brief Retrieves a parameter by index from the effect's parameter group.
     * @param index_ The index of the parameter.
     * @return A pointer to the AudioParameter.
     */
    AudioParameter* getParameter(unsigned int index_) { return parameters.getParameter(index_); }

    /**
     * @brief Retrieves a parameter by ID from the effect's parameter group.
     * @param id_ The ID of the parameter.
     * @return A pointer to the AudioParameter.
     */
    AudioParameter* getParameter(String id_) { return parameters.getParameter(id_); }
    
    /**
     * @brief Gets the unique identifier (ID) of the effect.
     * @return The effect's ID as a string.
     */
    String getId() const { return id; }

protected:
    String id; /**< The unique identifier of the effect processor. */
        float sampleRate = 44100.f; /**< The sample rate for audio processing. */
        unsigned int blockSize = 128; /**< The block size for audio processing. */
        AudioParameterGroup parameters; /**< The group of parameters specific to this effect. */
        AudioParameterGroup* engineParameters = nullptr; /**< Pointer to engine-wide parameters. */
        
        ExecutionFlow isProcessedIn = PARALLEL; /**< Specifies the execution flow (parallel or series). */
        
        float dryGain = 0.f; /**< Gain applied to the dry signal (unprocessed input). */
        LinearRamp wetGain; /**< Linear ramp for the wet (processed) signal gain. */
        LinearRamp muteGain; /**< Linear ramp for muting transitions. */
        
        static const uint RAMP_BLOCKSIZE; /**< Block size used for ramp transitions. */
        static const uint RAMP_BLOCKSIZE_WRAP; /**< Wrapped block size for ramp transitions. */
};

// =======================================================================================
// MARK: - REVERB
// =======================================================================================

class ReverbProcessor : public EffectProcessor
{
public:    
    using EffectProcessor::EffectProcessor;
    
    ~ReverbProcessor() {}
    
    void setup() override;
    
    float32x2_t processAudioSamples(const float32x2_t input_, const uint sampleIndex_) override;
        
    void parameterChanged(AudioParameter *param_) override;
    
private:
    void initializeParameters();
    void initializeListeners();
    
    Reverberation::Reverb reverb;
};


// =======================================================================================
// MARK: - GRANULATOR
// =======================================================================================

class GranulatorProcessor : public EffectProcessor
{
public:
    using EffectProcessor::EffectProcessor;
    
    void setup() override;
    
    ~GranulatorProcessor() {}
    
    float32x2_t processAudioSamples(const float32x2_t input_, const uint sampleIndex_) override;
    
    void updateAudioBlock() override;
    
    void synchronize() override;
    
    void parameterChanged(AudioParameter *param_) override;
    
private:
    void initializeParameters();
    void initializeListeners();
    
    Granulation::Granulator granulator;
};


// =======================================================================================
// MARK: - RINGMODULATOR
// =======================================================================================

class RingModulatorProcessor : public EffectProcessor
{
public:
    using EffectProcessor::EffectProcessor;
    
    void setup() override;
    
    ~RingModulatorProcessor() {}
    
    float32x2_t processAudioSamples(const float32x2_t input_, const uint sampleIndex_) override;
    
    void updateAudioBlock() override;
    
    void synchronize() override;
    
    void parameterChanged(AudioParameter *param_) override;

private:
    void initializeParameters();
    void initializeListeners();
    
    RingModulation::RingModulator ringModulator;
};

#endif /* effects_hpp */
