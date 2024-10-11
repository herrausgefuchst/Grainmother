/**
 * @file Engine.h
 * @brief Header file that defines the core components of the audio processing system, including the audio engine, user interface, metronome, and tempo tapper.
 *
 * This header file contains the declarations of the main classes used in the audio processing system:
 * - `AudioEngine`: Manages the audio processing, including effects, parameters, and signal flow.
 * - `UserInterface`: Handles user interactions, displaying parameters, and managing UI elements like buttons, LEDs, and potentiometers.
 * - `TempoTapper`: Detects and calculates tempo based on user input (tapping).
 * - `Metronome`: Generates regular beats (tics) in sync with the audio tempo, with adjustable timing based on the tempo.
 *
 * Each of these classes plays a crucial role in the operation of the audio processing system, allowing for real-time audio manipulation, user control, and synchronization.
 */

// TODO: responiveness of potentiometers for catching is not very good...

#ifndef Engine_h
#define Engine_h

#include "functions.h"
#include "uielements.hpp"
#include "effects.hpp"
#include "parameters.hpp"
#include "menu.hpp"
#include "outputs.hpp"

// =======================================================================================
// MARK: - AUDIO ENGINE
// =======================================================================================

typedef std::function<StereoFloat(StereoFloat, uint)> ProcessFunctionPointer;

/**
 * @class AudioEngine
 * @brief Manages audio processing, effects, and parameters.
 *
 * The AudioEngine class handles the core audio processing tasks, including setting up effects, managing
 * audio parameters, processing audio samples, and handling bypass and ramping for effects. It is responsible
 * for coordinating the signal flow through various effects, updating parameter values, and ensuring that
 * audio processing occurs in a smooth and controlled manner.
 */
class AudioEngine
{
public:
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
     *
     * This function initializes the audio engine by setting the sample rate and block size, initializing
     * engine parameters, and allocating memory for the effect processors. It also sets up the effects,
     * adds them to the parameter group, and configures the global wet/dry ramp.
     *
     * @param sampleRate_ The sample rate.
     * @param blockSize_ The block size.
     */
    void setup(const float sampleRate_, const unsigned int blockSize_);

    /**
     * @brief Processes a stereo input and returns the processed stereo output.
     *
     * This function processes audio samples by applying the configured effects in a chain or parallel,
     * depending on the setup. It mixes the processed output with the original input using global wet/dry
     * controls, returning the final processed stereo output.
     *
     * @param input_ The stereo input to process.
     * @param sampleIndex_ The index of the sample within the current block.
     * @return The processed stereo output.
     */
    StereoFloat processAudioSamples(StereoFloat input_, uint sampleIndex_);
    
    void updateAudioBlock();
    
    /**
     * @brief Sets the processing order for the effects.
     *
     * This function determines the order in which effects are processed, based on internal logic and
     * configuration. It is crucial for managing the signal flow within the audio engine.
     */
    void setEffectOrder();
    
    /**
     * @brief Recalculates the parallel weighting for effects.
     *
     * This function calculates the weights for parallel processing of effects. It determines how much
     * each effect should contribute to the final output when multiple effects are processed in parallel.
     */
    void recalculateParallelWeighting();
    
    /**
     * @brief Sets the bypass state of the audio engine.
     *
     * This function enables or disables bypass for the entire audio engine by ramping the global wet
     * signal up or down. When bypassed, the effects are effectively muted, allowing the original input
     * to pass through unchanged.
     *
     * @param bypassed_ Whether to bypass the engine (true to bypass, false to process normally).
     */
    void setBypass(bool bypassed_);
    
    /**
     * @brief Updates the ramps for the wet/dry mix.
     *
     * This function processes the ramp for the global wet signal, adjusting the mix between the wet
     * and dry signals. It checks if the ramp has finished and updates the bypass state accordingly.
     */
    void updateRamps();
    
    
    void setGlobalMix();
    
    
    /**
     * @brief Retrieves an audio parameter by its ID.
     *
     * This function retrieves an AudioParameter based on a string identifier, allowing access to
     * parameters by their unique IDs.
     *
     * @param parameterID_ The ID of the parameter to retrieve.
     * @return A pointer to the requested AudioParameter.
     */
    AudioParameter* getParameter(const String parameterID_);

