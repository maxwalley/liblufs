#pragma once

#include <vector>

namespace LUFS
{

struct Block
{
    std::vector<float> channelMeanSquares;

    //Conforms to lj in ITU 1770, may not be used for non gated calculations
    float loudness = 0.0f;
};

}