#include "engine.hpp"

// MARK: - AUDIO ENGINE
// =======================================================================================

AudioEngine::AudioEngine() : engineParameters("Engine", AudioParameterGroup::Type::ENGINE)
{}

void AudioEngine::setup(const float _fs, const int _blocksize)
{
    // Member Variables
    fs = _fs;
    blocksize = _blocksize;
    
    // Parameters
    engineParameters.addParameter("tempo", "Tempo", "BPM", 20.f, 300.f, 0.f, 60.f);
    engineParameters.addParameter("globalbypass", "Global Bypass", ButtonParameter::COUPLED);
    engineParameters.addParameter("beatrepeat", "Beatrepeat", ButtonParameter::TOGGLE);
    engineParameters.addParameter("granulator", "Granulator", ButtonParameter::TOGGLE);
    engineParameters.addParameter("delay", "Delay", ButtonParameter::TOGGLE);
    const String choices[] = { "Beatrepeat", "Granulator", "Delay" };
    engineParameters.addParameter("effecteditfocus", "Effect Edit Focus", 3, choices);
    
    // Effects
    effects.at(0) = new Beatrepeat(&engineParameters, "Beatrepeat");
    effects.at(1) = new Granulator(&engineParameters, "Granulator");
    effects.at(2) = new Delay(&engineParameters, "Delay");
    
    effects.at(0)->setup(fs, blocksize);
    effects.at(1)->setup(fs, blocksize);
    effects.at(2)->setup(fs, blocksize);
    
    // add all Parameters to a vector of AudioParameterGroups which holds all Program Parameters
    programParameters.at(0) = (&engineParameters);
    for (unsigned int n = 0; n < 3; n++)
        programParameters.at(n+1) = effects.at(n)->getParameterGroup();
    
    // Tempo & Metronome
    tempotapper.setup(engineParameters.getParameter("tempo")->getMin(), engineParameters.getParameter("tempo")->getMax(), fs);
    metronome.setup(fs, engineParameters.getParameter("tempo")->getValueF());
}

AudioEngine::~AudioEngine()
{
    for (auto i : effects) delete i;
}

StereoFloat AudioEngine::process (const StereoFloat _input)
{
    // Tempotapper
    if (tempotapper.process()) getParameter("tempo")->setValue(tempotapper.getBPM());
    
    // Metronome
    metronome.process();
    
    // Effects
    StereoFloat output = _input;
    if (getParameter(AudioParameterGroup::ENGINE, BYPASS)->getValueI() == ButtonParameter::UP)
    {
        if (engineParameters.getParameter(BEATREPEAT)->getValueI() == ButtonParameter::DOWN)
            output = effects[Effect::BEATREPEAT]->process(output);

        if (engineParameters.getParameter(GRANULATOR)->getValueI() == ButtonParameter::DOWN)
            output = effects[Effect::GRANULATOR]->process(output);

        if (engineParameters.getParameter(DELAY)->getValueI() == ButtonParameter::DOWN)
            output = effects[Effect::DELAY]->process(output);
    }
    
    return output;
    
    // TODO: order of effects
}

void AudioEngine::processBlock()
{
    if (getParameter(AudioParameterGroup::ENGINE, BYPASS)->getValueI() == ButtonParameter::UP)
    {
        if (engineParameters.getParameter(BEATREPEAT)->getValueI() == ButtonParameter::DOWN)
            effects[Effect::BEATREPEAT]->processBlock();

        if (engineParameters.getParameter(GRANULATOR)->getValueI() == ButtonParameter::DOWN)
            effects[Effect::GRANULATOR]->processBlock();

        if (engineParameters.getParameter(DELAY)->getValueI() == ButtonParameter::DOWN)
            effects[Effect::DELAY]->processBlock();
    }
}


