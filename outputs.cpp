#include "Outputs.hpp"

//#define CONSOLE_PRINT

// =======================================================================================
// MARK: - DISPLAY-CACHE
// =======================================================================================


void Display::DisplayCache::newMessage(const String& message_)
{
    message = message_;
    clear();
}


void Display::DisplayCache::add(const float value_) 
{
    floats.push_back(value_);
}


void Display::DisplayCache::add(const int value_) 
{
    ints.push_back(value_);
}


void Display::DisplayCache::add(const String& value_)
{
    strings.push_back(value_);
}


void Display::DisplayCache::add(String* value_, const size_t size_) 
{
    for (unsigned int n = 0; n < size_; ++n)
        strings.push_back(value_[n]);
}


void Display::DisplayCache::clear()
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


void Display::DisplayCache::printToConsole()
{
    for (unsigned int n = 0; n < DISPLAY_NUM_ROWS; ++n)
        rt_printf("%s \n", rows[n].c_str());
    
    
    rt_printf("\n");
}


// =======================================================================================
// MARK: - DISPLAY
// =======================================================================================


const uint Display::DISPLAY_AUTOHOMESCREEN = 48;
const uint Display::DISPLAY_NUM_ROWS = 10;


void Display::setup(Menu::Page* presetPage_)
{
    presetPage = presetPage_;
    
#ifdef BELA_CONNECTED
    oscTransmitter.setup(7562, "192.168.7.2");
#endif
    
    // create the first message
    createPresetMessage();
    
    // set flag for new message detected
    newMessageCache = true;
    
    // set the state for duration to permanent
    stateDuration = PERMANENT;
}

bool Display::update()
{
    bool needsRefreshment = false;
    
    // new message in the cache?
    if (newMessageCache)
    {
#ifdef BELA_CONNECTED
        // send to Osc Reveceiver Program
        oscTransmitter.send();
#endif
        
#ifdef CONSOLE_PRINT
        // print to console
        displayCache.printToConsole();
#endif
        
        // reset flag for new message cache
        newMessageCache = false;
        
        // Resets the display frame counter
        resetDisplayCounter = DISPLAY_AUTOHOMESCREEN;
        
        // return true
        needsRefreshment = true;
    }
    
    // if the time of a temporary display view has ran out
    // return to showing the home page (the current preset page)
    else if (stateDuration == TEMPORARY && --resetDisplayCounter == 0)
    {
        // create the preset message
        createPresetMessage();
        
        // set flag for new message cache
        newMessageCache = true;
        
        // clear the pointer to the temporary shown parameter
        tempParameter = nullptr;
        
        // set state
        stateDuration = PERMANENT;
    }
    
    return needsRefreshment;
}

void Display::parameterCalledDisplay(AudioParameter* param_)
{  
    // determine what type of parameter has changed
    // and create corresponding message
    if (instanceof<SlideParameter>(param_)) creatSlideParameterMessage(param_);
    else if (instanceof<ChoiceParameter>(param_)) createChoiceParameterMessage(param_);
    else if (instanceof<ButtonParameter>(param_)) createButtonParameterMessage(param_);
    
    // set flag for a new message cache
    newMessageCache = true;
    
    // Set the state duration for the display view.
    // All AudioParameters that have a coupled UIElement have an index <= NUM_POTENTIOMETERS.
    // These should be temporary. All other parameters (i.e. menu-controlled parameters or tempo)
    // should be set to a permanent view.
    stateDuration = (param_->getIndex() >= NUM_UIPARAMS) ? PERMANENT : TEMPORARY;
    
    // save the pointer to the parameter
    tempParameter = param_;
}

void Display::menuPageChanged(Menu::Page* page_)
{
    // handle special cases
    // - the page is the home page: create a preset message
    if (page_->getID() == "load_preset")
    {
        createPresetMessage();
    }
    // - the page is a menu-controlled-parameter page:
    // get its connected parameter, and create a parameter message with it
    else if (instanceof<Menu::ParameterPage>(page_))
    {
        Menu::ParameterPage* page = static_cast<Menu::ParameterPage*>(page_);
        parameterCalledDisplay(page->getParameter());
    }
    else if (instanceof<Menu::NamingPage>(page_))
    {
        createNamingPageMessage(page_);
    }
    // - all other menu pages have the same message type
    else
    {
        createMenuPageMessage(page_);
    }
    
    // set flag for new message detected
    newMessageCache = true;
    
    // set the state for duration to permanent
    stateDuration = PERMANENT;
}

