#include "GatedChannelProcessor.h"

namespace LUFS
{

GatedChannelProcessor::GatedChannelProcessor(float channelWeighting)  : ChannelProcessor(channelWeighting)
{

}

GatedChannelProcessor::~GatedChannelProcessor()
{

}

std::optional<float> GatedChannelProcessor::process(std::span<const float> channelBuffer)
{
    return std::nullopt;
}

}