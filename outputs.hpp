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
// 2. GUI (via update() function, returns bool, if yes, vector of String with corresponding rows can be taken from DisplayCache
// 3. CONSOLE (via update() function, _withConsole indicates if the Displaycatch should be printed

static const int DISPLAY_AUTOHOMESCREEN = 48; // x * DISPLAY_FRAMERATE
static const int DISPLAY_NUM_ROWS = 10;

class Display : public AudioParameter::Listener
{
public:
    enum StateDuration { PERMANENT, TEMPORARY };
    
    struct DisplayCache
    {
        inline void newMessage(const String& message_);
        
        void add(const float value_);
        void add(const int value_);
        void add(const String& value_);
        void add(String* value_, const size_t size_);
        
        void createRows();
        inline void clear();
        void print();
        
        String message;
        std::vector<String> strings;
        std::vector<float> floats;
        std::vector<int> ints;
        std::vector<String> rows;

    } displayCache;
    
    Display() {}
    
    ~Display() {}
    
    void setup(Menu::Page* presetPage_);
                
    bool update(const bool withConsole_ = false);
    
    void parameterCalledDisplay(AudioParameter* param_) override;
    void menuPageChanged(Menu::Page* page_);
    
    void displaySlideParameter(AudioParameter* param_);
    void displayChoiceParameter(AudioParameter* param_);
    void displayButtonParameter(AudioParameter* param_);
    void displayMenuPage(Menu::Page* page_);
    void displayPreset();
    
    void refreshResetDisplayCounter() { resetDisplayCounter = DISPLAY_AUTOHOMESCREEN; }
    
    StateDuration getStateDuration() const { return stateDuration; }
    
    AudioParameter* getTemporaryParameter() const { return tempParameter; }
    
private:
#ifdef BELA_CONNECTED
    OscSender oscTransmitter;
#endif
    StateDuration stateDuration = TEMPORARY;
    
    AudioParameter* tempParameter = nullptr;
    
    unsigned int resetDisplayCounter = 0;
    bool newMessageCache = false;
    
    Menu::Page* presetPage = nullptr;
};

#endif /* display_hpp */



// MARK: - LED
// ********************************************************************************

// Statemachine:
// VALUE: LED only represents value of connected parameter
// VALUEFOCUS: LED represents value of connected parameter and blinks (effect edit focus)
// ALERT: LED blinks LED_NUM_BLINK times and sets back to previous state afterwards (i.e. preset change)
// BLINKONCE: LED blinks once ands returns to previous state afterwards

class LED : public UIElement::Listener , public AudioParameter::Listener
{
public:
    LED() {}
    ~LED() {}
    
    void setup(const String& id_);

    void setValue(const float value_) { value = value_; }
    
    void alert();
    
    void blinkOnce();
    
    float getValue();
    
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