AudioParameter* AudioEngine::getParameter (const String _parameterID)
{
    AudioParameter* parameter = nullptr;
    
    for (auto i : programParameters)
    {
        parameter = i->getParameter(_parameterID, false);
        if (parameter) break;
    }
    
    if (!parameter)
        engine_rt_error( "AudioEngine couldnt find Parameter with ID " + _parameterID, __FILE__, __LINE__, false);
    
    return parameter;
}

AudioParameter* AudioEngine::getParameter (const int _paramgroup, const int _paramindex)
{
    AudioParameter* parameter = programParameters[_paramgroup]->getParameter(_paramindex);
    
    if (!parameter)
        engine_rt_error("AudioEngine couldnt find Parameter with index " + TOSTRING(_paramindex) + " in Parametergroup " + TOSTRING(_paramgroup), __FILE__, __LINE__, false);
    
    return parameter;
}

Effect* AudioEngine::getEffect (const int _index)
{
    if (_index > effects.size()-1 || _index < 0)
        engine_rt_error("Audio Engine holds no Effect with Index " + std::to_string(_index), __FILE__, __LINE__, true);
    
    if (effects.size() == 0)
        engine_rt_error("Audio Engine holds no Effects", __FILE__, __LINE__, true);
    
    auto effect = effects[_index];
    
    if (!effect)
        engine_rt_error("Audio Engine can't find effect", __FILE__, __LINE__, true);
    
    return effect;
}

// MARK: - USER INTERFACE
// =======================================================================================

void UserInterface::setup(AudioEngine *_engine)
{
    engine = _engine;
  
    button[Button::FX1].setup(Button::FX1, "Effect 1");
    button[Button::FX2].setup(Button::FX2, "Effect 2");
    button[Button::FX3].setup(Button::FX3, "Effect 3");
    button[Button::ACTION].setup(Button::ACTION, "Action");
    button[Button::TEMPO].setup(Button::TEMPO, "Tempo");
    button[Button::BYPASS].setup(Button::BYPASS, "Bypass");
    button[Button::UP].setup(Button::UP, "Up");
    button[Button::DOWN].setup(Button::DOWN, "Down");
    button[Button::EXIT].setup(Button::EXIT, "Exit");
    button[Button::ENTER].setup(Button::ENTER, "Enter");
    
    potentiometer[0].setup(0, "Potentiometer 0");
    potentiometer[1].setup(1, "Potentiometer 1");
    potentiometer[2].setup(2, "Potentiometer 2");
    potentiometer[3].setup(3, "Potentiometer 3");
    potentiometer[4].setup(4, "Potentiometer 4");
    potentiometer[5].setup(5, "Potentiometer 5");
    potentiometer[6].setup(6, "Potentiometer 6");
    potentiometer[7].setup(7, "Potentiometer 7");
    
    led[LED::FX1].setup(LED::FX1, "Effect 1");
    led[LED::FX2].setup(LED::FX2, "Effect 2");
    led[LED::FX3].setup(LED::FX3, "Effect 3");
    led[LED::ACTION].setup(LED::ACTION, "Action");
    led[LED::TEMPO].setup(LED::TEMPO, "Tempo");
    led[LED::BYPASS].setup(LED::BYPASS, "Bypass");
    
    // helper functions for initialization
    initializeJSON();
    initializeGlobalParameters();
//    initializeListeners();
    
    // load last used Preset
    loadPresetFromJSON(globals.lastUsedPreset);
}

