#include "ConstantVariables.h"

/** @todo Reverb Modulation Rate doesnt initalize correctly */
/** @todo Midi In */
/** @todo Midi Out */

#ifdef BELA_CONNECTED

#include "BelaVariables.h"

using namespace BelaVariables;

// =======================================================================================
// MARK: - SETUP
// =======================================================================================

bool setup (BelaContext *context, void *userData)
{
    // scope
    #ifdef SCOPE_ACTIVE
    scope.setup(2, context->audioSampleRate);
    #endif
    
    // midi
    midi.readFrom("hw:0,0,0");
    midi.writeTo("hw:0,0,0");
    midi.enableParser(true);
    midi.getParser()->setCallback(midiMessageCallback, (void*) "hw:0,0,0");
    
    // display
    DISPLAY_BLOCKS_PER_FRAME = context->audioSampleRate / ( DISPLAY_FRAMERATE * context->audioFrames );
    displayBlockCtr = DISPLAY_BLOCKS_PER_FRAME;
    
    // leds
    LED_BLOCKS_PER_FRAME = context->audioSampleRate / ( LED_FRAMERATE * context->audioFrames );
    ledBlockCtr = LED_BLOCKS_PER_FRAME;
    std::fill(ledCache.begin(), ledCache.end(), 0.f);
    
    // ui rate
    UI_BLOCKS_PER_FRAME = context->audioSampleRate / ( UI_FRAMERATE * context->audioFrames );
    uiBlockCtr = UI_BLOCKS_PER_FRAME;
    
    // scrolling
    SCROLLING_BLOCKS_PER_FRAME = context->audioSampleRate / ( SCROLLING_FRAMERATE * context->audioFrames );
    scrollingBlockCtr = SCROLLING_BLOCKS_PER_FRAME;
    
    // aux tasks
    if((THREAD_updateUserInterface = Bela_createAuxiliaryTask(&updateUserInterface, 88, "updateUserInterface", context)) == 0) 
        return false;
    if((THREAD_updateNonAudioTasks = Bela_createAuxiliaryTask(&updateNonAudioTasks, 87, "updateNonAudioTasks", nullptr)) == 0) 
        return false;
    if((THREAD_updateAudioBlock = Bela_createAuxiliaryTask(&updateAudioBlock, 90, "updateAudioBlock", nullptr)) == 0)
        return false;
    
    // digital pinmodes
    for (unsigned int n = 0; n < NUM_BUTTONS; ++n) pinMode(context, 0, HARDWARE_PIN_BUTTON[n], INPUT);
    
    // effect engine
    engine.setup(context->audioSampleRate, context->audioFrames);
    
    // userinterface
    for (uint n = 0; n < NUM_POTENTIOMETERS; ++n)
        userinterface.potentiometer[n].setAnalogDefault(analogRead(context, 0, HARDWARE_PIN_POTENTIOMETER[n]));
    userinterface.setup(&engine, context->audioSampleRate);
        
    return true;
}


// =======================================================================================
// MARK: - RENDER
// =======================================================================================

void render (BelaContext *context, void *userData)
{ 
    // BLOCKWISE PROCESSING
    // ===================================================================================
    
    // update Effects blockwise
    Bela_scheduleAuxiliaryTask(THREAD_updateAudioBlock);
    
    // update user interface reading
    Bela_scheduleAuxiliaryTask(THREAD_updateUserInterface);
    
    // update Non Audio Tasks
    Bela_scheduleAuxiliaryTask(THREAD_updateNonAudioTasks);
    
    // update leds (doesnt work as an auxiliary task)
    updateLEDs();
        
    // write led analog output
    // this has to live here, running it in the thread doesnt seem to work
    for (unsigned int n = 0; n < NUM_LEDS; ++n)
        analogWrite(context, 0, HARDWARE_PIN_LED[n], ledCache[n]);
    
    // update display
    if (--displayBlockCtr <= 0)
    {
        displayBlockCtr = DISPLAY_BLOCKS_PER_FRAME;
        userinterface.display.update();
    }

    // SAMPLEWISE PROCESSING
    // ===================================================================================
    
    for(sampleIndex = 0; sampleIndex < context->audioFrames; ++sampleIndex)
    {
        userinterface.processNonAudioTasks();
        
        float32x2_t input = { audioRead(context, sampleIndex, 0), audioRead(context, sampleIndex, 1) };
        float32x2_t output = engine.processAudioSamples(input, sampleIndex);
        
        // write output buffer
        audioWrite(context, sampleIndex, 0, output[0]);
        audioWrite(context, sampleIndex, 1, output[1]);
        
        #ifdef SCOPE_ACTIVE
        // scope output
        scope.log(output[0], output[1]);
        #endif
    }
}


// =======================================================================================
// MARK: - CLEANUP
// =======================================================================================

void cleanup (BelaContext *context, void *userData)
{

}


// =======================================================================================
// MARK: - FUNCTIONS
// =======================================================================================

