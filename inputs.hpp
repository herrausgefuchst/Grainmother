#ifndef inputs_hpp
#define inputs_hpp

#include "functions.h"
#include "helpers.hpp"

#ifdef BELA_CONNECTED
#include <Bela.h>
#include <libraries/AudioFile/AudioFile.h>
#include <libraries/math_neon/math_neon.h>
#else
#include <iostream>
#include <vector>
#include <cmath>
#endif



void fillBuffer(void* arg);

// MARK: - AUDIO PLAYER
// ********************************************************************************

#define NUM_AUDIO_FILES 5

class AudioPlayer
{
public:
    AudioPlayer() {}
    ~AudioPlayer() {}

#ifdef BELA_CONNECTED
    bool setup();
    
    FloatPair process();
    
    AuxiliaryTask taskFillSampleBuffer;
#endif
    
    void setTrack (const int _track);
    int getTrack() const { return track; }

    int track = 0;
    std::vector<std::string> tracknames = {"waves.wav", "drums.wav", "vocals.wav", "orchestra.wav", "synth.wav"};
    int numFramesInTrack[NUM_AUDIO_FILES];

    std::vector<std::vector<float>> buffer[2];
    const int buflength = 22050;
    int read_ptr = buflength;
    int buf_read_ptr = 0;
    int doneLoadingBuffer = 1;
    int activeBuffer = 0;
};


// MARK: - OSCILLATOR
// ********************************************************************************

class Oscillator
{
public:
    Oscillator() = delete;
    Oscillator (const float _fs, const float _freq = 120.f);
    ~Oscillator() {}
        
    float process();
    
    void setFrequency (const float _freq);
    float getFrequency() const { return freq.getCurrent(); }
    
private:
    Ramp freq;
    const float inv_fs;
    const float two_pi = 2.f * (float)M_PI;
    float incr = 0.f;
    float phase = 0.f;
};


// MARK: - INPUT HANDLER
// ********************************************************************************

class InputHandler
{
public:
    enum Input { FILE, SINEWAVE, AUDIOIN };

    InputHandler() : InputHandler(44100.f, 120.f, 0.f) {}
    InputHandler (const float _fs) : InputHandler(_fs, 120.f, 0.f) {}
    InputHandler (const float _fs, const float _oscfreq, const float _volume);
        
#ifdef BELA_CONNECTED
    FloatPair process (BelaContext* _context, const int _frame);
#endif
    
    AudioPlayer player;
    Oscillator oscillator;
    
    void setInput (int _input) { input = _input; }
    void setVolume (float _volume) { volume.setRampTo(_volume, 100.f); }

    int getInput() const { return input; }
    float getVolume() const { return volume.getCurrent(); }
        
private:
    int input = FILE;
    Ramp volume;
};

#endif /* inputs_hpp */
