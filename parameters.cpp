#include "parameters.hpp"

// MARK: - CHOICE PARAMETER
// *******************************************************************************

ChoiceParameter::ChoiceParameter(const String id_, const String name_, const int numChoices_, const String* choices_)
    : AudioParameter(id_, name_)
    , numChoices(numChoices_)
    , choice_names(new String[numChoices_])
{
    for (unsigned int n = 0; n < numChoices; n++)
    {
        choice_names[n] = choices_[n];
    }
}

ChoiceParameter::~ChoiceParameter()
{
    delete[] choice_names;
}

void ChoiceParameter::setValue(const int value_, const bool withPrint_)
{
    if (value_ > numChoices - 1 || value_ < 0)
    {
        engine_rt_error("Trying to set a range exceeding value to AudioParameter '" + name + "'",
                        __FILE__, __LINE__, true);
    }

    choice = value_;
    notifyListeners(withPrint_);

    if (withPrint_)
    {
        consoleprint("AudioParameter(Choice) '" + name + "' received new value: " + TOSTRING(value_) +
                     ", name: " + choice_names[value_], __FILE__, __LINE__);
    }
}

void ChoiceParameter::setValue(const float value_, const bool withPrint_)
{
    setValue(static_cast<int>(value_), withPrint_);
}

void ChoiceParameter::potChanged(UIElement* uielement_)
{
    Potentiometer* pot = static_cast<Potentiometer*>(uielement_);

    float value = pot->getValue();
    float delta = value - pot->getLastValue();
    float step = 1.f / numChoices;
    int steps = static_cast<int>(value * (numChoices - 1));

    // Potentiometer has moved upwards
    if (delta > 0.f)
    {
        if (steps != choice)
        {
            setValue(steps);
        }
    }
    // Potentiometer has moved downwards
    else if (delta < 0.f)
    {
        if (steps < choice)
        {
            if (value <= (choice - 1) * step)
            {
                if (value != 0.f) steps++;
                setValue(steps);
            }
        }
    }
}

void ChoiceParameter::notifyListeners(const bool withPrint_)
{
    // ! DISPLAY MUST BE FIRST LISTENER OF EACH PARAMETER !
    if (withPrint_)
    {
        for (auto i : listeners)
        {
            if (i) i->parameterChanged(this);
        }
    }
    else
    {
        for (unsigned int n = 1; n < listeners.size(); n++)
        {
            listeners[n]->parameterChanged(this);
        }
    }

    for (auto i : onChange)
    {
        i();
    }
}


// MARK: - SLIDE PARAMETER
// *******************************************************************************

/**
 * note: Ramp follows the printValue
 */

SlideParameter::SlideParameter(const String id_, const String name_, const String unit_, const float min_, const float max_, const float step_, const float default_, const Scaling scaling_, const float ramptime_ms_)
    : AudioParameter(id_, name_)
    , unit(unit_), min(min_), max(max_), step(step_), range(max_ - min_), ramptime_ms(ramptime_ms_)
    , scaling(scaling_)
{
    engine_error(max_ <= min_,
                 "AudioParameter " + name + " has no suitable range: max <= min",
                 __FILE__, __LINE__, true);
    
    engine_error(default_ < min_ || default_ > max_,
                 "AudioParameter " + name + " has no suitable default value",
                 __FILE__, __LINE__, true);
    
    engine_error(step_ < 0.f,
                 "AudioParameter " + name + " has no suitable step value",
                 __FILE__, __LINE__, true);
    
    value.setup(default_);
    setValue(default_, false);
}

void SlideParameter::process()
{
    if (value.process())
    {
        notifyListeners(false);
    }
}

void SlideParameter::setValue(float value_, const bool withPrint_)
{
    // TODO: step configuration?
    
    if (value_ < min || value_ > max)
    {
        engine_rt_error("Trying to set a range exceeding value to AudioParameter " + name + " : " + std::to_string(value_),
                        __FILE__, __LINE__, false);
    }
    
    boundValue(value_, min, max);
    
    switch (scaling)
    {
        case Scaling::LIN:
            normalizedValue = mapValue(value_, min, max, 0.f, 1.f);
            break;
        case Scaling::FREQ:
        {
            float a = powf_neon(2.f, logbase(range + 1.f, 2.f));
            normalizedValue = logbase(value_ + 1.f - min, a);
            break;
        }
        default:
            break;
    }
    
    // Flag withPrint_ not only determines if the value is going to be displayed,
    // but also if the ramp should be skipped (i.e., in case of Preset Change)
    if (!withPrint_)
    {
        value.setValue(value_);
    }
    else
    {
        value.setRampTo(value_, ramptime_ms);
        consoleprint("AudioParameter(Slide) '" + name + "' received new value: " + TOSTRING(value.getGoal()), __FILE__, __LINE__);
    }
    
    notifyListeners(withPrint_);
}

