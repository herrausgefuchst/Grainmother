#ifndef display_hpp
#define display_hpp

#include "menu.hpp"

#ifdef BELA_CONNECTED
#include <libraries/OscSender/OscSender.h>
#endif

// =======================================================================================
// MARK: - DISPLAY
// =======================================================================================

/**
 * @class Display
 * @brief Manages the display output across various platforms such as OLED and Console.
 *
 * The `Display` class is responsible for handling and updating display content that can be rendered on
 * multiple platforms:
 * 1. **OLED Display**: Managed via the `OscSender` class (specific to BELA).
 * 3. **Console**: The display content can be printed to the console if `CONSOLE_PRINT` is defined.
 *
 * The display automatically returns to the home screen after a set duration (`DISPLAY_AUTOHOMESCREEN`).
 * It also manages different states, such as `PERMANENT` and `TEMPORARY`, for how long content should be displayed.
 */
class Display : public AudioParameter::Listener
{
public:
    /**
     * @enum StateDuration
     * @brief Defines the duration type of a display state.
     * - `PERMANENT`: The display state remains until manually changed.
     * - `TEMPORARY`: The display state will revert after a certain time.
     */
    enum StateDuration { PERMANENT, TEMPORARY };
    
    /**
     * @struct DisplayCache
     * @brief Caches display content, allowing it to be formatted and retrieved as needed.
     *
     * The `DisplayCache` struct stores messages and values that are to be displayed. It allows adding
     * different types of data (e.g., float, int, string), creating rows for display, and clearing or printing
     * the cache. The cache can be used to update the display across different platforms.
     */
    struct DisplayCache
    {
        /**
         * @brief Stores a new message in the display cache.
         * @param message_ The message to be stored.
         */
        void newMessage(const String& message_);
        
        /** @brief Adds a float value to the display cache. */
        void add(const float value_);
        
        /** @brief Adds an int value to the display cache. */
        void add(const int value_);
        
        /** @brief Adds a string value to the display cache. */
        void add(const String& value_);
        
        /**
         * @brief Adds an array of strings to the display cache.
         * @param value_ Pointer to the array of strings.
         * @param size_ The number of strings in the array.
         */
        void add(String* value_, const size_t size_);
        
        /** @brief Creates rows from the cached data for display. */
        void createRows();
        
        /** @brief Clears the display cache. */
        void clear();
        
        /** @brief Prints the current cache content to the console. */
        void printToConsole();
        
        String message;               /**< The current message stored in the cache. */
        std::vector<String> strings;  /**< Cached strings to be displayed. */
        std::vector<float> floats;    /**< Cached float values to be displayed. */
        std::vector<int> ints;        /**< Cached int values to be displayed. */
        std::vector<String> rows;     /**< Formatted rows ready for display. */
    } displayCache;
    
    /** Constructor for the Display class. */
    Display() {}
    
    /** Destructor for the Display class. */
    ~Display() {}
    
    /**
     * @brief Sets up the display with the specified preset page.
     * @param presetPage_ The menu page to associate with the display.
     */
    void setup(Menu::Page* presetPage_);
                
    /**
     * @brief Updates the display content.
     * @return True if the display content was updated, otherwise false.
     */
    bool update();
    
    /**
     * @brief Handles display updates when a parameter is called.
     * @param param_ Pointer to the parameter that triggered the display update.
     */
    void parameterCalledDisplay(AudioParameter* param_) override;
    
    /**
     * @brief Handles display updates when the menu page changes.
     * @param page_ Pointer to the new menu page.
     */
    void menuPageChanged(Menu::Page* page_);
    
    /** @brief Resets the display counter, causing the display to stay active for the duration of `DISPLAY_AUTOHOMESCREEN`. */
    void refreshResetDisplayCounter() { resetDisplayCounter = DISPLAY_AUTOHOMESCREEN; }
    
    /** @brief Gets the current display state duration. @return The current state duration. */
    StateDuration getStateDuration() const { return stateDuration; }
    
    /** @brief Gets the current temporary parameter being displayed. @return Pointer to the temporary parameter. */
    AudioParameter* getTemporaryParameter() const { return tempParameter; }
    
private:
    /**
     * @brief Displays content related to a slide parameter.
     * @param param_ Pointer to the slide parameter.
     */
    void creatSlideParameterMessage(AudioParameter* param_);
    
    /**
     * @brief Displays content related to a choice parameter.
     * @param param_ Pointer to the choice parameter.
     */
    void createChoiceParameterMessage(AudioParameter* param_);
    
