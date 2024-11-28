#include "Engine.h"

#define CONSOLE_PRINT

// =======================================================================================
// MARK: - AUDIO ENGINE
// =======================================================================================


const uint AudioEngine::RAMP_BLOCKSIZE = 8;
const uint AudioEngine::RAMP_BLOCKSIZE_WRAP = RAMP_BLOCKSIZE - 1;


AudioEngine::AudioEngine() : engineParameters("engine", Engine::NUM_PARAMETERS)
{}


void AudioEngine::setup(const float sampleRate_, const unsigned int blockSize_)
{
    // Assign member variables
    sampleRate = sampleRate_;
    blockSize = blockSize_;
    
    // Initialize engine parameters
    initializeEngineParameters();
    
    // Define the alignment - typically 16 bytes for SIMD types
    constexpr std::size_t alignment = 16;

    // Aligned allocation and object construction for effects
    void* memEffect1 = nullptr;
    void* memEffect2 = nullptr;
    void* memEffect3 = nullptr;
    
    if (posix_memalign(&memEffect1, alignment, sizeof(ReverbProcessor)) != 0) { throw std::bad_alloc(); }
    if (posix_memalign(&memEffect2, alignment, sizeof(GranulatorProcessor)) != 0) { throw std::bad_alloc(); }
    if (posix_memalign(&memEffect3, alignment, sizeof(ResonatorProcessor)) != 0) { throw std::bad_alloc(); }
    
    effectProcessor[0] = new (memEffect1) ReverbProcessor(&engineParameters, Reverberation::NUM_PARAMETERS, "reverb", sampleRate, blockSize);
    effectProcessor[1] = new (memEffect2) GranulatorProcessor(&engineParameters, Granulation::NUM_PARAMETERS, "granulator", sampleRate, blockSize);
    effectProcessor[2] = new (memEffect3) ResonatorProcessor(&engineParameters, 8, "resonator", sampleRate, blockSize);
    
    // The setup functions of the effect processors create a set of parameters and initialize the listener connections.
    // They also initialize the actual effect objects.
    effectProcessor[0]->setup();
    effectProcessor[1]->setup();
    // TODO: Add resonator setup
    
    // Add all parameters to a vector of AudioParameterGroups, which holds all program parameters
    programParameters.at(0) = (&engineParameters);
    for (unsigned int n = 1; n < NUM_EFFECTS + 1; ++n)
        programParameters.at(n) = effectProcessor[n - 1]->getEffectParameterGroup();
    
    // Set up the wet ramp for global bypass control and initialize the corresponding dry multiplier
    globalWet.setup(1.f, sampleRate, RAMP_BLOCKSIZE);
    globalWetCache = globalWet();
    globalDry = 1.f - globalWet();
}


void AudioEngine::initializeEngineParameters()
{
    using namespace Engine;
    
    // tempo
    engineParameters.addParameter<SlideParameter>
    (UIPARAM_SEPCIAL, parameterID[TEMPO], parameterName[TEMPO],
     " bpm", 30.f, 300.f, 1.f, 120.f, sampleRate);

    // global bypass
    engineParameters.addParameter<ButtonParameter>
    (MENUPARAMETER, parameterID[GLOBAL_BYPASS], parameterName[GLOBAL_BYPASS],
     std::initializer_list<String>{ "OFF", "ON" });

    // effect 1, 2, 3 engaged
    engineParameters.addParameter<ToggleParameter>
    (MENUPARAMETER, parameterID[EFFECT1_ENGAGED], parameterName[EFFECT1_ENGAGED],
     std::initializer_list<String>{ "OFF", "ON" });
    engineParameters.addParameter<ToggleParameter>
    (MENUPARAMETER, parameterID[EFFECT2_ENGAGED], parameterName[EFFECT2_ENGAGED],
     std::initializer_list<String>{ "OFF", "ON" });
    engineParameters.addParameter<ToggleParameter>
    (MENUPARAMETER, parameterID[EFFECT3_ENGAGED], parameterName[EFFECT3_ENGAGED],
     std::initializer_list<String>{ "OFF", "ON" });

    // effect edit focus
    engineParameters.addParameter<ChoiceParameter>
    (MENUPARAMETER, parameterID[EFFECT_EDIT_FOCUS], parameterName[EFFECT_EDIT_FOCUS],
     std::initializer_list<String>{ "Reverb", "Granulator", "Resonator" });
    
    // effect order
    engineParameters.addParameter<ChoiceParameter>
    (MENUPARAMETER, parameterID[EFFECT_ORDER], parameterName[EFFECT_ORDER],
     std::initializer_list<String>{
        "1 - 2 - 3",
        "1 | 2 | 3",
        "3 - 2 - 1",
        "3 - 1 - 2",
        "2 - 3 - 1",
        "2 - 1 - 3",
        "1 - 3 - 2"});
    
    // set tempo to?
    engineParameters.addParameter<ChoiceParameter>
    (MENUPARAMETER, parameterID[TEMPO_SET], parameterName[TEMPO_SET],
     std::initializer_list<String>{ "Current Effect", "All Effects" });
    
    engineParameters.addParameter<SlideParameter>(NUM_POTENTIOMETERS-1, parameterID[GLOBAL_MIX], parameterName[GLOBAL_MIX],
                                                  " %", 0.f, 100.f, 0.5f, 70.f, sampleRate);
}


