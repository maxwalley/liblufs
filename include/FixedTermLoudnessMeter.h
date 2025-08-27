#pragma once

#include <chrono>
#include <cmath>
#include <span>
#include <mutex>

#include "LiblufsAPI.h"
#include "DoubleBuffer.h"
#include "ChannelProcessor.h"

namespace LUFS
{

class LIBLUFS_API FixedTermLoudnessMeter
{
public:
    //Min level will be used if there is silence or not enough data has been processed
    FixedTermLoudnessMeter(const std::vector<Channel>& channelSet, const std::chrono::milliseconds& windowLength, float minLevel = -100.0f);
    ~FixedTermLoudnessMeter();
    
    //Deinterleaved and interleaved
    void process(std::span<const float* const> audio, int numSamplesPerChannel);
    void process(std::span<const float> audio);
    
    //This is not threadsafe with process calls
    void reset();

    //This can only be called from the same thread as the process calls
    float getLoudnessRealtime();

    //This is threadsafe with process calls
    float getLoudness() const;

private:
    int getBlockLengthSamples(const std::chrono::milliseconds& windowLength) const;

    std::vector<ChannelProcessor> generateChannelProcessors() const;

    const std::vector<Channel> channels;
    const int blockLengthSamples;
    const float min;

    mutable std::mutex channelProcessorsOfflineLock;
    DoubleBuffer<std::vector<ChannelProcessor>> channelProcessors;

    size_t blockWritePos = 0;

    static constexpr int sampleRate = 48000;
};

}
