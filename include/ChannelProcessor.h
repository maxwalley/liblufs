#include <span>
#include "Filter.h"

namespace LUFS
{

class ChannelProcessor
{
public:
    ChannelProcessor(const std::chrono::milliseconds& windowLength);
    ~ChannelProcessor();
    
    void prepare(double sampleRate);
    float process(std::span<const float> channelBuffer);
    
private:
    void initialiseFilters();
    float calculateMeanSquares(std::span<const float> buffer) const;
    
    std::chrono::milliseconds length;
    std::vector<float> window;
    
    Filter filt1;
    Filter filt2;
};

}