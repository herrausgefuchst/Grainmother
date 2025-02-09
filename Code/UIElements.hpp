#ifndef uielements_hpp
#define uielements_hpp

#include "Functions.h"
#include "Helpers.hpp"

// =======================================================================================
// MARK: - UIELEMENT
// =======================================================================================

/**
 * @class UIElement
 * @brief A class representing a UI element with listeners for various events.
 */
class UIElement
{
public:
    /** @brief: default constructor */
    UIElement() {}
    
    /**
     * @brief Virtual destructor for UIElement.
     */
    virtual ~UIElement() {}

    /**
     * @class Listener
     * @brief A listener class for handling UIElement events.
     */
    class Listener
    {
    public:
        /**
         * @brief Virtual destructor for Listener.
         */
        virtual ~Listener() {}

        /**
         * @brief Called when a potentiometer changes.
         * @param uielement_ Pointer to the UIElement that triggered the event.
         */
        virtual void potChanged(UIElement* uielement_) {}
    
        /**
         * @brief Called when a button is clicked.
         * @param uielement_ Pointer to the UIElement that triggered the event.
         */
        virtual void buttonClicked(UIElement* uielement_) {}

        /**
         * @brief Called when a button is pressed.
         * @param uielement_ Pointer to the UIElement that triggered the event.
         */
        virtual void buttonPressed(UIElement* uielement_) {}

        /**
         * @brief Called when a button is released.
         * @param uielement_ Pointer to the UIElement that triggered the event.
         */
        virtual void buttonReleased(UIElement* uielement_) {}
    };

    /**
     * @brief Adds a listener to the UIElement.
     * @param listener_ Pointer to the Listener to add.
     */
    void addListener(Listener* listener_);

    /**
     * @brief Focuses a listener for the UIElement.
     *
     * clears the existing listener and swaps it with the passed in listener.
     *
     * @param listener_ Pointer to the Listener to focus.
     */
    void swapListener(Listener* listener_);

    /**
     * @brief Notifies all listeners of an event.
     * @param specifier_ An optional specifier for the event. Default is -1.
     */
    virtual void notifyListener(const int specifier_ = -1) = 0;

    /**
     * @brief sets up the midi output connectivity
     * @param ccIndex_ the cc index for the midi output
     * @param callbackFunction_ the function that is called to output midi data
     */
    void setupMIDI(const uint ccIndex_, std::function<void(uint, uint)> callbackFunction_);
    
    /** @brief sets a new midi cc output index */
    void setCCIndex(const uint ccIndex_) { ccIndex = ccIndex_; }
    
    // getters
    const int getIndex() const { return index; }
    const uint getCCIndex() const { return ccIndex; }
    const String getID() const { return id; }

protected:
    Listener* listener; /**< List of listeners attached to the UIElement */
    unsigned int index; /**< Index of the UIElement */
    String id; /**< Name of the UIElement */
    unsigned int ccIndex = 0; /**< midi cc Index for midi Output */
    std::function<void(uint, uint)> midiCallbackFunction = nullptr; /**< midi output callback function */
};


// =======================================================================================
// MARK: - POTENTIOMETER
// =======================================================================================

/**
 * @class Potentiometer
 * @brief A class representing a potentiometer UI element with various input sources.
 *
 * This class can read inputs from the GUI, Bela Analog Input, and MIDI.
 * 
 * One of these input sources is always in focus, and the class monitors
 * if another input source can take over (when incoming values are close
 * to the current value).
 *
 * Call `update()` periodically to ensure proper functioning.
 *
 * If a preset change occurs, you need to call `decouple()` to release the
 * current focus and set a new reference value.
 *
 * If a change is detected and the class decides to adopt it as the new value,
 * it notifies its listeners and updates the cache with the new value.
 *
 * Smoothing of analog input is achieved by comparing the difference between
 * the incoming value and the cached value against a predefined pot noise threshold.
 * If the change exceeds the noise threshold, the value is accepted.
 *
 */

static const float POT_CATCHING_TOLERANCE = 0.008f; /**< Tolerance for catching potentiometer */
static const float POT_MAX_VOLTAGE = 0.831f; /**< Maximum voltage for potentiometer */

class Potentiometer : public UIElement
{
public:
    /**
     * @enum ListenTo
     * @brief Enumeration for input sources the potentiometer can listen to.
     */
    enum class InputSource { GUI, ANALOG, MIDI, NONE };
    
    /** Constructor of Potentiometer */
    Potentiometer() {}
    
    /** Destructor for Potentiometer. */
    ~Potentiometer() {}

    /**
     * @brief sets up a Potentiometer with specified parameters.
     * @param index_ The index of the potentiometer.
     * @param id_ The name of the potentiometer.
     * @param guidefault_ Default value for the GUI.
     */
    void setup(const int index_, const String& id_, const float guidefault_ = 0.f);
    
    void setAnalogDefault(const float analogDefault_) { analogCache = analogDefault_; }
    
    void setInitialValue(const float value_) { current = value_; } 
    
