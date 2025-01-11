#include "Granulation.h"

//#define CONSOLE_PRINT

using namespace Granulation;

// =======================================================================================
// MARK: - HIGHCUT FILTER
// =======================================================================================


FilterStereo::FilterStereo()
{
    for (uint n = 0; n < numLowpassFilter; ++n) LPF[n].setAlpha(g);
    
    APF.setup(g, TPT1stOrderFilterStereo::APF);
}


void FilterStereo::setup(const float sampleRate_, const float cutoff_)
{
    sampleRate = sampleRate_;
    invSampleRate = 1.f / sampleRate;

    setCutoffFrequency(cutoff_);
}


float32x2_t FilterStereo::processAudioSamples(float32x2_t input_)
{
    // --- sum up all 1 pole Filter Feedback values
    float32x2_t sum = vdup_n_f32(0.f);
    
    if (model == MOOGLADDER)
    {
        for (uint n = 0; n < numLowpassFilter; ++n)
            sum = vadd_f32(sum, LPF[n].getFeedbackValue());
    }
    
    else // (model == MOOGHALFLADDER)
    {
        for (uint n = 0; n < 2; ++n)
            sum = vadd_f32(sum, LPF[n].getFeedbackValue());

        sum = vadd_f32(sum, APF.getFeedbackValue());
    }
    
    // combine sum and the current new sample
    float32x2_t u = vmls_n_f32(input_, sum, resonance);
    u = vmul_n_f32(u, alpha0);
    
    // cascade through the 1 pole filters
    if (model == MOOGLADDER)
        return LPF[3].processAudioSamples(LPF[2].processAudioSamples(LPF[1].processAudioSamples(LPF[0].processAudioSamples(u))));
    
    else // (model == MOOGHALFLADDER)
        return APF.processAudioSamples(LPF[1].processAudioSamples(LPF[0].processAudioSamples(u)));
}


void FilterStereo::setCutoffFrequency(const float freq_)
{
    // save cutoff frequency internally and bound it to valid values
    cutoff = freq_;
    boundValue(cutoff, 40.0f, 22000.0f);
    
    // prewarp
    float k = tanf_neon(PI * cutoff * invSampleRate);
    
    // precalc for faster cpu
    float k1 = 1.0f / (k + 1.0f);
    
    // g = feedforward coeff in the 1 pole filter
    g = k * k1;
    g_apf = 2.0f * g - 1.0f;
    
    // calc the beta coeffs for 1 pole filters
    if (model == MOOGLADDER)
    {
        LPF[0].setBeta(g * g * g * k1);
        LPF[1].setBeta(g * g * k1);
        LPF[2].setBeta(g * k1);
        LPF[3].setBeta(k1);
    }
    
    else // (model == MOOGHALFLADDER)
    {
        LPF[0].setBeta(g_apf * g * k1);
        LPF[1].setBeta(g_apf * k1);
        APF.setBeta(2.0f * k1);
    }
    
    calcResonance();
}


void FilterStereo::setResonance(const float reso_)
{
    resonanceAmount = reso_;
    calcResonance();
}


void FilterStereo::setFilterModel(const Model model_)
{
    model = model_;
    
    for (uint n = 0; n < numLowpassFilter; ++n) LPF[n].reset();
    APF.reset();
    
    setCutoffFrequency(cutoff);
}


void FilterStereo::calcResonance()
{
    // resonance is cutoff frequency dependant
    // map the frequency to an amount of resonance
    float reso = mapValue(cutoff, 120.f, 20000.f, 0.f, 1.f);
    // for smoother transition: make it logarithmic
    reso = lin2log(reso);
    // turn it around (low frequency = high resonance)
    reso = 1.f - reso;
    
    reso *= resonanceAmount;
    
    // map and bound Resonance
    resonance = lin2log(reso);
    
    if (model == MOOGLADDER)
    {
        resonance *= 3.9999f;
        boundValue(resonance, 0.f, 3.9999f);
    }
    else // MOOGHALFLADDER
    {
        resonance *= 2.0f;
        boundValue(resonance, 0.f, 2.0f);
        alpha0 = 1.0f / (1.0f + resonance * g_apf * g * g);
    }
    
    if (model == MOOGLADDER)
        alpha0 = 1.0f / (1.0f + resonance * g * g * g * g);
    
    else // MOOGHALFLADDER
        alpha0 = 1.0f / (1.0f + resonance * g_apf * g * g);
}


