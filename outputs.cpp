#include "outputs.hpp"


// MARK: - DISPLAY-CATCH
// ********************************************************************************

inline void Display::DisplayCache::newMessage(const String& message_)
{
    message = message_;
    clear();
}

void Display::DisplayCache::add(const float value_) {
    floats.push_back(value_);
}

void Display::DisplayCache::add(const int value_) {
    ints.push_back(value_);
}

void Display::DisplayCache::add(const String& value_) {
    strings.push_back(value_);
}

void Display::DisplayCache::add(String* value_, const size_t size_) {
    for (unsigned int n = 0; n < size_; ++n)
        strings.push_back(value_[n]);
}

inline void Display::DisplayCache::clear()
{
    strings.clear();
    ints.clear();
    floats.clear();
    rows.clear();
}

void Display::DisplayCache::createRows()
{
    if (message == "/parameterChange_bipolar" || message == "/parameterChange_unipolar")
    {
        rows.push_back("_________________________________________________");
        rows.push_back("|+|");
        rows.push_back("|+|      " + strings[0]);
        rows.push_back("|+|");
        rows.push_back("|+|      " + TOSTRING(floats[2]) + " " + strings[1]);
        rows.push_back("|+|");
        rows.push_back("|+|      min: " + TOSTRING(floats[0]) + " | max: " + TOSTRING(floats[1]));
        rows.push_back("|+|");
        rows.push_back("|+|      MESSAGE: " + message);
        rows.push_back("|+|______________________________________________");
    }
    else if (message == "/parameterChange_choice")
    {
        String name = strings[0];
        uint size = ints[0];
        uint choiceIndex = ints[1] + 1;
        uint lowerIndex = (choiceIndex == 1) ? size : choiceIndex-1;
        uint upperIndex = (choiceIndex == size) ? 1 : choiceIndex+1;
        
        rows.push_back("_________________________________________________");
        rows.push_back("|+|");
        rows.push_back("|+|      " + name);
        rows.push_back("|+|");
        rows.push_back("|+|      " + strings[upperIndex]);
        rows.push_back("|+|  --> " + strings[choiceIndex]);
        if (size > 2)
        {
            rows.push_back("|+|      " + strings[lowerIndex]);
        }
        else
            rows.push_back("|+|");
        rows.push_back("|+|");
        rows.push_back("|+|      MESSAGE: " + message);
        rows.push_back("|+|______________________________________________");
    }
    else if (message == "/parameterChange_button")
    {
        rows.push_back("_________________________________________________");
        rows.push_back("|+|");
        rows.push_back("|+|      " + strings[0]);
        rows.push_back("|+|");
        if (ints[0] == 1)
        {
            rows.push_back("|+|      ON ON ON");
            rows.push_back("|+|      ON ON ON");
            rows.push_back("|+|      ON ON ON");
        }
        else
        {
            rows.push_back("|+|      OFF  OFF");
            rows.push_back("|+|      OFF  OFF");
            rows.push_back("|+|      OFF  OFF");
        }
        rows.push_back("|+|");
        rows.push_back("|+|      MESSAGE: " + message);
        rows.push_back("|+|______________________________________________");
    }
    else if (message == "/menupage")
    {
        String name = strings[0];
        uint size = ints[0];
        uint choiceIndex = ints[1] + 1;
        uint upperIndex = (choiceIndex == 1) ? size : choiceIndex-1;
        uint lowerIndex = (choiceIndex == size) ? 1 : choiceIndex+1;
        
        rows.push_back("_________________________________________________");
        rows.push_back("|+|");
        rows.push_back("|+|      " + name);
        rows.push_back("|+|");
        rows.push_back("|+|      " + strings[upperIndex]);
        rows.push_back("|+|  --> " + strings[choiceIndex]);
        if (size > 2)
        {
            rows.push_back("|+|      " + strings[lowerIndex]);
        }
        else
            rows.push_back("|+|");
        rows.push_back("|+|");
        rows.push_back("|+|      MESSAGE: " + message);
        rows.push_back("|+|______________________________________________");
    }
    else if (message == "/preset")
    {
        rows.push_back("_________________________________________________");
        rows.push_back("|+|");
        rows.push_back("|+|");
        rows.push_back("|+|");
        rows.push_back("|+|      " + TOSTRING(ints[0]) + ": " + strings[0]);
        rows.push_back("|+|");
        rows.push_back("|+|");
        rows.push_back("|+|");
        rows.push_back("|+|      MESSAGE: " + message);
        rows.push_back("|+|______________________________________________");
    }
}

