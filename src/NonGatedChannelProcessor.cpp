#include "NonGatedChannelProcessor.h"

namespace LUFS
{

NonGatedChannelProcessor::NonGatedChannelProcessor(float channelWeighting)  : ChannelProcessor(channelWeighting)
{

}

NonGatedChannelProcessor::~NonGatedChannelProcessor()
{

}

std::optional<float> NonGatedChannelProcessor::process(std::span<const float> channelBuffer)
{
    return std::nullopt;
}

}