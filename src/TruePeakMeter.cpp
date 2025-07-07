#include "TruePeakMeter.h"

namespace LUFS
{

TruePeakMeter::TruePeakMeter(double sampleRate, int numChannels, int maxBufferSize)  : expectedNumChannels(numChannels), bufferSizeLimit(maxBufferSize), sampleRateConversionRatio(targetSampleRate / sampleRate)
{
    int error;
    sampleRateConverter = src_new(SRC_SINC_FASTEST, numChannels, &error);

    if(!sampleRateConverter || error != 0)
    {
        sampleRateConverter = nullptr;
        throw(std::runtime_error("Failed to create sample rate converter"));
    }

    if(src_is_valid_ratio(sampleRateConversionRatio) == 0)
    {
        throw(std::runtime_error("Input sample rate is not supported"));
    }

    //Set buffers to the max size they can be
    conversionInputBuffer.resize(expectedNumChannels * bufferSizeLimit);
    conversionOutputBuffer.resize(conversionInputBuffer.size() * sampleRateConversionRatio);

    sampleRateConverterState.data_in = conversionInputBuffer.data();
    sampleRateConverterState.data_out = conversionOutputBuffer.data();
    sampleRateConverterState.src_ratio = sampleRateConversionRatio;
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

void TruePeakMeter::process(std::span<const float* const> audio, int numSamplesPerChannel)
{
    assert(audio.size() == expectedNumChannels);
    assert(numSamplesPerChannel <= bufferSizeLimit);

    if(audio.size() == 0)
    {
        return;
    }

    //Set conversion size
    sampleRateConverterState.input_frames = numSamplesPerChannel;
    sampleRateConverterState.output_frames = numSamplesPerChannel * sampleRateConversionRatio;

    //Copy to interleaved input buffer
    for(int channelIndex = 0; channelIndex < expectedNumChannels; ++channelIndex)
    {
        const float* const channelBuffer = audio[channelIndex];

        for(int sampleIndex = 0; sampleIndex < numSamplesPerChannel; ++sampleIndex)
        {
            conversionInputBuffer[sampleIndex * expectedNumChannels + channelIndex] = channelBuffer[sampleIndex] * inputGain;
        }
    }

    process();
}

void TruePeakMeter::process(std::span<const float> audio)
{
    assert(audio.size() % expectedNumChannels == 0);

    const size_t bufferSize = audio.size() / expectedNumChannels;

    assert(bufferSize <= bufferSizeLimit);

    //Set conversion size
    sampleRateConverterState.input_frames = static_cast<int32_t>(bufferSize);
    sampleRateConverterState.output_frames = static_cast<int32_t>(bufferSize) * sampleRateConversionRatio;

    std::transform(audio.begin(), audio.end(), conversionInputBuffer.begin(), [this](float sample)
    {
        return sample * inputGain;
    });

    process();
}

void TruePeakMeter::reset()
{
    currentTruePeakGain = 0.0f;
}

float TruePeakMeter::getTruePeak() const
{
    if(currentTruePeakGain == 0.0f)
    {
        return 0.0f;
    }

    return 20.0f * std::log10(currentTruePeakGain);
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