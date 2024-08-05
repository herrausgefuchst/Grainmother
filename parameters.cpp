#include "parameters.hpp"

// MARK: - CHOICE PARAMETER
// *******************************************************************************

ChoiceParameter::ChoiceParameter (const String _id, const String _name, const int _numChoices, const String* _choices)
    : AudioParameter(_id, _name)
    , numChoices(_numChoices)
    , choice_names(new String[_numChoices])
{
    for (unsigned int n = 0; n < numChoices; n++) choice_names[n] = _choices[n];
}

ChoiceParameter::~ChoiceParameter()
{
    delete [] choice_names;
}

void ChoiceParameter::setValue (const int _value, const bool _withPrint)
{
    if (_value > numChoices-1 || _value < 0)
        engine_rt_error("trying to set a range exceeding value to AudioParameter '" + name + "'",
                     __FILE__, __LINE__, true);
    
    choice = _value;
    
    notifyListeners (_withPrint);
    
    if (_withPrint)
    {
        consoleprint("AudioParameter(Choice) '" + name + "' received new value: " + TOSTRING(_value) + ", name: " + choice_names[_value], __FILE__, __LINE__);
    }
}

void ChoiceParameter::setValue (const float _value, const bool _withPrint)
{
    setValue((int)_value, _withPrint);
}
    
void ChoiceParameter::potChanged (UIElement* _uielement)
{
    Potentiometer* pot = static_cast<Potentiometer*>(_uielement);
    
    float value = pot->getValue();
    float delta = value - pot->getLastValue();
    float step = 1.f / numChoices;
    int steps = value * (numChoices - 1);
    
    // potentiometer has moved upwards
    if (delta > 0.f)
    {
        if (steps != choice) setValue(steps);
    }
    // potentiometer has moved downwards
    else if (delta < 0.f)
    {
        if (steps < choice)
            if (value <= (choice - 1) * step)
            {
                if (value != 0.f) steps++;
                setValue(steps);
            }
    }
}

void ChoiceParameter::notifyListeners (const bool _withPrint)
{
    // ! DISPLAY MUST BE FIRST LISTENERS OF EACH PARAMETER !
    if (_withPrint)
    {
        for (auto i : listeners)
            if (i) i->parameterChanged(this);
    }
    else
    {
        for (unsigned int n = 1; n < listeners.size(); n++) listeners[n]->parameterChanged(this);
    }
    for (auto i : onChange) i();
}


// MARK: - SLIDE PARAMETER
// *******************************************************************************

/**
 * note: Ramp follows the printValue
 */

SlideParameter::SlideParameter (const String _id, const String _name, const String _unit, const float _min, const float _max, const float _step, const float _default, const Scaling _scaling, const float _ramptime_ms)
    : AudioParameter(_id, _name)
    , unit(_unit), min(_min), max(_max), step(_step), range(_max-_min), ramptime_ms(_ramptime_ms)
    , scaling(_scaling)
{
    engine_error(_max <= _min,
          "AudioParameter " + name + " has no suitable range: max <= min",
          __FILE__, __LINE__, true);
    
    engine_error(_default < _min || _default > _max,
          "AudioParameter " + name + " has no suitable default value",
          __FILE__, __LINE__, true);
    
    engine_error(_step < 0.f,
          "AudioParameter " + name + " has no suitable step value",
          __FILE__, __LINE__, true);
    
    value.setValue(_default);
    setValue(_default, false);
}

void SlideParameter::process()
{
    if (value.process()) notifyListeners(false);
}

void SlideParameter::setValue (float _value, const bool _withPrint)
{
    //TODO: step configuration?
    
    if ((_value < min || _value > max))
        engine_rt_error("trying to set a range exceeding value to AudioParameter " + name + " : " + std::to_string(_value),
                     __FILE__, __LINE__, false);
    
    boundValue(_value, min, max);
    
    switch (scaling)
    {
        case Scaling::LIN: {
            normalizedValue = mapValue(_value, min, max, 0.f, 1.f);
            break;
        }
        case Scaling::FREQ: {
            float a = powf_neon(2.f, logbase(range + 1.f, 2.f));
            normalizedValue = logbase(_value + 1.f - min, a);
            break;
        }
        default:
            break;
    }
    
    // flag _withPrint not only determines if the value is going to be displayed,
    // but also if the ramp should be skipped (i.e. in case of Preset Change)
    if (!_withPrint)
    {
        value.setValue(_value);
    }
    else
    {
        value.setRampTo(_value, ramptime_ms);
        consoleprint("AudioParameter(Slide) '" + name + "' received new value: " + TOSTRING(value.getGoal()), __FILE__, __LINE__);
    }
    
    notifyListeners(_withPrint);
}

