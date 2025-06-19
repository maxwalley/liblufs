#pragma once

#include <vector>

#include "Block.h"

namespace LUFS
{

struct HistogramBlock
{
    void addBlock(const Block& block)
    {
        if(block.channelMeanSquares.size() != accumulatedChannelMeanSquares.size())
        {
            throw(std::runtime_error("Block sizes do not match"));
        }

        std::transform(accumulatedChannelMeanSquares.begin(), accumulatedChannelMeanSquares.end(), block.channelMeanSquares.begin(), accumulatedChannelMeanSquares.begin(), [](float currentVal, float newVal)
        {
            return currentVal + newVal;
        });

        ++numBlocks;
    }

    std::vector<float> accumulatedChannelMeanSquares;

    size_t numBlocks = 0;
};

}