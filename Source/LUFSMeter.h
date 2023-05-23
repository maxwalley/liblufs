#pragma once

#include <vector>
#include <span>
#include <optional>
#include "Channel.h"

namespace LUFS
{

enum class MeasurementType
{
    MomentaryLoudness,
    ShortTermLoudness,
    IntegratedLoudness,
    TruePeak
};

struct Measurement
{
    std::optional<float> momentaryLoudness;
    std::optional<float> shortTermLoudness;
    std::optional<float> integratedLoudness;
    std::optional<float> truePeak;
};

class Meter
{
public:
    Meter(const std::vector<MeasurementType>& measurements = {}, const std::vector<Channel>& channelConfig = {});
    ~Meter();

    void setMeasurementTypes(const std::vector<MeasurementType>& measurements);
    void setChannelConfig(const std::vector<Channel>& channelConfig);

    //Measurement Types and channel config are maintained but internal measurements are reset
    void reset();
    void prepare(double sampleRate, int bufferSize);
    Measurement process(const std::span<const std::span<const float>> audio);
};

}