void SlideParameter::setValue(const int _value, const bool _withPrint)
{
    setValue((float)_value, _withPrint);
}

void SlideParameter::setNormalizedValue (float _value, const bool _withPrint)
{
    if (_value < 0.f || _value > 1.f)
        engine_rt_error("trying to set a normalized, range exceeding value to AudioParameter " + name + std::to_string(_value),
                     __FILE__, __LINE__, false);
    
    boundValue(_value, 0.f, 1.f);
    
    switch (scaling)
    {
        case Scaling::LIN: {
            _value = mapValue(_value, 0.f, 1.f, min, max);
            break;
        }
        case Scaling::FREQ: {
            // f(x) = 2 ^(log2(range + 1) * x) - 1 + min
            _value = powf_neon(2.f, logbase(range + 1.f, 2.f) * _value) - 1.f + min;
            break;
        }
        default:
            break;
    }
    
    setValue(_value);
}

void SlideParameter::potChanged (UIElement* _uielement)
{
    Potentiometer* pot = static_cast<Potentiometer*>(_uielement);
    
    consoleprint("AudioParameter(Slide) '" + name + "' received new value of potentiometer " + TOSTRING(pot->getIndex()) + ", value: " + TOSTRING(pot->getValue()), __FILE__, __LINE__);
    
    setNormalizedValue(pot->getValue());
}

void SlideParameter::nudgeValue (const int _direction)
{
    float nudge = _direction * (range * 0.01f);

    float newvalue = nudge + value.getCurrent();

    if (newvalue <= max && newvalue >= min) setValue(newvalue);
    
    // TODO: fine tune the nudge steps so that they jump in 0.5 steps i.e.
}

void SlideParameter::notifyListeners (const bool _withPrint)
{
    // ! DISPLAY MUST BE FIRST LISTENERS OF EACH PARAMETER !
    if (_withPrint)
    {
        for (auto i : listeners)
            if (i) i->parameterChanged(this);
    }
    else
    {
        for (unsigned int n = 1; n < listeners.size(); n++)
            listeners[n]->parameterChanged(this);
    }
    for (auto i : onChange) i();
}


// MARK: - BUTTON PARAMETER
// ********************************************************************************

ButtonParameter::ButtonParameter (const String _id, const String _name, const Type _type)
    : AudioParameter(_id, _name)
    , type(_type)
{}

void ButtonParameter::setValue (const float _value, const bool _withPrint)
{
    setValue((int)_value, _withPrint);
}

void ButtonParameter::setValue (const int _value, const bool _withPrint)
{
    if (_value != 1 && _value != 0)
        engine_rt_error("Button Paramater '" + name + "' only accepts binary values",
                     __FILE__, __LINE__, true);
    
    value = _value;
    
    notifyListeners(_withPrint);
    
    if (_withPrint)
        consoleprint("AudioParameter(Button) '" + name + "' received new value, toggle: " + TOSTRING(value), __FILE__, __LINE__);
}

void ButtonParameter::buttonClicked (UIElement* _uielement)
{
    Button* button = static_cast<Button*>(_uielement);
    
    if (type == TOGGLE || type == COUPLED)
    {
        value = !value;
        
        consoleprint("AudioParameter(Button) '" + name + "' received Click of button " + TOSTRING(button->getIndex()) + ", toggle: " + TOSTRING(value), __FILE__, __LINE__);
        
        notifyListeners(true);
    }
    
    for (auto i : onClick) i();
}

