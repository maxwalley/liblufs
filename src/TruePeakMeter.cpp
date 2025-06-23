#include "TruePeakMeter.h"

namespace LUFS
{

TruePeakMeter::TruePeakMeter(double sampleRate, int numChannels, int bufferSize)  : expectedNumChannels(numChannels), expectedBufferSize(bufferSize)
{
    int error;
    sampleRateConverter = src_new(SRC_SINC_MEDIUM_QUALITY, numChannels, &error);

    if(!sampleRateConverter || error != 0)
    {
        sampleRateConverter = nullptr;
        throw(std::runtime_error("Failed to create sample rate converter"));
    }

    constexpr double targetSampleRate = 192000.0f;
    const double conversionRatio = targetSampleRate / sampleRate;

    if(src_is_valid_ratio(conversionRatio) == 0)
    {
        throw(std::runtime_error("Input sample rate is not supported"));
    }

    conversionInputBuffer.resize(expectedNumChannels * expectedBufferSize);
    conversionOutputBuffer.resize(conversionInputBuffer.size() * conversionRatio);

    sampleRateConverterState.data_in = conversionInputBuffer.data();
    sampleRateConverterState.data_out = conversionOutputBuffer.data();
    sampleRateConverterState.input_frames = expectedBufferSize;
    sampleRateConverterState.output_frames = expectedBufferSize * conversionRatio;
    sampleRateConverterState.src_ratio = conversionRatio;
    sampleRateConverterState.input_frames_used = 0;
    sampleRateConverterState.output_frames_gen = 0;
    sampleRateConverterState.end_of_input = 0;
}

TruePeakMeter::~TruePeakMeter()
{
    if(sampleRateConverter)
    {
        src_delete(sampleRateConverter);
    }
}

void TruePeakMeter::process(const std::vector<std::vector<float>>& audio)
{
    assert(audio.size() == expectedNumChannels);
    assert(std::all_of(audio.begin(), audio.end(), [this](const std::vector<float>& channelBuffer)
    {
        return channelBuffer.size() == expectedBufferSize;
    }));

    if(audio.size() == 0)
    {
        return;
    }

    //Copy to interleaved input buffer
    for(int channelIndex = 0; channelIndex < expectedNumChannels; ++channelIndex)
    {
        const float* channelBuffer = audio[channelIndex].data();

        for(int sampleIndex = 0; sampleIndex < expectedBufferSize; ++sampleIndex)
        {
            conversionInputBuffer[sampleIndex * expectedNumChannels + channelIndex] = *channelBuffer++ * inputGain;
        }
    }

    process();
}

void TruePeakMeter::process(const std::vector<float>& audio)
{
    assert(audio.size() == expectedNumChannels * expectedBufferSize);

    std::transform(audio.begin(), audio.end(), conversionInputBuffer.begin(), [this](float sample)
    {
        return sample * inputGain;
    });

    process();
}

float TruePeakMeter::getTruePeak() const
{
    if(currentTruePeakGain == 0.0f)
    {
        return 0.0f;
    }

    return 20.0f * log10(currentTruePeakGain);
}

void TruePeakMeter::process()
{
    if(!sampleRateConverter)
    {
        return;
    }

    if(src_process(sampleRateConverter, &sampleRateConverterState) != 0)
    {
        //SOME SORT OF ERROR
        return;
    }

    std::for_each(conversionOutputBuffer.begin(), conversionOutputBuffer.end(), [this](float convertedSample)
    {
        currentTruePeakGain = std::max(std::abs(convertedSample) * outputGain, currentTruePeakGain);
    });
}

}