void Display::DisplayCache::print()
{
    for (unsigned int n = 0; n < DISPLAY_NUM_ROWS; ++n)
        rt_printf("%s \n", rows[n].c_str());
    
    rt_printf("\n");
}

// MARK: - DISPLAY
// ********************************************************************************


void Display::setup(Menu::Page* presetPage_)
{
    presetPage = presetPage_;
    
#ifdef BELA_CONNECTED
    oscTransmitter.setup(7562, "192.168.7.2");
#endif
}

bool Display::update(const bool withConsole_)
{
    bool needsRefreshment = false;
    
    if (newMessageCache)
    {
        
#ifdef BELA_CONNECTED
        oscTransmitter.send();
#endif
        
        if (withConsole_) displayCache.print();
        
        newMessageCache = false;
        resetDisplayCounter = DISPLAY_AUTOHOMESCREEN;
        needsRefreshment = true;
    }
    
    else
    {
        if (stateDuration == TEMPORARY)
        {
            if (--resetDisplayCounter == 0)
            {
                displayPreset();
                
                newMessageCache = true;
                
                tempParameter = nullptr;
                
                stateDuration = PERMANENT;
            }
        }
    }
    
    return needsRefreshment;
}

void Display::parameterCalledDisplay(AudioParameter* param_)
{
    if (instanceof<SlideParameter>(param_)) displaySlideParameter(param_);
    else if (instanceof<ChoiceParameter>(param_)) displayChoiceParameter(param_);
    else if (instanceof<ButtonParameter>(param_)) displayButtonParameter(param_);
    
    newMessageCache = true;
    
    stateDuration = (param_->getIndex() > NUM_POTENTIOMETERS) ? PERMANENT : TEMPORARY;
    
    tempParameter = param_;
}

void Display::displaySlideParameter(AudioParameter* param_)
{
    SlideParameter* parameter = static_cast<SlideParameter*>(param_);
    
#ifdef BELA_CONNECTED
    // order of cache elements
    // 1. name of parameter
    // 2. suffix of parameter (unit)
    // 3. current value of parameter
    // 4. normalized value of parameter
    
    // parameter is bipolar
    if (parameter->getMin() < 0.f) oscTransmitter.newMessage("/parameterChange_bipolar");
    // parameter is unipolar
    else oscTransmitter.newMessage("/parameterChange_unipolar");
    
    oscTransmitter.add(parameter->getName());
    oscTransmitter.add(parameter->getUnit());
    oscTransmitter.add(parameter->getPrintValueAsFloat());
    oscTransmitter.add(parameter->getNormalizedValue());
#endif
    // order of cache elements
    // 1. name of parameter
    // 2. suffix of parameter (unit)
    // 3. minimum of parameter
    // 4. max of parameter
    // 5. value of parameter
    
    // parameter is bipolar
    if (parameter->getMin() < 0.f) displayCache.newMessage("/parameterChange_bipolar");
    // parameter is unipolar
    else displayCache.newMessage("/parameterChange_unipolar");
    
    displayCache.add(parameter->getName());
    displayCache.add(parameter->getUnit());
    displayCache.add(parameter->getMin());
    displayCache.add(parameter->getMax());
    displayCache.add(parameter->getPrintValueAsFloat());
    displayCache.createRows();
}

