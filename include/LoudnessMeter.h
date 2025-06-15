#pragma once

#include "ChannelProcessor.h"
#include "Block.h"

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
    
    void prepare();
    void process(const std::vector<std::vector<float>>& buffer);
    
private:
    std::vector<ChannelProcessor> channelProcessors;
    std::vector<Block> blocks;
    size_t blocksWritePos = 0;

    size_t currentBlockWritePos = 0;

    static constexpr int sampleRate = 48000;
    static constexpr int blockLengthMs = 400;
    static constexpr int blockLengthSamples = (blockLengthMs / 1000.0) * sampleRate;
    static constexpr float overlap = 0.75f;
    static constexpr int overlapLengthMs = blockLengthMs * (1.0f - overlap);
    static constexpr int overlapLengthSamples = (overlapLengthMs / 1000.0) * sampleRate;
};

}
