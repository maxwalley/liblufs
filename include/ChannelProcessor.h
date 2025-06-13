#pragma once

#include <span>

#include "Channel.h"
#include "Filter.h"

namespace LUFS
{

class ChannelProcessor
{
public:
    /*
    Leave window length nullopt for full integrated loudness, if specified it must be over 400ms
    */
    ChannelProcessor(float channelWeighting, bool gated, const std::optional<int>& windowLengthMs = std::nullopt);
    virtual ~ChannelProcessor();
    
    void prepare();
    std::optional<float> process(std::span<const float> channelBuffer);

private:
    void initialiseFilters();
    std::optional<int> calculateNumBlocks(const std::optional<int>& windowLengthMs);

    float filterSample(float sample);
    float calculateMeanSquares(std::span<const float> buffer) const;
    
    static constexpr int sampleRate = 48000;
    static constexpr int blockLengthMs = 400;
    static constexpr int blockLengthSamples = (blockLengthMs / 1000.0) * sampleRate;
    static constexpr float overlap = 0.75f;
    static constexpr int overlapLengthMs = blockLengthMs * (1.0f - overlap);
    static constexpr int overlapLengthSamples = (overlapLengthMs / 1000.0) * sampleRate;

    const float weighting;
    const std::optional<int> numBlocks;

    Filter filt1;
    Filter filt2;

    std::vector<float> blockMeanSquares;
    std::vector<float> currentBlock;
    int currentBlockWritePos = 0;
};

}