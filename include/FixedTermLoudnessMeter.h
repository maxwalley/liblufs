#pragma once

#include <chrono>
#include <cmath>
#include <span>

#include "LiblufsAPI.h"
#include "DoubleBuffer.h"
#include "ChannelProcessor.h"

namespace LUFS
{

class LIBLUFS_API FixedTermLoudnessMeter
{
public:
    FixedTermLoudnessMeter(const std::vector<Channel>& channelSet, const std::chrono::milliseconds& windowLength);
    ~FixedTermLoudnessMeter();
    
    //Deinterleaved and interleaved
    void process(std::span<const float* const> audio, int numSamplesPerChannel);
    void process(std::span<const float> audio);
    
    //This is not threadsafe with process calls
    void reset();

    //This is threadsafe with process calls, but must only be called from a single thread at a time
    float getLoudness() const;

private:
    int getBlockLengthSamples(const std::chrono::milliseconds& windowLength) const;

    std::vector<ChannelProcessor> generateChannelProcessors() const;

    const std::vector<Channel> channels;
    const int blockLengthSamples;

    DoubleBuffer<std::vector<ChannelProcessor>> channelProcessors;

    size_t blockWritePos = 0;

    static constexpr int sampleRate = 48000;
};

}
