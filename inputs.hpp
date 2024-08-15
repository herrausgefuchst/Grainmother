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

/**
 * @class AudioPlayer
 * @brief A class for handling audio playback and processing.
 */
class AudioPlayer
{
public:
#ifdef BELA_CONNECTED
    /**
     * @brief Sets up the audio player (e.g., initializing buffers).
     * @return True if setup is successful, false otherwise.
     */
    bool setup();
    
    /**
     * @brief Processes the audio and returns the stereo output.
     * @return The processed stereo audio output.
     */
    StereoFloat process();

    AuxiliaryTask taskFillSampleBuffer; /**< Thread for filling the sample buffer */
#endif
    
    /**
     * @brief Sets the current track to be played.
     * @param track_ The index of the track to set.
     */
    void setTrack(const int track_);

    /**
     * @brief Gets the current track index.
     * @return The index of the current track.
     */
    int getTrack() const { return track; }

public:
    int track = 0; /**< Index of the current track */
    std::vector<std::string> tracknames = {"waves.wav", "drums.wav", "vocals.wav", "orchestra.wav", "synth.wav"}; /**< Names of available tracks */
    int numFramesInTrack[NUM_AUDIO_FILES]; /**< Number of frames in each track */

    std::vector<std::vector<float>> buffer[2]; /**< Double buffer for audio data */
    const int buflength = 22050; /**< Length of each buffer */
    int read_ptr = buflength; /**< Read pointer for the buffer */
    int buf_read_ptr = 0; /**< Buffer read pointer */
    int doneLoadingBuffer = 1; /**< Flag indicating if the buffer is done loading */
    int activeBuffer = 0; /**< Index of the active buffer */
};


// MARK: - OSCILLATOR
// ********************************************************************************

/**
 * @class Oscillator
 * @brief A class representing an oscillator for generating waveforms.
 */
class Oscillator
{
public:
    /**
     * @brief sets up an Oscillator with a specified sampling rate and frequency.
     * @param fs_ The sampling rate.
     * @param freq_ The oscillator frequency (default is 120 Hz).
     */
    void setup(const float fs_, const float freq_ = 120.f);

    /**
     * @brief Processes the oscillator to generate the next sample.
     * @return The generated sample.
     */
    float process();

    /**
     * @brief Sets a new frequency for the oscillator.
     * @param freq_ The new frequency to set.
     */
    void setFrequency(const float freq_);

    /**
     * @brief Gets the current frequency of the oscillator.
     * @return The current frequency.
     */
    float getFrequency() const { return freq.getCurrent(); }

private:
    Ramp freq; /**< Ramp object to handle frequency changes */
    float inv_fs; /**< Inverse of the sampling rate */
    const float two_pi = 2.f * (float)M_PI; /**< Constant for 2 * PI */
    float incr = 0.f; /**< Increment value per sample */
    float phase = 0.f; /**< Current phase of the oscillator */
};

// MARK: - INPUT HANDLER
// ********************************************************************************

/**
 * @class InputHandler
 * @brief A class responsible for handling different input sources and processing audio.
 */
class InputHandler
{
public:
    /**
     * @enum Input
     * @brief Enumeration for the types of input sources.
     */
    enum Input { FILE, SINEWAVE, AUDIOIN };
    
    /**
     * @brief Sets up the InputHandler with the specified sampling rate, oscillator frequency, and volume.
     * @param fs_ The sampling rate.
     * @param oscfreq_ The oscillator frequency.
     * @param volume_ The volume level.
     */
    void setup(const float fs_, const float oscfreq_, const float volume_);

#ifdef BELA_CONNECTED
    /**
     * @brief Processes the audio for a given frame in the Bela context.
     * @param context_ The Bela context pointer.
     * @param frame_ The frame index to process.
     * @return The processed stereo audio output.
     */
    StereoFloat process(BelaContext* context_, const unsigned int frame_);
#endif
    
    AudioPlayer player; /**< The audio player instance */
    Oscillator oscillator; /**< The oscillator instance */
    
    /**
     * @brief Sets the input source.
     * @param input_ The input source to set.
     */
    void setInput(Input input_) { input = input_; }

    /**
     * @brief Sets the volume level with a ramp.
     * @param volume_ The target volume level.
     */
    void setVolume(float volume_) { volume.setRampTo(volume_, 100.f); }

    /**
     * @brief Gets the current input source.
     * @return The current input source.
     */
    Input getInput() const { return input; }

    /**
     * @brief Gets the current volume level.
     * @return The current volume level.
     */
    float getVolume() const { return volume.getCurrent(); }
        
private:
    Input input = FILE; /**< The current input source */
    Ramp volume; /**< The volume ramp control */
};

#endif /* inputs_hpp */