// =======================================================================================
// MARK: - ENVELOPES
// =======================================================================================


ParabolicEnvelope::ParabolicEnvelope(const uint durationSamples_, const float grainAmplitude_)
    : Envelope(durationSamples_, grainAmplitude_)
{
    float r = 1.f / (float)durationSamples_;
    float r2 = r * r;
    slope = 4.f * grainAmplitude * (r - r2);
    curve = -8.f * grainAmplitude * r2;
}


float ParabolicEnvelope::getNextAmplitude()
{
    nextAmplitude += slope;
    slope += curve;
    return nextAmplitude;
}


HannEnvelope::HannEnvelope(const uint durationSamples_, const float grainAmplitude_)
    : Envelope(durationSamples_, grainAmplitude_)
{
    phase = 0;
    invMaxPhase = 1.f / (float)(durationSamples - 1);
}


float HannEnvelope::getNextAmplitude()
{
    nextAmplitude = 0.5f * (1.f - cosf_neon(TWOPI * phase * invMaxPhase));
    nextAmplitude *= grainAmplitude;
    
    ++phase;
    
    return nextAmplitude;
}


TriangularEnvelope::TriangularEnvelope(const uint durationSamples_, const float grainAmplitude_)
    : Envelope(durationSamples_, grainAmplitude_)
{
    phase = 0;
    invMaxPhase = 1.f / (float)(durationSamples - 1);
}


float TriangularEnvelope::getNextAmplitude()
{
    nextAmplitude = 1.f - fabsf_neon(2.f * phase * invMaxPhase - 1.f);
    nextAmplitude *= grainAmplitude;
    
    ++phase;
    
    return nextAmplitude;
}


// =======================================================================================
// MARK: - GRAIN PROPERTIES MANAGER
// =======================================================================================

const int GrainPropertiesManager::MIN_INITDELAY = 5;
const int GrainPropertiesManager::MAX_INITDELAY = 5000;


void GrainPropertiesManager::setup(float sampleRate_)
{
    // define sample rate dependant constants,
    // boundaries for InterOnset time and Grainlength in samples
    MIN_INTERONSET = sampleRate_ / MAX_DENSITY;
    MAX_INTERONSET = sampleRate_ / MIN_DENSITY;
        
    MIN_GRAINLENGTH_SAMPLES = MIN_GRAINLENGTH_MS * sampleRate_ / 1000.f;
    MAX_GRAINLENGTH_SAMPLES = MAX_GRAINLENGTH_MS * sampleRate_ / 1000.f;
}


void GrainPropertiesManager::setLengthVariation(const float variation_)
{
    // variation in grainlength will only appear when slider is higher than this threshold
    static const float sliderThreshold = 0.55f;
    // may be adjusted at will, controls the maximum range that the grainlength can vary in
    static const uint maxVariationSamples = 2300;
    
    if (variation_ < sliderThreshold) lengthRange = 0.f;
    else
    {
        // rescale the slider value to a value between 0...1
        float variationAmount = mapValue(variation_, sliderThreshold, 1.f, 0.f, 1.f);
        // calculate the variation range in samples
        lengthRange = variationAmount * maxVariationSamples;
    }
}


void GrainPropertiesManager::setInterOnsetVariation(const float variation_)
{
    // variation in interonset time will only appear when slider is higher than this threshold
    static const float sliderThreshold = 0.68f;
    // may be adjusted at will, controls the maximum range that the interonset can vary in
    static const uint maxVariationSamples = 15000;
    
    if (variation_ < sliderThreshold) interOnsetRange = 0.f;
    else
    {
        // rescale the slider value to a value between 0...1
        float variationAmount = mapValue(variation_, sliderThreshold, 1.f, 0.f, 1.f);
        // rescale to logarithmic values
        variationAmount = lin2log(variationAmount);
        // calculate the variation range in samples
        interOnsetRange = variationAmount * maxVariationSamples;
    }
}


