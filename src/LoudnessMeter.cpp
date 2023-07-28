#include "LoudnessMeter.h"

namespace LUFS
{

LoudnessMeter::LoudnessMeter(const std::chrono::milliseconds& windowLength)  : length(windowLength)
{
    initialiseFilters();
}

LoudnessMeter::~LoudnessMeter()
{
    
}

void LoudnessMeter::prepare(double sampleRate)
{
    window.resize((length.count() / 1000.0) * sampleRate);
    std::fill(window.begin(), window.end(), 0.0f);
    lastLoudness = 0.0f;
    
    filt1.reset();
    filt2.reset();
}

float LoudnessMeter::addSamples(std::span<float> audio)
{
    //Fill the end of window with new samples
    size_t numNewSamples = 0;
    
    if(audio.size() <= window.size())
    {
        numNewSamples = audio.size();
        
        //Make space for new data and copy it in passing through the filters at the same time
        std::copy(window.begin() + numNewSamples, window.end(), window.begin());
        std::transform(audio.begin(), audio.end(), window.begin() + window.size() - numNewSamples, [this](float sample)
        {
            return filt2.processSample(filt1.processSample(sample));
        });
    }
    else
    {
        numNewSamples = window.size();
        const size_t firstUsuableNewSampleIndex = audio.size() - window.size();
        
        //Pass any samples that will not be used into the filters
        std::for_each(audio.begin(), audio.begin() + firstUsuableNewSampleIndex, [this](float sample)
        {
            filt2.processSample(filt1.processSample(sample));
        });
        
        //Copy whatever of the audio buffer we can fit, running the samples through the filters at the same time
        std::transform(audio.begin() + firstUsuableNewSampleIndex, audio.end(), window.begin(), [this](float sample)
        {
            return filt2.processSample(filt1.processSample(sample));
        });
    }
    
    //Filter the new samples
}

float LoudnessMeter::getCurrentLoudness() const
{
    return lastLoudness;
}

void LoudnessMeter::initialiseFilters()
{
    filt1.a1 = -1.69065929318241;
    filt1.a2 = 0.73248077421585;
    filt1.b0 = 1.53512485958697;
    filt1.b1 = -2.69169618940638;
    filt1.b2 = 1.19839281085285;
    
    filt2.a1 = -1.99004745483398;
    filt2.a2 = 0.99007225036621;
    filt2.b0 = 1.0;
    filt2.b1 = -2.0;
    filt2.b2 = 1.0;
}

}