void Display::displayChoiceParameter(AudioParameter* param_)
{
    ChoiceParameter* parameter = static_cast<ChoiceParameter*>(param_);
    
    String* choices = parameter->getChoiceNames();
 
#ifdef BELA_CONNECTED
    oscTransmitter.newMessage("/parameterChange_choice");
    int index = parameter->getValueAsInt();
    
    // order of cache elements
    // 1. name of parameter
    // 2. current choice name
    oscTransmitter.add(parameter->getName());
    oscTransmitter.add(choices[index]);
#endif
    // order of cache elements
    // 1. name of parameter
    // 2. an array of choice names, size of array
    // 3. size of array
    // 4. current choice index
    displayCache.newMessage("/parameterChange_choice");
    displayCache.add(parameter->getName());
    displayCache.add(choices, parameter->getNumChoices());
    displayCache.add((int)parameter->getNumChoices());
    displayCache.add(parameter->getValueAsInt());
    displayCache.createRows();
}

void Display::displayButtonParameter(AudioParameter* param_)
{
    // order of cache elements
    // 1. name of parameter
    // 2. current state
    
    ButtonParameter* parameter = static_cast<ButtonParameter*>(param_);
    
#ifdef BELA_CONNECTED
    oscTransmitter.newMessage("/parameterChange_button");
    oscTransmitter.add(parameter->getName());
    oscTransmitter.add(parameter->getValueAsInt());
#endif
    
    displayCache.newMessage("/parameterChange_button");
    displayCache.add(parameter->getName());
    displayCache.add(parameter->getValueAsInt());
    displayCache.createRows();
}

void Display::menuPageChanged(Menu::Page* page_)
{
    if (page_->getID() == "load_preset")
    {
        displayPreset();
    }
    else if (instanceof<Menu::ParameterPage>(page_))
    {
        Menu::ParameterPage* page = static_cast<Menu::ParameterPage*>(page_);
        parameterCalledDisplay(page->getParameter());
    }
    else
        displayMenuPage(page_);
    
    newMessageCache = true;
    stateDuration = PERMANENT;
}

void Display::displayMenuPage(Menu::Page* page_)
{
    size_t currentChoice = page_->getCurrentChoice();
    String* choiceNames = page_->getChoiceNames();
    size_t numChoices = page_->getNumChoices();
    
#ifdef BELA_CONNECTED
    oscTransmitter.newMessage("/menupage");
    
    String current = page_->getCurrentPrintValue();
    String upper = (currentChoice == 0) ? "" : choiceNames[currentChoice-1];
    String lower = (currentChoice == numChoices-1) ? "" : choiceNames[currentChoice+1];
    
    oscTransmitter.add(page_->getName());
    oscTransmitter.add(current);
    oscTransmitter.add(upper);
    oscTransmitter.add(lower);
#endif
    
    displayCache.newMessage("/menupage");
    displayCache.add(page_->getName());
    displayCache.add(choiceNames, numChoices);
    displayCache.add((int)numChoices);
    displayCache.add((int)currentChoice);
    displayCache.createRows();
}

void Display::displayPreset()
{
#ifdef BELA_CONNECTED
    oscTransmitter.newMessage("/preset");
    oscTransmitter.add(presetPage->getCurrentPrintValue());
    oscTransmitter.add((int)presetPage->getCurrentChoice());
#endif
    
    displayCache.newMessage("/preset");
    displayCache.add(presetPage->getCurrentPrintValue());
    displayCache.add((int)presetPage->getCurrentChoice());
    displayCache.createRows();
}


// MARK: - LED
// ********************************************************************************


const uint LED::BLINKING_RATE = 10;
const uint LED::ALERT_RATE = 23;
const uint LED::NUM_BLINKS = 4;


void LED::setup(const String& id_)
{
    // set ID
    id = id_;
    
    // setup counter for the time of one blink
    rateCounter = BLINKING_RATE;
    
    // setup counter for the number of blinks
    // 2 x number of blinks for on and off states
    numBlinksCounter = NUM_BLINKS * 2;
}


