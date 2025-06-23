#pragma once

#include <vector>
#include <cassert>

#include "samplerate.h"

namespace LUFS
{

class TruePeakMeter
{
public:
    TruePeakMeter(double sampleRate, int numChannels, int bufferSize);
    ~TruePeakMeter();

    //Deinterleaved and Interleaved
    void process(const std::vector<std::vector<float>>& audio);
    void process(const std::vector<float>& audio);

    float getTruePeak() const;

private:
    void process();

    const int expectedNumChannels;
    const int expectedBufferSize;

    SRC_STATE* sampleRateConverter = nullptr;
    SRC_DATA sampleRateConverterState;

    std::vector<float> conversionInputBuffer;
    std::vector<float> conversionOutputBuffer;

    //-12.04dB in gain
    static constexpr float inputGain = 0.2491f;

    //12.04dB in gain
    static constexpr float outputGain = 3.999f;

    float currentTruePeak = 0.0f;
};

}