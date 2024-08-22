#include "parameters.hpp"

//#define CONSOLE_PRINT

// =======================================================================================
// MARK: - Audio Parameter (Base Class)
// =======================================================================================


void AudioParameter::notifyListeners(const bool withPrint_)
{
    // notify listener about changed parameter
    for (uint n = 0; n < listeners.size(); ++n)
    {
        listeners[n]->parameterChanged(this);
        if (withPrint_) listeners[n]->parameterCalledDisplay(this);
    }
    
    // call lambda functions
    for (auto i : onChange) i();
}


// =======================================================================================
// MARK: - CHOICE PARAMETER
// =======================================================================================


ChoiceParameter::ChoiceParameter(const uint index_, const String id_, const String name_,
                                 const String* choiceNames_, const unsigned int numChoices_)
    : AudioParameter(index_, id_, name_)
    , numChoices(numChoices_)
    , choiceNames(std::make_unique<String[]>(numChoices_))
{
    if (numChoices_ <= 1)
        engine_rt_error("ChoiceParameter cannot have 0 or 1 choices",
                        __FILE__, __LINE__, true);
    
    // copy the names of choices into member array
    for (int i = 0; i < numChoices_; ++i) choiceNames[i] = choiceNames_[i];
}


void ChoiceParameter::setValue(const int value_, const bool withPrint_)
{
    // Check for out-of-range values
    if (value_ > numChoices - 1 || value_ < 0)
        engine_rt_error("Trying to set a range exceeding value to AudioParameter '" 
                        + name + "'",
                        __FILE__, __LINE__, true);
    
    // set value and notify listeners
//    if (choice != value_)
//    {
        choice = value_;
        notifyListeners(withPrint_);
//    }

    #ifdef CONSOLE_PRINT
    if (withPrint_)
        consoleprint("AudioParameter(Choice) '" + name + "' received new value: " 
                     + TOSTRING(choice) + ", name: " + choiceNames[choice],
                     __FILE__, __LINE__);
    #endif
}


void ChoiceParameter::setValue(const float value_, const bool withPrint_)
{
    setValue(static_cast<int>(value_), withPrint_);
}


void ChoiceParameter::potChanged(UIElement* uielement_)
{
    // choice 0
    
    // Cast the generic UI element pointer to a Potentiometer pointer
    Potentiometer* pot = static_cast<Potentiometer*>(uielement_);

    // Retrieve the current value of the potentiometer
    // 0.1
    float value = pot->getValue();

    // Calculate the change (delta) in value since the last check
    // last value = 0.0, delta = 0.1
    float delta = value - pot->getLastValue();

    // Determine the step size based on the number of choices available
    // 1/2
    float step = 1.f / static_cast<float>(numChoices - 1);

    // Calculate the current step index based on the potentiometer value
    // 0
    int steps = static_cast<int>(value * (numChoices - 1));

    // Potentiometer has moved upwards (positive delta)
    if (delta > 0.f)
    {
        // Check if the new step index is different from the current choice
        if (steps != choice)
        {
            // Update the value with the new step index
            setValue(steps);
        }
    }
    // Potentiometer has moved downwards (negative delta)
    else if (delta < 0.f)
    {
        // Check if the current step index is less than the current choice
        // true
        if (steps < choice)
        {
            // Check if the potentiometer value is less than or equal to the previous step threshold
            // choice - 1 * step = 1/2 -> true
            if (value <= (choice - 1) * step)
            {
                // If the value is not zero, increment the step index
                if (value != 0.f) ++steps;
                
                // Update the value with the new step index
                setValue(steps);
            }
        }
    }
}


void ChoiceParameter::nudgeValue(const int direction_)
{
    int newChoice;
    
    // direction should not be 0, 
    // if thats the case, function processes anyways with upward direction
    if (direction_ == 0)
        engine_rt_error("trying to nudge tempo without defined direction",
                        __FILE__, __LINE__, false);
    
    // nudge up or down and wrap if necessary
    if (direction_ >= 0) newChoice = (choice + 1) >= numChoices ? 0 : choice + 1;
    else newChoice = (choice == 0) ? numChoices - 1 : choice - 1;
    
    // set new value
    setValue(newChoice);
}


void ChoiceParameter::buttonClicked(UIElement* uielement_)
{
    // increment choice + 1
    int newChoice = (choice + 1) >= numChoices ? 0 : choice + 1;
        
    // set new choice
    setValue(newChoice);
}


// =======================================================================================
// MARK: - SLIDE PARAMETER
// =======================================================================================