    /**
     * @brief Creates and displays a message related to a button parameter.
     * @param param_ Pointer to the button parameter.
     */
    void createButtonParameterMessage(AudioParameter* param_);

    /** @brief Creates and displays a message for the current preset. */
    void createPresetMessage();

    /**
     * @brief Creates and displays a message for the specified menu page.
     * @param page_ Pointer to the menu page.
     */
    void createMenuPageMessage(Menu::Page* page_);
    
#ifdef BELA_CONNECTED
    OscSender oscTransmitter; /**< Handles OSC transmission for OLED display on BELA. */
#endif
    StateDuration stateDuration = TEMPORARY;  /**< The duration type of the current display state. */
    
    AudioParameter* tempParameter = nullptr;  /**< Pointer to the parameter being temporarily displayed. */
    Menu::Page* presetPage = nullptr;         /**< Pointer to the current preset menu page. */
    
    unsigned int resetDisplayCounter = 0;     /**< Counter for determining when to reset to the home screen. */
    bool newMessageCache = false;             /**< Flag indicating if there is a new message in the cache. */
    
    static const uint DISPLAY_AUTOHOMESCREEN; /**< Duration before auto return to home screen, in frames. */
    static const uint DISPLAY_NUM_ROWS;       /**< Number of rows the display can show. */
};

#endif /* display_hpp */


// =======================================================================================
// MARK: - LED
// =======================================================================================

/**
 * @class LED
 * @brief Manages the LED state and behavior based on the connected parameter and UI interaction.
 *
 * The `LED` class controls an LED that reflects the state of a connected parameter. It supports
 * multiple states, including:
 * - VALUE: LED represents the value of the connected parameter.
 * - VALUEFOCUS: LED represents the value and blinks to indicate focus.
 * - ALERT: LED blinks a set number of times to indicate an alert (e.g., preset change), then reverts to the previous state.
 * - BLINKONCE: LED blinks once and then returns to the previous state.
 */
class LED : public UIElement::Listener, public AudioParameter::Listener
{
public:
    /**
     * @enum State
     * @brief Defines the possible states of the LED.
     * - VALUE: LED only represents the value of the connected parameter.
     * - VALUEFOCUS: LED represents the value of the connected parameter and blinks to indicate edit focus.
     * - ALERT: LED blinks a set number of times (NUM_BLINKS) and then returns to the previous state.
     * - BLINKONCE: LED blinks once and then returns to the previous state.
     */
    enum State { VALUE, VALUEFOCUS, ALERT, BLINKONCE };

    /** Constructor for the LED class. */
    LED() {}
    
    /** Destructor for the LED class. */
    ~LED() {}
    
    /**
     * @brief Sets up the LED with a specific ID.
     * @param id_ The ID to associate with this LED.
     */
    void setup(const uint index_, const String& id_);

    /**
     * @brief Sets the value that the LED represents.
     * @param value_ The value to display on the LED.
     */
    void setValue(const float value_) { value = value_; }
    
    /** @brief Triggers the LED to enter the ALERT state and blink a set number of times. */
    void alert();
    
    /** @brief Triggers the LED to blink once and then return to the previous state. */
    void blinkOnce();
    
    /** @brief sets a new state. */
    void setState(State state_);
    
    /** @brief Retrieves the current value represented by the LED. @return The current value. */
    float getValue();
    
    uint getIndex() { return index; }
    String getID() { return id; }
    
    /**
     * @brief Called when the connected parameter changes.
     * @param param_ Pointer to the changed parameter.
     */
    void parameterChanged(AudioParameter* param_) override;
    
    /** @brief Called when the connected Potentiometer catches a value. */
    void potCatchedValue() override;
    
private:
    uint index;
    String id; /**< The ID associated with this LED. */
    
    float value = 0.f;      /**< The current value represented by the LED. */
    float blinkValue = 0.f; /**< The value to display during a blink state. */
    
    State state = VALUE;    /**< The current state of the LED. */
    State lastState = state; /**< The previous state before a temporary state like ALERT or BLINKONCE. */
    
    unsigned int rateCounter; /**< Counter for managing the blinking rate. */
    unsigned int numBlinksCounter; /**< Counter for tracking the number of blinks in ALERT state. */
    
    /** Blinking rate multiplier, corresponds to the led framerate as defined in main.cpp / render.cpp. */
    static const uint BLINKING_RATE;
    /** Alert blinking rate multiplier. */
    static const uint ALERT_RATE;
    /** Number of blinks in ALERT state. */
    static const uint NUM_BLINKS;
};
