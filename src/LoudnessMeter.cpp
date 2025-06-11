#include "LoudnessMeter.h"

namespace LUFS
{

LoudnessMeter::LoudnessMeter(const std::chrono::milliseconds& windowLength)
{

}

LoudnessMeter::~LoudnessMeter()
{
    
}

void LoudnessMeter::prepare(double sampleRate)
{
}

void LoudnessMeter::process()
{
}

float LoudnessMeter::getCurrentLoudness() const
{
    return lastLoudness;
}

}
