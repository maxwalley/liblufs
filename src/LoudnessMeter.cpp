#include "LoudnessMeter.h"

namespace LUFS
{
    
LoudnessMeter::LoudnessMeter(const std::vector<Channel>& channels, bool gated, const std::optional<std::chrono::milliseconds>& windowLength)  : gate{gated}, integrated{!windowLength}
{
    std::transform(channels.begin(), channels.end(), std::back_insert_iterator(channelProcessors), [](const Channel& channel)
    {
        return ChannelProcessor(channel.getWeighting(), blockLengthSamples);
    });
    
    //Fixed length
    if(!integrated)
    {
        if(windowLength->count() < blockLengthMs)
        {
            throw(std::runtime_error("Integration time must be more than " + std::to_string(blockLengthMs)));
        }
        
        const size_t numBlocks = std::floor((windowLength->count() - blockLengthMs) / float(overlapLengthMs)) + 1;
        blocks.resize(numBlocks, Block{channelProcessors.size()});
    }
    
    //Integrated
    else
    {
        constexpr size_t numHistogramElements = (1.0f / integratedHistogramResolution) * (highestIntegratedValue - lowestIntegratedValue);
        blockHistogram.resize(numHistogramElements, HistogramBlock{channelProcessors.size()});
        
        histogramMappingSlope = (numHistogramElements - 1.0f) / (highestIntegratedValue - lowestIntegratedValue);
    }
}

LoudnessMeter::~LoudnessMeter()
{
    
}

void LoudnessMeter::process(std::span<const float* const> audio, int numSamplesPerChannel)
{
    assert(audio.size() == channelProcessors.size());
    
    size_t numSamplesToAdd = std::min(blockLengthSamples - currentBlockWritePos, static_cast<size_t>(numSamplesPerChannel));
    int bufferReadPos = 0;
    
    while(numSamplesToAdd > 0)
    {
        for(size_t channelIndex = 0; channelIndex < channelProcessors.size(); ++channelIndex)
        {
            ChannelProcessor& channelProcessor = channelProcessors[channelIndex];
            std::vector<float>& channelCurrentBlock = channelProcessor.currentBlockData; 
            const float* const channelData = audio[channelIndex];
            
            //Add samples to the channels current block, filtering each one
            std::transform(channelData + bufferReadPos, channelData + bufferReadPos + numSamplesToAdd, channelCurrentBlock.begin() + currentBlockWritePos, std::bind(&ChannelProcessor::filterSample, &channelProcessor, std::placeholders::_1));
        }
        
        currentBlockWritePos += numSamplesToAdd;
        
        //Check if we're ready to process
        if(currentBlockWritePos >= blockLengthSamples)
        {
            processCurrentBlock();
        }
        
        //Check if we can add/process more data
        bufferReadPos += numSamplesToAdd;
        
        if(bufferReadPos < numSamplesPerChannel)
        {
            numSamplesToAdd = std::min(blockLengthSamples - currentBlockWritePos, static_cast<size_t>(numSamplesPerChannel - bufferReadPos));
        }
        else
        {
            numSamplesToAdd = 0;
        }
    }
}

void LoudnessMeter::process(std::span<const float> audio)
{
    
}

void LoudnessMeter::reset()
{
    std::for_each(channelProcessors.begin(), channelProcessors.end(), [](ChannelProcessor& processor)
    {
        processor.reset();
    });
    
    if(!integrated)
    {
        std::fill(blocks.begin(), blocks.end(), Block{channelProcessors.size()});
    }
    else
    {
        std::fill(blockHistogram.begin(), blockHistogram.end(), HistogramBlock{channelProcessors.size()});
    }
    
    currentBlockWritePos = 0;
}

float LoudnessMeter::getLoudness() const
{
    return integrated ? getLoudnessIntegrated() : getLoudnessFixedLength();
}

void LoudnessMeter::processCurrentBlock()
{
    //This needs to be worked out so that it doesn't allocate
    Block newBlock{channelProcessors.size()};
    
    for(size_t channelIndex = 0; channelIndex < channelProcessors.size(); ++channelIndex)
    {
        ChannelProcessor& channelProcessor = channelProcessors[channelIndex];
        std::vector<float>& channelCurrentBlock = channelProcessor.currentBlockData; 
        
        //Process mean squares for channel
        newBlock.channelMeanSquares[channelIndex] = channelProcessor.getCurrentBlockMeanSquares();
        
        //Move current block back by overlap samples
        std::copy(channelCurrentBlock.begin() + overlapLengthSamples, channelCurrentBlock.end(), channelCurrentBlock.begin());
    }
    
    if(gate || integrated)
    {
        calculateBlockLoudness(newBlock);
    }
    
    addBlock(newBlock);
    
    //Move write pos back to overlap samples
    currentBlockWritePos = blockLengthSamples - overlapLengthSamples;
}

void LoudnessMeter::calculateBlockLoudness(Block& block) const
{
    assert(block.channelMeanSquares.size() == channelProcessors.size());
    
    float currentTotal = 0.0f;
    
    for(size_t channelIndex = 0; channelIndex < channelProcessors.size(); ++channelIndex)
    {
        currentTotal += channelProcessors[channelIndex].weighting * block.channelMeanSquares[channelIndex]; 
    }
    
    block.loudness = -0.691f + 10 * std::log10(currentTotal);
}

void LoudnessMeter::addBlock(const Block& newBlock)
{
    if(!integrated)
    {
        blocks[blocksWritePos] = newBlock;
        
        if(++blocksWritePos >= blocks.size())
        {
            blocksWritePos = 0;
        }
    }
    else
    {
        HistogramBlock& histBlock = blockHistogram[getHistogramBinIndexForLoudness(newBlock.loudness)];
        histBlock.addBlock(newBlock);
    }
}

size_t LoudnessMeter::getHistogramBinIndexForLoudness(float loudness) const
{
    return std::min(blockHistogram.size() - 1, std::max(size_t(0), size_t(std::round(histogramMappingSlope * (loudness - lowestIntegratedValue)))));
}

float LoudnessMeter::getLoudnessFixedLength() const
{
    //Make this realtime safe, take a snapshot copy of blocks
    
    const auto calculateLoudnessWithThreshold = [this](const std::optional<float>& threshold)
    {
        float channelAccum = 0.0f;
        
        for(int channelIndex = 0; channelIndex < channelProcessors.size(); ++channelIndex)
        {
            float meanSquaresAccum = 0.0f;
            size_t numBlocksUsed = 0;
            
            std::for_each(blocks.begin(), blocks.end(), [&](const Block& block)
            {
                if(!threshold || block.loudness > threshold)
                {
                    meanSquaresAccum += block.channelMeanSquares[channelIndex];
                    ++numBlocksUsed;
                }
            });
            
            channelAccum += channelProcessors[channelIndex].weighting * ((1.0f / float(numBlocksUsed)) * meanSquaresAccum);
        }
        
        return -0.691 + 10 * log10(channelAccum);
    };
    
    if(gate)
    {
        float relativeThreshold = calculateLoudnessWithThreshold(gateAbsoluteThreshold);
        return calculateLoudnessWithThreshold(relativeThreshold - 10.0f);
    }
    else
    {
        return calculateLoudnessWithThreshold(std::nullopt);
    }
}

float LoudnessMeter::getLoudnessIntegrated() const
{
    const auto calculateLoudnessWithThreshold = [this](const std::optional<float>& threshold)
    {
        float channelAccum = 0.0f;
        
        for(int channelIndex = 0; channelIndex < channelProcessors.size(); ++channelIndex)
        {
            float meanSquaresAccum = 0.0f;
            size_t numBlocksAccum = 0;
            
            const size_t firstBinIndex = threshold ? getHistogramBinIndexForLoudness(*threshold) : size_t(0);
            
            //THIS NEEDS TO BE WORKED OUT BASED ON LOWEST HIST POS FROM THRESH
            std::for_each(blockHistogram.begin() + firstBinIndex, blockHistogram.end(), [&](const HistogramBlock& block)
            {
                if(block.numBlocks == 0)
                {
                    return;
                }
                
                meanSquaresAccum += block.accumulatedChannelMeanSquares[channelIndex];
                numBlocksAccum += block.numBlocks;
            });
            
            channelAccum += channelProcessors[channelIndex].weighting * ((1.0f / float(numBlocksAccum)) * meanSquaresAccum);
        }
        
        return -0.691 + 10 * log10(channelAccum);
    };
    
    if(gate)
    {
        float relativeThreshold = calculateLoudnessWithThreshold(gateAbsoluteThreshold);
        return calculateLoudnessWithThreshold(relativeThreshold - 10.0f);
    }
    else
    {
        return calculateLoudnessWithThreshold(std::nullopt);
    }
}
    
}
