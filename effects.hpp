#ifndef effects_hpp
#define effects_hpp

#include "functions.h"
#include "parameters.hpp"
#include "Reverberation/Reverberation.h"

// MARK: - EFFECT
// ********************************************************************************

/**
 * @class Effect
 * @brief A base class representing an audio effect, with setup and processing capabilities.
 */
class Effect : public AudioParameter::Listener
{
public:
    /**
     * @brief Deleted default constructor to enforce parameterized construction.
     */
    Effect() = delete;

    /**
     * @brief Constructs an Effect with specified engine parameters and name.
     * @param engineParameters_ Pointer to the AudioParameterGroup containing engine parameters.
     * @param numParameters_ the number of parameters for this effect
     * @param name_ The name of the effect parameter group.
     * @param sampleRate_ The sample rate
     * @param blockSize_ The audio block size
     */
    Effect(AudioParameterGroup* engineParameters_, 
           const unsigned int numParameters_, const String& name_,
           const float sampleRate_, const unsigned int blockSize_);

    /**
     * @brief Virtual destructor for Effect.
     */
    virtual ~Effect() {}

    virtual void setup() {}
    
    /**
     * @brief Processes audio samples and returns the processed stereo output.
     * @param input_ The stereo input to process.
     * @return The processed stereo output.
     */
    virtual StereoFloat processAudioSamples(const StereoFloat input_, const uint sampleIndex_) = 0;

    /**
     * @brief Updates the audio block for the effect.
     */
    virtual void updateAudioBlock() = 0;

    
    void engage(bool engaged_) { engaged = engaged_; }
    
    
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
    
    String getId() const { return id; }

protected:
    String id;
    float sampleRate = 44100.f; /**< The sample rate for the effect */
    unsigned int blockSize = 128; /**< The block size for the effect */
    AudioParameterGroup parameters; /**< The group of parameters specific to this effect */
    AudioParameterGroup* engineParameters = nullptr; /**< Pointer to engine-wide parameters */
    
public:
    bool engaged = true;
};

// MARK: - BEATREPEAT
// ********************************************************************************

class Reverb : public Effect
{
public:    
    using Effect::Effect;
    
    ~Reverb() {}
    
    void setup() override;
    
    StereoFloat processAudioSamples(const StereoFloat input_, const uint sampleIndex_) override;
    
    void updateAudioBlock() override;
    
private:
    void initializeParameters();
    void initializeListeners();
    
    Reverberation::Reverb reverb;
};


// MARK: - GRANULATOR
// ********************************************************************************

class Granulator : public Effect
{
public:
    enum Parameters { GRAN1, GRAN2, GRAN3, GRAN4, GRAN5, GRAN6, GRAN7, GRAN8, GRAN9 };
    
    using Effect::Effect;
    
    void setup() override;
    
    ~Granulator() {}
    
    StereoFloat processAudioSamples(const StereoFloat input_, const uint sampleIndex_) override;
    
    void updateAudioBlock() override;
    
private:
    void initializeParameters();
    void initializeListeners();
};


// MARK: - DELAY
// ********************************************************************************

class Resonator : public Effect
{
public:
    enum Parameters { GRAN1, GRAN2, GRAN3, GRAN4, GRAN5, GRAN6, GRAN7, GRAN8, GRAN9 };
    
    using Effect::Effect;
    
    ~Resonator() {}
    
    StereoFloat processAudioSamples(const StereoFloat input_, const uint sampleIndex_) override;
    
    void updateAudioBlock() override;

private:
    void initializeParameters();
    void initializeListeners();
};

#endif /* effects_hpp */