void GrainPropertiesManager::setInitDelayVariation(const float variation_)
{
    // variation in grainlength will only appear when slider is higher than this threshold
    static const float sliderThreshold = 0.22f;
    // may be adjusted at will, controls the maximum range that the initial delay can vary in
    static const uint maxVariationSamples = 2 * MAX_INITDELAY;
    
    if (variation_ < sliderThreshold) initDelayRange = 0.f;
    else
    {
        // rescale the slider value to a value between 0...1
        float variationAmount = mapValue(variation_, sliderThreshold, 1.f, 0.f, 1.f);
        // calculate the variation range in samples
        initDelayRange = variationAmount * maxVariationSamples;
    }
}


void GrainPropertiesManager::setPanningVariation(const float variation_)
{
    static const float maxVariation = 0.9f;
    
    // scale the value exponentially
    // changes should appear faster at the beginning of the slider turn
    float variationAmount = powf(variation_, 0.5f);
    // calculate the variation range
    panningRange = variationAmount * maxVariation;
}


int GrainPropertiesManager::getNextInterOnset()
{
    int nextInterOnset;
    
    if (interOnsetRange == 0) nextInterOnset = interOnsetCenter;
    else
    {
        int min = interOnsetCenter - 0.5f * interOnsetRange;
        if (min < MIN_INTERONSET) min = MIN_INTERONSET;
        int max = interOnsetCenter + 0.5f * interOnsetRange;
        if (max > MAX_INTERONSET) max = MAX_INTERONSET;
        
        // uniform distribution in the range around the predefined center position
        nextInterOnset = min + rand() * RAND_MAX_INVERSED * (max-min);
    }
        
    return nextInterOnset;
}


GrainProperties* GrainPropertiesManager::getNextGrainProperties()
{
    // inital delay
    if (initDelayRange == 0) props.initDelay = initDelayCenter;
    else
    {
        int min = initDelayCenter - 0.5f * initDelayRange;
        if (min < MIN_INITDELAY) min = MIN_INITDELAY;
        int max = initDelayCenter + 0.5f * initDelayRange;
        if (max > MAX_INITDELAY) max = MAX_INITDELAY;
        
        // Define standard deviation as a fraction of the range
        float stddev = initDelayRange * 0.04166667f; // /= 24
        
        // Generate a Gaussian-distributed random number
        float randomDelay = generateGaussian(initDelayCenter, stddev);

        // Clip the value within the bounds [min, max]
        if (randomDelay < min) randomDelay = min;
        if (randomDelay > max) randomDelay = max;
        
        props.initDelay = randomDelay;
    }
    
    // grainlength
    if (lengthRange == 0) props.length = lengthCenter;
    else
    {
        int min = lengthCenter - 0.5f * lengthRange;
        if (min < MIN_GRAINLENGTH_SAMPLES) min = MIN_GRAINLENGTH_SAMPLES;
        int max = lengthCenter + 0.5f * lengthRange;
        if (max > MAX_GRAINLENGTH_SAMPLES) max = MAX_GRAINLENGTH_SAMPLES;
        
        // Define standard deviation as a fraction of the range
        float stddev = lengthRange * 0.25f; // /= 4

        // Generate a Gaussian-distributed random number
        float randomLength = generateGaussian(lengthCenter, stddev);

        // Clip the value within the bounds [min, max]
        if (randomLength < min) randomLength = min;
        if (randomLength > max) randomLength = max;
        
        props.length = randomLength;
    }
    
    // ampltiude scaling
    // grainOverlapScalar may be adjusted at will
    // if it's set to 1.0 the system should be safe, means that all values are within boundaries -1....1 
    // (if input is also bounded)
    // if it's higher than 1.0, values may jump over the boundaries and will be clipped later on
    static float grainOverlapScalar = 4.f;
    props.envelopeAmplitude = grainOverlapScalar * (float)interOnsetCenter / (float)lengthCenter;
    // make sure that the scalar doesn't push the amplitude over 1.0
    if (props.envelopeAmplitude > 1.f) props.envelopeAmplitude = 1.f;
    
    // panning
    if (panningRange == 0.f)
    {
        props.panHomeChannel = 1.f;
        props.panNeighbourChannel = 0.f;
    }
    else
    {
        float panOffset = panningRange * rand() * RAND_MAX_INVERSED;
        props.panHomeChannel = 1.f - panOffset;
        props.panNeighbourChannel = 1.f - props.panHomeChannel;
    }
    
    return &props;
}


