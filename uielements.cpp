#include "uielements.hpp"

#define USING_ANALOG_INS
//#define USING_GUI

#define CONSOLE_PRINT

// =======================================================================================
// MARK: - UIELEMENT
// =======================================================================================


void UIElement::addListener (Listener* listener_)
{
    listener = listener_;
}


void UIElement::swapListener(Listener* listener_)
{
    listener = listener_;
}


// =======================================================================================
// MARK: - POTENTIOMETER
// =======================================================================================

static const float POT_NOISE = 0.005f; /**< Noise threshold for potentiometer */

PotBehaviour Potentiometer::potBehaviour = PotBehaviour::CATCH;


void Potentiometer::setup(const int index_, const String& id_, const float guidefault_)
{
    // set identifiers
    index = index_;
    id = id_;
    
    // setup caches
    guiCache = guidefault_;
}


void Potentiometer::update(const float guivalue_, const float analogvalue_)
{
    #ifdef USING_GUI
    // check for change in the GUI
    if (guivalue_ != guiCache)
    {
        // update cache
        guiCache = guivalue_;
        
        #ifdef CONSOLE_PRINT
        consoleprint("Potentiometer " + TOSTRING(index) + " detected new GUI Value: " + TOSTRING(guivalue_), __FILE__, __LINE__);
        #endif

        // Set value if:
        // 1. inputFocus is set to GUI.
        // 2. Pot behavior is JUMP (only if inputFocus is already set).
        // 3. Pot behavior is CATCH, and the new value is within tolerance of the current value.
        if (inputFocus == InputSource::GUI ||
            (potBehaviour == PotBehaviour::JUMP && inputFocus != InputSource::NONE) ||
            isClose(guiCache, current, POT_CATCHING_TOLERANCE))
        {
            // update inputFocus
            inputFocus = InputSource::GUI;
            
            // set the new value
            setValue(guiCache);
        }
    }
    #endif
    
    #ifdef USING_ANALOG_INS
    // calculte the absolute value (the step of change)
    float absValue = analogvalue_ - analogCache;
    absValue = absValue < 0 ? -absValue : absValue;
    
    // chack for change in Analog Input, any change smaller than the POT_NOISE will be ignored
    if (absValue > POT_NOISE)
    {
        // update cache
        analogCache = analogvalue_;
    
        // map the incoming value (0...MAX_VOLTAGE) to normalized value (0...1)
        // round it to three decimal places
        // -0.003 is to ensure that potentiometer really reaches 0.0
        float value = round_float_3(mapValue(analogCache, 0.f, POT_MAX_VOLTAGE, -0.003f, 1.0f));
        
        // bound the value
        boundValue(value, 0.f, 1.f);
        
        #ifdef CONSOLE_PRINT
        consoleprint("Potentiometer " + TOSTRING(index) + " detected new ANALOG Value: " + TOSTRING(value), __FILE__, __LINE__);
        #endif
        
        // Set value if:
        // 1. inputFocus is set to ANALOG.
        if (inputFocus == InputSource::ANALOG) 
        {
            setValue(value);
        }
        // 2. Pot behavior is JUMP
        else if (potBehaviour == PotBehaviour::JUMP)
        {
            setValue(value);
            
            // set input source to analog
            inputFocus = InputSource::ANALOG;
            
            // notify listeners (i.e. LEDs)
            if (onCatch) onCatch();
        }
        // 3. Pot behavior is CATCH, and the new value is within tolerance of the current value.
        else if (isClose(value, current, POT_CATCHING_TOLERANCE))
        {
            setValue(value);
            
            // set input source to analog
            inputFocus = InputSource::ANALOG;
            
            // notify listeners (i.e. LEDs)
            if (onCatch) onCatch();
        }
        // if nothing of this is true, at least call any connected function
        // that should react on a touch (i.e. display)
        else
        {
            if (onTouch) onTouch();
        }
    }
    #endif // USING_ANALOG_INS
}


void Potentiometer::setNewMIDIMessage(const float midivalue_)
{
    // Set value if:
    // 1. inputFocus is set to MIDI.
    // 2. Pot behavior is JUMP (only if inputFocus is already set).
    // 3. Pot behavior is CATCH, and the new value is within tolerance of the current value.
    if (inputFocus == InputSource::MIDI ||
        (potBehaviour == PotBehaviour::JUMP && inputFocus != InputSource::NONE) ||
        isClose(midivalue_, current, POT_CATCHING_TOLERANCE))
    {
        // update inputFocus
        inputFocus = InputSource::MIDI;
        
        // set the new value
        setValue(midivalue_);
    }
}


