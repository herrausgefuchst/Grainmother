#include "inputs.hpp"

//#define CONSOLE_PRINT

// MARK: - AUDIO PLAYER
// ********************************************************************************

void AudioPlayer::setup()
{
#ifdef BELA_CONNECTED
    taskFillSampleBuffer = Bela_createAuxiliaryTask(&fillBuffer, 70, "audioPlayer", this);
    
    for (unsigned int n = 0; n < NUM_AUDIO_FILES; ++n)
    {
        numFramesInTrack[n] = AudioFileUtilities::getNumFrames(trackPath[n]);
        if (numFramesInTrack[n] <= 0)
            engine_rt_error("AudioFile is empty!", __FILE__, __LINE__, true);
        if (numFramesInTrack[n] <= bufferLength)
            engine_rt_error("AudioFile is too short!", __FILE__, __LINE__, true);
    }
    
    buffer[0] = AudioFileUtilities::load(trackPath[ENUM2INT(track)], bufferLength, 0);
    buffer[1] = buffer[0];
#endif
}

StereoFloat AudioPlayer::process()
{
#ifdef BELA_CONNECTED
    StereoFloat output = { 0.f, 0.f };
    
    if (++readPtr >= bufferLength)
    {
        if (!doneLoadingBuffer)
            engine_rt_error("Couldn't load buffer in time :( -- try increasing buffer size!", __FILE__, __LINE__, false);
        
        readPtr = 0;
        doneLoadingBuffer = false;
        activeBuffer = !activeBuffer;
        Bela_scheduleAuxiliaryTask(taskFillSampleBuffer);
    }

    output[0] = buffer[activeBuffer][0][readPtr];
    output[1] = buffer[activeBuffer][1][readPtr];
    
    return output;
#else
    return { 0.f, 0.f };
#endif
}

void AudioPlayer::setTrack(Track track_)
{
    track = track_;
    
#ifdef CONSOLE_PRINT
    consoleprint("New Track selected with Path: " + trackPath[ENUM2INT(track)], __FILE__, __LINE__);
#endif
    
    readPtr = 0;
    bufferReadPtr = 0;
    doneLoadingBuffer = true;
    activeBuffer = 0;

#ifdef BELA_CONNECTED
    Bela_scheduleAuxiliaryTask(taskFillSampleBuffer);
#endif
}

void fillBuffer(void* arg)
{
#ifdef BELA_CONNECTED
    AudioPlayer* player = (AudioPlayer*)arg;
    
    // Increment buffer read pointer by buffer length
    player->bufferReadPtr += player->bufferLength;

    // Reset buffer pointer if it exceeds the number of frames in the file
    if (player->bufferReadPtr >= player->numFramesInTrack[ENUM2INT(player->track)])
        player->bufferReadPtr = 0;

    int endFrame = player->bufferReadPtr + player->bufferLength;
    int zeroPad = 0;

    // If reaching the end of the file, take note of the last frame index
    // so we can zero-pad the rest later
    if ((player->bufferReadPtr + player->bufferLength) >= player->numFramesInTrack[ENUM2INT(player->track)] - 1)
    {
        endFrame = player->numFramesInTrack[ENUM2INT(player->track)] - 1;
        zeroPad = 1;
    }
    
    for (unsigned int ch = 0; ch < player->buffer[0].size(); ++ch)
    {
        // Fill (non-active) buffer
        AudioFileUtilities::getSamples(trackPath[ENUM2INT(player->track)], player->buffer[!player->activeBuffer][ch].data(), ch,
                                       player->bufferReadPtr, endFrame);
        
        // Zero-pad if necessary
        if (zeroPad)
        {
            int numFramesToPad = player->bufferLength - (endFrame - player->bufferReadPtr);
            for (int n = 0; n < numFramesToPad; n++)
            {
                player->buffer[!player->activeBuffer][ch][n + (player->bufferLength - numFramesToPad)] = 0;
            }
        }
    }

    player->doneLoadingBuffer = true;
#endif
}



// MARK: - OSCILLATOR
// ********************************************************************************

void Oscillator::setup(const float fs_, const float freq_)
{
    inv_fs = 1.f / fs_;
    freq = freq_;
    incr = TWOPI * freq * inv_fs;
}

float Oscillator::process()
{
    phase += incr;
    if (phase > TWOPI) phase -= TWOPI;

    return approximateSine(phase);
}

void Oscillator::setFrequency(const float freq_)
{
    freq = freq_;
    incr = TWOPI * freq * inv_fs;
}


// MARK: - INPUT HANDLER
// ********************************************************************************

void InputHandler::setup(const float fs_, const float oscfreq_, const float volume_)
{
    oscillator.setup(fs_, oscfreq_);
    
    player.setup();
    
    volume = volume_;
    
    input = FILE;
}

#ifdef BELA_CONNECTED

StereoFloat InputHandler::process(BelaContext* context_, const unsigned int frame_)
{
    StereoFloat output = { 0.f, 0.f };
    
    if (input == FILE)
    {
        output = player.process();
    }
    else if (input == SINEWAVE)
    {
        output[0] = oscillator.process(); // 0.1 = loudness compensation
        output[1] = output[0];
    }
    else /* input == AUDIOIN */
    {
        output[0] = audioRead(context_, frame_, 0);
        output[1] = audioRead(context_, frame_, 1);
    }
    
    output *= volume;
    
    return output;
}

#endif
