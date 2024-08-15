#include "inputs.hpp"

// MARK: - AUDIO PLAYER
// ********************************************************************************

#ifdef BELA_CONNECTED

bool AudioPlayer::setup()
{
    if((taskFillSampleBuffer = Bela_createAuxiliaryTask(&fillBuffer, 90, "fill-buffer", this)) == 0) return false;
    
    for(unsigned int n = 0; n < NUM_AUDIO_FILES; n++)
    {
        numFramesInTrack[n] = AudioFileUtilities::getNumFrames(tracknames[n]);
        if(numFramesInTrack[n] <= 0) return false;
        if(numFramesInTrack[n] <= buflength) { printf("sample too short\n"); return false; }
    }
    
    buffer[0] = AudioFileUtilities::load(tracknames[track], buflength, 0);
    buffer[1] = buffer[0];
    
    return true;
}

StereoFloat AudioPlayer::process()
{
    StereoFloat output = { 0.f,0.f };
    
    if (++read_ptr >= buflength)
    {
        if(!doneLoadingBuffer)
            rt_printf("Couldn't load buffer in time :( -- try increasing buffer size!");
        read_ptr = 0;
        doneLoadingBuffer = 0;
        activeBuffer = !activeBuffer;
        Bela_scheduleAuxiliaryTask(taskFillSampleBuffer);
    }

    output[0] = buffer[activeBuffer][0][read_ptr];
    output[1] = buffer[activeBuffer][1][read_ptr];
    
    return output;
}

void AudioPlayer::setTrack (int _track)
{
    if (_track < 0 || _track >= NUM_AUDIO_FILES) rt_printf("track doesnt exist!");
    track = _track;
    
    read_ptr = 0;
    buf_read_ptr = 0;
    doneLoadingBuffer = 1;
    activeBuffer = 0;
    
    Bela_scheduleAuxiliaryTask(taskFillSampleBuffer);
}

void fillBuffer(void* arg) {
    AudioPlayer* player = (AudioPlayer*)arg;
    
    // increment buffer read pointer by buffer length
    player->buf_read_ptr += player->buflength;

    // reset buffer pointer if it exceeds the number of frames in the file
    if(player->buf_read_ptr >= player->numFramesInTrack[player->track]) player->buf_read_ptr = 0;

    int endFrame = player->buf_read_ptr + player->buflength;
    int zeroPad = 0;

    // if reaching the end of the file take note of the last frame index
    // so we can zero-pad the rest later
    if((player->buf_read_ptr + player->buflength) >= player->numFramesInTrack[player->track] - 1) {
          endFrame = player->numFramesInTrack[player->track] - 1;
          zeroPad = 1;
    }

    for (unsigned int ch = 0; ch < player->buffer[0].size(); ++ch)
    {
        // fill (nonactive) buffer
        AudioFileUtilities::getSamples(player->tracknames[player->track], player->buffer[!player->activeBuffer][ch].data(), ch
                    , player->buf_read_ptr, endFrame);

        // zero-pad if necessary
        if(zeroPad) {
            int numFramesToPad = player->buflength - (endFrame - player->buf_read_ptr);
            for(int n = 0; n < numFramesToPad; n++)
                player->buffer[!player->activeBuffer][ch][n + (player->buflength - numFramesToPad)] = 0;
        }
    }

    player->doneLoadingBuffer = 1;
}

#endif


// MARK: - OSCILLATOR
// ********************************************************************************

Oscillator::Oscillator (const float _fs, const float _freq)
: inv_fs(1.f / _fs), freq(_freq, _fs)
{}

float Oscillator::process()
{
    if (freq.process()) incr = two_pi * freq.getCurrent() * inv_fs;
    
    phase += incr;
    if (phase > (float)M_PI) phase -= two_pi;

    return sinf_neon(phase);
}

void Oscillator::setFrequency (const float _freq)
{
    freq.setRampTo(_freq, 100.f);
    incr = two_pi * freq.getCurrent() * inv_fs;
}


// MARK: - INPUT HANDLER
// ********************************************************************************

InputHandler::InputHandler (const float _fs, const float _oscfreq, const float _volume)
: oscillator(_fs, _oscfreq), volume(_volume, _fs)
{}

#ifdef BELA_CONNECTED

StereoFloat InputHandler::process (BelaContext* _context, const int _frame)
{
    volume.process();
    
    StereoFloat output = { 0.f,0.f };
    
    if (input == FILE)
    {
        output = player.process();
    }
    
    else if (input == SINEWAVE)
    {
        output[0] = 0.1f * oscillator.process(); // 0.1 = loudness compensation
        output[1] = output[1];
    }
    
    else /* input == AUDIOIN */
    {
        output[0] = audioRead(_context, _frame, 0);
        output[1] = audioRead(_context, _frame, 1);
    }
    
    output *= volume.getCurrent();
    
    return output;
}

#endif
