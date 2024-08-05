#ifndef uielements_hpp
#define uielements_hpp

#include "globals.h"
#include "functions.h"
#include "helpers.hpp"

static const int NUM_BUTTONS = 10;
static const int NUM_POTENTIOMETERS = 8;

// MARK: - UIELEMENT
// ********************************************************************************

class UIElement
{
public:
    UIElement() = delete;
    UIElement (const int _index, const String _name);
    virtual ~UIElement() {}
    
    class Listener
    {
    public:
        virtual ~Listener() {}
        
        virtual void potChanged (UIElement* _uielement) {}
        virtual void buttonClicked (UIElement* _uielement) {}
        virtual void buttonPressed (UIElement* _uielement) {}
        virtual void buttonReleased (UIElement* _uielement) {}
    };
    
    void addListener (Listener* _listener);
    void focusListener (Listener* _listener);
    virtual void notifyListeners (const int _specifier = -1) = 0;
    
    int getIndex() const { return index; }
    String getName() const { return name; }
    
protected:
    std::vector<Listener*> listeners;
    const String name;
    const int index;
};


// MARK: - POTENTIOMETER
// ********************************************************************************

class Potentiometer : public UIElement
{
public:
    enum ListenTo { GUI, ANALOG, MIDI, NONE};
    
    Potentiometer (const int _index, const String _name, GlobalParameters* _parameters, const float _guidefault = 0.f, const float _analogdefault = 0.f);
    ~Potentiometer() {}
        
    void update (const float _guivalue, const float _analogvalue = 0.f);
    
    void setNewMIDIMessage (const float _midivalue);
    
    void notifyListeners (const int _specifier = -1) override;
    std::vector<std::function<void()>> onChange;
    
    void setValue (const float _value);
    void decouple (const float _newcurrent);
    
    float getValue() const { return current; }
    float getLastValue() const { return last; }
    
private:
    GlobalParameters* globalparameters = nullptr;
    
    float current = 0.f;
    float last = 0.f;
    
    float gui_cache, analog_cache, analog_average;
    float analog_history[8] = { 0.f };
    int analog_ptr = 0;
    
    int listen = NONE;
    
    static const float CATCHING_POTENTIOMETER_TOLERANCE;
    static const float POT_NOISE;
    static const float MAX_VOLTAGE;
};


// MARK: - BUTTON
// ********************************************************************************

// all incoming Buttons should be of Type Momentary
// if Button receives new values in update():
// - case guivalue: need to determine wether its a click, press or release message
// - case analog: debounce first, then determine wether its a click, press or release message

class Button : public UIElement
{
public:
    enum Phase { LOW, HIGH };
    enum Action { CLICK, PRESS, RELEASE };
    enum ID { FX1, FX2, FX3, ACTION, BYPASS, TEMPO, UP, DOWN, EXIT, ENTER };
    
    Button (const int _index, const String _name, const int _guidefault = HIGH, const int _analogdefault = HIGH);
    ~Button() {}
        
    void update (const int _guivalue, const int _analogvalue = HIGH);
            
    void notifyListeners (const int _specifier) override;
    std::vector<std::function<void()>> onClick;
    std::vector<std::function<void()>> onPress;
    std::vector<std::function<void()>> onRelease;
    
    int getPhase() const { return phase; }
    
private:
    int phase = HIGH;
    int analog_cache = HIGH;
    int gui_cache = HIGH;
    
    enum State { JUST_CHANGED, AWAITING_LONGPRESS, NO_ACTION };
    int state = NO_ACTION;
    int state_counter = 0;

    int lastaction = CLICK;
    
    Debouncer debouncer;
    
    static const int DEBOUNCING_UNITS;
    static const int LONGPRESS_UNITS;
};

#endif /* uielements_hpp */
