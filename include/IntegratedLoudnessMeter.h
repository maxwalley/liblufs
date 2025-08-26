#pragma once

#include <functional>
#include <optional>
#include <cmath>
#include <span>
#include <iterator>

#include "LiblufsAPI.h"
#include "ChannelProcessor.h"
#include "Block.h"
#include "HistogramBlock.h"
#include "DoubleBuffer.h"

namespace LUFS
{

class LIBLUFS_API IntegratedLoudnessMeter
{
public:
    //Min level will be used if there is silence or not enough data has been processed
    IntegratedLoudnessMeter(const std::vector<Channel>& channels, float minLevel = -100.0f);
    ~IntegratedLoudnessMeter();
    
    //Deinterleaved and interleaved
    void process(std::span<const float* const> audio, int numSamplesPerChannel);
    void process(std::span<const float> audio);
    
    //This is not threadsafe with process calls
    void reset();

    //This is for getting the loudness on the same thread as process calls, it is not threadsafe with process calls
    float getLoudnessRealtime();

    //This is threadsafe with process calls and itself
    float getLoudness() const;

private:
    void processCurrentBlock();
    void calculateBlockLoudness(Block& block) const;

    size_t getHistogramBinIndexForLoudness(float loudness) const;

    std::vector<HistogramBlock> generateBlankHistogram(size_t numChannels) const;

    double calculateLoudnessWithThreshold(const std::vector<HistogramBlock>& histogram, const std::optional<float>& threshold) const;

    const float min;
    
    std::vector<ChannelProcessor> channelProcessors;

    Block processBlock;

    mutable std::mutex histogramOfflineLock;
    DoubleBuffer<std::vector<HistogramBlock>> blockHistogram;
    float histogramMappingSlope = 0.0f;

    size_t currentBlockWritePos = 0;

    static constexpr int sampleRate = 48000;
    static constexpr int blockLengthMs = 400;
    static constexpr int blockLengthSamples = (blockLengthMs / 1000.0) * sampleRate;
    static constexpr float overlap = 0.75f;
    static constexpr int overlapLengthMs = blockLengthMs * (1.0f - overlap);
    static constexpr int overlapLengthSamples = (overlapLengthMs / 1000.0) * sampleRate;

    static constexpr float gateAbsoluteThreshold = -70.0f;

    static constexpr float highestIntegratedValue = 0.0f;
    static constexpr float lowestIntegratedValue = gateAbsoluteThreshold;
    static constexpr float integratedHistogramResolution = 0.01f;
    static constexpr size_t numHistogramElements = (1.0f / integratedHistogramResolution) * (highestIntegratedValue - lowestIntegratedValue);
};

}
