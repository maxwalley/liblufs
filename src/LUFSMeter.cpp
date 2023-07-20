#include "LUFSMeter.h"

namespace LUFS
{

Meter::Meter(const std::vector<MeasurementType>& measurements, const std::vector<Channel>& channelConfig)
{

}

Meter::~Meter()
{

}

void Meter::setMeasurementTypes(const std::vector<MeasurementType>& measurements)
{

}

void Meter::setChannelConfig(const std::vector<Channel>& channelConfig)
{

}

void Meter::reset()
{

}

void Meter::prepare(double sampleRate, int bufferSize)
{

}

Measurement Meter::process(const std::span<const std::span<const float>> audio)
{

}

}