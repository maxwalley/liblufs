#include "ChannelProcessor.h"

namespace LUFS
{

ChannelProcessor::ChannelProcessor(float channelWeighting, bool gated, const std::optional<int>& integrationTimeMs)  : weighting(channelWeighting)
{
    initialiseFilters();
}

ChannelProcessor::~ChannelProcessor()
{
    
}

void ChannelProcessor::prepare()
{
    filt1.reset();
    filt2.reset();
}

std::optional<float> ChannelProcessor::process(std::span<const float> channelBuffer)
{

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

float ChannelProcessor::filterSample(float sample)
{
    return filt2.processSample(filt1.processSample(sample));
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