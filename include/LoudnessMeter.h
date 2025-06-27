#pragma once

#include "ChannelProcessor.h"
#include "Block.h"
#include "HistogramBlock.h"

namespace LUFS
{

class LoudnessMeter
{
public:
    /*
    Leave windowLength as nullopt for integrated loudness
    */
    LoudnessMeter(const std::vector<Channel>& channels, bool gated, const std::optional<std::chrono::milliseconds>& windowLength = std::nullopt);
    ~LoudnessMeter();
    
    //Deinterleaved and interleaved
    void process(std::span<const float* const> audio, int numSamplesPerChannel);
    void process(std::span<const float> audio);
    
    //This is not threadsafe with process calls
    void reset();

    //This is threadsafe with process calls
    float getLoudness() const;

private:
    void processCurrentBlock();
    void calculateBlockLoudness(Block& block) const;
    void addBlock(const Block& newBlock);

    size_t getHistogramBinIndexForLoudness(float loudness) const;

    float getLoudnessFixedLength() const;
    float getLoudnessIntegrated() const;

    const bool gate;
    const bool integrated;

    std::vector<ChannelProcessor> channelProcessors;

    //For fixed time applications
    std::vector<Block> blocks;
    size_t blocksWritePos = 0;

    //For integrated metering
    std::vector<HistogramBlock> blockHistogram;
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
};

}