void updateUserInterface(void* arg_)
{
    BelaContext* context = static_cast<BelaContext*>(arg_);
    
    static bool firstFunctionCall = true;
    if (firstFunctionCall)
    {
        firstFunctionCall = false;
        
        for (unsigned int n = 0; n < NUM_POTENTIOMETERS; ++n)
        {
            float initialRawPotValue = analogRead(context, 0, HARDWARE_PIN_POTENTIOMETER[n]);
            userinterface.potentiometer[n].setAnalogDefault(initialRawPotValue);
        }
    }
    
    // buttons & potentiometer
    if (--uiBlockCtr <= 0)
    {
        // reset counter
        uiBlockCtr = UI_BLOCKS_PER_FRAME;
        
        // update buttons and potentiometers
        for (unsigned int n = 0; n < NUM_BUTTONS; ++n)
            userinterface.button[n].update(0.f, digitalRead(context, 0, HARDWARE_PIN_BUTTON[n]));
        for (unsigned int n = 0; n < NUM_POTENTIOMETERS; ++n)
            userinterface.potentiometer[n].update(0.f, analogRead(context, 0, HARDWARE_PIN_POTENTIOMETER[n]));
    }
}


void updateNonAudioTasks(void* arg_)
{
    if (--scrollingBlockCtr == 0)
    {
        scrollingBlockCtr = SCROLLING_BLOCKS_PER_FRAME;
        
        userinterface.updateNonAudioTasks();
    }
}


void updateAudioBlock(void* arg_)
{
    engine.updateAudioBlock();
}


void updateLEDs()
{
    if (--ledBlockCtr <= 0)
    {
        ledBlockCtr = LED_BLOCKS_PER_FRAME;
        
        for (unsigned int n = 0; n < NUM_LEDS; ++n)
            ledCache[n] = userinterface.led[n].getValue();
    }
}


void midiMessageCallback(MidiChannelMessage message, void* arg)
{
    if(arg != NULL) rt_printf("Message from midi port %s ", (const char*) arg);
    message.prettyPrint();
    
    int midiInChannel = userinterface.menu.getMidiInChannel() - 1;
    int midiOutChannel = userinterface.menu.getMidiOutChannel() - 1;
    
//    rt_printf("Midi In Channel: %i, Midi Out Channel: %i \n", midiInChannel, midiOutChannel);
//    rt_printf("Midi Channel of Message: %i \n", message.getChannel());
    
    if (message.getChannel() == midiInChannel)
    {
//        rt_printf("Correct IN Channel! \n");
//        rt_printf("Type of Message: %i \n", message.getType());
//        rt_printf("Program Change Index: %i \n", kmmProgramChange);
        
        if(message.getType() == kmmProgramChange)
        {
            uint presetIndex = message.getDataByte(0);
            
//            rt_printf("Program Change detected! New Programm Index: %i \n", presetIndex);
            
            userinterface.menu.handleMidiProgramChangeMessage(presetIndex);
        }
    }
}

#endif // BELA_CONNECTED


/**
 * @mainpage GRAINMOTHER
 *
 * @section intro Introduction
 *
 * @section reverb Reverb
 * The Reverb simulates 24 early reflections using a tap delay, then sends this signal into a late reverberation algorithm similar to those from Schroeder and Moorer. The late reverberation consists of a series of all-pass filters, a set of parallel comb filters, and another set of series all-pass filters. To combine these two elements correctly, the decay has to be delayed to align right after the latest early reflection. In addition, some equalizers can shape the sound. A parametric EQ is implemented pre-FX and used as a “multiplier” of the input signal. A low-cut and high-cut filter can be set upon request; these are implemented post-FX.
 *
 * @subsection reverbtypes The Reverb Types
 * The user can change the reverb type, which involves many internal parameter changes (referred to as typeParameters). The EarlyReflections class includes three different reflection simulations: Church, Foyer, and Bathroom. This set of tap delays changes according to the type, as well as a diffusion factor (the all-pass filter gain in the early reflections) and a damping factor (low-pass gain in the early reflections). The decay primarily adjusts the composition of all-pass and comb filters and their corresponding delays. Additionally, damping, diffusion, and the rate and depth of modulating the all-pass filter delay times are type parameters.
 *
 * Four different types have been implemented:
 * •    CHURCH
 * •    DIGITAL VINTAGE REVERB
 * •    SEASICK
 * •    ROOM
 *
 * You can look up the corresponding parameter sets in the Reverberation::Reverb::setReverbType() function.
 *
 * @section granulator Granulator
 * The Granulator processes audio in real-time using a Tapped Delay Line approach, decomposing sound into small grains (1–100 ms) and reassembling them into new textures. Grain properties like length, density, delay, and panning can be randomized or adjusted manually. Grain density, measured as the inter-onset interval, defines the temporal spacing between grains.
 *
 * @subsection advancedfeatures Advanced Features
 * - **Pitch and Reverse:** Allows pitch shifts by adjusting grain playback rates; reverse playback avoids real-time pitch-up limitations.
 * - **Feedback:** Adds harmonic richness with saturation, high-pass filtering, and dynamic feedback to prevent instability.
 * - **Filter & Delay:** Integrated Moog-style filter and a delay effect tied to grain density for synchronized echoes.
 *
 * @section ringmodulator Ringmodulator
 */
