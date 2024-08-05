#include "uielements.hpp"

//#define USING_ANALOG_INS

// MARK: - UIELEMENT
// ********************************************************************************

UIElement::UIElement (const int _index, const String _name)
    : index(_index)
    , name(_name)
{}

void UIElement::addListener (Listener* _listener)
{
    listeners.push_back(_listener);
}

void UIElement::focusListener (Listener* _listener)
{
    listeners.clear();
    listeners.push_back(_listener);
}


// MARK: - POTENTIOMETER
// ********************************************************************************

const float Potentiometer::CATCHING_POTENTIOMETER_TOLERANCE = 0.008f;
const float Potentiometer::POT_NOISE = 0.001f;
const float Potentiometer::MAX_VOLTAGE = 0.831f;

Potentiometer::Potentiometer (const int _index, const String _name, GlobalParameters* _parameters, const float _guidefault, const float _analogdefault)
    : UIElement(_index, _name)
    , globalparameters(_parameters)
{
    gui_cache = _guidefault;
    analog_cache = _analogdefault;
    analog_average = _analogdefault;
    for (unsigned int n = 0; n < 8; n++) analog_history[n] = 0.125f * analog_cache;
}

void Potentiometer::update (const float _guivalue, const float _analogvalue)
{
    if (_guivalue != gui_cache)
    {
        gui_cache = _guivalue;
        
        consoleprint("Potentiomteter " + TOSTRING(index) + " detected new GUI Value: " + TOSTRING(_guivalue) , __FILE__, __LINE__);

        if (listen == GUI ||
            (globalparameters->potBehaviour == POTBEHAVIOUR_JUMP && listen != NONE) ||
            isClose(gui_cache, current, CATCHING_POTENTIOMETER_TOLERANCE))
        {
            listen = GUI;
            setValue(gui_cache);
        }
    }
    
    #ifdef USING_ANALOG_INS
    // calculating moving average
    analog_average -= analog_history[analog_ptr];
    analog_history[analog_ptr] = _analogvalue * 0.125f;
    analog_average += analog_history[analog_ptr];
    if (++analog_ptr >= 8) analog_ptr = 0;
    
    // is change detected?
    if (fabsf_neon(analog_average - analog_cache) > POT_NOISE)
    {
        analog_cache = analog_average;
        float value = round_float_3(mapValue(analog_cache, 0.001f, MAX_VOLTAGE, 0.f, 1.0f));
        boundValue(value, 0.f, 1.f);
        
        consoleprint("Potentiomteter " + TOSTRING(index) + " detected new ANALOG Value: " + TOSTRING(value) , __FILE__, __LINE__);

        if (listen == ANALOG ||
            (globalparameters->potBehaviour == POTBEHAVIOUR_JUMP && listen != NONE) ||
            isClose(value, current, CATCHING_POTENTIOMETER_TOLERANCE))
        {
            listen = ANALOG;
            setValue(value);
        }
    }
    #endif
}

void Potentiometer::setNewMIDIMessage (const float _midivalue)
{
    if (listen == MIDI ||
        (globalparameters->potBehaviour == POTBEHAVIOUR_JUMP && listen != NONE) ||
        isClose(_midivalue, current, CATCHING_POTENTIOMETER_TOLERANCE))
    {
        listen = MIDI;
        setValue(_midivalue);
    }
}

void Potentiometer::notifyListeners (const int _specifier)
{
    for (auto i : listeners) i->potChanged(this);
}

void Potentiometer::setValue (const float _value)
{
    if (_value < 0.f || _value > 1.f)
        engine_rt_error("new value for " + name + " exceeds range 0..1: " + std::to_string(_value),
                        __FILE__, __LINE__, true);

    last = current;
    current = _value;
    
    notifyListeners();
}

void Potentiometer::decouple (const float _newcurrent)
{
    current = _newcurrent;
    listen = NONE;
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
