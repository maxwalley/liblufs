#pragma once

#include <cassert>
#include <vector>
#include <cmath>

#include "Channel.h"
#include "Filter.h"

namespace LUFS
{

class ChannelProcessor
{
public:
    ChannelProcessor(float channelWeighting, size_t blockSizeSamples);
    
    float getWeighting() const;

    float filterSample(float sample);

    void reset();

    float getCurrentBlockMeanSquares() const;

    std::vector<float> currentBlockData;

private:
    void initialiseFilters();
    
    float weighting;

    Filter filt1;
    Filter filt2;
};

}