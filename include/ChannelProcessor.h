#pragma once

#include <span>
#include <cassert>
#include <vector>

#include "Channel.h"
#include "Filter.h"

namespace LUFS
{

class ChannelProcessor
{
public:
    ChannelProcessor(float channelWeighting, size_t blockSizeSamples);
    ~ChannelProcessor();
    
    float filterSample(float sample);

    void reset();

    float getCurrentBlockMeanSquares() const;

    const float weighting;

    std::vector<float> currentBlockData;

private:
    void initialiseFilters();
    
    Filter filt1;
    Filter filt2;
};

}