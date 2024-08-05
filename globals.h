#ifndef globals_h
#define globals_h

#include "functions.h"

#define NUM_PRESETS 4

enum PotBehaviour { POTBEHAVIOUR_JUMP, POTBEHAVIOUR_CATCH };

struct GlobalParameters
{
    
    unsigned int midiInChannel = 1;
    
    unsigned int midiOutChannel = 1;
    
    unsigned int lastUsedPreset = 0;
    
    unsigned int potBehaviour = POTBEHAVIOUR_JUMP;
    
    String presetNames[NUM_PRESETS] = { ("empty") };
    
};

#endif /* globals_h */
