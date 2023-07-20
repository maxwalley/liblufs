#include "LoudnessMeter.h"

namespace LUFS
{

LoudnessMeter::LoudnessMeter()
{

}

float LoudnessMeter::calculateMeanSquares(std::span<float> data) const
{
    float accumulatedSquares = std::accumulate(data.begin(), data.end(), 0.0f, [](float current, float second)
    {
        return current + std::pow(second, 2);
    });

    return accumulatedSquares / data.size();
}

float LoudnessMeter::calculateLoudnessForBlock(std::span<float> data) const
{
    float accumulatedChannelLoudnesses = 0.0f;
    
    //for each channel
        //First stage filter
        //Second stage filter
        const float meanSquare = calculateMeanSquares();
        //Multiply by channel coefficient
        //Add to accumulated channel loudnesses

    return -0.691f + 10.0f * std::log10(accumulatedChannelLoudnesses)
}

}