#pragma once

#include <vector>

namespace LUFS
{

struct HistogramBlock
{
    std::vector<float> accumulatedChannelMeanSquares;

    size_t numBlocks = 0;
};

}