// TODO: check if ramps work

SlideParameter::SlideParameter(const uint index_, const String id_, const String name_, const String unit_,
                               const float min_, const float max_,
                               const float nudgeStep_, const float default_,
                               const float sampleRate_,
                               const Scaling scaling_, const float ramptimeMs_)
    : AudioParameter(index_, id_, name_)
    , unit(unit_), min(min_), max(max_), nudgeStep(round_float_3(nudgeStep_)), defaultValue(default_), range(max_ - min_), ramptimeMs(ramptimeMs_)
    , scaling(scaling_)
{
    // handling wrong initialization values
    engine_error(max_ <= min_,
                 "AudioParameter " + name + " has no suitable range: max <= min",
                 __FILE__, __LINE__, true);
    
    engine_error(default_ < min_ || default_ > max_,
                 "AudioParameter " + name + " has no suitable default value",
                 __FILE__, __LINE__, true);
    
    engine_error(nudgeStep_ <= 0.f,
                 "AudioParameter " + name + " has no suitable step value",
                 __FILE__, __LINE__, true);
    
    // setup ramp
    // TODO: blockwise processing?
    value.setup(default_, sampleRate_, 1);
     
    // set value, no display print
    setValue(default_, false);
}


void SlideParameter::processRamp()
{
    if (!value.rampFinished)
    {
        value.processRamp();
        notifyListeners(false);
    }
}


void SlideParameter::setValue(float value_, const bool withPrint_)
{
    // check for out-of-range values
    if (value_ < min || value_ > max)
        engine_rt_error("Trying to set a range exceeding value to AudioParameter " 
                        + name + " : " + TOSTRING(value_),
                        __FILE__, __LINE__, false);
    
    // bound value
    boundValue(value_, min, max);
    
    // calculate and set corresponding normalized value (0...1)
    switch (scaling)
    {
        case Scaling::LIN:
        {
            normalizedValue = mapValue(value_, min, max, 0.f, 1.f);
            boundValue(normalizedValue, 0.f, 1.f);
            break;
        }
        case Scaling::FREQ:
        {
            float a = powf_neon(2.f, logbase(range + 1.f, 2.f));
            normalizedValue = logbase(value_ + 1.f - min, a);
            boundValue(normalizedValue, 0.f, 1.f);
            break;
        }
        default:
            break;
    }
    
    // set print/ramp value
    setRampValue(value_, withPrint_);
    
    // notify listeners
    notifyListeners(withPrint_);
}


void SlideParameter::setValue(const int value_, const bool withPrint_)
{
    setValue(static_cast<float>(value_), withPrint_);
}


void SlideParameter::setNormalizedValue(float value_, const bool withPrint_)
{
    // check for out-of-range values
    if (value_ < 0.f || value_ > 1.f)
        engine_rt_error("Trying to set a normalized, range exceeding value to AudioParameter " 
                        + name + TOSTRING(value_),
                        __FILE__, __LINE__, false);
    
    // bound value
    boundValue(value_, 0.f, 1.f);
    
    // set normalized value
    normalizedValue = value_;
    
    // calculate corresponding print value
    float printValue;
    
    switch (scaling)
    {
        // linear scaling
        case Scaling::LIN:
        {
            printValue = mapValue(value_, 0.f, 1.f, min, max);
            break;
        }
        // frequency related scaling
        // f(x) = 2 ^(log2(range + 1) * x) - 1 + min
        case Scaling::FREQ:
        {
            printValue = powf_neon(2.f, logbase(range + nudgeStep + 1.f, 2.f) * value_) - 1.f + min;
            boundValue(printValue, min, max);
            break;
        }
        default:
            break;
    }
    
    // set print/ramp value
    setRampValue(printValue);
    
    // notify listeners
    notifyListeners(withPrint_);
}


void SlideParameter::setDefault()
{
    setValue(defaultValue);
}


void SlideParameter::setRampValue(const float value_, const bool withRamp_)
{
    // flag withPrint_ not only determines if the value is going to be displayed,
    // but also if the ramp should be skipped (i.e., in case of Preset Change)
    if (!withRamp_)
    {
        value = value_;
    }
    else
    {
        value.setRampTo(value_, ramptimeMs * 0.001f);
        #ifdef CONSOLE_PRINT
        consoleprint("AudioParameter(Slide) '" + name + "' received new value: "
                     + TOSTRING(value.getTarget()),
                     __FILE__, __LINE__);
        #endif
    }
}