void LED::parameterChanged(AudioParameter* param_)
{
    // find out which parameter type has changed
    // - Button and Toggle
    if (instanceof<ButtonParameter>(param_) || instanceof<ToggleParameter>(param_))
    {
        // just copy the parameter value (0 or 1)
        value = param_->getValueAsFloat();
    }
    
    // - Choice but not EffectEditFocus:
    else if (instanceof<ChoiceParameter>(param_)
             && param_->getParameterID() != "effect_edit_focus")
    {
        // cast to a ChoiceParameter
        ChoiceParameter* param = static_cast<ChoiceParameter*>(param_);
        
        // set value to a ratio: parameter value / number of choices
        value = 0.3f + 0.7f * ((param->getValueAsFloat() + 1.f) / (float)param->getNumChoices());
    }
    
    // - EffectEditFocus:
    else if (param_->getParameterID() == "effect_edit_focus")
    {
        // check if this LED is an EffectLED
        // TODO: rather do this via a std::function?
        if ((param_->getValueAsInt() == 0 && id == "effect1")
            || (param_->getValueAsInt() == 1 && id == "effect2")
            || (param_->getValueAsInt() == 2 && id == "effect3"))
        {
            // set state VALUEFOCUS
            // if we are still in alert mode, set the state the LED returns to afterwards
            if (state == ALERT) lastState = VALUEFOCUS;
            
            // else set the state
            else state = VALUEFOCUS;
        }
        
        // TODO: do we need this?
        else state = VALUE;
    }
}


void LED::potCatchedValue()
{
    // TODO: rather do this in a std::function?
    blinkOnce();
}


void LED::alert()
{
    // reset blink value
    blinkValue = 0.f;
    
    // reset counter for the time of one blink
    rateCounter = ALERT_RATE;
    
    // Save the previous state.
    // Avoid overwriting the previous state if ALERT is called repeatedly.
    // This could cause the system to get stuck in ALERT.
    if (state != ALERT) lastState = state;
    
    // set state
    state = ALERT;
}


void LED::blinkOnce()
{
    // prevent blinking if the current state is alert.
    if (state != ALERT)
    {
        // set blink value based on the current value (turn on if value > 0).
        blinkValue = value > 0.f ? 0.f : 1.f;
        
        // reset the counter for the duration of one blink.
        rateCounter = BLINKING_RATE;
        
        // save the current state before changing it.
        if (state != BLINKONCE) lastState = state;
        
        // update the state to indicate a single blink.
        state = BLINKONCE;
    }
}


float LED::getValue()
{
    float output = 0.f;
        
    switch (state)
    {
        case VALUE:
        {
            // return the current value
            output = value;
            break;
        }

        case VALUEFOCUS:
        {
            // switch the blink value if the time of one blink ran out
            if (--rateCounter == 0)
            {
                rateCounter = ALERT_RATE;
                
                blinkValue = !blinkValue;
            }
            
            // effect led can have a value (0 or 1 = bypass on or off)
            // and a blink function. depending on the value we return
            // either a softer or stronger blink effect
            if (value > 0.5f) output = 0.68f * value + 0.32f * blinkValue;
            else output = 0.08f * blinkValue + 0.42f;
            break;
        }
            
        case ALERT:
        {
            // switch the blink value if the time of one blink ran out
            if (--rateCounter == 0)
            {
                // reset the state after the specified number of blinks
                if (--numBlinksCounter == 0)
                {
                    numBlinksCounter = NUM_BLINKS * 2;
                    state = lastState;
                }
                
                blinkValue = !blinkValue;
                
                rateCounter = ALERT_RATE;
            }
            
            // return the blink value
            output = blinkValue;
            
            break;
        }
            
        case BLINKONCE:
        {
            // return blink value
            output = blinkValue;
            
            // reset the state if the time of one blink ran out
            if (--rateCounter == 0)
            {
                blinkValue = !blinkValue;
                
                state = lastState;
                
                rateCounter = BLINKING_RATE;
            }
            
            break;
        }

        default: 
        {
            rt_printf("no valid LED state!\n");
            break;
        }
    }
    
    return output;
}