StereoFloat AudioEngine::processAudioSamples(StereoFloat input_, uint sampleIndex_)
{
    // don't process anything if the bypassed flag is set true
    if (bypassed) return input_;
    
    // process the ramp for wetness in a certain rate
    if ((sampleIndex_ & RAMP_BLOCKSIZE_WRAP) == 0) updateRamps();
    
    StereoFloat input = input_;
    StereoFloat output = {0.f, 0.f};
    
    // Counter to keep track of how many effects have been processed.
    uint processedEffects = 0;
    
    // Iterate through all effects in series to apply processing functions.
    for (uint m = 0; m < NUM_EFFECTS; ++m)
    {
        // Iterate through all effects in parallel to apply processing functions.
        for (uint n = 0; n < NUM_EFFECTS; ++n)
        {
            // Check if there is a valid processing function for the current combination of effects.
            if (processFunction[m][n])
            {
                // Apply the processing function to the input, and accumulate the result into 'output'.
                output += processFunction[m][n](input, sampleIndex_);
                
                // Increment the processed effects counter.
                // Exit if all effects have been processed.
                if (++processedEffects == NUM_EFFECTS) break;
            }
        }
        
        // Exit the outer loop if all effects have been processed.
        if (processedEffects == NUM_EFFECTS) break;
        
        else
        {
            // If not all effects have been processed, set the input to the current output.
            input = output;
            
            // Reset the output for the next iteration of processing.
            output = { 0.f, 0.f };
        }
    }

    // Return the final output after applying the global wet/dry mix.
    // The output is mixed with the original input, weighted by globalWet and globalDry parameters.
    return output * globalWet() + input_ * globalDry;
}


void AudioEngine::updateAudioBlock()
{
    // granulator update function
    effectProcessor[1]->updateAudioBlock();
}


void AudioEngine::setEffectOrder()
{
    // clear the effect order array and the effect index array
    for (uint n = 0; n < 3; ++n)
    {
        for (uint m = 0; m < 3; ++m)
        {
            processFunction[n][m] = nullptr;
            processIndex[n][m] = -1;
        }
    }
    
    // retrieve the current choice of effect order
    String effectOrder = getParameter("effect_order")->getValueAsString();
    
    // Split the effectOrder string into parallel segments
    std::stringstream stringStream(effectOrder);
    
    std::string segment;
    
    uint row = 0;
    
    // split string into the rows = into every series processing
    while (std::getline(stringStream, segment, '-'))
    {
        // Split the segment string into parallel segments
        std::stringstream segmentSegment(segment);
        
        std::string effectID;
        
        uint col = 0;
        
        // split string into the columns of rows = into every parallel processing
        while (std::getline(segmentSegment, effectID, '|'))
        {
            // trim any whitespace from the extracted effect ID string
            effectID = trimWhiteSpace(effectID);
            
            // effectID shouldnt be empty or any other than a digit
            if (!effectID.empty() && std::all_of(effectID.begin(), effectID.end(), ::isdigit))
            {
                // effect index is one less than the effect ID
                int effectIndex = std::stoi(effectID) - 1;
                
                // check if effect Index is in valid range
                if (effectIndex >= 0 && effectIndex < NUM_EFFECTS)
                {
                    
                    // insert the process function of this effect to the precise array slot
                    processFunction[row][col] = [this, effectIndex](StereoFloat input, uint sampleIndex) {
                        return effectProcessor[effectIndex]->processAudioSamples(input, sampleIndex);
                    };
                    
                    // insert the process effect index to the precise array slot
                    processIndex[row][col] = effectIndex;
                    
                    // need to tell the effect how it is getting processed. this affects how the wet variable is used
                    // in the process function. parallel: wet controls the input gain, series: wet controls dry/wet
                    // if the column in the process index array is greater than one = processed in parallel and all
                    // other effects in the same row are processed in parallel too
                    if (col > 0)
                    {
                        effectProcessor[effectIndex]->setExecutionFlow(EffectProcessor::PARALLEL);
                        
                        if (col == 1)
                            effectProcessor[processIndex[row][0]]->setExecutionFlow(EffectProcessor::PARALLEL);
                        if (col == 2)
                            effectProcessor[processIndex[row][1]]->setExecutionFlow(EffectProcessor::PARALLEL);
                    }
                    
                    // otherwise: processed in series
                    else
                        effectProcessor[effectIndex]->setExecutionFlow(EffectProcessor::SERIES);
                    
                    // increment column for the next parallel effect
                    ++col;
                }
                else
                {
                    engine_rt_error("Effect index out of range: " + TOSTRING(effectIndex), __FILE__, __LINE__, true);
                }
            }
            else
            {
                engine_rt_error("Invalid effect id: " + effectID, __FILE__, __LINE__, true);
            }
        }
        // increment row for the next series effect(s)
        ++row;
    }
}


void AudioEngine::setBypass(bool bypassed_)
{
    // If bypass is enabled, ramp down the wet signal to 0 over 0.05 seconds.
    if (bypassed_)
    {
        globalWetCache = globalWet();
        globalWet.setRampTo(-0.01f, 0.05f);
    }
    // If bypass is disabled, ramp up the wet signal to 1 over 0.05 seconds and set bypassed to false.
    else
    {
        globalWet.setRampTo(globalWetCache, 0.05f);
        bypassed = false;
    }
    
    // Update the dry signal to be the cosine inverse of the wet signal.
    globalDry = sqrtf_neon(1.f - globalWet() * globalWet());
}


