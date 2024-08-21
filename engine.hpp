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

    TempoTapper* getTempoTapper() { return &tempoTapper; }
    
    Metronome* getMetronome() { return &metronome; }
        
private:
    std::array<std::unique_ptr<Effect>, NUM_EFFECTS> effects; /**< Array of unique pointers to effects */
    std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> programParameters; /**< Array of program parameter groups */
    AudioParameterGroup engineParameters; /**< Parameters specific to the audio engine */
    
    TempoTapper tempoTapper; /**< The tempo tapper instance */
    Metronome metronome; /**< The metronome instance */
    
    float sampleRate; /**< Sample rate */
    unsigned int blockSize; /**< Block size */
};

// =======================================================================================
// MARK: - USER INTERFACE
// =======================================================================================

class UserInterface : public Menu::Listener
{
public:
    void setup(AudioEngine* _engine);
    
    void processNonAudioTasks();
    
    void loadPresetFromJSON();
    
    void setEffectEditFocus (const bool _withNotification = true);
    
    void globalSettingChanged(Menu::Page* page_) override;
    
    void effectOrderChanged() override;
    
    void nudgeTempo (const int _direction);
        
private:
    void initializeListeners();
    
    AudioEngine* engine = nullptr;
    Menu menu;

public:
    Button button[NUM_BUTTONS];
    Potentiometer potentiometer[NUM_POTENTIOMETERS];
    LED led[NUM_LEDS];
    Display display;
};

#endif /* engine_hpp */
