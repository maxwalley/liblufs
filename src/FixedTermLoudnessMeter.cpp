#include "FixedTermLoudnessMeter.h"

namespace LUFS
{
    
FixedTermLoudnessMeter::FixedTermLoudnessMeter(const std::vector<Channel>& channels, const std::chrono::milliseconds& windowLength)  : blockLengthSamples{getBlockLengthSamples(windowLength)}
{
    std::transform(channels.begin(), channels.end(), std::back_insert_iterator(channelProcessors), [this](const Channel& channel)
    {
        return ChannelProcessor(channel.getWeighting(), blockLengthSamples);
    });
}

FixedTermLoudnessMeter::~FixedTermLoudnessMeter()
{
    
}

void FixedTermLoudnessMeter::process(std::span<const float* const> audio, int numSamplesPerChannel)
{
    assert(audio.size() == channelProcessors.size());
    
    for(size_t sampleIndex = 0; sampleIndex < numSamplesPerChannel; ++sampleIndex)
    {
        for(int channelIndex = 0; channelIndex < channelProcessors.size(); ++channelIndex)
        {
            channelProcessors[channelIndex].currentBlockData[blockWritePos] =  channelProcessors[channelIndex].filterSample(audio[channelIndex][sampleIndex]);
        }

        if(++blockWritePos >= blockLengthSamples)
        {
            blockWritePos = 0;
        }
    }
}

void FixedTermLoudnessMeter::process(std::span<const float> audio)
{
    assert(audio.size() % channelProcessors.size() == 0);

    const size_t numSamplesPerChannel = audio.size() / channelProcessors.size();
    
    const float* inputData = audio.data();

    for(size_t sampleIndex = 0; sampleIndex < numSamplesPerChannel; ++sampleIndex)
    {
        for(int channelIndex = 0; channelIndex < channelProcessors.size(); ++channelIndex)
        {
            channelProcessors[channelIndex].currentBlockData[blockWritePos] =  channelProcessors[channelIndex].filterSample(*inputData++);
        }

        if(++blockWritePos >= blockLengthSamples)
        {
            blockWritePos = 0;
        }
    }
}

void FixedTermLoudnessMeter::reset()
{
    std::for_each(channelProcessors.begin(), channelProcessors.end(), [](ChannelProcessor& processor)
    {
        processor.reset();
    });
    
    blockWritePos = 0;
}

float FixedTermLoudnessMeter::getLoudness() const
{
    float accumulatedChannels = 0.0f;

    std::for_each(channelProcessors.begin(), channelProcessors.end(), [&accumulatedChannels](const ChannelProcessor& processor)
    {
        accumulatedChannels += processor.getCurrentBlockMeanSquares() * processor.weighting;
    });

    return -0.691f + 10.0f * std::log10(accumulatedChannels);
}

int FixedTermLoudnessMeter::getBlockLengthSamples(const std::chrono::milliseconds& windowLength) const
{
    return (windowLength.count() / 1000.0f) * sampleRate;
}
    
}