    /**
     * @brief Retrieves an audio parameter by its group and index.
     *
     * This function retrieves an AudioParameter based on its group and index within that group,
     * facilitating access to parameters that are organized into groups.
     *
     * @param paramGroup_ The group index of the parameter.
     * @param paramIndex_ The index of the parameter within the group.
     * @return A pointer to the requested AudioParameter.
     */
    AudioParameter* getParameter(const unsigned int paramGroup_, const unsigned int paramIndex_);
    
    /**
     * @brief Retrieves an audio parameter by its group name and parameter ID.
     *
     * This function retrieves an AudioParameter by the name of its group and the specific parameter ID,
     * enabling more intuitive access to parameters within named groups.
     *
     * @param paramGroup_ The name of the parameter group.
     * @param paramID_ The ID of the parameter within the group.
     * @return A pointer to the requested AudioParameter.
     */
    AudioParameter* getParameter(const String paramGroup_, const String paramID_);
    
    /**
     * @brief Retrieves an audio parameter by its group name and index.
     *
     * This function retrieves an AudioParameter by the name of its group and the index of the parameter
     * within that group, offering flexible access to parameters organized by name.
     *
     * @param paramGroup_ The name of the parameter group.
     * @param paramIndex_ The index of the parameter within the group.
     * @return A pointer to the requested AudioParameter.
     */
    AudioParameter* getParameter(const String& paramGroup_, const uint paramIndex_);
    
    /**
     * @brief Gets the program parameters.
     *
     * This function returns an array of pointers to the program parameter groups, providing access
     * to the entire set of parameters managed by the audio engine.
     *
     * @return An array of pointers to the program parameter groups.
     */
    std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> getProgramParameters() { return programParameters; }
    
    /**
     * @brief Gets an effect by its index.
     *
     * This function retrieves an effect processor based on its index, allowing direct access to specific
     * effects within the engine.
     *
     * @param index_ The index of the effect to retrieve.
     * @return A pointer to the requested EffectProcessor.
     */
    EffectProcessor* getEffect(const unsigned int index_);
        
private:
    /**
     * @brief Initializes the engine parameters.
     *
     * This function sets up the initial parameters specific to the audio engine, preparing them for use in
     * audio processing and effect management.
     */
    void initializeEngineParameters();
    
    EffectProcessor* effectProcessor[NUM_EFFECTS]; /**< Array of pointers to effect processors. */
    
    std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> programParameters; /**< Array of program parameter groups. */
    AudioParameterGroup engineParameters; /**< Parameters specific to the audio engine. */
    
    bool bypassed = false;  ///< Flag indicating whether the engine is currently bypassed.
    LinearRamp globalWet;  ///< Ramp for controlling the wet signal in the global bypass control.
    float globalWetCache;
    float globalDry;  ///< Multiplier for the dry signal in the global bypass control.
    
    ProcessFunctionPointer processFunction[3][3];  ///< Function pointers for processing audio through the effects.
    int processIndex[3][3];  ///< Indexes associated with the process functions.
    float parallelWeight[NUM_EFFECTS] = { 0.f, 0.f, 0.f };  ///< Weights for parallel effect processing.
    
    float sampleRate;  ///< Sample rate of the audio engine.
    unsigned int blockSize;  ///< Block size for audio processing.
    
    static const uint RAMP_BLOCKSIZE;  ///< Block size for the wet/dry ramp processing.
    static const uint RAMP_BLOCKSIZE_WRAP;  ///< Wrap size for the wet/dry ramp processing.
};


// =======================================================================================
// MARK: - TempoTapper
// =======================================================================================

/**
 * @class TempoTapper
 * @brief Handles tempo detection based on user taps, calculates tempo values, and provides access to these values in different units.
 *
 * The TempoTapper class is designed to detect tempo (BPM) by analyzing the timing of user taps. It calculates the tempo
 * in beats per minute, seconds, milliseconds, and samples, and allows the system to update and retrieve these values.
 * The class also handles the counting process to determine if a valid tempo has been tapped within a specified range.
 */
