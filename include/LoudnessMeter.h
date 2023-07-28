#pragma once

#include <chrono>
#include <vector>
#include <span>

namespace LUFS
{

class LoudnessMeter
{
public:
    LoudnessMeter(const std::chrono::milliseconds& windowLength);
    ~LoudnessMeter();
    
    void prepare(double sampleRate);
    float addSamples(std::span<float> audio);
    
    float getCurrentLoudness() const;
    
private:
    std::chrono::milliseconds length;
    std::vector<float> window;
    
    float lastLoudness = 0.0f;
};

}
