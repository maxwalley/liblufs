#include "ChannelProcessor.h"

namespace LUFS
{

class NonGatedChannelProcessor  : public ChannelProcessor
{
public:
    NonGatedChannelProcessor(float channelWeighting);
    ~NonGatedChannelProcessor() override;

    std::optional<float> process(std::span<const float> channelBuffer) override;
};

}