void SlideParameter::potChanged(UIElement* uielement_)
{
    Potentiometer* pot = static_cast<Potentiometer*>(uielement_);
    
    #ifdef CONSOLE_PRINT
    consoleprint("AudioParameter(Slide) '" + name 
                 + "' received new value of potentiometer "
                 + TOSTRING(pot->getIndex()) + ", value: " 
                 + TOSTRING(pot->getValue()),
                 __FILE__, __LINE__);
    #endif
    
    setNormalizedValue(pot->getValue());
}


void SlideParameter::nudgeValue(const int direction_)
{
    // direction should not be 0,
    // if this is the case, function processes anyways with upward direction
    if (direction_ == 0)
        engine_rt_error("trying to nudge tempo without defined direction",
                        __FILE__, __LINE__, false);
    
    float currentValue = value();
    float newValue;
    
    // modulo defines if the current value is in the grid of steps
    float modulo = round_float_3(fabsf_neon(fmodf_neon(currentValue, nudgeStep)));
    // check for floating point precision issues, set flag for in Grid value
    bool inGrid = false;
    if (isClose(modulo, nudgeStep, 0.001f) || isClose(modulo, 0.f, 0.001f)) inGrid = true;
    
    // the current value is in the grid of steps:
    // nudge one step up or down
    if (inGrid)
    {
        newValue = direction_ >= 0 ? currentValue + nudgeStep : currentValue - nudgeStep;
    }
    
    // the current value is not in the grid of steps:
    // nudge up or down to the next closest step in the grid
    else
    {
        if (direction_ >= 0) 
        {
            if (currentValue >= 0.f) newValue = currentValue + (nudgeStep - modulo);
            else newValue = currentValue + modulo;
        }
        else
        {
            if (currentValue >= 0.f) newValue = currentValue - modulo;
            else newValue = currentValue - (nudgeStep - modulo);
        }
    }
    
    // bound the new value to minimum and maximum
    boundValue(newValue, min, max);
    newValue = round_float_3(newValue);
        
    // if its different than the current value, set new value
    if (newValue != currentValue) setValue(newValue);
}


// =======================================================================================
// MARK: - BUTTON PARAMETER
// =======================================================================================

// TODO: i dont understand the meaning of the parameter types, lets see...

ButtonParameter::ButtonParameter(const uint index_, const String id_, const String name_,
                                 const Type type_, const String* toggleStateNames_)
    : AudioParameter(index_, id_, name_)
    , type(type_)
{
    // if its valid, copy the names of toogle states into member array
    if (toggleStateNames_)
    {
        toggleStateNames = std::make_unique<String[]>(2);
        for (int i = 0; i < 2; ++i) toggleStateNames[i] = toggleStateNames_[i];
    }
}


void ButtonParameter::setValue(const float value_, const bool withPrint_)
{
    setValue(static_cast<int>(value_), withPrint_);
}


void ButtonParameter::setValue(const int value_, const bool withPrint_)
{
    // check if value is not 0 or 1
    if (value_ != 1 && value_ != 0)
        engine_rt_error("Button Parameter '" + name + "' only accepts binary values",
                        __FILE__, __LINE__, true);
    
    // set new value
    value = INT2ENUM(value_, ToggleState);
    
    // notify listeners
    notifyListeners(withPrint_);
    
    // console print
    #ifdef CONSOLE_PRINT
    if (withPrint_)
        consoleprint("AudioParameter(Button) '" + name + "' received new value, toggle: " 
                     + TOSTRING(value) 
                     + ", name: " + toggleStateNames[value], __FILE__, __LINE__);
    #endif
}


void ButtonParameter::buttonClicked(UIElement* uielement_)
{
    // cast the generic UI element pointer to a button pointer
    Button* button = static_cast<Button*>(uielement_);
    
    // TOGGLE- or COUPLED ButtonParameters toggle their value
    if (type == TOGGLE || type == COUPLED)
    {
        // toggle
        value = (value == UP) ? DOWN : UP;
        
        // console print
        #ifdef CONSOLE_PRINT
        consoleprint("AudioParameter(Button) '" + name
                     + "' received Click of button "
                     + TOSTRING(button->getIndex()) 
                     + ", toggle: " + TOSTRING(value)
                     + ", print: " + getPrintValueAsString()
                     , __FILE__, __LINE__);
        #endif
        
        // notify listeners with display print
        notifyListeners(true);
    }
    
    // call any connected functions that should react on a click
    for (auto i : onClick) i();
}


