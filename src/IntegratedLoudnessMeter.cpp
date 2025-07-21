#include "IntegratedLoudnessMeter.h"

namespace LUFS
{
    
IntegratedLoudnessMeter::IntegratedLoudnessMeter(const std::vector<Channel>& channels)  : processBlock{channels.size()}
{
    std::transform(channels.begin(), channels.end(), std::back_insert_iterator(channelProcessors), [this](const Channel& channel)
    {
        return ChannelProcessor(channel.getWeighting(), blockLengthSamples);
    });
    
    constexpr size_t numHistogramElements = (1.0f / integratedHistogramResolution) * (highestIntegratedValue - lowestIntegratedValue);
    blockHistogram.resize(numHistogramElements, HistogramBlock{channelProcessors.size()});
        
    histogramMappingSlope = (numHistogramElements - 1.0f) / (highestIntegratedValue - lowestIntegratedValue);
}

IntegratedLoudnessMeter::~IntegratedLoudnessMeter()
{
    
}

void IntegratedLoudnessMeter::process(std::span<const float* const> audio, int numSamplesPerChannel)
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

void IntegratedLoudnessMeter::process(std::span<const float> audio)
{
    assert(audio.size() % channelProcessors.size() == 0);

    size_t numSamplesPerChannel = audio.size() / channelProcessors.size();
    
    size_t numSamplesToAdd = std::min(blockLengthSamples - currentBlockWritePos, static_cast<size_t>(numSamplesPerChannel));
    int bufferReadPos = 0;
    
    while(numSamplesToAdd > 0)
    {
        //Copy the samples to the channel buffers filtering each one
        for(size_t channelIndex = 0; channelIndex < channelProcessors.size(); ++channelIndex)
        {
            ChannelProcessor& channelProcessor = channelProcessors[channelIndex];
            std::vector<float>& channelCurrentBlock = channelProcessor.currentBlockData; 
            
            for(size_t sampleIndex = 0; sampleIndex < numSamplesToAdd; ++sampleIndex)
            {
                const size_t fullSampleIndex = sampleIndex + bufferReadPos;
                const float sample = audio[fullSampleIndex * channelProcessors.size() + channelIndex];
                channelCurrentBlock[currentBlockWritePos + sampleIndex] = channelProcessor.filterSample(sample);
            }
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

void IntegratedLoudnessMeter::reset()
{
    std::for_each(channelProcessors.begin(), channelProcessors.end(), [](ChannelProcessor& processor)
    {
        processor.reset();
    });
    
    std::fill(blockHistogram.begin(), blockHistogram.end(), HistogramBlock{channelProcessors.size()});
    
    currentBlockWritePos = 0;
}

float IntegratedLoudnessMeter::getLoudness() const
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
            
            channelAccum += channelProcessors[channelIndex].getWeighting() * ((1.0f / float(numBlocksAccum)) * meanSquaresAccum);
        }
        
        return -0.691 + 10 * std::log10(channelAccum);
    };
    
    float relativeThreshold = calculateLoudnessWithThreshold(gateAbsoluteThreshold);
    return calculateLoudnessWithThreshold(relativeThreshold - 10.0f);
}

void IntegratedLoudnessMeter::processCurrentBlock()
{
    for(size_t channelIndex = 0; channelIndex < channelProcessors.size(); ++channelIndex)
    {
        ChannelProcessor& channelProcessor = channelProcessors[channelIndex];
        std::vector<float>& channelCurrentBlock = channelProcessor.currentBlockData; 
        
        //Process mean squares for channel
        processBlock.channelMeanSquares[channelIndex] = channelProcessor.getCurrentBlockMeanSquares();
        
        //Move current block back by overlap samples
        std::copy(channelCurrentBlock.begin() + overlapLengthSamples, channelCurrentBlock.end(), channelCurrentBlock.begin());
    }
    
    calculateBlockLoudness(processBlock);
    
    HistogramBlock& histBlock = blockHistogram[getHistogramBinIndexForLoudness(processBlock.loudness)];
    histBlock.addBlock(processBlock);
    
    //Move write pos back to overlap samples
    currentBlockWritePos = blockLengthSamples - overlapLengthSamples;
}

void IntegratedLoudnessMeter::calculateBlockLoudness(Block& block) const
{
    assert(block.channelMeanSquares.size() == channelProcessors.size());
    
    float currentTotal = 0.0f;
    
    for(size_t channelIndex = 0; channelIndex < channelProcessors.size(); ++channelIndex)
    {
        currentTotal += channelProcessors[channelIndex].getWeighting() * block.channelMeanSquares[channelIndex]; 
    }
    
    block.loudness = -0.691f + 10 * std::log10(currentTotal);
}

size_t IntegratedLoudnessMeter::getHistogramBinIndexForLoudness(float loudness) const
{
    return std::min(blockHistogram.size() - 1, std::max(size_t(0), size_t(std::round(histogramMappingSlope * (loudness - lowestIntegratedValue)))));
}

}