UserInterface::~UserInterface()
{
#ifdef JSON_USED
#ifndef BELA_CONNECTED
    // consoleprint - directory
    std::ofstream writefilePresets("/Users/julianfuchs/Desktop/MULTIEFFECT/Multieffect_V0.02_231023/ConsoleCode/presets.json");
    std::ofstream writefileGlobals("/Users/julianfuchs/Desktop/MULTIEFFECT/Multieffect_V0.02_231023/ConsoleCode/globals.json");
#else
    // bela - directory
    std::ofstream writefilePresets("presets.json");
    std::ofstream writefileGlobals("globals.json");
#endif
    
    engine_error(!writefilePresets.is_open(), "presets.json not found, therefore not able to save", __FILE__, __LINE__, true);
    engine_error(!writefileGlobals.is_open(), "globals.json not found, therefore not able to save", __FILE__, __LINE__, true);
    
    JSONglobals["midiInChannel"] = globals.midiInChannel;
    JSONglobals["midiOutChannel"] = globals.midiOutChannel;
    JSONglobals["potBehaviour"] = globals.potBehaviour;
    JSONglobals["lastUsedPreset"] = globals.lastUsedPreset;
    
    writefilePresets << JSONpresets.dump(4);
    writefileGlobals << JSONglobals.dump(4);
#endif
}

inline void UserInterface::initializeJSON()
{
#ifdef JSON_USED
    std::ifstream readfilePresets;
    std::ifstream readfileGlobals;
    
#ifndef BELA_CONNECTED
    readfilePresets.open("/Users/julianfuchs/Desktop/MULTIEFFECT/Multieffect_V0.02_231023/ConsoleCode/presets.json");
    readfileGlobals.open("/Users/julianfuchs/Desktop/MULTIEFFECT/Multieffect_V0.02_231023/ConsoleCode/globals.json");
#else
    readfilePresets.open("presets.json");
    readfileGlobals.open("globals.json");
#endif
    
    engine_error(!readfilePresets.is_open(), "presets.json not found, therefore not able to open", __FILE__, __LINE__, true);
    engine_error(!readfileGlobals.is_open(), "globals.json not found, therefore not able to open", __FILE__, __LINE__, true);
    
    JSONpresets = json::parse(readfilePresets);
    JSONglobals = json::parse(readfileGlobals);
#endif
}

inline void UserInterface::initializeGlobalParameters()
{
#ifdef JSON_USED
    globals.midiInChannel = JSONglobals["midiInChannel"];
    globals.midiOutChannel = JSONglobals["midiOutChannel"];
    globals.lastUsedPreset = JSONglobals["lastUsedPreset"];
    globals.potBehaviour = JSONglobals["potBehaviour"];
    
    for (unsigned int n = 0; n < NUM_PRESETS; n++) globals.presetNames[n] = JSONpresets[n]["name"];
#endif
}

