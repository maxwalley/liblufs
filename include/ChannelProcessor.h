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
    Leave integration time nullopt for full integrated loudness
    */
    ChannelProcessor(float channelWeighting, bool gated, const std::optional<int>& integrationTimeMs = std::nullopt);
    virtual ~ChannelProcessor();
    
    void prepare();
    std::optional<float> process(std::span<const float> channelBuffer);

private:
    void initialiseFilters();

    float filterSample(float sample);
    float calculateMeanSquares(std::span<const float> buffer) const;
    
    const float weighting;

    Filter filt1;
    Filter filt2;
};

}