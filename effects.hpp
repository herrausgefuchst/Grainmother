#ifndef effects_hpp
#define effects_hpp

#include "functions.h"
#include "parameters.hpp"

// MARK: - EFFECT
// ********************************************************************************

class Effect
{
public:
    enum Types { BEATREPEAT, GRANULATOR, DELAY };
    
    Effect() = delete;
    Effect (AudioParameterGroup* _engineparameters, const String _name);
    virtual ~Effect() {}
    
    virtual void setup (const float _fs = 44100.f, const int _blocksize = 128);
    virtual StereoFloat process (const StereoFloat _input) = 0;
    virtual void processBlock() = 0;
            
    AudioParameterGroup* getParameterGroup() { return &parameters; }
    AudioParameter* getParameter (int _index) { return parameters.getParameter(_index); }
    AudioParameter* getParameter (String _id) { return parameters.getParameter(_id); }
        
protected:
    float fs = 44100.f;
    int blocksize = 128;
    AudioParameterGroup parameters;
    AudioParameterGroup* engineparameters = nullptr;
};


// MARK: - BEATREPEAT
// ********************************************************************************

static const float triggerlength[12] = { 0.03125f, 0.0625f, 0.125f, 0.25f, 0.5f, 1.f, 1.25f, 1.5f, 1.75f, 2.f, 3.f, 4.f };
static const float slicelength[16] = { 0.00390625f, 0.0078125f, 0.010416666666667f, 0.015625f, 0.020833333333333f, 0.03125f, 0.041666666666667f, 0.0625f, 0.083333333333333f, 0.125f, 0.166666666666667f, 0.25f, 0.333333333333333f, 0.5f, 0.75f, 1.f };
static const float gatelength[22] = { 0.0625f, 0.125f, 0.1875f, 0.25f, 0.3125f, 0.375f, 0.4375f, 0.5f, 0.5625f, 0.625f, 0.6875f, 0.75f, 0.8125f, 0.875f, 0.9375f, 1.f, 1.25f, 1.5f, 1.75f, 2.f, 3.f, 4.f };

class Beatrepeat : public Effect
{
public:
    enum Parameters { SLICELENGTH, GATE, TRIGGER, CHANCE, VARIATION, PITCH, PITCHDECAY, MIX, FREEZE };
    
    Beatrepeat (AudioParameterGroup* _engineparameters, const String _name = "Beatrepeat")
        : Effect(_engineparameters, _name) {}
    ~Beatrepeat() {}
    
    void setup (const float _fs = 44100.f, const int _blocksize = 128) override;
    StereoFloat process (const StereoFloat _input) override;
    void processBlock() override;
    
private:
    void initializeParameters();
    void initializeListeners();
};


// MARK: - GRANULATOR
// ********************************************************************************

class Granulator : public Effect
{
public:
    enum Parameters { GRAN1, GRAN2, GRAN3, GRAN4, GRAN5, GRAN6, GRAN7, GRAN8, GRAN9 };
    
    Granulator (AudioParameterGroup* _engineparameters, const String _name = "Granulator")
        : Effect(_engineparameters, _name) {}
    ~Granulator() {}
    
    void setup (const float _fs = 44100.f, const int _blocksize = 128) override;
    StereoFloat process (const StereoFloat _input) override;
    void processBlock() override;
    
private:
    void initializeParameters();
    void initializeListeners();
};


// MARK: - DELAY
// ********************************************************************************

class Delay : public Effect
{
public:
    enum Parameters { GRAN1, GRAN2, GRAN3, GRAN4, GRAN5, GRAN6, GRAN7, GRAN8, GRAN9 };
    
    Delay (AudioParameterGroup* _engineparameters, const String _name = "Delay")
        : Effect(_engineparameters, _name) {}
    ~Delay() {}
    
    void setup (const float _fs = 44100.f, const int _blocksize = 128) override;
    StereoFloat process (const StereoFloat _input) override;
    void processBlock() override;

private:
    void initializeParameters();
    void initializeListeners();
};

#endif /* effects_hpp */
