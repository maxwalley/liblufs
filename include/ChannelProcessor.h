#pragma once

#include <span>
#include <cassert>

#include "Channel.h"
#include "Filter.h"

namespace LUFS
{

class ChannelProcessor
{
public:
    ChannelProcessor(float channelWeighting, size_t blockSizeSamples);
    ~ChannelProcessor();
    
    void prepare();

    float filterSample(float sample);

    float getCurrentBlockMeanSquares() const;

    const float weighting;

    std::vector<float> currentBlockData;

private:
    void initialiseFilters();
    
    Filter filt1;
    Filter filt2;
};

}