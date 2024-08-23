#ifndef display_hpp
#define display_hpp

#include "parameters.hpp"
#include "menu.hpp"
#include "functions.h"

#ifdef BELA_CONNECTED
#include <libraries/OscSender/OscSender.h>
#endif

// =======================================================================================
// MARK: - DISPLAY
// =======================================================================================

// display should be printable to:
// 1. OLED Display (via OscSender class of BELA)
// 2. GUI (via update() function, returns bool, if yes, vector of String with corresponding rows can be taken from DisplayCache
// 3. CONSOLE (via update() function, _withConsole indicates if the Displaycatch should be printed

static const int DISPLAY_AUTOHOMESCREEN = 48; // x * DISPLAY_FRAMERATE
static const int DISPLAY_NUM_ROWS = 10;

class Display : public AudioParameter::Listener
{
public:
    enum StateDuration { PERMANENT, TEMPORARY };
    
    struct DisplayCache
    {
        inline void newMessage(const String& message_);
        
        void add(const float value_);
        void add(const int value_);
        void add(const String& value_);
        void add(String* value_, const size_t size_);
        
        void createRows();
        inline void clear();
        void print();
        
        String message;
        std::vector<String> strings;
        std::vector<float> floats;
        std::vector<int> ints;
        std::vector<String> rows;

    } displayCache;
    
    Display() {}
    
    ~Display() {}
    
    void setup(Menu::Page* presetPage_);
                
    bool update();
    
    void parameterCalledDisplay(AudioParameter* param_) override;
    void menuPageChanged(Menu::Page* page_);
    
    void displaySlideParameter(AudioParameter* param_);
    void displayChoiceParameter(AudioParameter* param_);
    void displayButtonParameter(AudioParameter* param_);
    void displayMenuPage(Menu::Page* page_);
    void displayPreset();
    
    void refreshResetDisplayCounter() { resetDisplayCounter = DISPLAY_AUTOHOMESCREEN; }
    
    StateDuration getStateDuration() const { return stateDuration; }
    
    AudioParameter* getTemporaryParameter() const { return tempParameter; }
    
private:
#ifdef BELA_CONNECTED
    OscSender oscTransmitter;
#endif
    StateDuration stateDuration = TEMPORARY;
    
    AudioParameter* tempParameter = nullptr;
    
    unsigned int resetDisplayCounter = 0;
    bool newMessageCache = false;
    
    Menu::Page* presetPage = nullptr;
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
    void setup(const String& id_);

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
    
    /**
     * @brief Called when the connected parameter changes.
     * @param param_ Pointer to the changed parameter.
     */
    void parameterChanged(AudioParameter* param_) override;
    
    /** @brief Called when the connected Potentiometer catches a value. */
    void potCatchedValue() override;
    
private:
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