void ButtonParameter::buttonPressed(UIElement* uielement_)
{
    // cast the generic UI element pointer to a button pointer
    Button* button = static_cast<Button*>(uielement_);
    
    // MOMENTARY- or COUPLED ButtonParameters toggle their value
    // TODO: check if this is right?
    if (type == MOMENTARY || type == COUPLED)
    {
        // toggle
        value = (value == UP) ? DOWN : UP;
        
        // console print
        #ifdef CONSOLE_PRINT
        consoleprint("AudioParameter(Button) '" + name
                     + "' received Long Press of button "
                     + TOSTRING(button->getIndex()) + ", toggle: "
                     + TOSTRING(value), __FILE__, __LINE__);
        #endif
        
        // notify listeners with display print
        notifyListeners(true);
    }
    
    // call any connected functions that should react on a press
    for (auto i : onPress) i();
}


void ButtonParameter::buttonReleased(UIElement* uielement_)
{
    // cast the generic UI element pointer to a button pointer
    Button* button = static_cast<Button*>(uielement_);
    
    // MOMENTARY- or COUPLED ButtonParameters toggle their value
    // TODO: check if this is right?
    if (type == MOMENTARY || type == COUPLED)
    {
        // toggle
        value = (value == UP) ? DOWN : UP;
        
        // console print
        #ifdef CONSOLE_PRINT
        consoleprint("AudioParameter(Button) '" + name
                     + "' received Release of button "
                     + TOSTRING(button->getIndex()) + ", toggle: "
                     + TOSTRING(value), __FILE__, __LINE__);
        #endif
        
        // notify listeners with display print
        notifyListeners(true);
    }
    
    // call any connected functions that should react on a press
    for (auto i : onPress) i();
}


String ButtonParameter::getPrintValueAsString() const
{
    if (toggleStateNames)
        return value == DOWN ? toggleStateNames[DOWN] : toggleStateNames[UP];
    
    else return TOSTRING(value);
}


// =======================================================================================
// MARK: - TOGGLE PARAMETER
// =======================================================================================


ToggleParameter::ToggleParameter(const uint index_, const String id_, const String name_,
                                 const String* toggleStateNames_)
    : AudioParameter(index_, id_, name_)
{
    // if its valid, copy the names of toogle states into member array
    if (toggleStateNames_)
    {
        toggleStateNames = std::make_unique<String[]>(2);
        for (int i = 0; i < 2; ++i) toggleStateNames[i] = toggleStateNames_[i];
    }
}


void ToggleParameter::setValue(const float value_, const bool withPrint_)
{
    setValue(static_cast<int>(value_), withPrint_);
}


void ToggleParameter::setValue(const int value_, const bool withPrint_)
{
    // check if value is not 0 or 1
    if (value_ != 1 && value_ != 0)
        engine_rt_error("Button Parameter '" + name + "' only accepts binary values",
                        __FILE__, __LINE__, true);
    
    // set new value
    value = INT2ENUM(value_, ToggleState);
    
    // notify listeners
    notifyListeners(withPrint_);
    
    // console print
    #ifdef CONSOLE_PRINT
    if (withPrint_)
        consoleprint("AudioParameter(Button) '" + name + "' received new value, toggle: " 
                     + TOSTRING(value), __FILE__, __LINE__);
    #endif
}


void ToggleParameter::buttonClicked(UIElement* uielement_)
{
    // cast the generic UI element pointer to a button pointer
    Button* button = static_cast<Button*>(uielement_);

    // toggle
    value = (value == INACTIVE) ? ACTIVE : INACTIVE;
        
    // console print
    #ifdef CONSOLE_PRINT
    consoleprint("AudioParameter(Button) '" + name
                 + "' received Click of button "
                 + TOSTRING(button->getIndex())
                 + ", toggle: " + TOSTRING(value)
                 + ", print: " + getPrintValueAsString()
                 , __FILE__, __LINE__);
    #endif
    
    // notify listeners with display print
    notifyListeners(true);
    
    // call any connected functions that should react on a click
    for (auto i : onClick) i();
}

void ToggleParameter::buttonPressed(UIElement* uielement_)
{
    // call any connected functions that should react on a long press
    for (auto i : onPress) i();
}


String ToggleParameter::getPrintValueAsString() const
{
    if (toggleStateNames)
        return value == ACTIVE ? toggleStateNames[ACTIVE] : toggleStateNames[INACTIVE];
    
    else return TOSTRING(value);
}



// =======================================================================================
// MARK: - AUDIO PARAMETER GROUP
// =======================================================================================