// =======================================================================================
// MARK: - GRAIN DATA
// =======================================================================================


GrainData::GrainData(SourceData* sourceData_, GrainProperties* props_)
    : sourceData(sourceData_)
    , reverse(props_->reverse)
{
    // set the increment the read pointer should move every other sample
    incr = props_->pitchIncrement;
    
    // calculate glide increment (the amount that is being added to the pitch increment
    // every other sample
    // calculate the incremental goal
    float glideGoal = incr * props_->glideAmount;
    
    if (props_->glideAmount != 1.f)
    {
        // bound the goal to 0.5 and 2 (1 octave higher or lower)
        boundValue(glideGoal, 0.5f, 2.f);
        // calculate the distance between momentary increment and incremntal goal
        float glideDistance = glideGoal - incr;
        // the increment thats being added every other sample is the distance divided by the
        // samplelength of the grain
        glideIncr = glideDistance / (float)props_->length;
    }
    
    // calculate read pointer position with initial delay
    // first subtract the initial delay from the write pointer position
    readPointer = sourceData->getWritePointer() - props_->initDelay;
    if (readPointer < 0.f) readPointer += BUFFERSIZE;
    
    // find out the highest pitchincrement (either the usual pitch increment or the goal where
    // to glide to
    float pitchRampMax = glideGoal > incr ? glideGoal : incr;
    
    // if pitch or pitchramp exceeds increment size 1.0, the initial delay must be increased
    // to avoid reading faster than writing
    // if we are in reverse mode, this is not necessary since we read into the past anyway
    if (pitchRampMax > 1.f && !reverse)
    {
        readPointer -= (pitchRampMax - 1.f) * props_->length;
        if (readPointer < 0.f) readPointer += BUFFERSIZE;
    }
}


float GrainData::getNextData(const float envelope_)
{
    float data;
    
    // get data from sourceData with linear interpolation
    if (readPointer == (int)readPointer)
    {
        data = sourceData->get((uint)readPointer);
    }
    else
    {
        int lo = (int)readPointer;
        int hi = (lo+1) >= BUFFERSIZE ? 0 : lo+1;
        float frac = readPointer - (float)lo;
        float loData = sourceData->get(lo);
        data = loData + frac * (sourceData->get(hi) - loData);
    }
    
    // non-reverse mode
    if (!reverse)
    {
        readPointer += incr;
        if (readPointer >= BUFFERSIZE) readPointer -= BUFFERSIZE;
    }
    // reverse mode (just decrement the read pointer instead of incrementing)
    else
    {
        readPointer -= incr;
        if (readPointer < 0.f) readPointer += BUFFERSIZE;
    }
    
    // add the increment of gliding to the pitch increment
    // if glide Ramp is 1 -> glide Increment is 0.f -> nothing happens here
    incr += glideIncr;
    
    // return the evaluated data multiplied with the given envelope amplitude
    return data * envelope_;
}


// =======================================================================================
// MARK: - GRAIN
// =======================================================================================


Grain::Grain(GrainProperties* props_, SourceData* sourceData_)
    : panHomeChannel(props_->panHomeChannel)
    , panNeighbourChannel(props_->panNeighbourChannel)
{
    // create a new data-set object
    data = new GrainData(sourceData_, props_);
    
    // create an envelope object
    switch (props_->envelopeType)
    {
        case Envelope::Type::PARABOLIC:
            envelope = new ParabolicEnvelope(props_->length, props_->envelopeAmplitude);
            break;
        case Envelope::Type::HANN:
            envelope = new HannEnvelope(props_->length, props_->envelopeAmplitude);
            break;
        case Envelope::Type::TRIANGULAR:
            envelope = new TriangularEnvelope(props_->length, props_->envelopeAmplitude);
            break;
    }
    
    // set the life counter to the samplelength of the grain
    lifeCounter = props_->length;
    // this grain is just born
    isAlive = true;
}


