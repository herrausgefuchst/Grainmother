#include "outputs.hpp"


// MARK: - DISPLAY-CATCH
// ********************************************************************************

inline void Display::DisplayCatch::newMessage (const String _message)
{
    message = _message;
    clear();
}

inline void Display::DisplayCatch::clear()
{
    strings.clear();
    ints.clear();
    floats.clear();
    rows.clear();
}

void Display::DisplayCatch::createRows()
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
        int size = ints[0];
        int value = ints[1];
        
        rows.push_back("_________________________________________________");
        rows.push_back("|+|");
        rows.push_back("|+|      " + strings[0]);
        rows.push_back("|+|");
        int idx = value == 0 ? size : value;
        rows.push_back("|+|      " + strings[idx]);
        rows.push_back("|+|  --> " + strings[value+1]);
        if (size > 2)
        {
            idx += 2;
            if (idx > size) idx -= size;
            rows.push_back("|+|      " + strings[idx]);
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
        int size = ints[0];
        int value = ints[1];
        
        rows.push_back("_________________________________________________");
        rows.push_back("|+|");
        rows.push_back("|+|      " + strings[0]);
        rows.push_back("|+|");
        int idx = value == 0 ? size : value;
        rows.push_back("|+|      " + strings[idx]);
        rows.push_back("|+|  --> " + strings[value+1]);
        if (size > 2)
        {
            idx += 2;
            if (idx > size) idx -= size;
            rows.push_back("|+|      " + strings[idx]);
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

void Display::DisplayCatch::print()
{
    
#ifdef BELA_CONNECTED
    for (unsigned int n = 0; n < DISPLAY_NUM_ROWS; n++)
        rt_printf("%s \n", rows[n].c_str());
    rt_printf("\n");
    
#else
    for (unsigned int n = 0; n < DISPLAY_NUM_ROWS; n++)
        printf("%s \n", rows[n].c_str());
    printf("\n");
    
#endif
    
}

// MARK: - DISPLAY
// ********************************************************************************

Display::Display()
{
#ifdef BELA_CONNECTED
    oscTransmitter.setup(DISPLAY_OSC_REMOTE_PORT, OSC_REMOTE_IP);
#endif
}

void Display::setPresetCatch (const int _index, const String _name)
{
    preset_index = _index;
    preset_name = _name;
}

bool Display::update (const bool _withConsole)
{
    bool needsRefreshment = false;
    
    if (message_catch)
    {
        
#ifdef BELA_CONNECTED
        oscTransmitter.send();
#endif
        
        if (_withConsole) displaycatch.print();
        
        message_catch = !message_catch;
        autodisplay_ctr = DISPLAY_AUTOHOMESCREEN;
        needsRefreshment = true;
    }
    
    else
    {
        if (display_type == TEMPORARILY)
        {
            if (--autodisplay_ctr <= 0)
            {
                displayPreset(preset_index, preset_name);
                message_catch = true;
                display_type = CONSTANT;
            }
        }
    }
    
    return needsRefreshment;
}

void Display::parameterChanged(AudioParameter *_param)
{
    if (instanceof<SlideParameter>(_param)) displaySlideParameter(_param);
    else if (instanceof<ChoiceParameter>(_param)) displayChoiceParameter(_param);
    else if (instanceof<ButtonParameter>(_param)) displayButtonParameter(_param);
    
    message_catch = true;
    display_type = TEMPORARILY;
}

void Display::displaySlideParameter (AudioParameter* _param)
{
    SlideParameter* parameter = static_cast<SlideParameter*>(_param);
    
#ifdef BELA_CONNECTED
    // parameter is bipolar
    if (parameter->getMin() < 0.f) oscTransmitter.newMessage("/parameterChange_bipolar");
    // parameter is unipolar
    else oscTransmitter.newMessage("/parameterChange_unipolar");
    
    oscTransmitter.add(parameter->getName());
    oscTransmitter.add(parameter->getUnit());
    oscTransmitter.add(parameter->getMin());
    oscTransmitter.add(parameter->getMax());
    oscTransmitter.add(parameter->getPrintValueAsFloat());
#endif
    
    // parameter is bipolar
    if (parameter->getMin() < 0.f) displaycatch.newMessage("/parameterChange_bipolar");
    // parameter is unipolar
    else displaycatch.newMessage("/parameterChange_unipolar");
    
    displaycatch.add(parameter->getName());
    displaycatch.add(parameter->getUnit());
    displaycatch.add(parameter->getMin());
    displaycatch.add(parameter->getMax());
    displaycatch.add(parameter->getPrintValueAsFloat());
    displaycatch.createRows();
}

void Display::displayChoiceParameter (AudioParameter *_param)
{
    ChoiceParameter* parameter = static_cast<ChoiceParameter*>(_param);
    
    String* choices = parameter->getChoiceNames();
 
#ifdef BELA_CONNECTED
    oscTransmitter.newMessage("/parameterChange_choice");
    int index = parameter->getValueAsInt();
    int scrollable = 2;
    if (index == 0) scrollable = 1;
    if (index == parameter->getNumChoices() - 1) scrollable = -1;
    if (parameter->getNumChoices() == 0) scrollable = 0;
    oscTransmitter.add(parameter->getName());
    oscTransmitter.add(choices[index]);
    oscTransmitter.add(scrollable);
#endif
    
    displaycatch.newMessage("/parameterChange_choice");
    displaycatch.add(parameter->getName());
    displaycatch.add(choices, parameter->getNumChoices());
//    displaycatch.add(parameter->getNumChoices());
    displaycatch.add(parameter->getValueAsInt());
    displaycatch.createRows();
}

void Display::displayButtonParameter (AudioParameter *_param)
{
    ButtonParameter* parameter = static_cast<ButtonParameter*>(_param);
    
#ifdef BELA_CONNECTED
    oscTransmitter.newMessage("/parameterChange_button");
    oscTransmitter.add(parameter->getName());
    oscTransmitter.add(parameter->getValueAsInt());
#endif
    
    displaycatch.newMessage("/parameterChange_button");
    displaycatch.add(parameter->getName());
    displaycatch.add(parameter->getValueAsInt());
    displaycatch.createRows();
}

void Display::menuPageChanged(Menu::Page* _page)
{
    if (_page->getID() == "home")
        displayPreset((int)_page->getCurrentChoice(), _page->getCurrentPrintValue());
    
    else
        displayMenuPage(_page);
    
    message_catch = true;
    display_type = CONSTANT;
}

void Display::displayMenuPage (Menu::Page* _page)
{
    // convert vecotr of strings to array of strings
//    std::vector<Menu::Page::Item*> items = _page->getItems();
//    String choices[_page->getNumChoices()];
//    for (unsigned int n = 0; n < _page->getNumChoices(); n++) choices[n] = items[n]->name;
//    
//#ifdef BELA_CONNECTED
//    oscTransmitter.newMessage("/menupage");
//    
//    int currentChoice = _page->getCurrentChoice();
//    String current = items[currentChoice]->name;
//    String upper = currentChoice == 0 ? "" : items[currentChoice - 1]->name;
//    String lower = currentChoice == (_page->getNumChoices() - 1) ? "" : items[currentChoice + 1]->name;
//    
//    oscTransmitter.add(_page->getName());
//    oscTransmitter.add(current);
//    oscTransmitter.add(upper);
//    oscTransmitter.add(lower);
//#endif
//    
//    displaycatch.newMessage("/menupage");
//    displaycatch.add(_page->getName());
//    displaycatch.add(choices, _page->getNumChoices());
//    displaycatch.add(_page->getNumChoices());
////    displaycatch.add(_page->getCurrentChoice());
//    displaycatch.createRows();
}

void Display::displayPreset (const int _index, const String _name)
{
    
#ifdef BELA_CONNECTED
    oscTransmitter.newMessage("/preset");
    oscTransmitter.add(_name);
    oscTransmitter.add(_index);
#endif
    
    displaycatch.newMessage("/preset");
    displaycatch.add(_name);
    displaycatch.add(_index);
    displaycatch.createRows();
}


// MARK: - LED
// ********************************************************************************


const uint LED::BLINKING_RATE = 1;
const uint LED::NUM_BLINKS = 5;


void LED::setup(const String& id_)
{
    id = id_;
    
    blinkRateCounter = BLINKING_RATE;
    numBlinksCounter = NUM_BLINKS * 2;
}

void LED::parameterChanged(AudioParameter* param_)
{
    if (instanceof<ButtonParameter>(param_) || instanceof<ToggleParameter>(param_))
    {
        value = param_->getValueAsFloat();
    }
    
    else if (instanceof<ChoiceParameter>(param_) 
             && param_->getParameterID() != "effect_edit_focus")
    {
        ChoiceParameter* param = static_cast<ChoiceParameter*>(param_);
        
        value = 0.3f + 0.7f * ((param->getValueAsFloat() + 1.f) / (float)param->getNumChoices());
    }
    
    else if (param_->getParameterID() == "effect_edit_focus")
    {
        if ((param_->getValueAsInt() == 0 && id == "effect1")
            || (param_->getValueAsInt() == 1 && id == "effect2")
            || (param_->getValueAsInt() == 2 && id == "effect3"))
        {
            if (state == ALERT) lastState = VALUEFOCUS;
            
            else state = VALUEFOCUS;
        }
        
        else state = VALUE;
    }
}

void LED::potCatchedValue()
{
    blinkOnce();
}


void LED::alert()
{
    blinkValue = 0.f;
    
    blinkRateCounter = BLINKING_RATE;
    
    if (state != ALERT) lastState = state;
    
    state = ALERT;
}


void LED::blinkOnce()
{
    if (state != ALERT)
    {
        blinkValue = value > 0.f ? 1.f : 0.f;
        
        blinkRateCounter = BLINKING_RATE;
        
        lastState = state;
        
        state = BLINKONCE;
    }
}


float LED::get()
{
    float output = 0.f;
        
    switch (state)
    {
        case VALUE:
        {
            output = value;
            
            break;
        }

        case VALUEFOCUS:
        {
            if (--blinkRateCounter == 0)
            {
                blinkRateCounter = BLINKING_RATE;
                
                blinkValue = !blinkValue;
            }
            
            if (value > 0.5f) output = 0.68f * value + 0.32f * blinkValue;
            
            else output = 0.08f * blinkValue + 0.42f;
            
            break;
        }
            
        case ALERT:
        {
            if (--blinkRateCounter == 0)
            {
                if (--numBlinksCounter == 0)
                {
                    numBlinksCounter = NUM_BLINKS * 2;
                    
                    state = lastState;
                }
                
                blinkValue = !blinkValue;
                
                blinkRateCounter = BLINKING_RATE;
                
                output = blinkValue;
            }
            
            break;
        }
            
        case BLINKONCE:
        {
            if (--blinkRateCounter == 0)
            {
                blinkValue = !blinkValue;
                
                state = lastState;
                
                blinkRateCounter = BLINKING_RATE;
            }
            
            output = blinkValue;
            
            break;
        }

        default: break;
    }
    
    return output;
}


// MARK: - METRONOME
// ********************************************************************************

// TODO: check in BELA if LEDs and Metronome works correctly

void Metronome::setup (const float _fs, const float _defaultTempo_bpm)
{
    fs = _fs;
    tempo_samples = (int)(fs * 60.f) / _defaultTempo_bpm;
    counter = tempo_samples;
}

void Metronome::process()
{
    if (--counter <= 0)
    {
        counter = tempo_samples;
        for (auto i : onTic) i();
    }
}

void Metronome::parameterChanged (AudioParameter* _param)
{
    float tempo_bpm = _param->getValueAsFloat();
    
    // 60 bpm = 1 tic p sec = 44100 samples
    // 120 bpm = 2 tics p sec = 22050 samples
    // 30 bpm = 0.5 tics p sec = 88200 samples
    // x bpm = x / 60 tics p sec = fs / (x / 60) = fs * 60 / x
    tempo_samples = (int)((fs * 60.f) / tempo_bpm);
    counter = tempo_samples;
}
