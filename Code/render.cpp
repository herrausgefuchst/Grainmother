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
 * @section intro Grainmother - A Multi-Effect Audio Engine based on the BELA Platform
 * Grainmother is a customizable multi-effect audio engine built on the BELA platform, designed for musicians and sound designers who want to craft unique sounds without relying on external software. The project combines granular synthesis, reverb, and ring modulation into a single device that is controlled hands-on, rather than through traditional pedal interfaces. <br>
 * The goal of Grainmother is to provide a flexible, standalone effects processor optimized for live performances. By leveraging BELA’s real-time audio processing capabilities and a custom hardware interface, it offers intuitive controls that encourage creative experimentation. <br>
 * This project is fully open-source, with all code and schematics available for those interested in exploring or building upon the device. Grainmother is a practical tool for anyone looking to expand their sound design possibilities in a hardware-based setup.
 *
 * @subsection ringmodulator Ringmodulator
 * The Ring Modulator creates unique and complex sound textures by combining the input signal with a carrier signal. It supports advanced modulation types, including diode-based, transistor-based, and a hybrid combination of the two. Additional features include oversampling, bit crushing, and noise modulation.
 *
 * @subsubsection parameters Parameters
 *
 * The Ring Modulator offers extensive control through the following parameters:
 * - **Tune**: Sets the frequency of the carrier signal.
 * - **Rate**: Adjusts the speed of low-frequency modulation.
 * - **Depth**: Determines the intensity of low-frequency modulation.
 * - **Waveform**: Selects the waveform of low-frequency modulation, with options such as:
 *   - Sine
 *   - Triangle
 *   - Saw
 *   - Pulse
 *   - Random
 * - **Saturation**: Adds nonlinear distortion to the modulation process, simulating analog behavior.
 * - **Spread**: Introduces a phase shift between left and right channels for stereo widening.
 * - **Noise**: Adds randomized noise ring modulation for a grittier texture.
 * - **Bitcrush**: Reduces the bit depth for digital distortion effects.
 *
 * @subsubsection chain Signal Processing Chain
 *
 * 1. **Carrier Signal Generation**:
 *    - A configurable oscillator generates the carrier signal, modulated by an additional LFO.
 *    - The oscillator supports stereo processing, allowing independent phase shifts for each channel.
 *
 * 2. **Bitcrushing**:
 *    - The signal can be pre-processed through a bit crusher to reduce bit depth.
 *
 * 3. **Ring Modulation**:
 *    - The input signal is multiplied by the carrier signal using one of the following algorithms:
 *      - **Diode-Based Modulation**: Simulates the nonlinear behavior of diodes in an analog circuit.
 *      - **Transistor-Based Modulation**: Models the asymmetrical response of transistors.
 *      - **Hybrid Modulation**: Blends diode and transistor characteristics for a unique tone.
 *
 * 4. **Noise and Distortion**:
 *    - Optional noise is mixed into the modulation process for added texture.
 *
 * 5. **Oversampling**:
 *    - The modulation process uses an oversampling ratio of 2x for improved audio fidelity.
 *    - An interpolator and decimator handle upsampling and downsampling efficiently.
 *
 * @subsection granulator Granulator
 * The Granulator breaks the input signal into short fragments called grains. Each grain has a defined lifespan (grain length) between 1 and 100 ms and is shaped by an amplitude envelope. The type of envelope can be manually selected, with the options Parabolic, Hann, and Triangular. <br>
 * The distribution of grains is controlled by the parameter Density, which is internally defined as the interonset time between grains in samples. Each grain receives the samples to process from a class called SourceData. This class stores the input samples in a circular buffer, allowing them to be read from variable start points. When a new grain is created, the start position of the read pointer and its reading speed are defined. These parameters are referred to as Initial Delay and Pitch. Using the Glide parameter, a modulation of the reading speed within the grain can be generated. Both Pitch and Glide are limited to a maximum modulation of one octave up or down. Additionally, the reading direction can be inverted using the Reverse parameter. <br>
 * Grain properties such as Panning, Interonset (Density), Grain Length, and Initial Delay can be randomly distributed via the Variation parameter. Low values result in subtle panning variations that create a wider sound, while high values cause strong randomization of all four parameters. This produces fragmented, textural soundscapes.
 *
 * @subsubsection chain Signal Processing Chain
 *
 * 1. **Grain Clouds**:
 *    - The stereo input signals are processed independently, resulting in two grain clouds
 *      that sum the active grains per channel.
 *
 * 2. **Filter**:
 *    - The output signal is passed through a lowpass filter, selectable as either a
 *      **Moog Ladder** (-24 dB/octave) or **Moog Half Ladder** (-12 dB/octave).
 *    - The resonance is proportional to the cutoff frequency: lower frequencies result
 *      in higher resonance.
 *    - Both the filter type and resonance intensity are adjustable.
 *
 * 3. **Delay Effect**:
 *    - The delay time is directly linked to the **Density** parameter.
 *    - The **Delay Speed Ratio** parameter adjusts the ratio of the delay time to the
 *      interonset time.
 *
 * 4. **Dynamic Feedback**:
 *    - A dynamic feedback amplitude is calculated based on the output signal amplitude
 *      and the **Feedback** parameter.
 *    - The feedback signal is scaled, filtered through a highpass filter, and added back
 *      to the input signal.
 *
 * 5. **Saturation and Output**:
 *    - The final output is saturated using a **tanh** function to prevent clipping and
 *      create a harmonically rich sound.
 *
 *
 * @subsection reverb Reverb
 * The Reverb simulates 24 early reflections using a tap delay, then sends this signal into a late reverberation algorithm similar to those from Schroeder and Moorer. The late reverberation consists of a series of all-pass filters, a set of parallel comb filters, and another set of series all-pass filters. To combine these two elements correctly, the decay has to be delayed to align right after the latest early reflection. In addition, some equalizers can shape the sound. A parametric EQ is implemented pre-FX and used as a “multiplier” of the input signal. A low-cut and high-cut filter can be set upon request; these are implemented post-FX.
 *
 * @subsubsection reverbtypes The Reverb Types
 * The user can change the reverb type, which involves many internal parameter changes (referred to as typeParameters). The EarlyReflections class includes three different reflection simulations: Church, Foyer, and Bathroom. This set of tap delays changes according to the type, as well as a diffusion factor (the all-pass filter gain in the early reflections) and a damping factor (low-pass gain in the early reflections). The decay primarily adjusts the composition of all-pass and comb filters and their corresponding delays. Additionally, damping, diffusion, and the rate and depth of modulating the all-pass filter delay times are type parameters.
 *
 * Four different types have been implemented:
 * •    CHURCH
 * •    DIGITAL VINTAGE
 * •    SEASICK
 * •    ROOM
 *
 * You can look up the corresponding parameter sets in the Reverberation::Reverb::setReverbType() function.
 *
 * @section license License
 * Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License
 * @see https://creativecommons.org/licenses/by-sa/4.0/
*/