Grain::~Grain()
{
    delete envelope;
    delete data;
}


float Grain::getNextSample()
{
    // decrement life counter and set flag correspondingly
    if (--lifeCounter == 0) isAlive = false;
    
    // return the next grain sample (data * envelope)
    return data->getNextData(envelope->getNextAmplitude());
}


// =======================================================================================
// MARK: - GRANULATOR
// =======================================================================================


bool Granulator::setup(const float sampleRate_, const uint blockSize_)
{
    sampleRate = sampleRate_;
    blockSize = blockSize_;
    
    // this has to be checked, since the code only allows one new grain per block
    // grains will be created blockwise in the update() function
    if (blockSize > sampleRate / MAX_DENSITY) return false;
    
    // setup the grain property manager
    manager.setup(sampleRate);
    
    // initialize all manager parameters
    parameterChanged("granulator_grainlength", parameterInitialValue[(int)Parameters::GRAINLENGTH]);
    parameterChanged("granulator_density", parameterInitialValue[(int)Parameters::DENSITY]);
    parameterChanged("granulator_pitch", parameterInitialValue[(int)Parameters::PITCH]);
    parameterChanged("granulator_glide", parameterInitialValue[(int)Parameters::GLIDE]);
    parameterChanged("granulator_reverse", parameterInitialValue[(int)Parameters::REVERSE]);
    parameterChanged("granulator_variation", parameterInitialValue[(int)Parameters::VARIATION]);
    parameterChanged("granulator_envelopetype", parameterInitialValue[(int)Parameters::ENVELOPE_TYPE]);
    parameterChanged("granulator_feedback", parameterInitialValue[(int)Parameters::FEEDBACK]);
    
    // reserve the necessary space in the grain cloud of each channel
    grainCloud[LEFT].reserve(MAX_NUM_GRAINS);
    grainCloud[RIGHT].reserve(MAX_NUM_GRAINS);
    
    // setup the delay object
    delay.setup(sampleRate);
    
    // initialize all delay parameters
    parameterChanged("granulator_delay", parameterInitialValue[(int)Parameters::DELAY]);
    parameterChanged("granulator_delayspeedratio", parameterInitialValue[(int)Parameters::DELAY_SPEED_RATIO]);
    
    // setup the filter object
    filter.setup(sampleRate);
    
    // initialize all filter parameters
    parameterChanged("granulator_filtermodel", parameterInitialValue[(int)Parameters::FILTER_MODEL]);
    parameterChanged("granulator_highcut", parameterInitialValue[(int)Parameters::HIGHCUT]);
    parameterChanged("granulator_filterresonance", parameterInitialValue[(int)Parameters::FILTER_RESONANCE]);
    
    for (uint ch = 0; ch < 2; ++ch)
    {
        nextInterOnset[ch] = manager.getNextInterOnset();
        onsetCounter[ch] = nextInterOnset[ch];
    }
    
    feedbackHighpass.setup(80.f, sampleRate);
    
    return true;
}


void Granulator::update()
{
    // add new grain to graincloud if needed
    // iterate through the channels
    for (unsigned int ch = 0; ch < 2; ++ch)
    {
        // if the onset counter will reach zero in the next sample block
        if (onsetCounter[ch] <= blockSize)
        {
            // get and save the next interonset time (may be randomized)
            nextInterOnset[ch] = manager.getNextInterOnset();
            
            // if there's still a free slot in the grain vector
            if (grainCloud[ch].size() < grainCloud[ch].capacity())
            {
                //create a new grain
                grainCloud[ch].push_back(new Grain(manager.getNextGrainProperties(), &data[ch]));
                
                // since the new grain shouldn't be processed yet, we store the number of active grains
                // in a separate variable
                // size of grain cloud should never be zero here, but safety first
                if (grainCloud[ch].size() == 0) numActiveGrains[ch] = 0;
                else numActiveGrains[ch] = grainCloud[ch].size() - 1;
            }
        }
    }
}