void AudioEngine::setGlobalMix()
{
    // scale linear raw value to sine value
    float raw = getParameter("global_mix")->getValueAsFloat() * 0.01f;
    float wet = sinf_neon(raw * PIo2);
    
    // set the ramps target to the new value
    globalWet.setRampTo(wet, 0.01f);
}


void AudioEngine::updateRamps()
{
    // If the wet signal ramp is not yet finished, continue processing the ramp.
    if (!globalWet.rampFinished)
    {
        globalWet.processRamp();
        
        // Update the dry signal to be the inverse of the wet signal.
        globalDry = sqrtf_neon(1.f - globalWet() * globalWet());
    }
    // If the ramp is finished and the wet signal has reached 0, set bypassed to true.
    else if (!bypassed && globalWet() < 0.f)
    {
        bypassed = true;
        globalWet = 0.f;
    }
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


EffectProcessor* AudioEngine::getEffect(const unsigned int index_)
{
    if (index_ > NUM_EFFECTS-1)
        engine_rt_error("Audio Engine holds no Effect with Index " + TOSTRING(index_), __FILE__, __LINE__, true);
    
    auto effect = effectProcessor[index_];
    
    if (!effect)
        engine_rt_error("Audio Engine can't find effect", __FILE__, __LINE__, true);
    
    return effect;
}


// =======================================================================================
// MARK: - USER INTERFACE
// =======================================================================================


void UserInterface::setup(AudioEngine* engine_, const float sampleRate_)
{
    // Save a pointer to the AudioEngine instance in this object.
    engine = engine_;

    // Initialize all buttons, potentiometers, and LEDs.
    initializeUIElements();

    // Connect the LEDs to their corresponding parameters.
    // This must be done before the menu is initialized. When the first
    // preset is loaded, the parameters will be set, and the LEDs should reflect their
    // values at startup.
    engine->getParameter("global_bypass")->addListener(&led[LED_BYPASS]);
    engine->getParameter("effect1_engaged")->addListener(&led[LED_FX1]);
    engine->getParameter("effect2_engaged")->addListener(&led[LED_FX2]);
    engine->getParameter("effect3_engaged")->addListener(&led[LED_FX3]);
    engine->getParameter("reverb", NUM_POTENTIOMETERS)->addListener(&led[LED_ACTION]);
    engine->getParameter("granulator", NUM_POTENTIOMETERS)->addListener(&led[LED_ACTION]);
    // TODO: Add Resonator
    engine->getParameter("effect_edit_focus")->addListener(&led[LED_FX1]);
    engine->getParameter("effect_edit_focus")->addListener(&led[LED_FX2]);
    engine->getParameter("effect_edit_focus")->addListener(&led[LED_FX3]);

    // Set up the menu object.
    // This includes configuring the entire page architecture and hierarchy, setting up JSON,
    // and loading the first preset (based on the JSON value: lastUsedPreset).
    initializeMenu();

    // Connect all components that need to listen to each other. This function is essential
    // for the interaction between the UI, Parameters, Outputs (LEDs, Display), and the
    // Audio Engine.
    // Listeners (except for LEDs) will be initialized after the first preset is loaded,
    // ensuring the initial parameter set does not affect the entire interface.
    initializeListeners();

    // Set up the display: establish the OSC connection for the OLED display and set the
    // initial page to be displayed on startup.
    display.setup(menu.getPage("load_preset"));

    // Configure the tempo tapper.
    tempoTapper.setup(engine->getParameter("tempo")->getMin(), engine->getParameter("tempo")->getMax(), sampleRate_);

    // Configure the metronome.
    metronome.setup(sampleRate_, engine->getParameter("tempo")->getValueAsFloat());

    // Let the LEDs blink! Setup is complete!
    alertLEDs(LED::State::ALERT);
}


void UserInterface::initializeUIElements()
{
    button[BUTTON_FX1].setup(BUTTON_FX1, "effect1");
    button[BUTTON_FX2].setup(BUTTON_FX2, "effect2");
    button[BUTTON_FX3].setup(BUTTON_FX3, "effect3");
    button[BUTTON_ACTION].setup(BUTTON_ACTION, "action");
    button[BUTTON_TEMPO].setup(BUTTON_TEMPO, "tempo");
    button[BUTTON_BYPASS].setup(BUTTON_BYPASS, "bypass");
    button[BUTTON_UP].setup(BUTTON_UP, "up");
    button[BUTTON_DOWN].setup(BUTTON_DOWN, "down");
    button[BUTTON_EXIT].setup(BUTTON_EXIT, "exit");
    button[BUTTON_ENTER].setup(BUTTON_ENTER, "enter");
    
    potentiometer[0].setup(0, "pot1");
    potentiometer[1].setup(1, "pot2");
    potentiometer[2].setup(2, "pot3");
    potentiometer[3].setup(3, "pot4");
    potentiometer[4].setup(4, "pot5");
    potentiometer[5].setup(5, "pot6");
    potentiometer[6].setup(6, "pot7");
    potentiometer[7].setup(7, "pot8");
    
    led[LED_FX1].setup(LED_FX1, "effect1");
    led[LED_FX2].setup(LED_FX2, "effect2");
    led[LED_FX3].setup(LED_FX3, "effect3");
    led[LED_ACTION].setup(LED_ACTION, "action");
    led[LED_TEMPO].setup(LED_TEMPO, "tempo");
    led[LED_BYPASS].setup(LED_BYPASS, "bypass");
}


void UserInterface::initializeMenu()
{
    // Create Parameter Pages
    // This is done here because it's easier to access the correct parameters in this context
    // rather than within the menu.
    menu.addPage<Menu::ParameterPage>("effect_order", engine->getParameter("effect_order"));
    menu.addPage<Menu::ParameterPage>("tempo_set", engine->getParameter("tempo_set"));
    
    menu.addPage<Menu::ParameterPage>("reverb_lowcut", engine->getParameter("reverb", "reverb_lowcut"));
    menu.addPage<Menu::ParameterPage>("reverb_multfreq", engine->getParameter("reverb", "reverb_multfreq"));
    menu.addPage<Menu::ParameterPage>("reverb_multgain", engine->getParameter("reverb", "reverb_multgain"));
    
    menu.addPage<Menu::ParameterPage>("granulator_delayspeedratio", engine->getParameter("granulator", "granulator_delayspeedratio"));
    menu.addPage<Menu::ParameterPage>("granulator_filterresonance", engine->getParameter("granulator", "granulator_filterresonance"));
    menu.addPage<Menu::ParameterPage>("granulator_filtermodel", engine->getParameter("granulator", "granulator_filtermodel"));
    menu.addPage<Menu::ParameterPage>("granulator_envelopetype", engine->getParameter("granulator", "granulator_envelopetype"));
    menu.addPage<Menu::ParameterPage>("granulator_glide", engine->getParameter("granulator", "granulator_glide"));
    
    // Configure the menu: pass in the complete set of parameters.
    menu.setup(engine->getProgramParameters());
}


void UserInterface::initializeListeners()
{
    // BUTTON ACTIONS
    // ==================================================================================

    // FX parameters respond to the toggling of FX buttons.
    button[BUTTON_FX1].addListener(engine->getParameter("effect1_engaged"));
    button[BUTTON_FX2].addListener(engine->getParameter("effect2_engaged"));
    button[BUTTON_FX3].addListener(engine->getParameter("effect3_engaged"));
    button[BUTTON_BYPASS].addListener(engine->getParameter("global_bypass"));

    // The menu responds to the menu button actions.
    button[BUTTON_UP].addListener(&menu);
    button[BUTTON_DOWN].addListener(&menu);
    button[BUTTON_EXIT].addListener(&menu);
    button[BUTTON_ENTER].addListener(&menu);

    // The Tempo Tapper is triggered when the Tempo button is clicked.
    // It checks for a new tempo and updates the Tempo Parameter accordingly.
    button[BUTTON_TEMPO].onClick = [this] { evaluateNewTempo(); };

    // The display will react and show the Tempo Parameter when the Tempo button is 
    // long-pressed. This automatically sets the display to the TEMPORARY state,
    // enabling tempo nudging and scrolling.
    button[BUTTON_TEMPO].onPress = [this] {
        display.parameterCalledDisplay(engine->getParameter("tempo"));
    };

    // UI Parameters will be nudged or scrolled when a Direction Button is clicked
    // or pressed. The current UI Parameter will be reset to its default value
    // when a long press of the Enter Button is detected.
    // These lambda functions are called before the button listener notification.
    // They temporarily suspend the usual Menu Button actions until nudging,
    // scrolling, or resetting to default is complete.
    button[BUTTON_UP].onClick = [this] { nudgeUIParameter(1); };
    button[BUTTON_DOWN].onClick = [this] { nudgeUIParameter(-1); };

    button[BUTTON_UP].onPress = [this] { startScrollingUIParameter(1); };
    button[BUTTON_DOWN].onPress = [this] { startScrollingUIParameter(-1); };

    button[BUTTON_UP].onRelease = [this] { stopScrollingUIParameter(); };
    button[BUTTON_DOWN].onRelease = [this] { stopScrollingUIParameter(); };

    button[BUTTON_ENTER].onPress = [this] { setUIParameterToDefault(); };

    // On long presses of the FX buttons, the Effect Edit Focus Parameter is set.
    // When this parameter changes, the potentiometers need to update their assigned parameter.
    // The Action Button and LEDs should also update to reflect the corresponding states:
    // VALUE or VALUEFOCUS.
    button[BUTTON_FX1].onPress = [this] {
        engine->getParameter("effect_edit_focus")->setValue(0); };
    button[BUTTON_FX2].onPress = [this] {
        engine->getParameter("effect_edit_focus")->setValue(1); };
    button[BUTTON_FX3].onPress = [this] {
        engine->getParameter("effect_edit_focus")->setValue(2); };

    engine->getParameter("effect_edit_focus")->onChange.push_back([this] { setEffectEditFocus(); });

    // Set the current effect edit focus.
    // Adds the parameters of the currently focused effect as listeners to the potentiometers.
    setEffectEditFocus();
    
    
    // POTENTIOMETER ACTIONS
    // ==================================================================================
    
    potentiometer[NUM_POTENTIOMETERS-1].onChange = [this] { mixPotentiometerChanged(); };
    potentiometer[NUM_POTENTIOMETERS-1].decouple(engine->getParameter("global_mix")->getNormalizedValue());
    
    // If a potentiometer reaches its cached value, the Action parameter LED blinks once.
    for (uint n = 0; n < NUM_POTENTIOMETERS-1; ++n)
        potentiometer[n].onCatch = [this] { alertLEDs(LED::State::BLINKONCE); };
        
    // When a potentiometer is touched, the display shows its associated parameter.
    for (uint n = 0; n < NUM_POTENTIOMETERS-1; ++n)
        potentiometer[n].onTouch = [this, n] { displayTouchedParameter(n); };

    
    // PARAMETER ACTIONS
    // ==================================================================================
    
    // The display listens to all parameters that need to be displayed:
    // - The tempo parameter
    // - All effect parameters
    engine->getParameter("tempo")->addListener(&display);

    for (unsigned int n = 0; n < Reverberation::NUM_PARAMETERS; ++n)
        engine->getParameter("reverb", n)->addListener(&display);
    for (unsigned int n = 0; n < Granulation::NUM_PARAMETERS; ++n)
        engine->getParameter("granulator", n)->addListener(&display);
    // TODO: Add Resonator

    // The Metronome reacts to changes in the Tempo parameter.
    engine->getParameter("tempo")->addListener(&metronome);

    // If a new tempo is detected, we need to determine which parameters should react to it.
    // This depends on the preset setting 'Set Tempo To:', which specifies whether
    // a tempo change should affect all connected effect parameters or only those of the
    // currently focused effect.
    engine->getParameter("tempo")->onChange.push_back([this] { setTempoRelatedParameters(); });

    // If the effect order changes, the LEDs will briefly blink, and the algorithm
    // to reset the process functions to the new order is called.
    engine->getParameter("effect_order")->onChange.push_back([this] { effectOrderChanged(); });

    // Effects toggle their engaged flag based on the corresponding parameter changes.
    engine->getParameter("effect1_engaged")->addListener(engine->getEffect(0));
    engine->getParameter("effect2_engaged")->addListener(engine->getEffect(1));
    engine->getParameter("effect3_engaged")->addListener(engine->getEffect(2));
    
    engine->setEffectOrder();
    
    // Engine sets a small Ramp for input and effect output if the Global Bypass button is pressed
    engine->getParameter("global_bypass")->onChange.push_back([this] {
        engine->setBypass(engine->getParameter("global_bypass")->getValueAsFloat());
    });
    
    engine->getParameter("global_mix")->onChange.push_back([this] { engine->setGlobalMix(); });
    
    // MENU ACTIONS
    // ==================================================================================
    
    // The display reacts to a page change in the Menu.
    menu.onPageChange = [this] { display.menuPageChanged(menu.getCurrentPage()); };

    // For certain settings stored in the Menu (such as global settings, preset changes,
    // and effect order changes), the user interface must respond.
    menu.onPresetLoad = [this] { presetChanged(); };

    // The LEDs flash when a preset is saved.
    menu.onPresetSave = [this] { alertLEDs(LED::ALERT); };
    
    // when a global setting changed, call the corresponding code
    menu.onGlobalSettingChange = [this](Menu::Page* page) { globalSettingChanged(page); };
    
    // when the effect order changed, call the engine's effect order algorithm and LEDs blink
    menu.onEffectOrderChange = [this] { effectOrderChanged(); };
    
    
    // OTHER ACTIONS
    // ==================================================================================

    // The Tempo LED blinks in sync with the Metronome's tempo.
    metronome.onTic = [this] { led[LED_TEMPO].blinkOnce(); };
}


void UserInterface::processNonAudioTasks()
{
    // Tempotapper
    if (tempoTapper.isCounting) tempoTapper.process();
    
    // Metronome
    metronome.process();
}


void UserInterface::updateNonAudioTasks()
{
    // if a Menu Parameter is in Scrolling Mode, scroll it
    if (menu.isScrolling) menu.scroll();

    // if a UI Parameter is in Scrolling Mode
    if (scrollingParameter)
    {
        // scroll it
        scrollingParameter->nudgeValue(scrollingDirection);
        
        // since the parameter changed, the potentiometer needs to be decoupled
        // and refreshed with the new normalized value
        if (scrollingParameter->getIndex() < NUM_POTENTIOMETERS)
        {
            uint paramIndex = scrollingParameter->getIndex();
            float normalizedValue = scrollingParameter->getNormalizedValue();
            
            potentiometer[paramIndex].decouple(normalizedValue);
        }
    }
}


void UserInterface::globalSettingChanged(Menu::Page* page_)
{
    if (page_->getID() == "pot_behaviour")
    {
        Potentiometer::setPotBevaviour(INT2ENUM(page_->getCurrentChoiceIndex(), PotBehaviour));
    }
    
    //TODO: midi in
    //TODO: midi out
    
    alertLEDs(LED::ALERT);
}


void UserInterface::presetChanged()
{
    // When a preset changes, the tempo parameter would normally trigger
    // setTempoRelatedParameters() to update the effect parameters that
    // respond to a tempo change. This flag temporarily disables that behavior,
    // ensuring the effect parameters remain at the values specified by the preset.
    settingTempoIsOnHold = true;
    
    // let the LEDs alert
    alertLEDs(LED::ALERT);
}


void UserInterface::effectOrderChanged()
{
    // call the effect order algorithm
    engine->setEffectOrder();
    
    // led the LEDs blink once
    alertLEDs(LED::BLINKONCE);
}


void UserInterface::setEffectEditFocus()
{
    // get a pointer to the effect-edit-focus-parameter
    auto focus = engine->getParameter("effect_edit_focus");
    
    // retrieve the index of the focused effect from parameter
    auto effect = engine->getEffect(focus->getValueAsInt());
    
    // for all potentiometers:
    for (unsigned int n = 0; n < NUM_POTENTIOMETERS-1; n++)
    {
        // focus the corresponding effect parameter
        potentiometer[n].swapListener(effect->getParameter(n));
        
        // retrieve the current normalized value of the parameter
        float normalizedValue = effect->getParameter(n)->getNormalizedValue();
        
        // assign the normalized value to the potentiometers cache
        // decouple any input source from potentiometer
        potentiometer[n].decouple(normalizedValue);
    }
    
    // for the effect button: 
    // focus corresponding effect parameter
    button[BUTTON_ACTION].swapListener(effect->getParameter(NUM_POTENTIOMETERS));
    
    // notify action-button-led that the parameter changed
    led[LED_ACTION].parameterChanged(effect->getParameter(NUM_POTENTIOMETERS));
    
    // Calculate the indices of the focused effect LED and the others.
    int focussedEffectLedIndex = focus->getValueAsInt() + LED_FX1;
    std::vector<int> ledIndices = { LED_FX1, LED_FX2, LED_FX3 };

    // Remove the focused LED index from the vector to get the non-focused indices.
    ledIndices.erase(std::remove(ledIndices.begin(), ledIndices.end(), focussedEffectLedIndex), ledIndices.end());

    // Set the LED states: focused = VALUEFOCUS, non-focused = VALUE.
    led[focussedEffectLedIndex].setState(LED::State::VALUEFOCUS);
    led[ledIndices[0]].setState(LED::State::VALUE);
    led[ledIndices[1]].setState(LED::State::VALUE);
}


void UserInterface::mixPotentiometerChanged()
{
    // static variable saves the last parameter that was attached to this potentiometer
    static AudioParameter* lastAttachedParameter = nullptr;
    
    // receive the potentiometer value (0...1)
    float potValue = potentiometer[UIPARAM_POT8].getValue();
    
    // receive the momentary focussed parameter
    // if one of the effect buttons is pressed: this effects wet parameter should be focussed
    // else its the global wet parameter
    AudioParameter* focusedParameter;
    if (button[BUTTON_FX1].getPhase() == Button::LOW)
        focusedParameter = engine->getParameter("reverb", "reverb_mix");
    else if (button[BUTTON_FX2].getPhase() == Button::LOW)
        focusedParameter = engine->getParameter("granulator", "granulator_mix");
    // TODO: add third effect
//    else if (button[BUTTON_FX3].getPhase() == Button::LOW)
//        rt_printf("FX3 Wetness changed \n");
    else
        focusedParameter = engine->getParameter("global_mix");
    
    // find out if the focussed parameter is the same than the last attached one
    bool sameParameter;
    if (lastAttachedParameter == nullptr) sameParameter = false;
    else if (focusedParameter->getID() == lastAttachedParameter->getID()) sameParameter = true;
    else sameParameter = false;
    
    // this is needed to make the pot catching function work correctly
    if (sameParameter
        || isClose(focusedParameter->getNormalizedValue(), potValue, POT_CATCHING_TOLERANCE)
        || Potentiometer::potBehaviour == PotBehaviour::JUMP)
    {
        // send over the new value to the parameter
        focusedParameter->potChanged(&potentiometer[UIPARAM_POT8]);
        
        // if catched, save the new attached parameter and let the LEDs blink
        if (!sameParameter)
        {
            alertLEDs(LED::State::BLINKONCE);
            lastAttachedParameter = focusedParameter;
        }
    }
    
    // send over a new reference to the potentiometer
    else potentiometer[UIPARAM_POT8].decouple(focusedParameter->getNormalizedValue());
    
    // show the parameter on the display
    display.parameterCalledDisplay(focusedParameter);
}


void UserInterface::evaluateNewTempo()
{
    // This function is called when a tap occurs.
    // The tempoTapper evaluates whether a new tempo has been detected.
    // If a new tempo is captured, it returns true and saves the value internally.
    // At this point, the tempo parameter can be updated with the new value.
    bool newTempoDetected = tempoTapper.tapTempo();
    
    if (newTempoDetected)
        engine->getParameter("tempo")->setValue(tempoTapper.getTempoInBpm());
}


void UserInterface::setTempoRelatedParameters()
{
    // This flag temporarily disables the function.
    // Currently, the flag is only modified when a new preset is loaded, ensuring that
    // the Tempo parameter does not override the tempo-related parameters when a preset is loaded.
    if (settingTempoIsOnHold)
    {
        settingTempoIsOnHold = false;
        return;
    }

    // Retrieve the current tempo in BPM from the parameter.
    float tempoBpm = engine->getParameter("tempo")->getValueAsFloat();
    
    // Retrieve the menu setting 'Tempo Set'.
    String tempoSetOption = engine->getParameter("engine", "tempo_set")->getValueAsString();
    
    // check for the two valid options of 'Tempo Set'
    if (tempoSetOption == "Current Effect" || tempoSetOption == "All Effects")
    {
        // Get the index of the currently focused effect.
        int effectIndex = engine->getParameter("effect_edit_focus")->getValueAsInt();
        
        // Get a pointer to the current effect.
        auto effect = engine->getEffect(effectIndex);
        
        // Adjust tempo-related parameters for the reverb effect.
        if (effect->getId() == "reverb" || tempoSetOption == "All Effects")
        {
            // Get a pointer to the Predelay parameter.
            auto predelay = engine->getParameter("reverb", "reverb_predelay");
            
            // Convert BPM to milliseconds.
            // * 8.f: Fit the BPM range to the range of the predelay.
            // TODO: Adjust the values to ensure they are in the correct range.
            float tempoMs = bpm2msec(tempoBpm * 8.f);
            
            // Set the new predelay value without triggering a print notification.
            predelay->setValue(tempoMs, false);
            
            // Decouple the corresponding potentiometer and set its cache to the new normalized value.
            if (effectIndex == 0) potentiometer[predelay->getIndex()].decouple(predelay->getNormalizedValue());
        }
        
        // Adjust tempo-related parameters for the granulator effect.
        if (effect->getId() == "granulator" || tempoSetOption == "All Effects")
        {
            // Retrieve the Grain Length parameter for the granulator effect.
            auto density = engine->getParameter("granulator", "granulator_density");
            
            // Convert BPM to milliseconds.
            float tempoMs = bpm2msec(tempoBpm);
            float tempoRate = 1000.f / tempoMs;
            
            tempoRate *= 2.f;
            //TODO: uncomment this?
//            if (tempoRate < Granulation::MIN_DENSITY) tempoRate *= 2.f;
//            else if (tempoRate > Granulation::MAX_DENSITY) tempoRate = Granulation::MAX_DENSITY;
            
            // Set the new density value without triggering a print notification.
            density->setValue(tempoRate, false);
            
            // Decouple the corresponding potentiometer and set its cache to the new normalized value.
            if (effectIndex == 1) potentiometer[density->getIndex()].decouple(density->getNormalizedValue());
        }
        
        // TODO: Add Resonator
    }
    
    else
    {
        engine_rt_error("Couldn't find 'Tempo Set' option with name: " + tempoSetOption, __FILE__, __LINE__, false);
    }
}


void UserInterface::nudgeUIParameter(const int direction_)
{
    // If the display is in TEMPORARY state, it holds a pointer to the currently shown parameter.
    if (display.getStateDuration() == Display::TEMPORARY)
    {
        // Set the menu on hold to bypass the usual behavior of the Menu buttons.
        menu.onHold = true;
        
        // Refresh the counter for the TEMPORARY state of the display, extending its duration.
        display.refreshResetDisplayCounter();
        
        // Retrieve the currently shown parameter.
        AudioParameter* param = display.getTemporaryParameter();
        
        // Safety check to ensure the parameter is valid (shouldn't be nullptr when the 
        // display state is TEMPORARY).
        if (param)
        {
            // Nudge the parameter value in the specified direction.
            param->nudgeValue(direction_);
            
            // Decouple the corresponding potentiometer and update its cache to reflect 
            // the new normalized value.
            if (param->getIndex() < NUM_POTENTIOMETERS)
                potentiometer[param->getIndex()].decouple(param->getNormalizedValue());
        }
    }
}


void UserInterface::startScrollingUIParameter(const int direction_)
{
    // If the display is in TEMPORARY state, it holds a pointer to the currently shown parameter.
    if (display.getStateDuration() == Display::TEMPORARY)
    {
        // Set the menu on hold to bypass the usual behavior of the Menu buttons.
        menu.onHold = true;

        // Refresh the counter for the TEMPORARY state of the display, extending its duration.
        display.refreshResetDisplayCounter();
        
        // Save a pointer to the currently shown parameter in this object.
        // The scrolling action will be handled in the updateNonAudioTasks() function.
        scrollingParameter = display.getTemporaryParameter();
        
        // Safety check to ensure the parameter is valid (shouldn't be nullptr when the
        // display state is TEMPORARY).
        if (!scrollingParameter)
            engine_rt_error("display doesn't hold a parameter for scrolling", 
                            __FILE__, __LINE__, false);
            
        // Save the scrolling direction in this object.
        scrollingDirection = direction_;
    }
}


void UserInterface::stopScrollingUIParameter()
{
    // Called when the Up or Down button is released after a long press.
    // The scrollingParameter is set to nullptr, which acts as a flag for the
    // updateNonAudioTasks() function to indicate whether a parameter should be scrolled.
    scrollingParameter = nullptr;
}


void UserInterface::setUIParameterToDefault()
{
    // If the display is in TEMPORARY state, it holds a pointer to the currently shown parameter.
    if (display.getStateDuration() == Display::TEMPORARY)
    {
        // Set the menu on hold to bypass the usual behavior of the Menu buttons.
        menu.onHold = true;
        
        // Refresh the counter for the TEMPORARY state of the display, extending its duration.
        display.refreshResetDisplayCounter();
        
        // Retrieve the currently shown parameter.
        AudioParameter* param = display.getTemporaryParameter();
        
        // Safety check to ensure the parameter is valid (shouldn't be nullptr when the
        // display state is TEMPORARY).
        if (param)
        {
            // Set the parameter value to its default value.
            param->setDefaultValue();
            
            // Decouple the corresponding potentiometer and update its cache to reflect
            // the new normalized value.
            if (param->getIndex() < NUM_POTENTIOMETERS)
                potentiometer[param->getIndex()].decouple(param->getNormalizedValue());
        }
    }
}


void UserInterface::displayTouchedParameter(uint paramIndex_)
{
    // This function is called every time a potentiometer is touched, instructing the
    // display to show the associated parameter.
    
    // Since it's not possible to initialize the potentiometer's cache at startup with the
    // current potentiometer position, we include an initialization algorithm.
    // The function bypasses normal behavior for the first 8 calls (equal to the number of potentiometers).
    static bool initializing = true;
    static uint numInitCallsLeft = NUM_POTENTIOMETERS;

    if (initializing)
    {
        if (--numInitCallsLeft == 0) initializing = false;
        return;
    }

    // Retrieve the index of the currently focused effect.
    int focus = engine->getParameter("effect_edit_focus")->getValueAsInt();

    // Get a pointer to the focused effect.
    auto effect = engine->getEffect(focus);

    // Get the parameter associated with the touched potentiometer (using the same index).
    AudioParameter* connectedParam = effect->getParameter(paramIndex_);

    // Instruct the display object to show a parameter message for this parameter.
    display.parameterCalledDisplay(connectedParam);
}


void UserInterface::alertLEDs(LED::State state_)
{
    // call all LED's alert function
    if (state_ == LED::State::ALERT)
        for (unsigned int n = 0; n < NUM_LEDS; ++n)
            led[n].alert();
    
    // call all LED's blink once function
    else if (state_ == LED::State::BLINKONCE)
        for (unsigned int n = 0; n < NUM_LEDS; ++n)
            led[n].blinkOnce();
}


// =======================================================================================
// MARK: - TEMPOTAPPER
// =======================================================================================


void TempoTapper::setup(const float minBPM_, const float maxBPM_, const float sampleRate_)
{
    // Set the sample rate for the audio processing.
    sampleRate = sampleRate_;

    // Calculate the maximum and minimum BPM counts based on the sample rate and BPM limits.
    maxBpmCounts = (60.f * sampleRate) / maxBPM_;
    minBpmCounts = (60.f * sampleRate) / minBPM_;

    // Explanation:
    // The BPM counts represent the number of samples between beats at the given BPM.
    // A higher BPM results in a lower count (fewer samples between beats).
    // For example:
    //   60 BPM = (60 * sampleRate) / 60  -> 1 second per beat
    //    1 BPM = (60 * sampleRate)       -> 60 seconds per beat (one beat per minute)
    //  120 BPM = (60 * sampleRate) / 120 -> 0.5 seconds per beat
}


void TempoTapper::process()
{
    // Increment the tap counter and check if it exceeds the minimum BPM count threshold.
    if (++tapCounter > minBpmCounts)
    {
        // If the counter exceeds the minimum BPM count, stop counting.
        isCounting = false;
    }
}


void TempoTapper::calculateNewTempo()
{
    // Calculate tempo-related values based on the tapCounter (number of samples between taps).
    tempoSamples = tapCounter;
    tempoSec = tapCounter / sampleRate;
    tempoMsec = tempoSec * 1000.f;
    tempoBpm = 60.f / tempoSec;
}


bool TempoTapper::tapTempo()
{
    // A new tap arrives:
    // 1. The tap starts the counter (first tap).
    // 2. A previous tap was detected within a valid time range, which means a new 
    //    BPM should be calculated and the counter restarted.

    bool newTempoDetected = false;

    if (isCounting)
    {
        // Check if the time between taps falls within the valid BPM range.
        if (tapCounter >= maxBpmCounts && tapCounter <= minBpmCounts)
        {
            // Calculate the new tempo based on the time interval.
            calculateNewTempo();
            
            // Indicate that a new tempo has been detected.
            newTempoDetected = true;
        }
    }

    // Start or restart the counting process.
    isCounting = true;
    
    // Reset the tap counter for the next interval.
    tapCounter = 0;

    return newTempoDetected;
}


// =======================================================================================
// MARK: - METRONOME
// =======================================================================================


void Metronome::setup(const float sampleRate_, const float defaultTempoBpm_)
{
    // Initialize the sample rate.
    sampleRate = sampleRate_;
    
    // Convert the default tempo from BPM to the corresponding number of samples per beat.
    tempoSamples = bpm2samples(defaultTempoBpm_, sampleRate);
    
    // Initialize the counter to the number of samples per beat.
    counter = tempoSamples;
}


void Metronome::process()
{
    // Trigger the metronome tick (onTic) when the counter reaches the tempoSamples value.
    if (counter == tempoSamples) onTic();
    
    // Decrement the counter and reset it to tempoSamples when it reaches zero.
    if (--counter == 0) counter = tempoSamples;
}


void Metronome::setTempoSamples(const uint tempoSamples_)
{
    // Set the new tempo in terms of samples per beat.
    tempoSamples = tempoSamples_;
    
    // Reset the counter to the new tempoSamples value.
    counter = tempoSamples_;
}


void Metronome::parameterChanged(AudioParameter *param_)
{
    // Retrieve the new tempo in BPM from the provided parameter.
    float tempoBpm = param_->getValueAsFloat();
    
    // Convert the BPM to the number of samples per beat and update the metronome.
    setTempoSamples((uint)((sampleRate * 60.f) / tempoBpm));
}
