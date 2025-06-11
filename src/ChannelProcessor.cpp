#include "ChannelProcessor.h"

namespace LUFS
{

ChannelProcessor::ChannelProcessor(const std::chrono::milliseconds& windowLength)  : length(windowLength)
{
    initialiseFilters();
}

ChannelProcessor::~ChannelProcessor()
{
    
}

void ChannelProcessor::prepare(double sampleRate)
{
    window.resize((length.count() / 1000.0) * sampleRate);
    std::fill(window.begin(), window.end(), 0.0f);
    
    filt1.reset();
    filt2.reset();
}

float ChannelProcessor::process(std::span<const float> channelBuffer)
{
    return 0.0f;
}

void ChannelProcessor::initialiseFilters()
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

float ChannelProcessor::calculateMeanSquares(std::span<const float> buffer) const
{
    float meanSquares = 0.0f;
    
    std::for_each(buffer.begin(), buffer.end(), [&](double sample)
    {
        meanSquares += std::pow(sample, 2.0f);
    });
    
    meanSquares *= (1.0f / buffer.size());
    
    return meanSquares;
}

}