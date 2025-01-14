#include <iostream>
#include "Engine.h"

AudioEngine engine;
UserInterface userinterface;

unsigned int numBlocks = 2000;
unsigned int blockSize = 128;

void updateParameters()
{
}

int main(int argc, const char * argv[]) {

    engine.setup(44100.f, blockSize);
    userinterface.setup(&engine, 44100.f);
    
    for (unsigned int n = 0; n < numBlocks; ++n)
    {
        userinterface.display.update();
        userinterface.updateNonAudioTasks();
        
        for (unsigned int m = 0; m < blockSize; ++m)
        {
            if (m == 1 && n == 1) userinterface.button[BUTTON_TEMPO].pressButton();
            if (m == 1 && n == 40) userinterface.button[BUTTON_UP].clickButton();
            
            engine.processAudioSamples({0.f, 0.f}, m);
            userinterface.processNonAudioTasks();
        }
        
    }
    
    return 0;
}