void SlideParameter::setValue(const int value_, const bool withPrint_)
{
    setValue(static_cast<float>(value_), withPrint_);
}

void SlideParameter::setNormalizedValue(float value_, const bool withPrint_)
{
    if (value_ < 0.f || value_ > 1.f)
    {
        engine_rt_error("Trying to set a normalized, range exceeding value to AudioParameter " + name + std::to_string(value_),
                        __FILE__, __LINE__, false);
    }
    
    boundValue(value_, 0.f, 1.f);
    
    switch (scaling)
    {
        case Scaling::LIN:
            value_ = mapValue(value_, 0.f, 1.f, min, max);
            break;
        case Scaling::FREQ:
            // f(x) = 2 ^(log2(range + 1) * x) - 1 + min
            value_ = powf_neon(2.f, logbase(range + 1.f, 2.f) * value_) - 1.f + min;
            break;
        default:
            break;
    }
    
    setValue(value_);
}

void SlideParameter::potChanged(UIElement* uielement_)
{
    Potentiometer* pot = static_cast<Potentiometer*>(uielement_);
    
    consoleprint("AudioParameter(Slide) '" + name + "' received new value of potentiometer " + TOSTRING(pot->getIndex()) + ", value: " + TOSTRING(pot->getValue()), __FILE__, __LINE__);
    
    setNormalizedValue(pot->getValue());
}

void SlideParameter::nudgeValue(const int direction_)
{
    float nudge = direction_ * (range * 0.01f);

    float newvalue = nudge + value.getCurrent();

    if (newvalue <= max && newvalue >= min)
    {
        setValue(newvalue);
    }
    
    // TODO: fine tune the nudge steps so that they jump in 0.5 steps, i.e.
}

void SlideParameter::notifyListeners(const bool withPrint_)
{
    // ! DISPLAY MUST BE FIRST LISTENER OF EACH PARAMETER !
    if (withPrint_)
    {
        for (auto i : listeners)
        {
            if (i) i->parameterChanged(this);
        }
    }
    else
    {
        for (unsigned int n = 1; n < listeners.size(); n++)
        {
            listeners[n]->parameterChanged(this);
        }
    }
    for (auto i : onChange)
    {
        i();
    }
}


// MARK: - BUTTON PARAMETER
// ********************************************************************************

ButtonParameter::ButtonParameter(const String id_, const String name_, const Type type_)
    : AudioParameter(id_, name_)
    , type(type_)
{}

void ButtonParameter::setValue(const float value_, const bool withPrint_)
{
    setValue(static_cast<int>(value_), withPrint_);
}

void ButtonParameter::setValue(const int value_, const bool withPrint_)
{
    if (value_ != 1 && value_ != 0)
    {
        engine_rt_error("Button Parameter '" + name + "' only accepts binary values",
                        __FILE__, __LINE__, true);
    }
    
    value = value_;
    
    notifyListeners(withPrint_);
    
    if (withPrint_)
    {
        consoleprint("AudioParameter(Button) '" + name + "' received new value, toggle: " + TOSTRING(value), __FILE__, __LINE__);
    }
}

void ButtonParameter::buttonClicked(UIElement* uielement_)
{
    Button* button = static_cast<Button*>(uielement_);
    
    if (type == TOGGLE || type == COUPLED)
    {
        value = !value;
        
        consoleprint("AudioParameter(Button) '" + name + "' received Click of button " + TOSTRING(button->getIndex()) + ", toggle: " + TOSTRING(value), __FILE__, __LINE__);
        
        notifyListeners(true);
    }
    
    for (auto i : onClick)
    {
        i();
    }
}

void ButtonParameter::buttonPressed(UIElement* uielement_)
{
    Button* button = static_cast<Button*>(uielement_);
    
    if (type == MOMENTARY || type == COUPLED)
    {
        value = !value;
        
        consoleprint("AudioParameter(Button) '" + name + "' received Long Press of button " + TOSTRING(button->getIndex()) + ", toggle: " + TOSTRING(value), __FILE__, __LINE__);
        
        notifyListeners(true);
    }
    
    for (auto i : onPress)
    {
        i();
    }
}