float32x2_t Granulator::processAudioSamples(const float32x2_t input_, const uint sampleIndex_)
{
    StereoFloat output = { 0.f, 0.f };
    
    // iterate through the channels
    for (uint ch = 0; ch < 2; ++ch)
    {
        // write input samples to buffer
        if (feedback == 0.f)
            data[ch].writeBuffer(input_[ch]);
        else
        {
            data[ch].writeBuffer(input_[ch] + dynamicFeedback * previousOutput[ch]);
        }
        
        // counting to next onset of grain
        // if reached, the pre-added grain (see update() function)
        // will be included in the sum-calculation of all grains
        if (--onsetCounter[ch] == 0)
        {
            onsetCounter[ch] = nextInterOnset[ch];
            numActiveGrains[ch] = grainCloud[ch].size();
        }
    
        // sum all active grains and spatialize them
        // if a grain looses life, its index will be safed for reordering the graincloud-vector later on
        std::vector<int> deadGrainIndex;
    
        // channel indexes used for panning later on
        uint homeChannel = ch;
        uint neighbourChannel = (ch == LEFT) ? RIGHT : LEFT;
        
        // iterate through all active grains in the cloud
        for (uint n = 0; n < numActiveGrains[ch]; ++n)
        {
            // get the next processed grain sample
            float grain = grainCloud[ch].at(n)->getNextSample();

            // spatialize it
            output[homeChannel] += grainCloud[ch].at(n)->getHomeChannelPanning() * grain;
            output[neighbourChannel] += grainCloud[ch].at(n)->getNeighbourChannelPanning() * grain;
            
            // if it looses life, delete the pointer and safe its index
            if (!grainCloud[ch].at(n)->isAlive)
            {
                delete grainCloud[ch].at(n);
                grainCloud[ch].at(n) = nullptr;
                deadGrainIndex.push_back(n);
            }
        }
        
        // erasing empty space in graincloud vector
        for (uint n = 0; n < deadGrainIndex.size(); ++n)
        {
            grainCloud[ch].erase(grainCloud[ch].begin() + deadGrainIndex.at(n) - n);
            --numActiveGrains[ch];
        }
    }
    
    // write the channel outputs into a stereo neon vector
    float32x2_t output_simd = { output[LEFT], output[RIGHT] };
    
    // gain compensation
    output_simd = vmul_n_f32(output_simd, GAIN_COMPENSATION);
    
    // process highcut filter
    output_simd = filter.processAudioSamples(output_simd);
    
    // process the delay
    float32x2_t delayOutput = delay.processAudioSamples(output_simd, sampleIndex_);
    
    // dry granulator output + wet delay output
    output_simd = vadd_f32(vmul_n_f32(output_simd, delayDry), vmul_n_f32(delayOutput, delayWet));
    
    // turn neon vector back into a StereoFloat
    output[LEFT] = vget_lane_f32(output_simd, 0);
    output[RIGHT] = vget_lane_f32(output_simd, 1);
    
    // calculate dynamic feedback
    float absOutput[2] = { fabsf_neon(output[LEFT]), fabsf_neon(output[RIGHT])};
    float maxOutput = (absOutput[LEFT] >= absOutput[RIGHT]) ? absOutput[LEFT] : absOutput[RIGHT];
    dynamicFeedback = (maxOutput >= 1.f) ? 0.f : feedback * (1.f - maxOutput);
    
    // saturate the output signal
    output[LEFT] = approximateTanh(output[LEFT]);
    output[RIGHT] = approximateTanh(output[RIGHT]);
    
    previousOutput = feedbackHighpass.process(output);
    
    // return processed wet output + dry input
    return { output[LEFT], output[RIGHT] };
}


