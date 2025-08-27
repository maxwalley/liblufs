#include "FixedTermLoudnessMeter.h"

namespace LUFS
{
    
FixedTermLoudnessMeter::FixedTermLoudnessMeter(const std::vector<Channel>& channelSet, const std::chrono::milliseconds& windowLength, float minLevel)  : channels{channelSet}, blockLengthSamples{getBlockLengthSamples(windowLength)}, min{minLevel}, channelProcessors{generateChannelProcessors()}
{
    
}

FixedTermLoudnessMeter::~FixedTermLoudnessMeter()
{
    
}

void FixedTermLoudnessMeter::process(std::span<const float* const> audio, int numSamplesPerChannel)
{
    std::vector<ChannelProcessor>& processors = channelProcessors.realtimeAquire();

    assert(audio.size() == processors.size());
    
    for(size_t sampleIndex = 0; sampleIndex < numSamplesPerChannel; ++sampleIndex)
    {
        for(int channelIndex = 0; channelIndex < processors.size(); ++channelIndex)
        {
            processors[channelIndex].currentBlockData[blockWritePos] =  processors[channelIndex].filterSample(audio[channelIndex][sampleIndex]);
        }

        if(++blockWritePos >= blockLengthSamples)
        {
            blockWritePos = 0;
        }
    }

    channelProcessors.realtimeRelease();
}

void FixedTermLoudnessMeter::process(std::span<const float> audio)
{
    std::vector<ChannelProcessor>& processors = channelProcessors.realtimeAquire();

    assert(audio.size() % processors.size() == 0);

    const size_t numSamplesPerChannel = audio.size() / processors.size();
    
    const float* inputData = audio.data();

    for(size_t sampleIndex = 0; sampleIndex < numSamplesPerChannel; ++sampleIndex)
    {
        for(int channelIndex = 0; channelIndex < processors.size(); ++channelIndex)
        {
            processors[channelIndex].currentBlockData[blockWritePos] = processors[channelIndex].filterSample(*inputData++);
        }

        if(++blockWritePos >= blockLengthSamples)
        {
            blockWritePos = 0;
        }
    }

    channelProcessors.realtimeRelease();
}

void FixedTermLoudnessMeter::reset()
{
    channelProcessors.reset(generateChannelProcessors());
    
    blockWritePos = 0;
}

float FixedTermLoudnessMeter::getLoudnessRealtime()
{
    const std::vector<ChannelProcessor>& processors = channelProcessors.realtimeAquire();

    float accumulatedChannels = 0.0f;

    std::for_each(processors.begin(), processors.end(), [&accumulatedChannels](const ChannelProcessor& processor)
    {
        accumulatedChannels += processor.getCurrentBlockMeanSquares() * processor.getWeighting();
    });

    if(accumulatedChannels == 0.0f)
    {
        return min;
    }

    float loudness = -0.691f + 10.0f * std::log10(accumulatedChannels);

    channelProcessors.realtimeRelease();

    return loudness;
}

float FixedTermLoudnessMeter::getLoudness() const
{
    std::scoped_lock<std::mutex> lock{channelProcessorsOfflineLock};

    const std::vector<ChannelProcessor>& processors = channelProcessors.nonRealtimeRead();

    float accumulatedChannels = 0.0f;

    std::for_each(processors.begin(), processors.end(), [&accumulatedChannels](const ChannelProcessor& processor)
    {
        accumulatedChannels += processor.getCurrentBlockMeanSquares() * processor.getWeighting();
    });

    if(accumulatedChannels == 0.0f)
    {
        return min;
    }

    return -0.691f + 10.0f * std::log10(accumulatedChannels);
}

int FixedTermLoudnessMeter::getBlockLengthSamples(const std::chrono::milliseconds& windowLength) const
{
    return (windowLength.count() / 1000.0f) * sampleRate;
}
    
std::vector<ChannelProcessor> FixedTermLoudnessMeter::generateChannelProcessors() const
{
    std::vector<ChannelProcessor> tempChannelProcessors;

    std::transform(channels.begin(), channels.end(), std::back_insert_iterator(tempChannelProcessors), [this](const Channel& channel)
    {
        return ChannelProcessor(channel.getWeighting(), blockLengthSamples);
    });

    return tempChannelProcessors;
}

}