    /**
     * @brief Updates the potentiometer with new GUI and analog values.
     * @param guivalue_ The new GUI value.
     * @param analogvalue_ The new analog value (optional).
     */
    void update(const float guivalue_, const float analogvalue_ = 0.f);
    
    /**
     * @brief Sets a new MIDI message value.
     * @param midivalue_ The new MIDI value.
     */
    void setNewMIDIMessage(const float midivalue_);
    
    /**
     * @brief Notifies all listeners of an event.
     * @param specifier_ An optional specifier for the event. Default is -1.
     */
    void notifyListener(const int specifier_ = -1) override;

    /**
     * @brief Sets a new value for the potentiometer.
     * @param value_ The new value to set.
     */
    void setValue(const float value_);
    
    /**
     * @brief sets a new current value and releases any input source the potentiometer is listening to.
     * @param newcurrent_ The new current value after decoupling.
     */
    void decouple(const float newcurrent_);
    
    static void setPotBevaviour(PotBehaviour potBehaviour_) { potBehaviour = potBehaviour_; }
    
    // getters
    float getValue() const { return current; }
    float getLastValue() const { return last; }
    
    std::function<void()> onTouch;
    std::function<void()> onCatch;
    std::function<void()> onChange;
    
private:
    float current = 0.f; /**< Current value of the potentiometer */
    float last = 0.f; /**< Last value of the potentiometer (i.e. used in ChoiceParameter)  */
    
    float guiCache; /**< Cached GUI value */
    float analogCache; /**< Cached analog value */
    int midiCache; /**< Cached midi value */
    
    InputSource inputFocus = InputSource::NONE; /**< Current listening focus */

public:
    static PotBehaviour potBehaviour;
};


// =======================================================================================
// MARK: - BUTTON
// =======================================================================================

/**
 * @class Button
 * @brief A class representing a button UI element with various states and actions.
 *
 * This class can read inputs from the GUI and Bela Analog Input.
 * All incoming button values should be of type `momentary`, where:
 * - 0 indicates the button is pushed
 * - 1 indicates the button is not pushed
 *
 * Call `update()` periodically to ensure proper functioning.
 * The analog value will be debounced.
 *
 * If a new value is detected, the class determines the type of action:
 * CLICK, LONGPRESS, or RELEASE, and notifies its listeners with the
 * corresponding message.
 *
 */
class Button : public UIElement
{
public:
    /**
     * @enum Phase
     * @brief Enumeration for the phase of the button (LOW or HIGH).
     */
    enum Phase { LOW = 0, HIGH = 1 };

    /**
     * @enum Action
     * @brief Enumeration for the possible actions of the button (CLICK, PRESS, RELEASE).
     */
    enum Action { CLICK, LONGPRESS, RELEASE };

    /** Constructor for Button */
    Button() : debouncer(DEBOUNCING_UNITS) {}
    
    /**
     * @brief Destructor for Button.
     */
    ~Button() {}
    
    /**
     * @brief Sets up a Button with the specified parameters.
     * @param index_ The index of the button.
     * @param id_ The name of the button.
     * @param guidefault_ Default GUI value for the button.
     * @param analogdefault_ Default analog value for the button.
     */
    void setup(const int index_, const String& id_, const Phase guidefault_ = HIGH, const Phase analogdefault_ = HIGH);
    
    /**
     * @brief Updates the button state with new GUI and analog values.
     * @param guivalue_ The new GUI value.
     * @param analogvalue_ The new analog value (optional).
     */
    void update(const unsigned int guivalue_ = HIGH, const unsigned int analogvalue_ = HIGH);
            
    /**
     * @brief Notifies all listeners of an event.
     * @param specifier_ An optional specifier for the event.
     */
    void notifyListener(const int specifier_) override;

    std::function<void()> onClick; /**< List of functions to call on button click */
    std::function<void()> onPress; /**< List of functions to call on button press */
    std::function<void()> onRelease; /**< List of functions to call on button release */
    
    /**
     * @brief Gets the current phase of the button.
     * @return The current phase.
     */
    Phase getPhase() const { return phase; }
    
    void clickButton() { notifyListener(0); }
    
    void pressButton() { notifyListener(1); }
    
    void releaseButton() { notifyListener(2); }
    
private:
    Phase phase = HIGH; /**< Current phase of the button */
    Phase analogCache = HIGH; /**< Cached analog value */
    Phase guiCache = HIGH; /**< Cached GUI value */
    
    /**
     * @enum State
     * @brief Enumeration for the internal state of the button.
     */
    enum State { JUST_CHANGED, AWAITING_LONGPRESS, NO_ACTION };

    State state = NO_ACTION; /**< Current state of the button */
    int stateCounter = 0; /**< Counter for the state machine */
    Action lastAction = CLICK; /**< Last action performed by the button */
    
    Debouncer debouncer; /**< Debouncer object for handling button debouncing */
    
    static const int DEBOUNCING_UNITS; /**< Number of calls for debouncing */
    static const int LONGPRESS_UNITS; /**< Number of calls for detecting a long press */
};

#endif /* uielements_hpp */