class TempoTapper
{
public:
    /**
     * @brief Sets up the tempo tapper with the given BPM range and sample rate.
     *
     * This function initializes the TempoTapper with the minimum and maximum BPM values and the audio system's sample rate.
     * It calculates the corresponding sample counts for the BPM range, which will be used to determine valid tempo taps.
     *
     * @param minBPM_ The minimum BPM value that can be detected.
     * @param maxBPM_ The maximum BPM value that can be detected.
     * @param sampleRate_ The sample rate of the audio system.
     */
    void setup(const float minBPM_, const float maxBPM_, const float sampleRate_);
    
    /**
     * @brief Processes the tap counter to manage tempo timing.
     *
     * This function increments the internal tap counter and stops counting if the counter exceeds the threshold
     * for the minimum BPM. It is typically called regularly to manage the counting process.
     */
    void process();
    
    /**
     * @brief Registers a tap and calculates a new tempo if a valid interval is detected.
     *
     * This function is called when a tap is detected. It checks if the time since the last tap falls within the valid BPM range.
     * If so, it calculates a new tempo based on the interval and restarts the counting process.
     *
     * @return True if a new valid tempo was detected and calculated, otherwise false.
     */
    bool tapTempo();
    
    /** Returns the current tempo in beats per minute (BPM). */
    float getTempoInBpm() const { return tempoBpm; }

    /** Returns the current tempo in seconds. */
    float getTempoInSeconds() const { return tempoSec; }

    /** Returns the current tempo in milliseconds. */
    float getTempoInMilliseconds() const { return tempoMsec; }

    /** Returns the current tempo in audio samples. */
    uint getTempoInSamples() const { return tempoSamples; }
    
private:
    /**
     * @brief Calculates new tempo values based on the tap counter.
     *
     * This function calculates the tempo in BPM, seconds, milliseconds, and samples based on the time interval
     * (in samples) between taps. It updates the internal state with these calculated values.
     */
    void calculateNewTempo();
    
    float sampleRate;  ///< The sample rate of the audio system, used for tempo calculations.
    
    float tempoBpm;  ///< The current tempo in beats per minute (BPM).
    float tempoSec;  ///< The current tempo in seconds.
    float tempoMsec;  ///< The current tempo in milliseconds.
    uint tempoSamples;  ///< The current tempo in audio samples.
    
    uint maxBpmCounts;  ///< The maximum number of samples corresponding to the maximum BPM.
    uint minBpmCounts;  ///< The minimum number of samples corresponding to the minimum BPM.
    uint tapCounter = 0;  ///< Counter to track the number of samples between taps.

public:
    bool isCounting = false;  ///< Flag indicating whether the tap counting process is currently active.
};


// =======================================================================================
// MARK: - METRONOME
// =======================================================================================

/**
 * @class Metronome
 * @brief A metronome that synchronizes with audio parameters, triggering events at regular intervals based on tempo.
 *
 * The Metronome class is designed to generate regular beats (or "tics") based on a specified tempo. It operates by
 * counting audio samples and triggering a tic when the count matches the number of samples per beat. The class can
 * respond to tempo changes and can be integrated with audio parameters via the AudioParameter::Listener interface.
 */
class Metronome : public AudioParameter::Listener
{
public:
    /**
     * @brief Initializes the metronome with a given sample rate and default tempo.
     * @param sampleRate_ The sample rate of the audio system.
     * @param defaultTempoBpm_ The default tempo in beats per minute (BPM).
     */
    void setup(const float sampleRate_, const float defaultTempoBpm_);
    
    /**
     * @brief Processes the metronome, triggering tics based on the current tempo.
     *
     * This function should be called regularly (e.g., on each audio processing block) to advance the metronome's
     * internal counter and trigger a tic when the counter reaches the required number of samples per beat.
     */
    void process();
    
    /**
     * @brief Sets the tempo in terms of the number of samples per beat.
     * @param tempoSamples_ The number of samples per beat, derived from the tempo and sample rate.
     */
    void setTempoSamples(const uint tempoSamples_);
    