void Granulator::resetPhase()
{
    for (uint ch = 0; ch < 2; ++ch) onsetCounter[ch] = 1;
}


void Granulator::parameterChanged (const String parameterID, float newValue)
{
    bool parameterReceived = true;
    
    if (parameterID == "granulator_grainlength")
    {
        int lengthSamples = (int)(newValue * sampleRate * 0.001f); // ms to samples
        manager.setLength(lengthSamples);
    }
    else if (parameterID == "granulator_density")
    {
        // set interonset time in samples
        int interOnsetSamples = (int)(sampleRate / newValue); // frequency to samples
        manager.setInterOnset(interOnsetSamples);
        
        // for a smooth transition from low densitys to higher once we shorten the counter
        // if it is still higher than the new interonset time
        // otherwise we'd have to wait for the previous interonset time to pass, afterwards the
        // slider change would affect the audio
        for (uint ch = 0; ch < 2; ++ch)
            if (onsetCounter[ch] > interOnsetSamples) onsetCounter[ch] = interOnsetSamples;
        
        // set corresponding delay speed
        float delayMs = (1000.f / newValue) * delaySpeedRatio;
        delay.setDelayTimeRampInMs(delayMs);
    }
    else if (parameterID == "granulator_variation")
    {
        // Interonset Variation
        manager.setInterOnsetVariation(0.01f * newValue);
        
        // GrainLength Variation
        manager.setLengthVariation(0.01f * newValue);
        
        // Initial Delay Variation
        manager.setInitDelayVariation(0.01f * newValue);
        
        // Spatialize
        manager.setPanningVariation(0.01f * newValue);

        // if we return to zero variation, onsetctr has to be resynced to restore mono
        if (newValue == 0.f)
        {
            if (onsetCounter[0] > onsetCounter[1]) onsetCounter[1] = onsetCounter[0];
            else onsetCounter[0] = onsetCounter[1];
        }
    }
    else if (parameterID == "granulator_pitch")
    {
        float incr = powf(2.f, (newValue / 12.f)); // semitones to increment
        manager.setPitchIncrement(incr);
    }
    else if (parameterID == "granulator_glide")
    {
        float glidegoal = powf(2.f, newValue); // octave to increment
        manager.setGlideAmount(glidegoal);
    }
    else if (parameterID == "granulator_delay")
    {
        float delayFeedback = mapValue(newValue, 0.f, 100.f, 0.f, 0.907f); // percent to feedback gain
        delay.setFeedback(delayFeedback);

        delayWet = newValue * 0.01f * 0.6f;
        delayDry = 1.f - delayWet;
    }
    else if (parameterID == "granulator_highcut")
    {
        filter.setCutoffFrequency(newValue);
    }
    else if (parameterID == "granulator_reverse")
    {
        manager.setReverse(newValue);
    }
    else if (parameterID == "granulator_delayspeedratio")
    {
        delaySpeedRatio = 1.f / (newValue + 1);
        
        uint delaySamples = (uint)(manager.getInterOnset() * delaySpeedRatio);
        float delayMs = delaySamples / (sampleRate * 0.001f);
        delay.setDelayTimeRampInMs(delayMs);
    }
    else if (parameterID == "granulator_filterresonance")
    {
        filter.setResonance(newValue * 0.01f);
    }
    else if (parameterID == "granulator_filtermodel")
    {
        FilterStereo::Model model = newValue == 0 ? FilterStereo::MOOGLADDER : FilterStereo::MOOGHALFLADDER;
        filter.setFilterModel(model);
    }
    else if (parameterID == "granulator_envelopetype")
    {
        Envelope::Type type = INT2ENUM(newValue, Envelope::Type);
        manager.setEnvelopeType(type);
    }
    else if (parameterID == "granulator_feedback")
    {
        feedback = newValue;
    }
    else
    {
        parameterReceived = false;
    }
    
    if (parameterReceived)
    {
        #ifdef CONSOLE_PRINT
        consoleprint("Granulator received new Value for Paramaeter: " + parameterID + " = " + TOSTRING(newValue),
                     __FILE__, __LINE__);
        #endif
    }
}

