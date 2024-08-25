#ifndef predef_h
#define predef_h

#include <Bela.h>
#include <libraries/Gui/Gui.h>
#include <libraries/Scope/Scope.h>
#include <libraries/Midi/Midi.h>

#include "inputs.hpp"
#include "engine.hpp"

#define SCOPE_ACTIVE

//void updateGUIdisplay (void* _arg);
void updateLEDs(void* arg_);
void updateUserInterface(void* arg_);
void updateNonAudioTasks(void* arg_);
void midiMessageCallback(MidiChannelMessage message, void* arg);

namespace BelaVariables
{

unsigned int sampleIndex = 0;

// hardware pins for potentiometers, buttons and leds
static const int HARDWARE_PIN_POTENTIOMETER[] = { 6, 5, 4, 3, 7, 0, 1, 2 };
static const int HARDWARE_PIN_BUTTON[] = { 2, 4, 0, 5, 3, 1, 15, 13, 14, 12 };
static const int HARDWARE_PIN_LED[] = { 0, 1, 2, 3, 4, 5 };

// framerates = num updates per second
// ! for buttons: be sure to also change debouncetime and longpresstime when changing framerate !
static const unsigned int DISPLAY_FRAMERATE = 12;
static const unsigned int LED_FRAMERATE = 200;
static const unsigned int UI_FRAMERATE = 40;
static const unsigned int SCROLLING_FRAMERATE = 30;

// the variables for blocks per frame and corresponding counters manage when an update function is gonna be called
unsigned int DISPLAY_BLOCKS_PER_FRAME;
int displayBlockCtr;

unsigned int LED_BLOCKS_PER_FRAME;
int ledBlockCtr;
std::array<float, NUM_LEDS> ledCache;

unsigned int UI_BLOCKS_PER_FRAME;
int uiBlockCtr;

unsigned int SCROLLING_BLOCKS_PER_FRAME;
int scrollingBlockCtr;

// helpers for gui
static const unsigned int DISPLAY_NUM_LETTERS_IN_ROW = 30;
static const unsigned int NUM_GUI_CONTROLS = 4;
static const unsigned int GUI_INITIALIZATION_TIME_SEC = 20;

enum GuiBuffers { POTS, BUTTONS, GUICTRLS, LEDS, DSP1, DSP2, DSP3, DSP4, DSP5, DSP6, DSP7, DSP8, DSP9, DSP10, NUM_GUIBUFFERS };
int guiBufferIdx[NUM_GUIBUFFERS];
int guiInitializationCtr = 0;
bool guiIsInitializing = true;

// object: BELA
Gui gui;

#ifdef SCOPE_ACTIVE
Scope scope;
#endif

Midi midi;

// object for the audio player and input controls in GUI
InputHandler inputHandler;

// object for the processing engine
AudioEngine engine;

// object for handling the interfaces (gui, analog in, midi)
UserInterface userinterface;

// threads
//AuxiliaryTask taskUpdateGUIDisplay;
AuxiliaryTask THREAD_updateLEDs;
AuxiliaryTask THREAD_updateUserInterface;
AuxiliaryTask THREAD_updateNonAudioTasks;

}; // namespace BelaVariables


#endif /* predef_h */
