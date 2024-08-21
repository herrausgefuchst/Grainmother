#include "engine.hpp"

// =======================================================================================
// MARK: - AUDIO ENGINE
// =======================================================================================

AudioEngine::AudioEngine() 
    : engineParameters("engine", AudioParameterGroup::Type::ENGINE, NUM_ENGINEPARAMETERS)
{}

void AudioEngine::setup(const float sampleRate_, const unsigned int blockSize_)
{
    // Member Variables
    sampleRate = sampleRate_;
    blockSize = blockSize_;
    
    // engine parameters
    {
        // tempo
        engineParameters.addParameter(10u, engineParameterID[ENUM2INT(EngineParameters::TEMPO)],
                                      engineParameterName[ENUM2INT(EngineParameters::TEMPO)],
                                      "bpm", -300.f, 300.f, 8.f, 60.f, sampleRate);

        // global bypass
        engineParameters.addParameter(11u, engineParameterID[ENUM2INT(EngineParameters::GLOBALBYPASS)],
                                      engineParameterName[ENUM2INT(EngineParameters::GLOBALBYPASS)],
                                      ButtonParameter::COUPLED, { "OFF", "ON" });

        // effect bypasses
        engineParameters.addParameter(12u, engineParameterID[ENUM2INT(EngineParameters::EFFECT1BYPASS)],
                                      engineParameterName[ENUM2INT(EngineParameters::EFFECT1BYPASS)],
                                      { "OFF", "ON" }, ParameterTypes::TOGGLE);
        engineParameters.addParameter(13u, engineParameterID[ENUM2INT(EngineParameters::EFFECT2BYPASS)],
                                      engineParameterName[ENUM2INT(EngineParameters::EFFECT2BYPASS)],
                                      { "OFF", "ON" }, ParameterTypes::TOGGLE);
        engineParameters.addParameter(14u, engineParameterID[ENUM2INT(EngineParameters::EFFECT3BYPASS)],
                                      engineParameterName[ENUM2INT(EngineParameters::EFFECT3BYPASS)],
                                      { "OFF", "ON" }, ParameterTypes::TOGGLE);

        // effect edit focus
        engineParameters.addParameter(15u, engineParameterID[ENUM2INT(EngineParameters::EFFECTEDITFOCUS)],
                                      engineParameterName[ENUM2INT(EngineParameters::EFFECTEDITFOCUS)],
                                      { "Reverb", "Granulator", "Resonator" }, ParameterTypes::CHOICE);
        
        // effect order
        engineParameters.addParameter(16u, engineParameterID[ENUM2INT(EngineParameters::EFFECTORDER)],
                                      engineParameterName[ENUM2INT(EngineParameters::EFFECTORDER)], {
            "1->2->3",
            "2|3->1",
            "1|3->2",
            "1|2->3",
            "3->1|2",
            "2->1|3",
            "1->2|3",
            "1|2|3",
            "3->2->1",
            "3->1->2",
            "2->3->1",
            "2->1->3",
            "1->3->2"
        }, ParameterTypes::CHOICE);
    }
    
    // Effects
    effects.at(0) = std::make_unique<Reverb>(&engineParameters, GrainmotherReverb::NUM_PARAMETERS, "reverb", sampleRate, blockSize);
    effects.at(1) = std::make_unique<Granulator>(&engineParameters, GrainmotherGranulator::NUM_PARAMETERS, "granulator", sampleRate, blockSize);
    effects.at(2) = std::make_unique<Resonator>(&engineParameters, 8, "resonator", sampleRate, blockSize);
    
    effects.at(0)->setup();
    effects.at(1)->setup();
    
    // add all Parameters to a vector of AudioParameterGroups which holds all Program Parameters
    programParameters.at(0) = (&engineParameters);
    for (unsigned int n = 1; n < NUM_EFFECTS+1; ++n)
        programParameters.at(n) = effects.at(n-1)->getEffectParameterGroup();
    
    // Tempo & Metronome
    tempoTapper.setup(engineParameters.getParameter("tempo")->getMin(), engineParameters.getParameter("tempo")->getMax(), sampleRate);
    metronome.setup(sampleRate, engineParameters.getParameter("tempo")->getValueAsFloat());
}

