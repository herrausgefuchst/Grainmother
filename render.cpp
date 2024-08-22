#include "ConstantVariables.h"

#ifdef BELA_CONNECTED

#include "BelaVariables.h"

using namespace BelaVariables;

// MARK: - SETUP
// ********************************************************************************

bool setup (BelaContext *context, void *userData)
{
    // scope
#ifdef SCOPE_ACTIVE
    scope.setup(2, context->audioSampleRate);
#endif
    
    // gui
    gui.setup(context->projectName);
    guiBufferIdx[POTS] = gui.setBuffer('f', NUM_POTENTIOMETERS);
    guiBufferIdx[BUTTONS] = gui.setBuffer('f', NUM_BUTTONS);
    guiBufferIdx[GUICTRLS] = gui.setBuffer('f', NUM_GUI_CONTROLS);
    guiBufferIdx[LEDS] = gui.setBuffer('f', NUM_LEDS);
    for (unsigned int n = DSP1; n <= DSP10; n++) guiBufferIdx[n] = gui.setBuffer('c', DISPLAY_NUM_LETTERS_IN_ROW);
    
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
    guiInitializationCtr = GUI_INITIALIZATION_TIME_SEC * context->audioFrames;
    
    // scrolling
    SCROLLING_BLOCKS_PER_FRAME = context->audioSampleRate / ( SCROLLING_FRAMERATE * context->audioFrames );
    scrollingBlockCtr = SCROLLING_BLOCKS_PER_FRAME;
    
    // aux tasks
    if((THREAD_updateLEDs = Bela_createAuxiliaryTask(&updateLEDs, 89, "updateLEDs", nullptr)) == 0) return false;
    if((THREAD_updateUserInterface = Bela_createAuxiliaryTask(&updateUserInterface, 88, "updateUserInterface", context)) == 0) return false;
    if((THREAD_updateNonAudioTasks = Bela_createAuxiliaryTask(&updateNonAudioTasks, 87, "updateNonAudioTasks", nullptr)) == 0) return false;
//    if((taskUpdateGUIDisplay = Bela_createAuxiliaryTask(&updateGUIdisplay, 88, "update-GUI-display", nullptr)) == 0) return false;
    
    // digital pinmodes
    for (unsigned int n = 0; n < NUM_BUTTONS; ++n) pinMode(context, 0, HARDWARE_PIN_BUTTON[n], INPUT);
    
    // setup objects
    inputHandler.setup(context->audioSampleRate, 120.f, 0.7f);
    
    engine.setup(context->audioSampleRate, context->audioFrames);
    
    userinterface.setup(&engine, context->audioSampleRate);
    
    return true;
}

// MARK: - RENDER
// ********************************************************************************

void render (BelaContext *context, void *userData)
{

// update BLOCKWISE +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // update user interface
    Bela_scheduleAuxiliaryTask(THREAD_updateUserInterface);
    
    // update leds
    Bela_scheduleAuxiliaryTask(THREAD_updateLEDs);
    
    // update user interface
    Bela_scheduleAuxiliaryTask(THREAD_updateNonAudioTasks);
        
    // write led analog output
    // this has to live here, running it in the thread doesnt seem to work
    for (unsigned int n = 0; n < NUM_LEDS; ++n)
        analogWrite(context, 0, HARDWARE_PIN_LED[n], ledCache[n]);
    
    // display
    if (--displayBlockCtr <= 0)
    {
        displayBlockCtr = DISPLAY_BLOCKS_PER_FRAME;
        
        userinterface.display.update(true);
//        if(userinterface.display.update(false))
//            Bela_scheduleAuxiliaryTask(taskUpdateGUIDisplay);
    }
    
//    // update effects
//    engine.processBlock();

// process SAMPLEWISE ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
    sampleIndex = 0;
    for(; sampleIndex < context->audioFrames; ++sampleIndex)
    {
        userinterface.processNonAudioTasks();
        
        // process effects
        StereoFloat output = inputHandler.process(context, sampleIndex);
        
        // write output buffer
        audioWrite(context, sampleIndex, 0, output.leftSample);
        audioWrite(context, sampleIndex, 1, output.rightSample);
        
#ifdef SCOPE_ACTIVE
        // scope output
        scope.log(output.leftSample, output.rightSample);
#endif
    }
}

