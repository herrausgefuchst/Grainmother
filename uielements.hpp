#ifndef uielements_hpp
#define uielements_hpp

#include "globals.h"
#include "functions.h"
#include "helpers.hpp"

static const int NUM_BUTTONS = 10;
static const int NUM_POTENTIOMETERS = 8;

// MARK: - UIELEMENT
// ********************************************************************************

/**
 * @class UIElement
 * @brief A class representing a UI element with listeners for various events.
 */
class UIElement
{
public:
    UIElement() = delete;
    
    /**
     * @brief Constructs a UIElement with the specified index and name.
     * @param index_ The index of the UI element, has to be unique!
     * @param name_ The name of the UI element, has to be unique!
     */
    UIElement(const int index_, const String name_);

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
     * clears the existing list of listeners and adds only the one passed listener to the list.
     *
     * @param listener_ Pointer to the Listener to focus.
     */
    void focusListener(Listener* listener_);

    /**
     * @brief Notifies all listeners of an event.
     * @param specifier_ An optional specifier for the event. Default is -1.
     */
    virtual void notifyListeners(const int specifier_ = -1) = 0;

    // getters
    const int getIndex() const { return index; }
    const String getName() const { return name; }

protected:
    std::vector<Listener*> listeners; /**< List of listeners attached to the UIElement */
    const int index; /**< Index of the UIElement */
    const String name; /**< Name of the UIElement */
};


// MARK: - POTENTIOMETER
// ********************************************************************************

static const unsigned int POT_MOVINGAVG_SIZE = 8;
static const float INV_POT_MOVINGAVG_SIZE = 1.f / (float)POT_MOVINGAVG_SIZE;

/**
 * @class Potentiometer
 * @brief A class representing a potentiometer UI element with various input sources.
 */
class Potentiometer : public UIElement
{
public:
    /**
     * @enum ListenTo
     * @brief Enumeration for input sources the potentiometer can listen to.
     */
    enum class InputSource { GUI, ANALOG, MIDI, NONE };
    
    /**
     * @brief Constructs a Potentiometer with specified parameters.
     * @param index_ The index of the potentiometer.
     * @param name_ The name of the potentiometer.
     * @param guidefault_ Default value for the GUI.
     * @param analogdefault_ Default value for analog input.
     */
    Potentiometer(const int index_, const String name_, const float guidefault_ = 0.f, const float analogdefault_ = 0.f);
    
    /** Destructor for Potentiometer. */
    ~Potentiometer() {}

    /**
     * @brief sets dafault  values for input sources
     * @param guidefault_ Default value for the GUI.
     * @param analogdefault_ Default value for analog input.
     */
    void setDefaults(const float guidefault_ = 0.f, const float analogdefault_ = 0.f);
    
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
    void notifyListeners(const int specifier_ = -1) override;

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
    
    //TODO: make this work, has to be called when changes appear
    static void setPotBevaviour(PotBehaviour potBehaviour_) { potBehaviour = potBehaviour_; }
    
    // getters
    float getValue() const { return current; }
    float getLastValue() const { return last; }
    
    std::vector<std::function<void()>> onChange; /**< List of functions to call on value change */
    
private:
    float current = 0.f; /**< Current value of the potentiometer */
    float last = 0.f; /**< Last value of the potentiometer */
    
    float guiCache; /**< Cached GUI value */
    float analogCache; /**< Cached analog value */
    float analogAverage; /**< Average of analog values */
    std::array<float, POT_MOVINGAVG_SIZE> analogHistory; /**< History of analog values */
    unsigned int analogPtr = 0; /**< Pointer to the current position in analog history */
    
    InputSource inputFocus = InputSource::NONE; /**< Current listening focus */
    
    static PotBehaviour potBehaviour;
};


// MARK: - BUTTON
// ********************************************************************************

/**
 * @class Button
 * @brief A class representing a button UI element with various states and actions.
 *
 * all incoming Buttons should be of Type Momentary
 * if Button receives new values in update():
 * - case guivalue: need to determine wether its a click, press or release message
 * - case analog: debounce first, then determine wether its a click, press or release message
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

    /**
     * @enum ID
     * @brief Enumeration for the button IDs.
     */
    enum ID { FX1, FX2, FX3, ACTION, BYPASS, TEMPO, UP, DOWN, EXIT, ENTER };
    
    /**
     * @brief Constructs a Button with the specified parameters.
     * @param index_ The index of the button.
     * @param name_ The name of the button.
     * @param guidefault_ Default GUI value for the button.
     * @param analogdefault_ Default analog value for the button.
     */
    Button(const int index_, const String name_, const Phase guidefault_ = HIGH, const Phase analogdefault_ = HIGH);

    /**
     * @brief Destructor for Button.
     */
    ~Button() {}
        
    /**
     * @brief Updates the button state with new GUI and analog values.
     * @param guivalue_ The new GUI value.
     * @param analogvalue_ The new analog value (optional).
     */
    void update(const unsigned int guivalue_, const unsigned int analogvalue_ = HIGH);
            
    /**
     * @brief Notifies all listeners of an event.
     * @param specifier_ An optional specifier for the event.
     */
    void notifyListeners(const int specifier_) override;

    std::vector<std::function<void()>> onClick; /**< List of functions to call on button click */
    std::vector<std::function<void()>> onPress; /**< List of functions to call on button press */
    std::vector<std::function<void()>> onRelease; /**< List of functions to call on button release */
    
    /**
     * @brief Gets the current phase of the button.
     * @return The current phase.
     */
    Phase getPhase() const { return phase; }
    
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
    
    static const int DEBOUNCING_UNITS; /**< Number of units for debouncing */
    static const int LONGPRESS_UNITS; /**< Number of units for detecting a long press */
};

#endif /* uielements_hpp */