StereoFloat AudioEngine::processAudioSamples(const StereoFloat input_)
{
    // Tempotapper
    if (tempoTapper.process()) getParameter("tempo")->setValue(tempoTapper.getBPM());
    
    // Metronome
    metronome.process();
    
    // Effects
    StereoFloat output = input_;
//    if (getParameter(AudioParameterGroup::ENGINE, BYPASS)->getValueAsInt() == ButtonParameter::UP)
//    {
//        if (engineParameters.getParameter(BEATREPEAT)->getValueAsInt() == ButtonParameter::DOWN)
//            output = effects[Effect::BEATREPEAT]->process(output);
//
//        if (engineParameters.getParameter(GRANULATOR)->getValueAsInt() == ButtonParameter::DOWN)
//            output = effects[Effect::GRANULATOR]->process(output);
//
//        if (engineParameters.getParameter(DELAY)->getValueAsInt() == ButtonParameter::DOWN)
//            output = effects[Effect::DELAY]->process(output);
//    }
    
    return output;
    
    // TODO: order of effects
}

void AudioEngine::updateAudioBlock()
{
//    if (getParameter(AudioParameterGroup::ENGINE, BYPASS)->getValueAsInt() == ButtonParameter::UP)
//    {
//        if (engineParameters.getParameter(BEATREPEAT)->getValueAsInt() == ButtonParameter::DOWN)
//            effects[Effect::BEATREPEAT]->processBlock();
//
//        if (engineParameters.getParameter(GRANULATOR)->getValueAsInt() == ButtonParameter::DOWN)
//            effects[Effect::GRANULATOR]->processBlock();
//
//        if (engineParameters.getParameter(DELAY)->getValueAsInt() == ButtonParameter::DOWN)
//            effects[Effect::DELAY]->processBlock();
//    }
}

AudioParameter* AudioEngine::getParameter(const String parameterID_)
{
    AudioParameter* parameter = nullptr;
    
    for (auto i : programParameters)
    {
        parameter = i->getParameter(parameterID_);
        if (parameter) break;
    }
    
    if (!parameter)
        engine_rt_error( "AudioEngine couldnt find Parameter with ID " + parameterID_, __FILE__, __LINE__, false);
    
    return parameter;
}

AudioParameter* AudioEngine::getParameter(const unsigned int paramgroup_, const unsigned int paramindex_)
{
    AudioParameter* parameter = programParameters[paramgroup_]->getParameter(paramindex_);
    
    if (!parameter)
        engine_rt_error("AudioEngine couldnt find Parameter with index " + TOSTRING(paramindex_) + " in Parametergroup " + TOSTRING(paramgroup_), __FILE__, __LINE__, false);
    
    return parameter;
}


AudioParameter* AudioEngine::getParameter(const String paramGroup_, const String paramID_)
{
    AudioParameterGroup* parametergroup = nullptr;
    
    for (unsigned int n = 0; n < programParameters.size(); ++n)
    {
        if (programParameters[n]->getID() == paramGroup_)
        {
            parametergroup = programParameters[n];
            break;
        }
    }
    
    if (!parametergroup)
        engine_rt_error("AudioEngine couldnt find ParameterGroup with index " + paramGroup_, __FILE__, __LINE__, true);
    
    return parametergroup->getParameter(paramID_);
}


AudioParameter* AudioEngine::getParameter(const String& paramGroup_, const uint paramIndex_)
{
    AudioParameterGroup* parametergroup = nullptr;
    
    for (unsigned int n = 0; n < programParameters.size(); ++n)
    {
        if (programParameters[n]->getID() == paramGroup_)
        {
            parametergroup = programParameters[n];
            break;
        }
    }
    
    if (!parametergroup)
        engine_rt_error("AudioEngine couldnt find ParameterGroup with index " + paramGroup_, __FILE__, __LINE__, true);
    
    return parametergroup->getParameter(paramIndex_);
}


