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
    midi.getParser()->setCallback(midiInputMessageCallback, (void*) "hw:0,0,0");
    
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
    
    // midi output
    for (uint n = 0; n < NUM_POTENTIOMETERS; ++n)
        userinterface.potentiometer[n].setupMIDI(n+1, midiOutputMessageCallback);
        
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


void midiInputMessageCallback(MidiChannelMessage message, void* arg)
{
    int midiInChannel = userinterface.menu.getMidiInChannel() - 1;
    
    if (message.getChannel() == midiInChannel)
    {
//        message.prettyPrint();
        
        if(message.getType() == kmmProgramChange)
        {
            uint presetIndex = message.getDataByte(0);
                        
            userinterface.menu.handleMidiProgramChangeMessage(presetIndex);
        }
        
        else if (message.getType() == kmmControlChange)
        {
            uint ccIndex = message.getDataByte(0);
            uint ccValue = message.getDataByte(1);
            
//            consoleprint("New Control Change detected with Index: " + TOSTRING(ccIndex) + " and Value: " + TOSTRING(ccValue), __FILE__, __LINE__);
            
            userinterface.handleMidiControlChangeMessage(ccIndex, ccValue);
        }
    }
}


void midiOutputMessageCallback(uint ccIndex_, uint ccValue_)
{
    int midiOutChannel = userinterface.menu.getMidiOutChannel() - 1;

    midi.writeControlChange(midiOutChannel, ccIndex_, ccValue_);
}


#endif // BELA_CONNECTED


/**
 * @mainpage GRAINMOTHER
 *
 * A Multi-Effect Audio Engine based on the BELA Platform
 *
 * Grainmother is a multi-effect audio engine built on the BELA platform. It’s designed for musicians and sound designers who want to create unique sounds without relying on external software. The project combines granular synthesis, reverb, and ring modulation into a single device controlled hands-on, rather than through traditional pedal interfaces. <br>
 * The goal of Grainmother is to provide a flexible, standalone effects processor optimized for live performances. Leveraging BELA’s real-time audio processing capabilities and a custom hardware interface, it offers intuitive controls for creative experimentation.<br>
 * This project is fully open-source, with all code and schematics available for those interested in exploring or building upon the device.
 *
 * @section features Features
 * Grainmother features three distinct audio effects, each designed to provide unique sound-shaping capabilities.<br>
 * The Ring Modulator combines the input signal with a carrier signal, using analog simulations such as diode-based, transistor-based, and hybrid approaches. An additional LFO modulates the carrier signal, while bit crushing and noise modulation extend the effect. This allows for a range of sounds, from lush amplitude modulation to extremely harsh, crushed noise.<br>
 * The Granulator breaks audio into short grains ranging from 1 to 100 milliseconds. In addition to standard controls like Grain Length and Density, properties such as panning and length can be randomized. A delay, dynamic feedback, and filtering options provide powerful tools for intricate sound design. This versatile set of parameters enables everything from subtle enhancements to fragmented, evolving soundscapes.<br>
 * The Reverb delivers a rich spatial effect by blending early reflections with a late reverberation algorithm inspired by Schroeder and Moorer. Selectable types, including Church, Digital Vintage, Seasick, and Room, allow users to craft immersive spaces and fine-tune parameters such as decay, pre-delay, and modulation. A unique feature is the feedback loop within the early reflection algorithm: lower values increase echo density artificially, while higher values create unique, morphing sounds.<br>
 * The three effects can be processed in either parallel or series mode, with the user having the ability to define the processing order. Each effect features an individual Mix/Wet control, along with a master control for the entire effect engine. All processing is done true stereo.<br>
 * A dedicated menu, accessible via four buttons, allows for additional settings such as potentiometer behavior, MIDI input and output channels, and advanced effect parameters.<br>
 * An OLED display provides real-time feedback, showing parameter changes and navigating the menu.<br>
 * The Grainmother includes a built-in preset system that allows users to save and recall custom effect configurations. This feature provides quick access to stored settings, making it ideal for maintaining consistency during live performances or efficiently switching between sounds.<br>
 * The device supports full MIDI integration via the Mini-USB connector on the side. Parameters can be controlled using Control Change messages, and presets can be switched using Program Change messages from external MIDI devices. Additionally, the eight potentiometers can function as MIDI controllers, sending out Control Change messages for further flexibility.
 *
 * @section license License
 * Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License
 * @see https://creativecommons.org/licenses/by-sa/4.0/
*/