void ButtonParameter::buttonReleased(UIElement* uielement_)
{
    Button* button = static_cast<Button*>(uielement_);
    
    if (type == MOMENTARY || type == COUPLED)
    {
        value = !value;
        
        consoleprint("AudioParameter(Button) '" + name + "' received Release of button " + TOSTRING(button->getIndex()) + ", toggle: " + TOSTRING(value), __FILE__, __LINE__);
        
        notifyListeners(true);
    }
    
    for (auto i : onRelease)
    {
        i();
    }
}

void ButtonParameter::notifyListeners(const bool withPrint_)
{
    // ! DISPLAY MUST BE FIRST LISTENER OF EACH PARAMETER !
    if (withPrint_)
    {
        for (auto i : listeners)
        {
            if (i) i->parameterChanged(this);
        }
    }
    else
    {
        for (unsigned int n = 1; n < listeners.size(); n++)
        {
            listeners[n]->parameterChanged(this);
        }
    }
    for (auto i : onChange)
    {
        i();
    }
}



// MARK: - AUDIO PARAMETER GROUP
// *******************************************************************************

AudioParameterGroup::AudioParameterGroup(const String name_, const Type type_)
    : name(name_)
    , type(type_)
{}

AudioParameterGroup::~AudioParameterGroup()
{
    for (auto i : parametergroup)
    {
        delete i;
    }
    parametergroup.clear();
}

void AudioParameterGroup::addParameter(const String id_, const String name_, const String unit_, const float min_, const float max_, const float step_, const float default_, const SlideParameter::Scaling scaling_, const float ramptime_ms_)
{
    engine_error(type == Type::EFFECT && parametergroup.size() > 8,
                 "AudioParameterGroup '" + name + "' doesn't accept AudioParameter of type SlideParameter at slot " + TOSTRING(parametergroup.size()),
                 __FILE__, __LINE__, true);
    
    parametergroup.push_back(new SlideParameter(id_, name_, unit_, min_, max_, step_, default_, scaling_, ramptime_ms_));
}

void AudioParameterGroup::addParameter(const String id_, const String name_, const ButtonParameter::Type type_)
{
    engine_error(type == Type::EFFECT && parametergroup.size() != 8,
                 "AudioParameterGroup '" + name + "' shouldn't have an AudioParameter of type ButtonParameter at slot " + TOSTRING(parametergroup.size()),
                 __FILE__, __LINE__, true);
            
    parametergroup.push_back(new ButtonParameter(id_, name_, type_));
}

void AudioParameterGroup::addParameter(const String id_, const String name_, const int numChoices_, const String* array_)
{
    engine_error(type == Type::EFFECT && parametergroup.size() > 8,
                 "AudioParameterGroup '" + name + "' doesn't accept AudioParameter of type ChoiceParameter at slot " + TOSTRING(parametergroup.size()),
                 __FILE__, __LINE__, true);
    
    parametergroup.push_back(new ChoiceParameter(id_, name_, numChoices_, array_));
}

void AudioParameterGroup::addParameter(const String id_, const String name_, std::initializer_list<String> choices)
{
    engine_error(type == Type::EFFECT && parametergroup.size() > 8,
                 "AudioParameterGroup '" + name + "' doesn't accept AudioParameter of type ChoiceParameter at slot " + TOSTRING(parametergroup.size()),
                 __FILE__, __LINE__, true);
    
    parametergroup.push_back(new ChoiceParameter(id_, name_, static_cast<int>(choices.size()), choices.begin()));
}

AudioParameter* AudioParameterGroup::getParameter(const int index_)
{
    if (index_ >= parametergroup.size() || index_ < 0)
    {
        engine_rt_error("AudioParameterGroup " + name + " couldn't find Parameter with Index " + std::to_string(index_),
                        __FILE__, __LINE__, true);
    }
    
    AudioParameter* parameter = parametergroup[index_];
    
    if (!parameter)
    {
        engine_rt_error("Parameter is nullptr", __FILE__, __LINE__, true);
    }
    
    return parametergroup[index_];
}

AudioParameter* AudioParameterGroup::getParameter(const String id_, const bool withErrorMessage_)
{
    AudioParameter* parameter = nullptr;
    
    for (auto i : parametergroup)
    {
        if (i->getParameterID() == id_)
        {
            parameter = i;
            break;
        }
    }
    
    if (withErrorMessage_ && !parameter)
    {
        engine_rt_error("AudioParameterGroup " + name + " couldn't find parameter with ID " + id_,
                        __FILE__, __LINE__, false);
    }

    return parameter;
}