Effect* AudioEngine::getEffect(const unsigned int index_)
{
    if (index_ > effects.size()-1)
        engine_rt_error("Audio Engine holds no Effect with Index " + TOSTRING(index_), __FILE__, __LINE__, true);
    
    if (effects.size() == 0)
        engine_rt_error("Audio Engine holds no Effects", __FILE__, __LINE__, true);
    
    auto effect = effects.at(index_).get();
    
    if (!effect)
        engine_rt_error("Audio Engine can't find effect", __FILE__, __LINE__, true);
    
    return effect;
}


// =======================================================================================
// MARK: - USER INTERFACE
// =======================================================================================


void UserInterface::setup(AudioEngine* engine_)
{
    engine = engine_;
  
    initializeUIElements();
    
    initializeMenu();
    
    display.setup(menu.getPage("load_preset"));
    
    initializeListeners();
    
    alertLEDs();
    
    // need to tell the effect LEDs which effect is currently focused
    AudioParameter* effecteditfocus = engine->getParameter("effect_edit_focus");
    int focus = effecteditfocus->getValueAsInt();
    if (focus == 0) led[LED_FX1].parameterChanged(effecteditfocus);
    else if (focus == 1) led[LED_FX2].parameterChanged(effecteditfocus);
    else if (focus == 2) led[LED_FX3].parameterChanged(effecteditfocus);
}


void UserInterface::initializeUIElements()
{
    button[ButtonID::FX1].setup(ButtonID::FX1, "Effect 1");
    button[ButtonID::FX2].setup(ButtonID::FX2, "Effect 2");
    button[ButtonID::FX3].setup(ButtonID::FX3, "Effect 3");
    button[ButtonID::ACTION].setup(ButtonID::ACTION, "Action");
    button[ButtonID::TEMPO].setup(ButtonID::TEMPO, "Tempo");
    button[ButtonID::BYPASS].setup(ButtonID::BYPASS, "Bypass");
    button[ButtonID::UP].setup(ButtonID::UP, "Up");
    button[ButtonID::DOWN].setup(ButtonID::DOWN, "Down");
    button[ButtonID::EXIT].setup(ButtonID::EXIT, "Exit");
    button[ButtonID::ENTER].setup(ButtonID::ENTER, "Enter");
    
    potentiometer[0].setup(0, "Potentiometer 0");
    potentiometer[1].setup(1, "Potentiometer 1");
    potentiometer[2].setup(2, "Potentiometer 2");
    potentiometer[3].setup(3, "Potentiometer 3");
    potentiometer[4].setup(4, "Potentiometer 4");
    potentiometer[5].setup(5, "Potentiometer 5");
    potentiometer[6].setup(6, "Potentiometer 6");
    potentiometer[7].setup(7, "Potentiometer 7");
    
    led[LED_FX1].setup("effect1");
    led[LED_FX2].setup("effect2");
    led[LED_FX3].setup("effect3");
    led[LED_ACTION].setup("action");
    led[LED_TEMPO].setup("tempo");
    led[LED_BYPASS].setup("bypass");
}


void UserInterface::initializeMenu()
{
    menu.addPage<Menu::ParameterPage>("effect_order", engine->getParameter("effect_order"));
    
    menu.addPage<Menu::ParameterPage>("reverb_lowcut", engine->getParameter("reverb", "reverb_lowcut"));
    menu.addPage<Menu::ParameterPage>("reverb_multfreq", engine->getParameter("reverb", "reverb_multfreq"));
    menu.addPage<Menu::ParameterPage>("reverb_multgain", engine->getParameter("reverb", "reverb_multgain"));
    
    menu.setup(engine->getProgramParameters());
}


