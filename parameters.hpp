#ifndef parameters_hpp
#define parameters_hpp

#include "uielements.hpp"
#include "helpers.hpp"

// Print Value = value that is going to be printed on display
// Current Value = mostly same as Print Value, but can vary in case of Ramps (Slide Parameter)
// Normalized Value = value between 0..1 as received from GUI or Potentiometers

// MARK: - AUDIO PARAMETER
// *******************************************************************************

class AudioParameter : public UIElement::Listener
{
public:
    AudioParameter() = delete;
    AudioParameter (const String _id, const String _name) : id(_id), name(_name) {}
    
    virtual ~AudioParameter() {}
    
    class Listener
    {
    public:
        virtual ~Listener() {};
        
        virtual void parameterChanged (AudioParameter* _param) {};
    };
    
    void addListener (AudioParameter::Listener* _listener)
    {
        listeners.push_back(_listener);
    }
    
    virtual void notifyListeners (const bool _withPrint) = 0;
    std::vector<std::function<void()>> onChange;
    std::vector<std::function<void()>> onClick;
    std::vector<std::function<void()>> onPress;
    std::vector<std::function<void()>> onRelease;
    
    virtual void process() {}
    
    virtual void setValue (const float _value, const bool _withNotification = true) {}
    virtual void setValue (const int _value, const bool _withNotification = true) {}
    virtual void nudgeValue (const int _direction) {};

    String getName() const { return name; }
    String getParameterID() const { return id; }
    
    virtual float getValueF() const = 0;
    virtual float getPrintValueF() const = 0;
    virtual int getValueI() const = 0;
    virtual int getPrintValueI() const = 0;
    virtual String getPrintValueS() const = 0;
    virtual float getNormalizedValue() const { return 0.f; }
    
    virtual float getMin() const { return -1.f; }
    virtual float getMax() const { return -1.f; }
    virtual float getStep() const { return -1.f; }
    virtual float getRange() const { return -1.f; }
    
protected:
    std::vector<Listener*> listeners;
    const String id, name;
};


// MARK: - CHOICE PARAMETER
// *******************************************************************************

class ChoiceParameter : public AudioParameter
{
public:
    ChoiceParameter (const String _id, const String _name, const int _numChoices, const String* _choices);
    ~ChoiceParameter();
        
    void setValue (const int _value, const bool _withPrint = true) override;
    void setValue (const float _value, const bool _withPrint = true) override;
    
    void potChanged (UIElement* _uielement) override;
    
    void notifyListeners (const bool _withPrint) override;
    
    float getValueF() const override { return (float)getValueI(); }
    float getPrintValueF() const override { return getValueF(); }
    int getValueI() const override { return choice; }
    int getPrintValueI() const override { return getValueI(); }
    String getPrintValueS() const override { return choice_names[choice]; }
    
    int getNumChoices() const { return numChoices; }
    String* getChoiceNames() const { return choice_names; }
    
private:
    const int numChoices = 0;
    int choice = 0;
    String* choice_names;
};


// MARK: - SLIDE PARAMETER
// *******************************************************************************



class SlideParameter    : public AudioParameter
{
public:
    enum Scaling { LIN, FREQ };
    
    SlideParameter (const String _id, const String _name, const String _unit, const float _min, const float _max, const float _step, const float _default, const Scaling _scaling = LIN, const float _ramptime_ms = 0.f);
    ~SlideParameter() {}
    
    void process() override;
    
    void potChanged (UIElement* _uielement) override;
    
    void notifyListeners (const bool _withPrint) override;
    
    void setValue (float _value, const bool _withPrint = true) override;
    void setValue (const int _value, const bool _withPrint = true) override;
    void setNormalizedValue (float _value, const bool _withPrint = true);
    void nudgeValue (const int _direction) override;
    
    float getValueF() const override { return value.getCurrent(); }
    float getPrintValueF() const override { return value.getGoal(); }
    float getNormalizedValue() const override { return normalizedValue; }
    int getValueI() const override { return (int)getValueF(); }
    int getPrintValueI() const override { return (int)getPrintValueF(); }
    
    String getUnit() const { return unit; }
    String getPrintValueS() const override { return TOSTRING(value.getGoal()); }
    
    float getMin() const override { return min; }
    float getMax() const override { return max; }
    float getStep() const override { return step; }
    float getRange() const override { return range; }
    
private:
    const String unit;
    const float min, max, step, range, ramptime_ms;
    const Scaling scaling;
    Ramp value;
    float normalizedValue;
};


// MARK: - BUTTON PARAMETER
// ********************************************************************************

//  Button      Short       Long
//  _________________________________
//  FX 1..3     TOGGLE      no
//  ACTION      TOGGLE      MOMENTARY
//  TEMPO       no          MOMENTARY (no ButtonParameter connected)
//  BYPASS      TOGGLE      MOMENTARY
//  MENUs       no          MOMENTARY (no ButtonParameter coneected)

// "no" means: parameter doesn't need to save a value
// therefore there are three types:
// 1. only toggling
// 2. only momentary
// 3. toggling + momentary = coupled

class ButtonParameter : public AudioParameter
{
public:
    enum Type { TOGGLE, MOMENTARY, COUPLED };
    enum Toggle { UP, DOWN };
        
    ButtonParameter (const String _id, const String _name, const Type _type);
    ~ButtonParameter() {}
        
    void buttonClicked (UIElement* _uielement) override;
    void buttonPressed (UIElement* _uielement) override;
    void buttonReleased (UIElement* _uielement) override;
    
    void notifyListeners (const bool _withPrint) override;
    
    void setValue (const float _value, const bool _withPrint = true) override;
    void setValue (const int _value, const bool _withPrint = true) override;
    
    float getValueF() const override { return (float)getValueI(); }
    float getPrintValueF() const override { return getValueF(); }
    int getValueI() const override { return value; }
    int getPrintValueI() const override { return getValueI(); }
    String getPrintValueS() const override { return TOSTRING(value); } // TODO: enum to string?
    
private:
    const Type type;
    int value = UP;
};


// MARK: - AUDIO PARAMETER GROUP
// *******************************************************************************

class AudioParameterGroup
{
public:
    enum ID { ENGINE, BEATREPEAT, GRANULATOR, DELAY };
    enum class Type { ENGINE, EFFECT };
    
    AudioParameterGroup() = delete;
    AudioParameterGroup (const String _name, const Type _type);
    ~AudioParameterGroup();
        
    void addParameter (const String _id, const String _name, const String _unit, const float _min, const float _max, const float _step, const float _default, const SlideParameter::Scaling _scaling = SlideParameter::LIN, const float _ramptime_ms = 0.f);
    
    void addParameter (const String _id, const String _name, const ButtonParameter::Type _type);
    
    void addParameter (const String _id, const String _name, const int _numChoices, const String* _array);
    
    void addParameter(const String _id, const String _name, std::initializer_list<String> choices);
    
    AudioParameter* getParameter (const int _index);
    
    AudioParameter* getParameter (const String _id, const bool _withErrorMessage = true);
    
    String getName() const { return name; }
    int getNumParametersInGroup() const { return (int)parametergroup.size(); }
    
private:
    std::vector<AudioParameter*> parametergroup;
    const String name;
    const Type type;
};

#endif /* parameters_hpp */