void ButtonParameter::buttonPressed (UIElement* _uielement)
{
    Button* button = static_cast<Button*>(_uielement);
    
    if (type == MOMENTARY || type == COUPLED)
    {
        value = !value;
        
        consoleprint("AudioParameter(Button) '" + name + "' received Long Press of button " + TOSTRING(button->getIndex()) + ", toggle: " + TOSTRING(value), __FILE__, __LINE__);
        
        notifyListeners(true);
    }
    
    for (auto i : onPress) i();
}

void ButtonParameter::buttonReleased(UIElement *_uielement)
{
    Button* button = static_cast<Button*>(_uielement);
    
    if (type == MOMENTARY || type == COUPLED)
    {
        value = !value;
        
        consoleprint("AudioParameter(Button) '" + name + "' received Release of button " + TOSTRING(button->getIndex()) + ", toggle: " + TOSTRING(value), __FILE__, __LINE__);
        
        notifyListeners(true);
    }
    
    for (auto i : onRelease) i();
}

void ButtonParameter::notifyListeners (const bool _withPrint)
{
    // ! DISPLAY MUST BE FIRST LISTENERS OF EACH PARAMETER !
    if (_withPrint)
    {
        for (auto i : listeners)
            if(i) i->parameterChanged(this);
    }
    else
    {
        for (unsigned int n = 1; n < listeners.size(); n++) listeners[n]->parameterChanged(this);
    }
    for (auto i : onChange) i();
}



// MARK: - AUDIO PARAMETER GROUP
// *******************************************************************************

AudioParameterGroup::AudioParameterGroup (const String _name, const Type _type)
    : name(_name)
    , type(_type)
{}

AudioParameterGroup::~AudioParameterGroup()
{
    for (auto i : parametergroup) delete i;
    parametergroup.clear();
}

void AudioParameterGroup::addParameter (const String _id, const String _name, const String _unit, const float _min, const float _max, const float _step, const float _default, const SlideParameter::Scaling _scaling, const float _ramptime_ms)
{
    engine_error(type == Type::EFFECT && parametergroup.size() > 8,
          "AudioParameterGroup '" + name + "' doesn't accept AudioParameter of type SlideParameter at slot " + TOSTRING(parametergroup.size()),
          __FILE__, __LINE__, true);
    
    parametergroup.push_back(new SlideParameter(_id, _name, _unit, _min, _max, _step, _default, _scaling, _ramptime_ms));
}

void AudioParameterGroup::addParameter (const String _id, const String _name, const ButtonParameter::Type _type)
{
    engine_error(type == Type::EFFECT && parametergroup.size() != 8,
          "AudioParameterGroup '" + name + "' shouldn't have an AudioParameter of type ButtonParameter at slot " + TOSTRING(parametergroup.size()),
          __FILE__, __LINE__, true);
            
    parametergroup.push_back(new ButtonParameter(_id, _name, _type));
}

void AudioParameterGroup::addParameter (const String _id, const String _name, const int _numChoices, const String* _array)
{
    engine_error(type == Type::EFFECT && parametergroup.size() > 8,
          "AudioParameterGroup '" + name + "' doesn't accept AudioParameter of type ChoiceParameter at slot " + TOSTRING(parametergroup.size()),
          __FILE__, __LINE__, true);
    
    parametergroup.push_back(new ChoiceParameter(_id, _name, _numChoices, _array));
}

AudioParameter* AudioParameterGroup::getParameter (const int _index)
{
    if (_index >= parametergroup.size() || _index < 0)
        engine_rt_error("AudioParameterGroup " + name + " couldn't find Parmaeter with Index " + std::to_string(_index),
                     __FILE__, __LINE__, true);
    
    AudioParameter* parameter = parametergroup[_index];
    
    if (!parameter) engine_rt_error("Parameter is nullptr", __FILE__, __LINE__, true);
    
    return parametergroup[_index];
}

AudioParameter* AudioParameterGroup::getParameter (const String _id, const bool _withErrorMessage)
{
    AudioParameter* parameter = nullptr;
    
    for (auto i : parametergroup)
    {
        if (i->getParameterID() == _id)
        {
            parameter = i;
            break;
        }
    }
    
    if (_withErrorMessage)
        if (!parameter)
            engine_rt_error("AudioParameterGroup " + name + " couldn't find parameter with ID " + _id,
                            __FILE__, __LINE__, false);

    return parameter;
}
