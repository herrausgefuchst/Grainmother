#ifndef engine_hpp
#define engine_hpp

#include "functions.h"
#include "uielements.hpp"
#include "effects.hpp"
#include "parameters.hpp"
#include "menu.hpp"
#include "outputs.hpp"

// =======================================================================================
// MARK: - AUDIO ENGINE
// =======================================================================================

/**
 * @class AudioEngine
 * @brief A class that manages audio processing, effects, and parameters.
 */
class AudioEngine
{
public:
    /**
     * @enum Parameters
     * @brief Enumeration for the various parameters in the audio engine.
     */
    // TODO: what is this?
    enum Parameters { TEMPO, BYPASS, BEATREPEAT, GRANULATOR, DELAY, FXFOCUS };
    
    /**
     * @brief Constructor for AudioEngine.
     */
    AudioEngine();
    
    /**
     * @brief Destructor for AudioEngine.
     */
    ~AudioEngine() {}
    
    /**
     * @brief Sets up the audio engine with the specified sample rate and block size.
     * @param sampleRate_ The sample rate.
     * @param blockSize_ The block size.
     */
    void setup(const float sampleRate_, const unsigned int blockSize_);

    /**
     * @brief Processes a stereo input and returns the processed stereo output.
     * @param input_ The stereo input to process.
     * @return The processed stereo output.
     */
    StereoFloat processAudioSamples(const StereoFloat input_);

    /**
     * @brief update function, should be called blockwise
     */
    void updateAudioBlock();
    
    /**
     * @brief Retrieves an audio parameter by its ID.
     * @param parameterID_ The ID of the parameter to retrieve.
     * @return A pointer to the requested AudioParameter.
     */
    AudioParameter* getParameter(const String parameterID_);

    /**
     * @brief Retrieves an audio parameter by its group and index.
     * @param paramGroup_ The group index of the parameter.
     * @param paramIndex_ The index of the parameter within the group.
     * @return A pointer to the requested AudioParameter.
     */
    AudioParameter* getParameter(const unsigned int paramGroup_, const unsigned int paramIndex_);
    
    
    AudioParameter* getParameter(const String paramGroup_, const String paramID_);
    
    
    AudioParameter* getParameter(const String& paramGroup_, const uint paramIndex_);
    
    
    /**
     * @brief Gets the program parameters.
     * @return An array of pointers to the program parameter groups.
     */
    std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> getProgramParameters() { return programParameters; }
    
    /**
     * @brief Gets an effect by its index.
     * @param index_ The index of the effect to retrieve.
     * @return A pointer to the requested Effect.
     */
    Effect* getEffect(const unsigned int index_);
        
private:
    std::array<std::unique_ptr<Effect>, NUM_EFFECTS> effects; /**< Array of unique pointers to effects */
    std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> programParameters; /**< Array of program parameter groups */
    AudioParameterGroup engineParameters; /**< Parameters specific to the audio engine */
        
    float sampleRate; /**< Sample rate */
    unsigned int blockSize; /**< Block size */
};


// =======================================================================================
// MARK: - TempoTapper
// =======================================================================================

class TempoTapper
{
public:
    void setup(const float minBPM_, const float maxBPM_, const float sampleRate_);
    
    void process();
    
    bool tapTempo();
    
    float getTempoInBpm() const { return tempoBpm; }
    float getTempoInSeconds() const { return tempoSec; }
    float getTempoInMilliseconds() const { return tempoMsec; }
    uint getTempoInSamples() const { return tempoSamples; }
    
private:
    void calculateNewTempo();
    
    float sampleRate;
    
    float tempoBpm;
    float tempoSec;
    float tempoMsec;
    uint tempoSamples;
    
    uint maxBpmCounts;
    uint minBpmCounts;
    uint tapCounter = 0;

public:
    bool isCounting = false;
};


// =======================================================================================
// MARK: - METRONOME
// =======================================================================================

class Metronome : public AudioParameter::Listener
{
public:
    void setup(const float sampleRate_, const float defaultTempoBpm_);
    
    void process();
    
    void setTempoSamples(const float tempoSamples_);
    
    void parameterChanged(AudioParameter* param_) override;
    
    std::function<void()> onTic;

private:
    float sampleRate;
    uint counter = 0;
    uint tempoSamples = 0;
};


// =======================================================================================
// MARK: - USER INTERFACE
// =======================================================================================

class Metronome;

class UserInterface : public Menu::Listener
{
public:
    void setup(AudioEngine* engine_, const float sampleRate_);
    
    void processNonAudioTasks();
    
    void updateNonAudioTasks();
    
    void globalSettingChanged(Menu::Page* page_) override;
    
    void effectOrderChanged() override;
        
private:
    void initializeUIElements();
    void initializeMenu();
    void initializeListeners();
    
    void setEffectEditFocus();
    
    void nudgeUIParameter(const int direction_);
    
    void startScrollingUIParameter(const int direction_);
    void stopScrollingUIParameter();
    void setDefaultUIParameter();
    
    void alertLEDs(LED::State state_);
    
    AudioEngine* engine = nullptr;
    
    Menu menu;
    TempoTapper tempoTapper; /**< The tempo tapper instance */
    Metronome metronome;
    
    AudioParameter* scrollingParameter = nullptr;
    int scrollingDirection;

public:
    Button button[NUM_BUTTONS];
    Potentiometer potentiometer[NUM_POTENTIOMETERS];
    LED led[NUM_LEDS];
    Display display;
};


#endif /* engine_hpp */