// MARK: - CLEANUP
// ********************************************************************************

void cleanup (BelaContext *context, void *userData)
{

}

// MARK: - AUX TASKS
// ********************************************************************************

//void updateGUIdisplay (void* _arg)
//{
//    std::vector<String> _rows = userinterface.display.displaycatch.rows;
//    
//    // convert strings to chars
//    std::vector<std::vector<char>> rows;
//    
//    for (unsigned int n = 0; n < DISPLAY_NUM_ROWS; n++)
//    {
//        std::vector<char> row;
//        std::copy(_rows[n].begin(), _rows[n].end(), std::back_inserter(row));
//        for (unsigned int i = row.size(); i < DISPLAY_NUM_LETTERS_IN_ROW; i++) row.push_back('\0');
//        rows.push_back(row);
//    }
//    
//    // send rows to GUI
//    for (unsigned int n = 0; n < DISPLAY_NUM_ROWS; n++)
//        gui.sendBuffer(n+guiBufferIdx[DSP1], rows[n]);
//}
//


void updateUserInterface(void* arg_)
{
    BelaContext* context = static_cast<BelaContext*>(arg_);
    
    // Initializing UI
    if (guiIsInitializing)
    {
        // counter for awaiting the GUI page to reload
        if (--guiInitializationCtr <= 0) guiIsInitializing = false;
    }
    
    else
    {
        // buttons & potentiometer
        if (--uiBlockCtr <= 0)
        {
            // reset counter
            uiBlockCtr = UI_BLOCKS_PER_FRAME;
            
            // get GUI buffers
            float* guibuttons = gui.getDataBuffer(guiBufferIdx[BUTTONS]).getAsFloat();
            float* guipots = gui.getDataBuffer(guiBufferIdx[POTS]).getAsFloat();
            float* guictrls = gui.getDataBuffer(guiBufferIdx[GUICTRLS]).getAsFloat();
            
            // update buttons and potentiometers
            for (unsigned int n = 0; n < NUM_BUTTONS; ++n)
                userinterface.button[n].update(guibuttons[n], digitalRead(context, 0, HARDWARE_PIN_BUTTON[n]));
            for (unsigned int n = 0; n < NUM_POTENTIOMETERS; ++n)
                userinterface.potentiometer[n].update(guipots[n], analogRead(context, 0, HARDWARE_PIN_POTENTIOMETER[n]));
            
            // get GUI input controls
            InputHandler::Input input = INT2ENUM((int)guictrls[0], InputHandler::Input);
            Track track = INT2ENUM((int)guictrls[1], Track);
            float freq = guictrls[2];
            float volume = 0.01f * guictrls[3];
                        
            // set GUI input paramters if changed
            if (input != inputHandler.getInput()) inputHandler.setInput(input);
            if (track != inputHandler.player.getTrack()) inputHandler.player.setTrack(track);
            if (freq != inputHandler.oscillator.getFrequency()) inputHandler.oscillator.setFrequency(freq);
            if (volume != inputHandler.getVolume()) inputHandler.setVolume(volume);
        }
    }
}


void updateLEDs(void* arg_)
{
    if (--ledBlockCtr <= 0)
    {
        ledBlockCtr = LED_BLOCKS_PER_FRAME;
        
        for (unsigned int n = 0; n < NUM_LEDS; ++n)
            ledCache[n] = userinterface.led[n].getValue();
        
    //    gui.sendBuffer(guiBufferIdx[LEDS], ledCache);
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

#endif // BELA_CONNECTED