    /**
     * @brief Responds to changes in the associated audio parameter, updating the metronome's tempo.
     * @param param_ A pointer to the AudioParameter that has changed.
     *
     * This function overrides the AudioParameter::Listener method and updates the tempo in response to changes
     * in the associated audio parameter.
     */
    void parameterChanged(AudioParameter* param_) override;
    
    /**
     * @brief A callback function triggered on each metronome tic.
     *
     * This function is called whenever the metronome reaches the end of its counting cycle (i.e., at the start of a new beat).
     */
    std::function<void()> onTic;

private:
    float sampleRate;  ///< The sample rate of the audio system, used to calculate tempo in samples per beat.
    uint counter = 0;  ///< The internal counter tracking the number of samples until the next tic.
    uint tempoSamples = 0;  ///< The number of samples per beat, based on the current tempo.
};


// =======================================================================================
// MARK: - USER INTERFACE
// =======================================================================================

/**
 * @class UserInterface
 * @brief Manages the user interface components and their interactions with the audio engine.
 *
 * The UserInterface class is responsible for setting up and managing all user-facing controls, displays,
 * and indicators. It links these UI elements to the underlying audio engine parameters, ensuring that user
 * actions are correctly mapped to the corresponding audio processing functions. The class handles tasks
 * such as initializing UI components, managing button and potentiometer actions, updating display content,
 * and responding to changes in global settings, presets, and effect orders.
 *
 * @note The UserInterface class performs the following key tasks:
 * - Initializes UI elements such as buttons, potentiometers, LEDs, and the display.
 * - Sets up the menu system and links it to the audio engine's parameters.
 * - Manages the interaction between UI components and the audio engine, including the handling of
 *   non-audio tasks like tempo tapping and metronome processing.
 * - Provides methods for handling specific UI events, such as parameter nudging, scrolling, and resetting
 *   to default values.
 * - Responds to changes in global settings, presets, and effect orders, ensuring the UI reflects these changes.
 */
class UserInterface
{
public:
    /**
     * @brief Sets up the User Interface by initializing all components and linking them to the AudioEngine.
     *
     * This function initializes various elements of the user interface, including buttons, potentiometers, LEDs,
     * the menu, and listeners. It also configures the display, tempo tapper, and metronome. The function
     * establishes connections between UI elements and their corresponding audio parameters in the
     * AudioEngine, ensuring that the interface correctly reflects the current state of the audio processing
     * engine at startup.
     *
     * @param engine_ A pointer to the AudioEngine instance. This instance will be used to link UI elements to
     * their corresponding audio parameters.
     * @param sampleRate_ The sample rate at which the audio engine operates. This is used to configure
     * components like the tempo tapper and metronome.
     *
     * @note The function performs the following tasks:
     * - Saves the reference to the AudioEngine.
     * - Initializes UI elements such as buttons, potentiometers, and LEDs.
     * - Connects LEDs to their corresponding audio parameters to ensure they reflect parameter values.
     * - Initializes the menu structure, including setting up the page architecture and loading the first preset.
     * - Configures listeners for interactions between the UI, parameters, outputs, and the AudioEngine.
     * - Sets up the display, including establishing an OSC connection and setting the initial page to be displayed.
     * - Configures the tempo tapper with the appropriate minimum and maximum tempo values.
     * - Sets up the metronome based on the current tempo.
     * - Alerts the user by flashing LEDs upon successful setup completion.
     */
    void setup(AudioEngine* engine_, const float sampleRate_);
    
    /**
     * @brief Processes non-audio tasks related to the tempo tapper and metronome.
     *
     * This function handles the processing of non-audio tasks, specifically those related to the tempo tapper
     * and metronome. It checks if the tempo tapper is active and processes it if necessary, and it also processes
     * the metronome to ensure its timing functions are maintained.
     */
    void processNonAudioTasks();
    
    /**
     * @brief Updates non-audio tasks related to menu and UI parameter scrolling.
     *
     * This function handles the continuous updating of non-audio tasks within the user interface. It checks if the
     * menu is in scrolling mode and updates the scrolling accordingly. Additionally, if a UI parameter is in scrolling
     * mode, it scrolls the parameter value, ensuring that the corresponding potentiometer is decoupled and refreshed
     * to reflect the new normalized value.
     */
    void updateNonAudioTasks();
    
