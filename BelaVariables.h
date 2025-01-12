#ifndef predef_h
#define predef_h

#include <Bela.h>
#include <libraries/Midi/Midi.h>

#include "Engine.h"

//#define SCOPE_ACTIVE

#ifdef SCOPE_ACTIVE
#include <libraries/Scope/Scope.h>
#endif

void updateLEDs();
void updateUserInterface(void* arg_);
void updateAudioBlock(void* arg_);
void updateNonAudioTasks(void* arg_);
void midiInputMessageCallback(MidiChannelMessage message, void* arg);
void midiOutputMessageCallback(uint ccIndex_, uint ccValue_);

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
static const unsigned int UI_FRAMERATE = 120;
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

#ifdef SCOPE_ACTIVE
Scope scope;
#endif

Midi midi;

// object for the processing engine
AudioEngine engine;

// object for handling the interfaces (gui, analog in, midi)
UserInterface userinterface;

// threads
AuxiliaryTask THREAD_updateUserInterface;
AuxiliaryTask THREAD_updateNonAudioTasks;
AuxiliaryTask THREAD_updateAudioBlock;

}; // namespace BelaVariables


#endif /* predef_h */
