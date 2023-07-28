#pragma once

#include <chrono>
#include <vector>
#include <span>
#include "Filter.h"

namespace LUFS
{

class LoudnessMeter
{
public:
    LoudnessMeter(const std::chrono::milliseconds& windowLength);
    ~LoudnessMeter();
    
    void prepare(double sampleRate);
    float addSamples(std::span<const float> audio);
    
    float getCurrentLoudness() const;
    
private:
    void initialiseFilters();
    
    std::chrono::milliseconds length;
    std::vector<float> window;
    
    Filter filt1;
    Filter filt2;
    
    float lastLoudness = 0.0f;
};

}
