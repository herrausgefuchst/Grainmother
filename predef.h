#ifndef predef_h
#define predef_h

#include <Bela.h>
#include <libraries/Gui/Gui.h>
#include <libraries/Scope/Scope.h>

#include "inputs.hpp"
#include "engine.hpp"

#define SCOPE_ACTIVE

namespace BelaVariables
{
    static const int PIN_POT[] = { 6, 5, 4, 3, 7, 0, 1, 2 };
    static const int PIN_BUTTON[] = { 2, 4, 0, 5, 3, 1, 15, 13, 14, 12 };
    static const int PIN_LED[] = { 0, 1, 2, 3, 4, 5 };

    // framerate = num updates per second
    // ! for buttons: be sure to also change debouncetime and longpresstime when changing framerate !
    static const int DISPLAY_FRAMERATE = 12;
    static const int DISPLAY_NUM_LETTERS_IN_ROW = 30;
    static const int LED_FRAMERATE = 10;
    static const int UI_FRAMERATE = 40;
    static const int NUM_GUI_CONTROLS = 4;
    static const int GUI_INITIALIZATION_TIME_SEC = 20;

    int DISPLAY_BLOCKS_PER_FRAME;
    int display_block_ctr;

    int LED_BLOCKS_PER_FRAME;
    int led_block_ctr;
    std::vector<float> led_catch;

    int UI_BLOCKS_PER_FRAME;
    int ui_block_ctr;

    Gui gui;
    enum GuiBuffers { POTS, BUTTONS, GUICTRLS, LEDS, DSP1, DSP2, DSP3, DSP4, DSP5, DSP6, DSP7, DSP8, DSP9, DSP10, NUM_GUIBUFFERS };
    int guibuffer_idx[NUM_GUIBUFFERS];
    int gui_initialization_ctr = 0;
    bool guiIsInitializing = true;

    Scope scope;
    InputHandler inputHandler(FS);
    AudioEngine engine(FS, BLOCKSIZE);
    UserInterface userinterface(&engine);

    AuxiliaryTask taskUpdateGUIDisplay;
    AuxiliaryTask taskUpdateLEDS;
};


#endif /* predef_h */
