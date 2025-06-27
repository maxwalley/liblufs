#pragma once

#include <vector>

namespace LUFS
{

struct Block
{
    Block(size_t numChannels)  : channelMeanSquares(numChannels, 0.0f)
    {

    }

    Block& operator=(const Block& other)
    {
        if(channelMeanSquares.size() == other.channelMeanSquares.size())
        {
            //Avoid reallocation (If capacity does not match)
            std::copy(other.channelMeanSquares.begin(), other.channelMeanSquares.end(), channelMeanSquares.begin());
        }
        else
        {
            channelMeanSquares = other.channelMeanSquares;
        }

        loudness = other.loudness;

        return *this;
    }

    std::vector<float> channelMeanSquares;

    //Conforms to lj in ITU 1770, may not be used for non gated calculations
    float loudness = 0.0f;
};

}