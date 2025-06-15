#include "LoudnessMeter.h"

namespace LUFS
{

LoudnessMeter::LoudnessMeter(const std::vector<Channel>& channels, bool gated, const std::optional<std::chrono::milliseconds>& windowLength)  : gate(gated)
{
    std::transform(channels.begin(), channels.end(), std::back_insert_iterator(channelProcessors), [](const Channel& channel)
    {
        return ChannelProcessor(channel.getWeighting(), blockLengthSamples);
    });

    //Integrated is not yet implemented
    if(!windowLength)
    {
        assert(false);
    }

    if(windowLength->count() < blockLengthMs)
    {
        throw(std::runtime_error("Integration time must be more than " + std::to_string(blockLengthMs)));
    }

    blocks.resize(std::floor((windowLength->count() - blockLengthMs) / float(overlapLengthMs)) + 1);
}

LoudnessMeter::~LoudnessMeter()
{
    
}

void LoudnessMeter::prepare()
{
    std::fill(blocks.begin(), blocks.end(), Block{std::vector<float>(channelProcessors.size(), 0.0f), 0.0f});
    blocksWritePos = 0;

    std::for_each(channelProcessors.begin(), channelProcessors.end(), [](ChannelProcessor& processor)
    {
        processor.prepare();
    });

    currentBlockWritePos = 0;

    currentLoudness = 0.0f;
}

void LoudnessMeter::process(const std::vector<std::vector<float>>& buffer)
{
    if(buffer.size() != channelProcessors.size())
    {
        throw(std::runtime_error("Incorrect number of channels provided"));
    }

    if(buffer.size() == 0)
    {
        return;
    }

    const size_t numSamples = buffer[0].size();

    size_t numSamplesToAdd = std::min(blockLengthSamples - currentBlockWritePos, numSamples);
    int bufferReadPos = 0;

    while(numSamplesToAdd > 0)
    {
        for(size_t channelIndex = 0; channelIndex < buffer.size(); ++channelIndex)
        {
            ChannelProcessor& channelProcessor = channelProcessors[channelIndex];
            std::vector<float>& channelCurrentBlock = channelProcessor.currentBlockData; 
            const std::vector<float>& channelBuffer = buffer[channelIndex];

            //Add samples to the channels current block, filtering each one
            std::transform(channelBuffer.begin(), channelBuffer.begin() + numSamplesToAdd, channelCurrentBlock.begin() + currentBlockWritePos, std::bind(&ChannelProcessor::filterSample, &channelProcessor, std::placeholders::_1));
        }

        //Check if we're ready to process
        if(currentBlockWritePos >= blockLengthSamples)
        {
            Block& block = blocks[blocksWritePos];

            for(size_t channelIndex = 0; channelIndex < buffer.size(); ++channelIndex)
            {
                ChannelProcessor& channelProcessor = channelProcessors[channelIndex];
                std::vector<float>& channelCurrentBlock = channelProcessor.currentBlockData; 
                
                //Process mean squares for channel
                block.channelMeanSquares[channelIndex] = channelProcessor.getCurrentBlockMeanSquares();

                //Move current block back by overlap samples
                std::copy(channelCurrentBlock.begin() + overlapLengthSamples, channelCurrentBlock.end(), channelCurrentBlock.begin());
            }

            if(gate)
            {
                calculateBlockLoudness(block);
            }

            updateCurrentLoudness();

            if(++blocksWritePos >= blocks.size())
            {
                blocksWritePos = 0;
            }

            //Move write pos back to overlap samples
            currentBlockWritePos = blockLengthSamples - overlapLengthSamples;
        }

        //Check if we can add/process more data
        bufferReadPos += numSamplesToAdd;

        if(bufferReadPos < numSamples)
        {
            numSamplesToAdd = std::min(blockLengthSamples - currentBlockWritePos, numSamples - bufferReadPos);
        }
        else
        {
            numSamplesToAdd = 0;
        }
    }
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

void LoudnessMeter::updateCurrentLoudness()
{
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
        float relativeThreshold = calculateLoudnessWithThreshold(-70.0f);
        currentLoudness = calculateLoudnessWithThreshold(relativeThreshold - 10.0f);
    }
    else
    {
        currentLoudness = calculateLoudnessWithThreshold(std::nullopt);
    }
}

}
