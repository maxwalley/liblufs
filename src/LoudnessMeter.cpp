#include "LoudnessMeter.h"

namespace LUFS
{

LoudnessMeter::LoudnessMeter(const std::vector<Channel>& channels, bool gated, const std::optional<std::chrono::milliseconds>& windowLength)
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
    std::fill(blocks.begin(), blocks.end(), Block{});

    std::for_each(channelProcessors.begin(), channelProcessors.end(), [](ChannelProcessor& processor)
    {
        processor.prepare();
    });
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
            for(size_t channelIndex = 0; channelIndex < buffer.size(); ++channelIndex)
            {
                ChannelProcessor& channelProcessor = channelProcessors[channelIndex];
                std::vector<float>& channelCurrentBlock = channelProcessor.currentBlockData; 
                
                //Process block for channel

                //Move current block back by overlap samples
                std::copy(channelCurrentBlock.begin() + overlapLengthSamples, channelCurrentBlock.end(), channelCurrentBlock.begin());
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

}
