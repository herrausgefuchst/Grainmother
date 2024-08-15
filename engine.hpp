#ifndef engine_hpp
#define engine_hpp

#include <fstream>

#include "functions.h"
#include "globals.h"
#include "uielements.hpp"
#include "effects.hpp"
#include "parameters.hpp"
#include "menu.hpp"
#include "outputs.hpp"

#ifdef JSON_USED
#include "json.h"
using json = nlohmann::json;
#endif

// MARK: - AUDIO ENGINE
// =======================================================================================

class AudioEngine
{
public:
    enum Parameters { TEMPO, BYPASS, BEATREPEAT, GRANULATOR, DELAY, FXFOCUS };
    
    AudioEngine();
    ~AudioEngine();
    
    void setup (const float _fs, const int _blocksize);
    StereoFloat process (const StereoFloat _input);
    void processBlock();
    
    AudioParameter* getParameter (const String _parameterID);
    AudioParameter* getParameter (const int _paramgroup, const int _paramindex);
    
    std::array<AudioParameterGroup*, 4> getProgramParameters() { return programParameters; }
    
    Effect* getEffect (const int _index);
    TempoTapper* getTempoTapper() { return &tempotapper; }
    Metronome* getMetronome() { return &metronome; }
        
private:
    std::array<Effect*, 3> effects;
    std::array<AudioParameterGroup*, 4> programParameters;
    AudioParameterGroup engineParameters;
    
    TempoTapper tempotapper;
    Metronome metronome;
    
    float fs;
    int blocksize;
};


// MARK: - USER INTERFACE
// =======================================================================================

class UserInterface
{
public:
    UserInterface();
    ~UserInterface();
    
    void setup(AudioEngine* _engine);
    
    void savePresetToJSON (const int _index = -1);
    
    void loadPresetFromJSON (const int _index = -1);
    
    void setEffectEditFocus (const bool _withNotification = true);
    void nudgeTempo (const int _direction);
    
    int getNumButtons() const { return NUM_BUTTONS; }
    int getNumPotentiometers() const { return NUM_POTENTIOMETERS; }
    int getNumLEDs() const { return NUM_LEDS; }
        
private:
    inline void initializeJSON();
    inline void initializeGlobalParameters();
    inline void initializeListeners();
    
    AudioEngine* engine = nullptr;
    GlobalParameters globals;
    Menu menu;
    
#ifdef JSON_USED
    json JSONpresets;
    json JSONglobals;
#endif

public:
    Button button[NUM_BUTTONS];
    Potentiometer potentiometer[NUM_POTENTIOMETERS];
    LED led[NUM_LEDS];
    Display display;
};

#endif /* engine_hpp */
