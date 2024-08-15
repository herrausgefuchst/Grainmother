#include "ConstantVariables.h"

#ifdef BELA_CONNECTED

#include "BelaVariables.h"

using namespace BelaVariables;

// MARK: - SETUP
// ********************************************************************************

bool setup (BelaContext *context, void *userData)
{
    // scope
    scope.setup(2, context->audioSampleRate);
    
    // gui
    gui.setup(context->projectName);
    guiBufferIdx[POTS] = gui.setBuffer('f', userinterface.getNumPotentiometers());
    guiBufferIdx[BUTTONS] = gui.setBuffer('f', userinterface.getNumButtons());
    guiBufferIdx[GUICTRLS] = gui.setBuffer('f', NUM_GUI_CONTROLS);
    guiBufferIdx[LEDS] = gui.setBuffer('f', userinterface.getNumLEDs());
    for (unsigned int n = DSP1; n <= DSP10; n++) guiBufferIdx[n] = gui.setBuffer('c', DISPLAY_NUM_LETTERS_IN_ROW);
    
    // display
    DISPLAY_BLOCKS_PER_FRAME = context->audioSampleRate / ( DISPLAY_FRAMERATE * context->audioFrames );
    displayBlockCtr = DISPLAY_BLOCKS_PER_FRAME;
    
    // leds
    LED_BLOCKS_PER_FRAME = context->audioSampleRate / ( LED_FRAMERATE * context->audioFrames );
    ledBlockCtr = LED_BLOCKS_PER_FRAME;
    for (unsigned int n = 0; n < userinterface.getNumLEDs(); ++n) ledCache.push_back(0.f);
    
    // ui rate
    UI_BLOCKS_PER_FRAME = context->audioSampleRate / ( UI_FRAMERATE * context->audioFrames );
    uiBlockCtr = UI_BLOCKS_PER_FRAME;
    guiInitializationCtr = GUI_INITIALIZATION_TIME_SEC * context->audioFrames;
    
    // aux tasks
    if((taskUpdateLEDS = Bela_createAuxiliaryTask(&updateLEDs, 89, "update-LEDs", context)) == 0) return false;
    if((taskUpdateGUIDisplay = Bela_createAuxiliaryTask(&updateGUIdisplay, 88, "update-GUI-display", nullptr)) == 0) return false;
    
    // digital pinmodes
    for (unsigned int n = 0; n < userinterface.getNumButtons(); ++n) pinMode(context, 0, HARDWARE_PIN_BUTTON[n], INPUT);
    
    // setup objects
    inputHandler.setup(context->audioSampleRate, 120.f, 0.7f);
    
    engine.setup(context->audioSampleRate, context->audioFrames);
    
    userinterface.setup(&engine);

    return true;
}

// MARK: - RENDER
// ********************************************************************************

void render (BelaContext *context, void *userData)
{

// update BLOCKWISE +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // Initializing UI
    if (guiIsInitializing)
    {
        // analog default values must be read in render() function
        if (guiInitializationCtr == GUI_INITIALIZATION_TIME_SEC)
            FORLOOP(userinterface.getNumPotentiometers())
                userinterface.potentiometer[n].setDefaults(0.f, analogRead(context, 0, HARDWARE_PIN_POTENTIOMETER[n]));
                
        // counter for awaiting the GUI page to reload
        if (guiInitializationCtr-- <= 0) guiIsInitializing = false;
    }
    
    // buttons & potentiometer
    if (--uiBlockCtr <= 0)
    {
        // reset counter
        uiBlockCtr = UI_BLOCKS_PER_FRAME;
        
        if (!guiIsInitializing)
        {
            // get GUI buffers
            float* guibuttons = gui.getDataBuffer(guiBufferIdx[BUTTONS]).getAsFloat();
            float* guipots = gui.getDataBuffer(guiBufferIdx[POTS]).getAsFloat();
            float* guictrls = gui.getDataBuffer(guiBufferIdx[GUICTRLS]).getAsFloat();
            
            // update buttons and potentiometers
            for (unsigned int n = 0; n < userinterface.getNumButtons(); ++n) userinterface.button[n].update(guibuttons[n], digitalRead(context, 0, HARDWARE_PIN_BUTTON[n]));
            for (unsigned int n = 0; n < userinterface.getNumPotentiometers(); ++n) userinterface.potentiometer[n].update(guipots[n], analogRead(context, 0, HARDWARE_PIN_POTENTIOMETER[n]));
            
            // get GUI input controls
            int input = (int)guictrls[0];
            int track = (int)guictrls[1];
            float freq = guictrls[2];
            float volume = 0.01f * guictrls[3];
            
            // set GUI input paramters if changed
            if (input != inputHandler.getInput()) inputHandler.setInput(INT2ENUM(input, InputHandler::Input));
            if (track != inputHandler.player.getTrack()) inputHandler.player.setTrack(track);
            if (freq != inputHandler.oscillator.getFrequency()) inputHandler.oscillator.setFrequency(freq);
            if (volume != inputHandler.getVolume()) inputHandler.setVolume(volume);
        }
    }
    
    // leds
    if (--ledBlockCtr <= 0)
    {
        ledBlockCtr = LED_BLOCKS_PER_FRAME;
        Bela_scheduleAuxiliaryTask(taskUpdateLEDS);
    }
    FORLOOP(userinterface.getNumLEDs()) analogWrite(context, 0, HARDWARE_PIN_LED[n], ledCache[n]);
    
    // display
    if (--displayBlockCtr <= 0)
    {
        displayBlockCtr = DISPLAY_BLOCKS_PER_FRAME;
        if(userinterface.display.update(false))
            Bela_scheduleAuxiliaryTask(taskUpdateGUIDisplay);
    }
    
    // update effects
    engine.processBlock();

// process SAMPLEWISE ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
    for(unsigned int n = 0; n < context->audioFrames; n++)
    {
        // process effects
        StereoFloat output = engine.process(inputHandler.process(context, n));
        
        // write output buffer
        audioWrite(context, n, 0, output.leftSample);
        audioWrite(context, n, 1, output.rightSample);
        
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

void updateGUIdisplay (void* _arg)
{
    std::vector<String> _rows = userinterface.display.displaycatch.rows;
    
    // convert strings to chars
    std::vector<std::vector<char>> rows;
    
    for (unsigned int n = 0; n < DISPLAY_NUM_ROWS; n++)
    {
        std::vector<char> row;
        std::copy(_rows[n].begin(), _rows[n].end(), std::back_inserter(row));
        for (unsigned int i = row.size(); i < DISPLAY_NUM_LETTERS_IN_ROW; i++) row.push_back('\0');
        rows.push_back(row);
    }
    
    // send rows to GUI
    for (unsigned int n = 0; n < DISPLAY_NUM_ROWS; n++)
        gui.sendBuffer(n+guiBufferIdx[DSP1], rows[n]);
}

void updateLEDs (void* _arg)
{
    for (unsigned int n = 0; n < userinterface.getNumLEDs(); n++)
        ledCache[n] = userinterface.led[n].get();
    
    gui.sendBuffer(guiBufferIdx[LEDS], ledCache);
}

#endif // BELA_CONNECTED
