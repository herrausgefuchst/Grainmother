#ifndef display_hpp
#define display_hpp

#include "parameters.hpp"
#include "menu.hpp"
#include "functions.h"

#ifdef BELA_CONNECTED
#include <libraries/OscSender/OscSender.h>
#endif

// MARK: - DISPLAY
// ********************************************************************************

// display should be printable to:
// 1. OLED Display (via OscSender class of BELA)
// 2. GUI (via update() function, returns bool, if yes, vector of String with corresponding rows can be taken from DisplayCatch
// 3. CONSOLE (via update() function, _withConsole indicates if the Displaycatch should be printed

static const int DISPLAY_AUTOHOMESCREEN = 48; // x * DISPLAY_FRAMERATE
static const int DISPLAY_OSC_REMOTE_PORT = 7562;
static const char* OSC_REMOTE_IP = "192.168.7.2.";
static const int DISPLAY_NUM_ROWS = 10;

class Display   : public AudioParameter::Listener
                , public Menu::Listener
{
public:
    enum Type { CONSTANT, TEMPORARILY };
    
    struct DisplayCatch
    {
        inline void newMessage (const String _message);
        
        void add (const float _value) {
            floats.push_back(_value);
        }
        void add (const int _value) {
            ints.push_back(_value);
        }
        void add (const String _value) {
            strings.push_back(_value);
        }
        void add (String* _value, const int _size) {
            for (unsigned int n = 0; n < _size; n++)
                strings.push_back(_value[n]);
        }
        
        void createRows();
        inline void clear();
        void print();
        
        String message;
        std::vector<String> strings;
        std::vector<float> floats;
        std::vector<int> ints;
        std::vector<String> rows;

    } displaycatch;
    
    Display();
    ~Display() {}
    
    void setPresetCatch (const int _index, const String _name);
    
    bool update (const bool _withConsole = false);
    
    void parameterChanged (AudioParameter* _param) override;
    void menuPageChanged (Menu::Page* _page) override;
    
    void displaySlideParameter (AudioParameter* _param);
    void displayChoiceParameter (AudioParameter* _param);
    void displayButtonParameter (AudioParameter* _param);
    void displayMenuPage (Menu::Page* _page);
    void displayPreset (const int _index, const String _name);
    
private:
#ifdef BELA_CONNECTED
    OscSender oscTransmitter;
#endif
    
    Type display_type = TEMPORARILY;
    int autodisplay_ctr = 0;
    bool message_catch = false;
    
    int preset_index;
    String preset_name;
};

#endif /* display_hpp */



// MARK: - LED
// ********************************************************************************

// Statemachine:
// VALUE: LED only represents value of connected parameter
// VALUEFOCUS: LED represents value of connected parameter and blinks (effect edit focus)
// ALARM: LED blinks LED_NUM_BLINK times and sets back to previous state afterwards (i.e. preset change)
// BLINKONCE: LED blinks once ands returns to previous state afterwards

class LED : public UIElement::Listener , public AudioParameter::Listener
{
public:
    String states[4] = {"value", "valuefocus", "alarm", "blinkonce"};
    
    LED() {}
    ~LED() {}
    
    void setup(const String& id_);

    void setValue(const float value_) { value = value_; }
    
    void alert();
    
    void blinkOnce();
    
    float get();
    
    void parameterChanged(AudioParameter* param_) override;
    
    void potCatchedValue() override;
    
private:
    String id;
    
    float value = 0.f;
    float blinkValue = 0.f;
    
    enum State { VALUE, VALUEFOCUS, ALERT, BLINKONCE };
    State state = VALUE;
    State lastState = state;
    
    unsigned int blinkRateCounter;
    unsigned int numBlinksCounter;
    
    static const uint BLINKING_RATE; // x * frames (as defined in main.cpp / render.cpp)
    static const uint NUM_BLINKS;
};


// MARK: - METRONOME
// ********************************************************************************

class Metronome : public AudioParameter::Listener
{
public:
    Metronome (const float _fs = 44100.f, const float _defaultTempo_bpm = 120.f)
        { setup(_fs, _defaultTempo_bpm); }
        
    void setup (const float _fs = 44100.f, const float _defaultTempo_bpm = 120.f);
    void process();
    
    void parameterChanged (AudioParameter* _param);
    
    std::vector<std::function<void()>> onTic;

private:
    int counter = 0;
    int tempo_samples = 0;
    float fs;
};
