#ifndef globals_h
#define globals_h

#include "functions.h"

#define NUM_PRESETS 4

enum class PotBehaviour { JUMP, CATCH };

struct GlobalParameters
{
    
    unsigned int midiInChannel = 1;
    
    unsigned int midiOutChannel = 1;
    
    unsigned int lastUsedPreset = 0;
    unsigned int potBehaviour = ENUM2INT(PotBehaviour::JUMP);
    
    String presetNames[NUM_PRESETS] = { ("empty") };
    
};

#endif /* globals_h */
