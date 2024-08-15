#ifdef BELA_CONNECTED

#include "predef.h"

using namespace BelaVariables;

void updateGUIdisplay (void* _arg);
void updateLEDs (void* _arg);

// MARK: - SETUP
// ********************************************************************************

bool setup (BelaContext *context, void *userData)
{
    // scope
    scope.setup(2, context->audioSampleRate);
    
    // gui
    gui.setup(context->projectName);
    guibuffer_idx[POTS] = gui.setBuffer('f', userinterface.getNumPotentiometers());
    guibuffer_idx[BUTTONS] = gui.setBuffer('f', userinterface.getNumButtons());
    guibuffer_idx[GUICTRLS] = gui.setBuffer('f', NUM_GUI_CONTROLS);
    guibuffer_idx[LEDS] = gui.setBuffer('f', userinterface.getNumLEDs());
    for (unsigned int n = DSP1; n <= DSP10; n++) guibuffer_idx[n] = gui.setBuffer('c', DISPLAY_NUM_LETTERS_IN_ROW);
    
    // display
    DISPLAY_BLOCKS_PER_FRAME = context->audioSampleRate / ( DISPLAY_FRAMERATE * context->audioFrames);
    display_block_ctr = DISPLAY_BLOCKS_PER_FRAME;
    
    // leds
    LED_BLOCKS_PER_FRAME = context->audioSampleRate / (LED_FRAMERATE * context->audioFrames);
    led_block_ctr = LED_BLOCKS_PER_FRAME;
    for (unsigned int n = 0; n < userinterface.getNumLEDs(); n++) led_catch.push_back(0.f);
    
    // ui rate
    UI_BLOCKS_PER_FRAME = context->audioSampleRate / (UI_FRAMERATE * context->audioFrames);
    ui_block_ctr = UI_BLOCKS_PER_FRAME;
    gui_initialization_ctr = GUI_INITIALIZATION_TIME_SEC * context->audioFrames;
    
    // aux tasks
    if((taskUpdateLEDS = Bela_createAuxiliaryTask(&updateLEDs, 89, "update-LEDs", context)) == 0) return false;
    if((taskUpdateGUIDisplay = Bela_createAuxiliaryTask(&updateGUIdisplay, 88, "update-GUI-display", nullptr)) == 0) return false;

    // random seed
    srand((int)time(NULL));
    
    // digital pinmodes
    for (unsigned int n = 0; n < userinterface.getNumButtons(); n++) pinMode(context, 0, PIN_BUTTON[n], INPUT);

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
        if (gui_initialization_ctr == GUI_INITIALIZATION_TIME_SEC)
            FORLOOP(userinterface.getNumPotentiometers())
                userinterface.potentiometer[n].setDefaults(0.f, analogRead(context, 0, PIN_POT[n]));
                
        // counter for awaiting the GUI page to reload
        if (gui_initialization_ctr-- <= 0) guiIsInitializing = false;
    }
    
    // buttons & potentiometer
    if (--ui_block_ctr <= 0)
    {
        // reset counter
        ui_block_ctr = UI_BLOCKS_PER_FRAME;
        
        if (!guiIsInitializing)
        {
            // get GUI buffers
            float* guibuttons = gui.getDataBuffer(guibuffer_idx[BUTTONS]).getAsFloat();
            float* guipots = gui.getDataBuffer(guibuffer_idx[POTS]).getAsFloat();
            float* guictrls = gui.getDataBuffer(guibuffer_idx[GUICTRLS]).getAsFloat();
            
            // update buttons and potentiometers
            for (unsigned int n = 0; n < userinterface.getNumButtons(); n++) userinterface.button[n].update(guibuttons[n], digitalRead(context, 0, PIN_BUTTON[n]));
            for (unsigned int n = 0; n < userinterface.getNumPotentiometers(); n++) userinterface.potentiometer[n].update(guipots[n], analogRead(context, 0, PIN_POT[n]));
            
            // get GUI input controls
            int input = (int)guictrls[0];
            int track = (int)guictrls[1];
            float freq = guictrls[2];
            float volume = 0.01f * guictrls[3];
            
            // set GUI input paramters if changed
            if (input != inputHandler.getInput()) inputHandler.setInput(input);
            if (track != inputHandler.player.getTrack()) inputHandler.player.setTrack(track);
            if (freq != inputHandler.oscillator.getFrequency()) inputHandler.oscillator.setFrequency(freq);
            if (volume != inputHandler.getVolume()) inputHandler.setVolume(volume);
        }
    }
    
    // leds
    if (--led_block_ctr <= 0)
    {
        led_block_ctr = LED_BLOCKS_PER_FRAME;
        Bela_scheduleAuxiliaryTask(taskUpdateLEDS);
    }
    FORLOOP(userinterface.getNumLEDs()) analogWrite(context, 0, PIN_LED[n], led_catch[n]);
    
    // display
    if (--display_block_ctr <= 0)
    {
        display_block_ctr = DISPLAY_BLOCKS_PER_FRAME;
        if(userinterface.display.update(false))
            Bela_scheduleAuxiliaryTask(taskUpdateGUIDisplay);
    }
    
    // update effects
    engine.processBlock();

// process SAMPLEWISE ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
    for(unsigned int n = 0; n < context->audioFrames; n++)
    {
        // process effects
        std::pair<float,float> output = engine.process(inputHandler.process(context, n));
        
        // write output buffer
        audioWrite(context, n, 0, output.first);
        audioWrite(context, n, 1, output.second);
        
#ifdef SCOPE_ACTIVE
        // scope output
        scope.log(output.first, output.second);
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
        gui.sendBuffer(n+guibuffer_idx[DSP1], rows[n]);
}

void updateLEDs (void* _arg)
{
    for (unsigned int n = 0; n < userinterface.getNumLEDs(); n++)
        led_catch[n] = userinterface.led[n].get();
    
    gui.sendBuffer(guibuffer_idx[LEDS], led_catch);
}

#endif // BELA_CONNECTED
