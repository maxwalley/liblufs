#pragma once

#include <span>
#include <numeric>

namespace LUFS
{

class LoudnessMeter
{
public:
    LoudnessMeter();

private:
    float calculateMeanSquares(std::span<float> data) const;
    float calculateLoudnessForBlock(std::span<float> data) const;
};

}