void Display::creatSlideParameterMessage(AudioParameter* param_)
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
    oscTransmitter.add(parameter->getSuffix());
    oscTransmitter.add(parameter->getValueAsFloat());
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
    displayCache.add(parameter->getSuffix());
    displayCache.add(parameter->getMin());
    displayCache.add(parameter->getMax());
    displayCache.add(parameter->getValueAsFloat());
    displayCache.createRows();
}

void Display::createChoiceParameterMessage(AudioParameter* param_)
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

void Display::createButtonParameterMessage(AudioParameter* param_)
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

void Display::createMenuPageMessage(Menu::Page* page_)
{
    size_t currentChoice = page_->getCurrentChoiceIndex();
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

void Display::createPresetMessage()
{
#ifdef BELA_CONNECTED
    oscTransmitter.newMessage("/preset");
    oscTransmitter.add(presetPage->getCurrentPrintValue());
    oscTransmitter.add((int)presetPage->getCurrentChoiceIndex());
#endif
    
    displayCache.newMessage("/preset");
    displayCache.add(presetPage->getCurrentPrintValue());
    displayCache.add((int)presetPage->getCurrentChoiceIndex());
    displayCache.createRows();
}


void Display::createNamingPageMessage(Menu::Page *page_)
{
#ifdef BELA_CONNECTED
    oscTransmitter.newMessage("/namingpage");
    oscTransmitter.add(page_->getName());
    oscTransmitter.add(page_->getCurrentPrintValue());
    oscTransmitter.add((int)page_->getCurrentChoiceIndex());
    oscTransmitter.add((int)page_->getNumChoices());
#endif
}


// =======================================================================================
// MARK: - LED
// =======================================================================================


const uint LED::BLINKING_RATE = 20;
const uint LED::ALERT_RATE = 23;
const uint LED::NUM_BLINKS = 4;


void LED::setup(const uint index_, const String& id_)
{
    index = index_;
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
             && param_->getID() != "effect_edit_focus")
    {
        // cast to a ChoiceParameter
        ChoiceParameter* param = static_cast<ChoiceParameter*>(param_);
        
        // set value to a ratio: parameter value / number of choices
        value = 0.3f + 0.7f * lin2log((param->getValueAsFloat() + 1.f) / (float)param->getNumChoices());
    }
}


void LED::alert()
{
    // reset blink value
    blinkValue = 0.f;
    
    // reset counter for the time of one blink
    rateCounter = ALERT_RATE;
    
    // Save the previous state.
    // Avoid overwriting the previous state if ALERT or BLINKONCE is called repeatedly.
    // This could cause the system to get stuck in these states
    if (state != ALERT && state != BLINKONCE) lastState = state;
    
    // set state
    state = ALERT;
}


void LED::blinkOnce()
{
    // prevent blinking if the current state is ALERT or BLINKONCE.
    if (state != ALERT && state != BLINKONCE)
    {
        // set blink value based on the current value
        // always the opposite of the current value
        blinkValue = value > 0.f ? 0.f : 1.f;
        
        // reset the counter for the duration of one blink.
        rateCounter = BLINKING_RATE;
        
        // save the current state before changing it.
        // overwrite saftey is done before (see also: alert())
        lastState = state;
        
        // update the state to indicate a single blink.
        state = BLINKONCE;
    }
}


void LED::setState(State state_)
{
    if (state_ == VALUEFOCUS)
    {
        // set state VALUEFOCUS
        // if we are still in alert or blinkonce mode, set the state the LED returns to afterwards
        if (state == ALERT || state == BLINKONCE) lastState = VALUEFOCUS;
        
        // else set the state
        else state = VALUEFOCUS;
    }
    
    else if (state_ == VALUE)
        state = VALUE;
    
    else if (state_ == ALERT)
        alert();
    
    else if (state_ == BLINKONCE)
        blinkOnce();
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
            // effect led can have a value (0 or 1 = bypass on or off)
            // and a blink function. depending on the value we return
            // either a softer or stronger blink effect
            if (value > 0.5f) output = 0.68f * value + 0.32f * blinkValue;
            else output = 0.08f * blinkValue + 0.42f;
            
            // switch the blink value if the time of one blink ran out
            if (--rateCounter == 0)
            {
                rateCounter = ALERT_RATE;
                
                blinkValue = !blinkValue;
            }
            break;
        }
            
        case ALERT:
        {
            // return the blink value
            output = blinkValue;
            
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
            engine_rt_error("no valid LED state! LED index: " + TOSTRING(index) + ", state: " + TOSTRING(state), __FILE__, __LINE__, false);
            break;
        }
    }
    
    return output;
}
