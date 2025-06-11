#include "ChannelProcessor.h"

namespace LUFS
{

class GatedChannelProcessor  : public ChannelProcessor
{
public:
    GatedChannelProcessor(float channelWeighting);
    ~GatedChannelProcessor() override;

    std::optional<float> process(std::span<const float> channelBuffer) override;
};

}