#pragma once

#include "ChannelProcessor.h"

namespace LUFS
{

class LoudnessMeter
{
public:
    LoudnessMeter(const std::chrono::milliseconds& windowLength);
    ~LoudnessMeter();
    
    void process();
    void prepare(double sampleRate);
    
    float getCurrentLoudness() const;
    
private:
    std::vector<ChannelProcessor> channelProcessors;
    
    float lastLoudness = 0.0f;
};

}