AudioParameterGroup::AudioParameterGroup(const String id_, const Type type_, const size_t size_)
    : id(id_)
    , type(type_)
    , parameterGroup(size_)
{
    parameterGroup.reserve(size_);
}


AudioParameterGroup::~AudioParameterGroup()
{
    for (auto i : parameterGroup) delete i;
}


void AudioParameterGroup::addParameter(const uint index_, const String id_, const String name_, const String unit_,
                                       const float min_, const float max_,
                                       const float nudgeStep_, const float default_,
                                       const float sampleRate_,
                                       const SlideParameter::Scaling scaling_, 
                                       const float ramptimeMs_)
{
    int nextFreeIndex = getNextFreeGroupIndex();
    
    if (nextFreeIndex >= 0)
        parameterGroup[nextFreeIndex] = new SlideParameter
        (index_, id_, name_, unit_, min_, max_, nudgeStep_, default_, sampleRate_, scaling_, ramptimeMs_);
}


void AudioParameterGroup::addParameter(const uint index_, const String id_, const String name_,
                                       const ButtonParameter::Type type_,
                                       const String* toggleStateNames_)
{
    int nextFreeIndex = getNextFreeGroupIndex();
    
    if (nextFreeIndex >= 0)
        parameterGroup[nextFreeIndex] = new ButtonParameter
        (index_, id_, name_, type_, toggleStateNames_);
}


void AudioParameterGroup::addParameter(const uint index_, const String id_, const String name_,
                                       const ButtonParameter::Type type_,
                                       std::initializer_list<String> toggleStateNames_)
{
    int nextFreeIndex = getNextFreeGroupIndex();
    
    if (nextFreeIndex >= 0)
        parameterGroup[nextFreeIndex] = new ButtonParameter
        (index_, id_, name_, type_, toggleStateNames_.begin());
}


void AudioParameterGroup::addParameter(const uint index_, const String id_, const String name_,
                                       const String* array_, const int numChoices_)
{
    int nextFreeIndex = getNextFreeGroupIndex();
    
    if (nextFreeIndex >= 0)
        parameterGroup[nextFreeIndex] = new ChoiceParameter
        (index_, id_, name_, array_, numChoices_);
}


void AudioParameterGroup::addParameter(const uint index_, const String id_, const String name_,
                                       std::initializer_list<String> choices, ParameterTypes type_)
{
    int nextFreeIndex = getNextFreeGroupIndex();
    
    if (nextFreeIndex >= 0)
    {
        if (type_ == ParameterTypes::CHOICE)
            parameterGroup[nextFreeIndex] = new ChoiceParameter
            (index_, id_, name_, choices.begin(), static_cast<int>(choices.size()));
        
        else if (type_ == ParameterTypes::TOGGLE)
            parameterGroup[nextFreeIndex] = new ToggleParameter
            (index_, id_, name_, choices.begin());
        
        else
            engine_rt_error("Parameter of this Type can't be initialized with these variables",
                            __FILE__, __LINE__, true);
    }
}


AudioParameter* AudioParameterGroup::getParameter(const unsigned int index_)
{
    if (index_ >= parameterGroup.size())
        engine_rt_error("AudioParameterGroup " + id
                        + " couldn't find Parameter with Index " + TOSTRING(index_),
                        __FILE__, __LINE__, true);
    
    AudioParameter* parameter = parameterGroup[index_];
    
    if (!parameter) engine_rt_error("Parameter in Group " + id
                                    + " with index " + TOSTRING(index_)
                                    + " is nullptr", __FILE__, __LINE__, true);
    
    return parameter;
}


AudioParameter* AudioParameterGroup::getParameter(const String id_)
{
    AudioParameter* parameter = nullptr;
    
    for (unsigned int n = 0; n < parameterGroup.size(); ++n)
    {
        if (parameterGroup[n]->getParameterID() == id_)
        {
            parameter = parameterGroup[n];
            break;
        }
    }
    
    if (!parameter) engine_rt_error("Parameter with ID " + id_ + " is nullptr", __FILE__, __LINE__, true);

    return parameter;
}


int AudioParameterGroup::getNextFreeGroupIndex()
{
    int freeIndex = 0;

    while (parameterGroup[freeIndex] != nullptr)
    {
        ++freeIndex;
        
        // Return -1 if no free slot is found
        if (freeIndex >= parameterGroup.size())
        {
            engine_rt_error("This AudioParameterGroup is already full!",
                            __FILE__, __LINE__, false);
            return -1;
        }
    }
    
    // Return the index of the first free slot
    return freeIndex;
}
