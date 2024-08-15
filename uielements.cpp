#include "uielements.hpp"

#define USING_ANALOG_INS

// MARK: - UIELEMENT
// ********************************************************************************

UIElement::UIElement(const int index_, const String name_)
    : index(index_)
    , name(name_)
{}

void UIElement::addListener (Listener* listener_)
{
    listeners.push_back(listener_);
}

void UIElement::focusListener (Listener* listener_)
{
    listeners.clear();
    listeners.push_back(listener_);
}


// MARK: - POTENTIOMETER
// ********************************************************************************

static const float POT_CATCHING_TOLERANCE = 0.008f; /**< Tolerance for catching potentiometer */
static const float POT_NOISE = 0.001f; /**< Noise threshold for potentiometer */
static const float POT_MAX_VOLTAGE = 0.831f; /**< Maximum voltage for potentiometer */


Potentiometer::Potentiometer(const int index_, const String name_, const float guidefault_, const float analogdefault_)
    : UIElement(index_, name_)
{
    // setup caches
    guiCache = guidefault_;
    analogCache = analogdefault_;
    analogAverage = analogdefault_;
    
    // fill the analogHistory with all same values
    std::fill(analogHistory.begin(), analogHistory.end(), INV_POT_MOVINGAVG_SIZE * analogCache);
}


void Potentiometer::update(const float guivalue_, const float analogvalue_)
{
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
    
#ifdef USING_ANALOG_INS
    // TODO: coould i make this pointer static?
    // calculate moving average
    analogAverage -= analogHistory[analogPtr];
    analogHistory[analogPtr] = analogvalue_ * INV_POT_MOVINGAVG_SIZE;
    analogAverage += analogHistory[analogPtr];
    if (++analogPtr >= POT_MOVINGAVG_SIZE) analogPtr = 0;
    
    // calculte the absolute value (the step of change)
    float absValue = analogAverage - analogCache;
    absValue = absValue < 0 ? -absValue : absValue;
    
    // chack for change in Analog Input, any change smaller than the POT_NOISE will be ignored
    if (absValue > POT_NOISE)
    {
        // update cache
        analogCache = analogAverage;
    
        // map the incoming value (0...MAX_VOLTAGE) to unipolar value (0...1)
        // round it to three decimal places
        float value = round_float_3(mapValue(analogCache, 0.001f, POT_MAX_VOLTAGE, 0.f, 1.0f));
        // bounding the value, saftey first
        boundValue(value, 0.f, 1.f);
        
#ifdef CONSOLE_PRINT
        consoleprint("Potentiometer " + TOSTRING(index) + " detected new ANALOG Value: " + TOSTRING(value), __FILE__, __LINE__);
#endif
        
        // Set value if:
        // 1. inputFocus is set to ANALOG.
        // 2. Pot behavior is JUMP (only if inputFocus is already set).
        // 3. Pot behavior is CATCH, and the new value is within tolerance of the current value.
        if (inputFocus == InputSource::ANALOG ||
            (potBehaviour == PotBehaviour::JUMP && inputFocus != InputSource::NONE) ||
            isClose(value, current, POT_CATCHING_TOLERANCE))
        {
            // update inputFocus
            inputFocus = InputSource::ANALOG;
            
            // set the new value
            setValue(value);
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


void Potentiometer::notifyListeners(const int specifier_)
{
    for (auto i : listeners) i->potChanged(this);
}


void Potentiometer::setValue(const float value_)
{
    // check for unbounded value
    if (value_ < 0.f || value_ > 1.f)
        engine_rt_error("new value for " + name + " exceeds range 0..1: " + std::to_string(value_),
                        __FILE__, __LINE__, true);

    // TODO: for what do we need last?
    last = current;
    current = value_;
    
    notifyListeners();
}


void Potentiometer::decouple(const float newcurrent_)
{
    // set new current value
    current = newcurrent_;
    
    // inputFocus release
    inputFocus = InputSource::NONE;
}


// MARK: - BUTTON
// ********************************************************************************

const int Button::DEBOUNCING_UNITS = 1;
const int Button::LONGPRESS_UNITS = 25;

Button::Button (const int _index, const String _name, const int _guidefault, const int _analogdefault)
    : UIElement(_index, _name)
    , debouncer(DEBOUNCING_UNITS)
{
    gui_cache = _guidefault;
    analog_cache = _analogdefault;
    state_counter = LONGPRESS_UNITS;
}

void Button::update (const int _guivalue, const int _analogvalue)
{
    if (_guivalue != gui_cache)
    {
        gui_cache = _guivalue;
        
        phase = gui_cache;
        state = JUST_CHANGED;

        consoleprint("Button " + TOSTRING(index) + " detected new GUI Value: " + TOSTRING(_guivalue) , __FILE__, __LINE__);
    }
    
    #ifdef USING_ANALOG_INS
    bool debouncedvalue = debouncer.update((bool)_analogvalue);

    if (debouncedvalue != analog_cache)
    {
        analog_cache = debouncedvalue;
        
        phase = analog_cache;
        state = JUST_CHANGED;
        
        consoleprint("Button " + TOSTRING(index) + " detected new ANALOG Value: " + TOSTRING(debouncedvalue) , __FILE__, __LINE__);
    }
    #endif
    
    switch (state)
    {
        case JUST_CHANGED:
        {
            if (phase == HIGH)
            {
                if (lastaction == PRESS) notifyListeners(RELEASE);
                else notifyListeners(CLICK);
                state = NO_ACTION;
            }
            else // phase == LOW
            {
                state_counter = LONGPRESS_UNITS;
                state = AWAITING_LONGPRESS;
            }
            break;
        }
        case AWAITING_LONGPRESS:
        {
            if (state_counter <= 0)
            {
                notifyListeners(PRESS);
                state = NO_ACTION;
            }
            --state_counter;
            break;
        }
        case NO_ACTION:
        default:
            break;
    }
}

void Button::notifyListeners (const int _specifier)
{
    if (_specifier == CLICK)
    {
        for (auto i : onClick) i();
        for (auto i : listeners) i->buttonClicked(this);
    }
    
    else if (_specifier == PRESS)
    {
        for (auto i : onPress) i();
        for (auto i : listeners) i->buttonPressed(this);
    }
    
    else if (_specifier == RELEASE)
    {
        for (auto i : onRelease) i();
        for (auto i : listeners) i->buttonReleased(this);
    }
    
    consoleprint("Button " + name + " notifies Listeners with message: " + TOSTRING(_specifier), __FILE__, __LINE__);
    
    lastaction = _specifier;
}
