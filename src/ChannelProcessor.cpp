#include "ChannelProcessor.h"

namespace LUFS
{

ChannelProcessor::ChannelProcessor(float channelWeighting, size_t blockSizeSamples)  : weighting(channelWeighting), currentBlockData(blockSizeSamples, 0.0f)
{
    initialiseFilters();

    std::fill(currentBlockData.begin(), currentBlockData.end(), 0.0f);
}

ChannelProcessor::~ChannelProcessor()
{
    
}

float ChannelProcessor::filterSample(float sample)
{
    return filt2.processSample(filt1.processSample(sample));
}

void ChannelProcessor::reset()
{
    filt1.reset();
    filt2.reset();

    std::fill(currentBlockData.begin(), currentBlockData.end(), 0.0f);
}

float ChannelProcessor::getCurrentBlockMeanSquares() const
{
    float meanSquares = 0.0f;
    
    std::for_each(currentBlockData.begin(), currentBlockData.end(), [&](double sample)
    {
        meanSquares += std::pow(sample, 2.0f);
    });
    
    meanSquares *= (1.0f / currentBlockData.size());
    
    return meanSquares;
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

}