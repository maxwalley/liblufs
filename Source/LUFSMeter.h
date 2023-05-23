#pragma once

#include <vector>
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

class Meter
{
public:
    Meter(const std::vector<MeasurementType>& measurements = {}, const std::vector<Channel>& channelConfig = {});
    ~Meter();

    void setMeasurementTypes(const std::vector<MeasurementType>& measurements);
    void setChannelConfig(const std::vector<Channel>& channelConfig);

    void prepare(double sampleRate, int bufferSize);
};

}
