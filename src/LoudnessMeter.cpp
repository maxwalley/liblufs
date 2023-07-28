#include "LoudnessMeter.h"

namespace LUFS
{

LoudnessMeter::LoudnessMeter(const std::chrono::milliseconds& windowLength)  : length(windowLength)
{
    
}

LoudnessMeter::~LoudnessMeter()
{
    
}

void LoudnessMeter::prepare(double sampleRate)
{
    window.resize((length.count() / 1000.0) * sampleRate);
    std::fill(window.begin(), window.end(), 0.0f);
    lastLoudness = 0.0f;
}

float LoudnessMeter::addSamples(std::span<float> audio)
{
    //Fill the end of window with new samples
    size_t numNewSamples = 0;
    
    if(audio.size() <= window.size())
    {
        numNewSamples = audio.size();
        
        //Make space for new data and copy it in
        std::copy(window.begin() + numNewSamples, window.end(), window.begin());
        std::copy(audio.begin(), audio.end(), window.begin() + window.size() - numNewSamples);
    }
    else
    {
        numNewSamples = window.size();
        
        //Copy whatever of the audio buffer we can fit
        std::copy(audio.begin() + audio.size() - window.size(), audio.end(), window.begin());
    }
    
    //Filter the new samples
}

float LoudnessMeter::getCurrentLoudness() const
{
    return lastLoudness;
}

}
