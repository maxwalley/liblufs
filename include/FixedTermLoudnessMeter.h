#pragma once

#include <chrono>

#include "ChannelProcessor.h"

namespace LUFS
{

class FixedTermLoudnessMeter
{
public:
    FixedTermLoudnessMeter(const std::vector<Channel>& channels, const std::chrono::milliseconds& windowLength);
    ~FixedTermLoudnessMeter();
    
    //Deinterleaved and interleaved
    void process(std::span<const float* const> audio, int numSamplesPerChannel);
    void process(std::span<const float> audio);
    
    //This is not threadsafe with process calls
    void reset();

    //This is threadsafe with process calls
    float getLoudness() const;

private:
    int getBlockLengthSamples(const std::chrono::milliseconds& windowLength) const;

    const int blockLengthSamples;

    std::vector<ChannelProcessor> channelProcessors;

    size_t blockWritePos = 0;

    static constexpr int sampleRate = 48000;
};

}
