#pragma once

#include <span>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <cmath>

#include "LiblufsAPI.h"
#include "samplerate.h"

namespace LUFS
{

class LIBLUFS_API TruePeakMeter
{
public:
    //Buffers can come in smaller than this but must never exceed this number of samples per channel
    TruePeakMeter(double sampleRate, int numChannels, int maxBufferSize);
    ~TruePeakMeter();

    //Deinterleaved and Interleaved
    void process(std::span<const float* const> audio, int numSamplesPerChannel);
    void process(std::span<const float> audio);

    //This is not threadsafe with process calls
    void reset();

    //This is threadsafe with process calls
    float getTruePeak() const;

private:
    void process();

    const int expectedNumChannels;
    const int bufferSizeLimit;
    static constexpr double targetSampleRate = 192000.0;
    const double sampleRateConversionRatio;

    SRC_STATE* sampleRateConverter = nullptr;
    SRC_DATA sampleRateConverterState;

    std::vector<float> conversionInputBuffer;
    std::vector<float> conversionOutputBuffer;

    //-12.04dB in gain
    static constexpr float inputGain = 0.2491f;

    //12.04dB in gain
    static constexpr float outputGain = 3.999f;

    float currentTruePeakGain = 0.0f;
};

}