    /**
     * @brief Handles changes to global settings and updates the user interface accordingly.
     *
     * This function is called when a global setting is changed within the menu. It checks the specific setting
     * that was modified and updates relevant parameters or behaviors in the user interface.
     *
     * @param page_ A pointer to the `Menu::Page` object that represents the menu page where the setting was changed.
     */
    void globalSettingChanged(Menu::Page* page_);
    
    /**
     * @brief Handles actions that occur when a preset is changed.
     *
     * This function is called whenever a preset change occurs. It temporarily disables the automatic
     * updating of tempo-related parameters to ensure that the effect parameters remain as specified by
     * the new preset. Additionally, it triggers an alert on all LEDs to indicate that the preset has changed.
     */
    void presetChanged();
    
    /**
     * @brief Responds to changes in the effect order by updating the audio engine and signaling the user interface.
     *
     * This function is called when the effect order is changed. It triggers the audio engine to update its
     * processing order of effects and then signals the user interface by making all LEDs blink once,
     * indicating that the effect order has been successfully updated.
     */
    void effectOrderChanged();
    
private:
    /**
     * @brief Initializes all UI elements including buttons, potentiometers, and LEDs.
     *
     * This function sets up the UI elements by initializing each button, potentiometer,
     * and LED with their corresponding identifiers and names. This is a crucial step
     * in preparing the user interface for interaction, ensuring that each control element
     * is correctly mapped to its intended function.
     */
    void initializeUIElements();
    
    
    /**
     * @brief Initializes the menu system by creating parameter pages and configuring the menu with the complete set of parameters.
     *
     * This function sets up the menu for the user interface, creating various parameter pages that users can navigate and modify.
     * It adds specific parameter pages related to effect order, tempo settings, and reverb settings, ensuring that the menu
     * has access to the correct parameters from the AudioEngine. After creating these pages, the menu is configured with
     * the full set of program parameters from the AudioEngine.
     */
    void initializeMenu();
    
    
    /**
     * @brief Initializes the listeners for buttons, potentiometers, parameters, menu actions, and other components within the user interface.
     *
     * This function connects various UI elements to their corresponding actions and parameters. It sets up the behavior of buttons,
     * potentiometers, and other interface components by linking them to the appropriate audio engine parameters, menu actions,
     * and display updates. It ensures that the UI responds correctly to user inputs and parameter changes, facilitating interaction
     * between the user and the underlying audio processing.
     */
    void initializeListeners();
    
    /**
     * @brief Sets the focus of the user interface to the currently selected effect, updating the associated controls and indicators.
     *
     * This function retrieves the currently focused effect based on the "effect_edit_focus" parameter and updates the user interface
     * elements accordingly. It reassigns potentiometer controls to the parameters of the focused effect, updates the action button
     * to control the appropriate parameter, and adjusts the LED indicators to reflect which effect is currently being edited.
     */
    void setEffectEditFocus();
    
    
    void mixPotentiometerChanged();
    
    
    /**
     * @brief Evaluates whether a new tempo has been detected based on user input and updates the tempo parameter if necessary.
     *
     * This function is triggered when a tap occurs, typically through a button press. It uses the `tempoTapper` to check if a new
     * tempo has been captured. If a new tempo is detected, the tempo parameter in the audio engine is updated with the new BPM value.
     */
    void evaluateNewTempo();

    /**
     * @brief Updates tempo-related parameters based on the current tempo and user-selected settings.
     *
     * This function adjusts the parameters of effects that are related to tempo, such as predelay for reverb
     * and grain length for the granulator, according to the current tempo in BPM. It respects the user-defined
     * setting that determines whether the changes apply to the current effect or all effects. If a new preset
     * has been loaded, this function will not execute to avoid overriding preset settings.
     */
    void setTempoRelatedParameters();

