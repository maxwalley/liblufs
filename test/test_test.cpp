#include "gtest/gtest.h"

#include <filesystem>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "LoudnessMeter.h"

std::filesystem::path getTestContentPath()
{
    return std::filesystem::current_path() / "test_content";
}

TEST(ExampleTests, ExamplePass)
{
    std::filesystem::path testFile1 = getTestContentPath() / "seq-3341-1-16bit.wav";

    drwav audioFile;
    if(!drwav_init_file(&audioFile, testFile1.c_str(), nullptr))
    {
        FAIL() << "Failed to open test file";
    }

    //This should really read as we move through the file
    std::vector<float> fullInterleavedAudio(audioFile.channels * audioFile.totalPCMFrameCount);

    drwav_read_pcm_frames_f32(&audioFile, audioFile.totalPCMFrameCount, fullInterleavedAudio.data());

    std::vector<std::vector<float>> fullDeinterleavedAudio(audioFile.channels, std::vector<float>(audioFile.totalPCMFrameCount, 0.0f));

    for(int sampleIndex = 0; sampleIndex < audioFile.totalPCMFrameCount; ++sampleIndex)
    {
        for(int channelIndex = 0; channelIndex < audioFile.channels; ++channelIndex)
        {
            fullDeinterleavedAudio[channelIndex][sampleIndex] = fullInterleavedAudio[sampleIndex * audioFile.channels + channelIndex];
        }
    }

    std::vector<LUFS::Channel> channels({LUFS::Channel::createStandard(LUFS::StandardChannel::Left), LUFS::Channel::createStandard(LUFS::StandardChannel::Right)});

    LUFS::LoudnessMeter meter(channels, true, std::chrono::milliseconds(400));
    meter.prepare();

    const int bufferSize = 1024;
    std::vector<std::vector<float>> buffer(fullDeinterleavedAudio.size(), std::vector<float>(bufferSize, 0.0f));

    for(int sampleIndex = 0; sampleIndex < audioFile.totalPCMFrameCount - bufferSize; sampleIndex += bufferSize)
    {
        for(int channelIndex = 0; channelIndex < buffer.size(); ++channelIndex)
        {
            const std::vector<float>& sourceChannelBuffer = fullDeinterleavedAudio[channelIndex];
            std::copy(sourceChannelBuffer.begin() + sampleIndex, sourceChannelBuffer.begin() + sampleIndex + bufferSize, buffer[channelIndex].begin());
        }

        meter.process(buffer);

        std::cout << meter.getLoudness() << std::endl;
    }

    drwav_uninit(&audioFile);

    ASSERT_EQ(5, 5);
}

TEST(ExampleTests, ExampleFail)
{
    ASSERT_EQ(5, 4);
}