void Potentiometer::notifyListener(const int specifier_)
{
    if (listener) listener->potChanged(this);
    
    if (onChange) onChange();
}


void Potentiometer::setValue(const float value_)
{
    // check for unbounded value
    if (value_ < 0.f || value_ > 1.f)
        engine_rt_error("new value for " + id + " exceeds range 0..1: " + std::to_string(value_),
                        __FILE__, __LINE__, true);

    last = current;
    current = value_;
    
    notifyListener();
}


void Potentiometer::decouple(const float newcurrent_)
{
    // set new current value
    current = newcurrent_;
    
    // inputFocus release
    inputFocus = InputSource::NONE;
}


// =======================================================================================
// MARK: - BUTTON
// =======================================================================================


const int Button::DEBOUNCING_UNITS = 1;
const int Button::LONGPRESS_UNITS = 10;


void Button::setup(const int index_, const String& id_, const Phase guidefault_, const Phase analogdefault_)
{
    // set identifiers
    index = index_;
    id = id_;
        
    // setup caches with default value
    guiCache = guidefault_;
    analogCache = analogdefault_;
    
    // initialize counter
    stateCounter = LONGPRESS_UNITS;
}


void Button::update(const unsigned int guivalue_, const unsigned int analogvalue_)
{
    #ifdef USING_GUI
    // check for change in GUI
    if (guivalue_ != guiCache)
    {
        // update cache
        guiCache = INT2ENUM(guivalue_, Phase);
        
        // set momentary phase
        phase = guiCache;
        
        // set internal state, mark that the value just changed
        state = JUST_CHANGED;

        #ifdef CONSOLE_PRINT
        consoleprint("Button " + TOSTRING(index) + " detected new GUI Value: " + TOSTRING(guivalue_), __FILE__, __LINE__);
        #endif
    }
    #endif
    
    #ifdef USING_ANALOG_INS
    // update debouncer
    bool debouncedValue = debouncer.update((bool)analogvalue_);

    // check for change in Analog Input
    if (debouncedValue != analogCache)
    {
        analogCache = INT2ENUM(debouncedValue, Phase);
        
        // set momentary phase
        phase = analogCache;

        // set internal state, mark that the value just changed
        state = JUST_CHANGED;
        
        #ifdef CONSOLE_PRINT
        consoleprint("Button " + TOSTRING(index) + " detected new ANALOG Value: " + TOSTRING(debouncedValue), __FILE__, __LINE__);
        #endif
    }
    #endif // USING_ANALOG_INS
    
    // state machine looks for the appropriate Action that fits to the momentary behaviour
    switch (state)
    {
        // the button just changed
        case JUST_CHANGED:
        {
            // the button was released
            if (phase == HIGH)
            {
                // if the last action was a long press, this action is a release
                if (lastAction == LONGPRESS) notifyListener(RELEASE);
                
                // else: this action is a click
                else notifyListener(CLICK);
                
                // reset state variable
                state = NO_ACTION;
            }
            
            // the button was pressed
            else // phase == LOW
            {
                // reinitialize the counter for determing a long press
                stateCounter = LONGPRESS_UNITS;
                
                // set state variable
                state = AWAITING_LONGPRESS;
            }
            break;
        }
            
        // the button was pressed, looking for a long press
        case AWAITING_LONGPRESS:
        {
            // if time ran out and no other change was detected, this action is a long press
            if (stateCounter <= 0)
            {
                notifyListener(LONGPRESS);
                
                // reset state variable
                state = NO_ACTION;
            }
            
            // decrement state machine counter
            --stateCounter;
            
            break;
        }
            
        // default behaviour: break
        case NO_ACTION:
        default:
            break;
    }
}


void Button::notifyListener(const int specifier_)
{
    if (specifier_ == CLICK)
    {
        if (onClick) onClick();
        if (listener) listener->buttonClicked(this);
    }
    else if (specifier_ == LONGPRESS)
    {
        if (onPress) onPress();
        if (listener) listener->buttonPressed(this);
    }
    else if (specifier_ == RELEASE)
    {
        if (onRelease) onRelease();
        if (listener) listener->buttonReleased(this);
    }
    
    #ifdef CONSOLE_PRINT
    String message = specifier_ == 0 ? "CLICK" : "LONGPRESS";
    if (specifier_ == 2) message = "RELEASE";
    consoleprint("Button " + id + " notifies Listeners with message: " + message, __FILE__, __LINE__);
    #endif
    
    lastAction = INT2ENUM(specifier_, Action);
}
