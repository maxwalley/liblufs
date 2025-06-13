#include "ChannelProcessor.h"

namespace LUFS
{

ChannelProcessor::ChannelProcessor(float channelWeighting, bool gated, const std::optional<int>& windowLengthMs)  : weighting(channelWeighting), numBlocks(calculateNumBlocks(windowLengthMs))
{
    initialiseFilters();

    if(numBlocks)
    {
        blockMeanSquares.resize(*numBlocks, 0.0f);
    }

    currentBlock.resize(blockLengthSamples, 0.0f);
}

ChannelProcessor::~ChannelProcessor()
{
    
}

void ChannelProcessor::prepare()
{
    filt1.reset();
    filt2.reset();

    if(numBlocks)
    {
        std::fill(blockMeanSquares.begin(), blockMeanSquares.end(), 0.0f);
    }
    else
    {
        blockMeanSquares.clear();
    }

    std::fill(currentBlock.begin(), currentBlock.end(), 0.0f);
    currentBlockWritePos = 0;
}

std::optional<float> ChannelProcessor::process(std::span<const float> channelBuffer)
{
    size_t numSamplesToAdd = std::min(currentBlock.size() - currentBlockWritePos, channelBuffer.size());
    int bufferReadPos = 0;

    while(numSamplesToAdd > 0)
    {
        //Add samples to the current process window, filtering each one
        std::transform(channelBuffer.begin(), channelBuffer.begin() + numSamplesToAdd, currentBlock.begin() + currentBlockWritePos, [this](float sample)
        {
            return filterSample(sample);
        });

        if(currentBlockWritePos >= currentBlock.size())
        {
            //PROCESS BLOCK

            //Move current block back by overlap samples and start writing at the end
            std::copy(currentBlock.begin() + overlapLengthSamples, currentBlock.end(), currentBlock.begin());
            currentBlockWritePos = currentBlock.size() - overlapLengthSamples;
        }

        bufferReadPos += numSamplesToAdd;

        if(bufferReadPos < channelBuffer.size())
        {
            numSamplesToAdd = std::min(currentBlock.size() - currentBlockWritePos, channelBuffer.size() - bufferReadPos);
        }
        else
        {
            numSamplesToAdd = 0;
        }
    }

    return std::nullopt;
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

std::optional<int> ChannelProcessor::calculateNumBlocks(const std::optional<int>& windowLengthMs)
{
    if(!windowLengthMs)
    {
        return std::nullopt;
    }

    if(*windowLengthMs < windowLengthMs)
    {
        throw(std::runtime_error("Integration time must be more than " + std::to_string(blockLengthMs)));
    }

    return std::floor((*windowLengthMs - blockLengthMs) / float(overlapLengthMs)) + 1;
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