void UserInterface::initializeListeners()
{
//    // Buttons -> Parameters
    button[ButtonID::FX1].addListener(engine->getParameter("effect1_bypass"));
    button[ButtonID::FX2].addListener(engine->getParameter("effect2_bypass"));
    button[ButtonID::FX3].addListener(engine->getParameter("effect3_bypass"));
    button[ButtonID::BYPASS].addListener(engine->getParameter("global_bypass"));

    // Buttons -> Menu
    button[ButtonID::UP].addListener(&menu);
//    button[ButtonID::UP].onClick.push_back([this] { nudgeTempo(+1); });
    button[ButtonID::DOWN].addListener(&menu);
//    button[ButtonID::DOWN].onClick.push_back([this] { nudgeTempo(-1); });
    button[ButtonID::EXIT].addListener(&menu);
    button[ButtonID::ENTER].addListener(&menu);
//    button[ButtonID::TEMPO].onClick.push_back([this] { engine->getTempoTapper()->tapTempo(); });
    
    button[ButtonID::UP].onClick.push_back([this] { nudgeUIParameter(1); });
    button[ButtonID::DOWN].onClick.push_back([this] { nudgeUIParameter(-1); });
    
    button[ButtonID::UP].onPress.push_back([this] { startScrollingUIParameter(1); });
    button[ButtonID::DOWN].onPress.push_back([this] { startScrollingUIParameter(-1); });
    
    button[ButtonID::UP].onRelease.push_back([this] { stopScrollingUIParameter(); });
    button[ButtonID::DOWN].onRelease.push_back([this] { stopScrollingUIParameter(); });
    
    button[ButtonID::ENTER].onPress.push_back([this] { setDefaultUIParameter(); });
    
    // Buttons -> Effect Edit Focus
    button[ButtonID::FX1].onPress.push_back([this] {  engine->getParameter("effect_edit_focus")->setValue(0); });
    button[ButtonID::FX2].onPress.push_back([this] {  engine->getParameter("effect_edit_focus")->setValue(1); });
    button[ButtonID::FX3].onPress.push_back([this] {  engine->getParameter("effect_edit_focus")->setValue(2); });
    engine->getParameter("effect_edit_focus")->onChange.push_back([this] { setEffectEditFocus(); });
    
    // set the current effect edit focus
    // Potentiometers -> Current Effect-parameters
    // needs to live here, because the parameter must be made first listener of potentiometer!
    setEffectEditFocus();
    
    // Potentiometers -> LED
    for (uint n = 0; n < NUM_POTENTIOMETERS; ++n) potentiometer[n].addListener(&led[LED_ACTION]);
    
    // Parameters -> Display
    engine->getParameter("tempo")->addListener(&display);
    for (unsigned int n = 0; n < GrainmotherReverb::NUM_PARAMETERS; ++n) engine->getParameter("reverb", n)->addListener(&display);
    for (unsigned int n = 0; n < GrainmotherGranulator::NUM_PARAMETERS; ++n) engine->getParameter("granulator", n)->addListener(&display);
    //TODO: add Resonator
    
    // Parameters -> LEDs
    engine->getParameter("global_bypass")->addListener(&led[LED_BYPASS]);
    engine->getParameter("effect1_bypass")->addListener(&led[LED_FX1]);
    engine->getParameter("effect2_bypass")->addListener(&led[LED_FX2]);
    engine->getParameter("effect3_bypass")->addListener(&led[LED_FX3]);
    engine->getParameter((uint)ParameterGroupID::REVERB, NUM_POTENTIOMETERS)->addListener(&led[LED_ACTION]);
    engine->getParameter((uint)ParameterGroupID::GRANULATOR, NUM_POTENTIOMETERS)->addListener(&led[LED_ACTION]);
    //TODO: add Resonator
    engine->getParameter("effect_edit_focus")->addListener(&led[LED_FX1]);
    engine->getParameter("effect_edit_focus")->addListener(&led[LED_FX2]);
    engine->getParameter("effect_edit_focus")->addListener(&led[LED_FX3]);
//
//    // Parameter -> Metronome
//    engine->getParameter("tempo")->addListener(engine->getMetronome());
//    
//    // Metronome -> LED
//    engine->getMetronome()->onTic.push_back([this] { led[LED::TEMPO].setBlinkOnce(); });
//    
    // Menu -> Display
    menu.onPageChange = [this] { display.menuPageChanged(menu.getCurrentPage()); };
    
    // UserInterface -> Menu
    menu.addListener(this);
    
    // Menu -> JSON
    menu.onLoadMessage.push_back( [this] { alertLEDs(); } );
}