    /**
     * @brief Nudges the value of the currently displayed UI parameter in a specified direction.
     *
     * This function is used to increment or decrement the value of the parameter currently shown on the display
     * when the display is in its TEMPORARY state. It temporarily suspends normal menu operations, updates the
     * parameter's value, and ensures the corresponding potentiometer reflects the new value.
     *
     * @param direction_ The direction in which to nudge the parameter's value. Positive values increment the parameter,
     * and negative values decrement it.
     */
    void nudgeUIParameter(const int direction_);

    /**
     * @brief Initiates the scrolling of the currently displayed UI parameter in a specified direction.
     *
     * This function begins the process of continuously adjusting the value of the parameter currently shown
     * on the display when the display is in its TEMPORARY state. It suspends normal menu operations, saves
     * the parameter and scrolling direction, and prepares the system to handle the scrolling action, which
     * will occur in the `updateNonAudioTasks()` function.
     *
     * @param direction_ The direction in which to scroll the parameter's value. Positive values increase the parameter,
     * and negative values decrease it.
     */
    void startScrollingUIParameter(const int direction_);

    /**
     * @brief Stops the scrolling of the currently displayed UI parameter.
     *
     * This function is called when the Up or Down button is released after a long press, ending the
     * scrolling action of the parameter that was being adjusted. It sets the `scrollingParameter`
     * to `nullptr`, which acts as a flag to indicate that no parameter should be scrolled in the
     * `updateNonAudioTasks()` function.
     */
    void stopScrollingUIParameter();

    /**
     * @brief Resets the currently displayed UI parameter to its default value.
     *
     * This function resets the value of the parameter currently shown on the display to its default setting,
     * provided the display is in its TEMPORARY state. It temporarily suspends normal menu operations,
     * updates the parameter, and ensures the corresponding potentiometer reflects the new default value.
     */
    void setUIParameterToDefault();
    
    /**
     * @brief Displays the parameter associated with the touched potentiometer on the UI.
     *
     * This function is called whenever a potentiometer is touched, instructing the display to show the associated parameter.
     * During the initial startup phase, the function bypasses normal behavior to allow for potentiometer cache initialization.
     * After initialization, it retrieves the parameter corresponding to the touched potentiometer and updates the display
     * to reflect this parameter.
     *
     * @param paramIndex_ The index of the touched potentiometer, used to retrieve the corresponding parameter.
     */
    void displayTouchedParameter(uint paramIndex_);
    
    /**
     * @brief Triggers a specified alert state on all LEDs.
     *
     * This function activates a specified alert behavior on all LEDs. Depending on the provided state,
     * it either triggers the LEDs to enter an alert mode where they perform a defined alert action,
     * or it causes them to blink once.
     *
     * @param state_ The desired state for the LEDs, either `LED::State::ALERT` for an alert mode or
     * `LED::State::BLINKONCE` for a single blink.
     */
    void alertLEDs(LED::State state_);
    
    AudioEngine* engine = nullptr;  ///< Pointer to the AudioEngine instance, used to link UI components to audio parameters.

    TempoTapper tempoTapper;  ///< Instance of the tempo tapper, manages tempo detection and tapping functionality.
    Metronome metronome;  ///< Instance of the metronome, handles metronome timing and synchronization tasks.
    bool settingTempoIsOnHold = false;  ///< Flag to temporarily disable tempo-related parameter updates when a preset is loaded.
    
    AudioParameter* scrollingParameter = nullptr;  ///< Pointer to the currently scrolling parameter in the UI.
    int scrollingDirection;  ///< Direction in which the parameter is being scrolled (-1 for down, 1 for up).

public:
    Button button[NUM_BUTTONS];  ///< Array of buttons in the user interface, each mapped to a specific function.
    Potentiometer potentiometer[NUM_POTENTIOMETERS];  ///< Array of potentiometers in the user interface, each controlling a specific parameter.
    LED led[NUM_LEDS];  ///< Array of LEDs in the user interface, used to indicate status and changes in parameters.
    Display display;  ///< The display used in the user interface, shows parameter values and menu information.
    Menu menu;  ///< The menu system used in the user interface, allows navigation and parameter selection.
};


#endif /* Engine_h */
