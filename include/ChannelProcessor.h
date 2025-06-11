#pragma once

#include <span>

#include "Channel.h"
#include "Filter.h"

namespace LUFS
{

class ChannelProcessor
{
public:
    ChannelProcessor(float channelWeighting);
    virtual ~ChannelProcessor();
    
    void prepare(double sampleRate);
    virtual std::optional<float> process(std::span<const float> channelBuffer)=0;
    
protected:
    double getSampleRate() const;

    const float weighting;

private:
    virtual void prepare() {};

    void initialiseFilters();
    float calculateMeanSquares(std::span<const float> buffer) const;
    
    Filter filt1;
    Filter filt2;

    double sr;
};

}