void UserInterface::processNonAudioTasks()
{
    if (menu.isScrolling) menu.scroll();
    
    if (scrollingParameter)
    {
        scrollingParameter->scroll(scrollingDirection);
        potentiometer[scrollingParameter->getIndex()].decouple(scrollingParameter->getNormalizedValue());
    }
}


void UserInterface::globalSettingChanged(Menu::Page* page_)
{
    if (page_->getID() == "pot_behaviour")
    {
        rt_printf("Pot Behaviour will be changed!\n");
        Potentiometer::setPotBevaviour(INT2ENUM(page_->getCurrentChoice(), PotBehaviour));
    }
    
    //TODO: midi in
    //TODO: midi out
    
    alertLEDs();
}


void UserInterface::effectOrderChanged()
{
    rt_printf("Effect Order will be changed!\n");
    // TODO: effect order algorithm
    
    alertLEDs();
}


void UserInterface::setEffectEditFocus()
{
    // get a pointer to the effect-edit-focus-parameter
    auto focus = engine->getParameter("effect_edit_focus");
    
    // retrieve the index of the focused effect from parameter
    auto effect = engine->getEffect(focus->getValueAsInt());
    
    // for all potentiometers:
    for (unsigned int n = 0; n < NUM_POTENTIOMETERS; n++)
    {
        // focus the corresponding effect parameter
        potentiometer[n].focusListener(effect->getParameter(n));
        
        // retrieve the current normalized value of the parameter
        float normalizedValue = effect->getParameter(n)->getNormalizedValue();
        
        // assign the normalized value to the potentiometers cache
        // decouple any input source from potentiometer
        potentiometer[n].decouple(normalizedValue);
    }
    
    // for the effect button: 
    // focus corresponding effect parameter
    button[ButtonID::ACTION].focusListener(effect->getParameter(NUM_POTENTIOMETERS));
    
    // notify button-led that the parameter changed
    led[LED_ACTION].parameterChanged(effect->getParameter(NUM_POTENTIOMETERS));
}


void UserInterface::nudgeTempo(const int direction_)
{
    if (button[ButtonID::TEMPO].getPhase() == Button::LOW)
    {
//        menu.setBypass(true);
        engine->getParameter("tempo")->nudgeValue(direction_);
    }
}


void UserInterface::nudgeUIParameter(const int direction_)
{
    if (display.getStateDuration() == Display::TEMPORARY)
    {
        menu.onHold = true;
        display.refreshResetDisplayCounter();
        AudioParameter* param = display.getTemporaryParameter();
        if (param)
        {
            param->nudgeValue(direction_);
            potentiometer[param->getIndex()].decouple(param->getNormalizedValue());
        }
    }
}


void UserInterface::startScrollingUIParameter(const int direction_)
{
    if (display.getStateDuration() == Display::TEMPORARY)
    {
        menu.onHold = true;
        
        display.refreshResetDisplayCounter();
        
        scrollingParameter = display.getTemporaryParameter();
            
        scrollingDirection = direction_;
    }
}


void UserInterface::stopScrollingUIParameter()
{
    if (scrollingParameter)
    {
        scrollingParameter = nullptr;
    }
}


void UserInterface::setDefaultUIParameter()
{
    if (display.getStateDuration() == Display::TEMPORARY)
    {
        menu.onHold = true;
        
        display.refreshResetDisplayCounter();
        
        AudioParameter* param = display.getTemporaryParameter();
        
        if (param)
        {
            param->setDefault();
            
            potentiometer[param->getIndex()].decouple(param->getNormalizedValue());
        }
    }
}


void UserInterface::alertLEDs()
{
    // LED-notification
    for (unsigned int n = 0; n < NUM_LEDS; ++n)
        led[n].alert();
}