inline void UserInterface::initializeListeners()
{
    // Buttons -> Parameters
    button[Button::FX1].addListener(engine->getParameter("beatrepeat"));
    button[Button::FX2].addListener(engine->getParameter("granulator"));
    button[Button::FX3].addListener(engine->getParameter("delay"));
    button[Button::BYPASS].addListener(engine->getParameter("globalbypass"));
    
    // Buttons -> Menu
    button[Button::UP].addListener(&menu);
    button[Button::UP].onClick.push_back([this] { nudgeTempo(+1); });
    button[Button::DOWN].addListener(&menu);
    button[Button::DOWN].onClick.push_back([this] { nudgeTempo(-1); });
    button[Button::EXIT].addListener(&menu);
    button[Button::ENTER].addListener(&menu);
    button[Button::TEMPO].onClick.push_back([this] { engine->getTempoTapper()->tapTempo(); });
    
    // Buttons -> Effect Edit Focus
    button[Button::FX1].onPress.push_back([this] {  engine->getParameter("effecteditfocus")->setValue(0); });
    button[Button::FX2].onPress.push_back([this] {  engine->getParameter("effecteditfocus")->setValue(1); });
    button[Button::FX3].onPress.push_back([this] {  engine->getParameter("effecteditfocus")->setValue(2); });
    engine->getParameter("effecteditfocus")->onChange.push_back([this] { setEffectEditFocus(); });
    
    // ! DISPLAY MUST BE FIRST LISTENER OF EACH PARAMETER !
    // Parameters -> Display
    engine->getParameter("tempo")->addListener(&display);
    engine->getParameter("globalbypass")->addListener(nullptr);
    engine->getParameter("beatrepeat")->addListener(nullptr);
    engine->getParameter("granulator")->addListener(nullptr);
    engine->getParameter("delay")->addListener(nullptr);
    engine->getParameter("effecteditfocus")->addListener(nullptr);
    for (unsigned int n = 0; n < 9; n++) engine->getParameter(AudioParameterGroup::BEATREPEAT, n)->addListener(&display);
    for (unsigned int n = 0; n < 9; n++) engine->getParameter(AudioParameterGroup::GRANULATOR, n)->addListener(&display);
    //TODO: add Resonator
    
    // Parameters -> LEDs
    engine->getParameter("globalbypass")->addListener(&led[LED::BYPASS]);
    engine->getParameter("beatrepeat")->addListener(&led[LED::FX1]);
    engine->getParameter("granulator")->addListener(&led[LED::FX2]);
    engine->getParameter("delay")->addListener(&led[LED::FX3]);
    engine->getParameter(AudioParameterGroup::BEATREPEAT, NUM_POTENTIOMETERS)->addListener(&led[LED::ACTION]);
    engine->getParameter(AudioParameterGroup::GRANULATOR, NUM_POTENTIOMETERS)->addListener(&led[LED::ACTION]);
    //TODO: add Resonator
    engine->getParameter("effecteditfocus")->addListener(&led[LED::FX1]);
    engine->getParameter("effecteditfocus")->addListener(&led[LED::FX2]);
    engine->getParameter("effecteditfocus")->addListener(&led[LED::FX3]);
    
    // Parameter -> Metronome
    engine->getParameter("tempo")->addListener(engine->getMetronome());
    
    // Metronome -> LED
    engine->getMetronome()->onTic.push_back([this] { led[LED::TEMPO].setBlinkOnce(); });
    
    // Menu -> Display
    menu.addListener(&display);
    
    // Menu -> JSON
    menu.onSaveMessage.push_back( [this] { savePresetToJSON(); } );
    menu.onLoadMessage.push_back( [this] { loadPresetFromJSON(); } );
}

void UserInterface::setEffectEditFocus (const bool _withNotification)
{
    auto focus = engine->getParameter("effecteditfocus");
    auto effect = engine->getEffect(focus->getValueI());
    
    for (unsigned int n = 0; n < NUM_POTENTIOMETERS; n++)
    {
        potentiometer[n].focusListener( effect->getParameter(n) );
        float noramlizedValue = effect->getParameter(n)->getNormalizedValue();
        potentiometer[n].decouple(noramlizedValue);
    }
    
    button[Button::ACTION].focusListener( effect->getParameter(NUM_POTENTIOMETERS) );
    
    led[LED::ACTION].setValue(effect->getParameter(NUM_POTENTIOMETERS)->getValueF());
    
    
}

void UserInterface::nudgeTempo(const int _direction)
{
    if (button[Button::TEMPO].getPhase() == Button::LOW)
    {
        menu.setBypass(true);
        engine->getParameter("tempo")->nudgeValue(_direction);
    }
}

void UserInterface::savePresetToJSON (const int _index)
{
#ifdef JSON_USED
    // -- id
    int index = _index == -1 ? menu.getCurrentChoice() : _index;
    
    if (index >= NUM_PRESETS)
        engine_rt_error("the chosen preset index (" + TOSTRING(index) + ") exceeds the max number of presets (" + TOSTRING(NUM_PRESETS) + ")",
                        __FILE__, __LINE__, true);
    
    // -- name
    JSONpresets[index]["name"] = getDateAsString() + " Preset No. " + TOSTRING(index);
    menu.setNewPresetName(JSONpresets[index]["name"]);
    
    // -- engine parameters
    auto parameters = engine->getProgramParameters();
    auto engineP = parameters[AudioParameterGroup::ENGINE];
    auto beatrepeatP = parameters[AudioParameterGroup::BEATREPEAT];
    auto granulatorP = parameters[AudioParameterGroup::GRANULATOR];
    auto delayP = parameters[AudioParameterGroup::DELAY];
    
    // - save Data to JSON
    for (unsigned int n = 0; n < engineP->getNumParametersInGroup(); n++)
        JSONpresets[index]["engine"][n] = engineP->getParameter(n)->getPrintValueF();
    
    for (unsigned int n = 0; n < beatrepeatP->getNumParametersInGroup(); n++)
        JSONpresets[index]["beatrepeat"][n] = beatrepeatP->getParameter(n)->getPrintValueF();
    
    for (unsigned int n = 0; n < granulatorP->getNumParametersInGroup(); n++)
        JSONpresets[index]["granulator"][n] = granulatorP->getParameter(n)->getPrintValueF();
    
    for (unsigned int n = 0; n < delayP->getNumParametersInGroup(); n++)
        JSONpresets[index]["delay"][n] = delayP->getParameter(n)->getPrintValueF();
#endif
}

