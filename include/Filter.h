#pragma once

#include <algorithm>

namespace LUFS
{

struct Filter
{
public:
    double a1 = 0.0, a2 = 0.0, b0 = 0.0, b1 = 0.0, b2 = 0.0;
    
    void process(std::span<double> data)
    {
        std::transform(data.begin(), data.end(), data.begin(), [this](double sample)
        {
            return processSample(sample);
        });
    }
    
    double processSample(double sample)
    {
        const double sampleToSave = sample - prev * a1 - last * a2;
        const double output = sampleToSave * b0 + prev * b1 + last * b2;
        
        last = prev;
        prev = sampleToSave;
        
        return output;
    }
    
    void reset()
    {
        prev = 0.0;
        last = 0.0;
    }
    
private:
    double prev = 0.0f;
    double last = 0.0f;
};

}