void UserInterface::loadPresetFromJSON (const int _index)
{
    // parameters
    auto parameters = engine->getProgramParameters();
    auto engineP = parameters[AudioParameterGroup::ENGINE];
    auto beatrepeatP = parameters[AudioParameterGroup::BEATREPEAT];
    auto granulatorP = parameters[AudioParameterGroup::GRANULATOR];
    auto delayP = parameters[AudioParameterGroup::DELAY];
    
    // console print yes or no?
    bool withPrint = false;
    
#ifdef JSON_USED
    // index
    int index = _index == -1 ? menu.getCurrentChoice() : _index;
    
    if (index >= NUM_PRESETS || index == -1)
        engine_rt_error("the chosen preset index (" + TOSTRING(_index) + ") exceeds the max number of presets (" + TOSTRING(NUM_PRESETS) + ")",
                     __FILE__, __LINE__, true);
    
    // load from JSON file
    for (unsigned int n = 0; n < engineP->getNumParametersInGroup(); n++)
        engineP->getParameter(n)->setValue((float)JSONpresets[index]["engine"][n], withPrint);
    
    for (unsigned int n = 0; n < beatrepeatP->getNumParametersInGroup(); n++)
        beatrepeatP->getParameter(n)->setValue((float)JSONpresets[index]["beatrepeat"][n], withPrint);
    
    for (unsigned int n = 0; n < granulatorP->getNumParametersInGroup(); n++)
        granulatorP->getParameter(n)->setValue((float)JSONpresets[index]["granulator"][n], withPrint);
    
    for (unsigned int n = 0; n < delayP->getNumParametersInGroup(); n++)
        delayP->getParameter(n)->setValue((float)JSONpresets[index]["delay"][n], withPrint);
    
    // last used preset is now the current one
    globals.lastUsedPreset = index;
    
    // update Display Catch
    display.setPresetCatch(index, JSONpresets[index]["name"]);

#else
//    float defaultresonator[] = { 200.f, 80.f, 0.f, 0.f, 0.f, 2.f, 80.f, 100.f, 1.f };
//    for (unsigned int n = 0; n < resonatorP->getNumParametersInGroup(); n++)
//        resonatorP->getParameter(n)->setValue(defaultresonator[n], withPrint);
    
//    float defaultgranulator[] = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f};
//    for (unsigned int n = 0; n < granulatorP->getNumParametersInGroup(); n++)
//        granulatorP->getParameter(n)->setValue(defaultgranulator[n], withPrint);

    float defaultbeatrepeat[] = { 0.f, 10.f, 3.f, 100.f, 100.f, 0.f, 0.f, 100.f, ButtonParameter::UP };
    for (unsigned int n = 0; n < beatrepeatP->getNumParametersInGroup(); n++)
        beatrepeatP->getParameter(n)->setValue(defaultbeatrepeat[n], withPrint);
    
#endif
    
    // LED-notification
    for (unsigned int n = 0; n < NUM_LEDS; n++)
        led[n].setAlarm();
}
