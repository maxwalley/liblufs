#include "gtest/gtest.h"

#include <filesystem>
#include <cassert>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "LoudnessMeter.h"

std::optional<std::filesystem::path> getTestContentPath()
{
    const std::filesystem::path path = std::filesystem::current_path() / "test_content";
    return std::filesystem::exists(path) ? path : std::optional<std::filesystem::path>(std::nullopt);
}

std::unique_ptr<drwav> openTestFile(std::string_view testFileName)
{
    const std::optional<std::filesystem::path> contentPath = getTestContentPath();

    if(!contentPath)
    {
        return nullptr;
    }

    std::filesystem::path filePath = *contentPath / testFileName;

    std::unique_ptr<drwav> audioFile = std::make_unique<drwav>();

    if(!drwav_init_file(audioFile.get(), filePath.c_str(), nullptr))
    {
        return nullptr;
    }

    return audioFile;
}

std::vector<LUFS::Channel> createStereoConfig()
{
    return {
        LUFS::Channel::createStandard(LUFS::StandardChannel::Left), 
        LUFS::Channel::createStandard(LUFS::StandardChannel::Right)
    };
}

std::vector<LUFS::Channel> create5point0Config()
{
    return {
        LUFS::Channel::createStandard(LUFS::StandardChannel::Left), 
        LUFS::Channel::createStandard(LUFS::StandardChannel::Right),
        LUFS::Channel::createStandard(LUFS::StandardChannel::Centre), 
        LUFS::Channel::createStandard(LUFS::StandardChannel::LeftSurround),
        LUFS::Channel::createStandard(LUFS::StandardChannel::RightSurround),
    };
}

void processFile(drwav& file, std::function<void(const std::vector<std::vector<float>>&)> audioBufferCallback)
{
    assert(audioBufferCallback);

    const int bufferSize = 1024;

    std::vector<float> interleavedBuffer(file.channels * bufferSize);
    std::vector<std::vector<float>> deinterleavedBuffer(file.channels);
    uint64_t numFramesRead = bufferSize;

    while(numFramesRead >= bufferSize)
    {
        numFramesRead = drwav_read_pcm_frames_f32(&file, bufferSize, interleavedBuffer.data());

        //Resize each channel and deinterleave samples
        size_t channelIndex = 0;
        std::for_each(deinterleavedBuffer.begin(), deinterleavedBuffer.end(), [&](std::vector<float>& channelBuffer)
        {
            channelBuffer.resize(numFramesRead);

            for(int sampleIndex = 0; sampleIndex < numFramesRead; ++sampleIndex)
            {
                channelBuffer[sampleIndex] = interleavedBuffer[sampleIndex * file.channels + channelIndex];
            }

            ++channelIndex;
        });

        audioBufferCallback(deinterleavedBuffer);
    }
}

TEST(EBU3341_Test_Set, Test_1)
{
    const std::string testFileName = "seq-3341-1-16bit.wav";

    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);

    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }

    LUFS::LoudnessMeter momentaryMeter(createStereoConfig(), false, std::chrono::milliseconds(400));
    LUFS::LoudnessMeter shortTermMeter(createStereoConfig(), false, std::chrono::milliseconds(3000));
    LUFS::LoudnessMeter integratedMeter(createStereoConfig(), true);
    momentaryMeter.prepare();
    shortTermMeter.prepare();
    integratedMeter.prepare();

    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        momentaryMeter.process(buffer);
        shortTermMeter.process(buffer);
        integratedMeter.process(buffer);
    };

    processFile(*audioFile, audioBufferCallback);

    const float momentaryLoudness = momentaryMeter.getLoudness();
    const float shortTermLoudness = shortTermMeter.getLoudness();
    const float integratedLoudness = integratedMeter.getLoudness();

    ASSERT_LE(momentaryLoudness, -22.9f);
    ASSERT_GE(momentaryLoudness, -23.1f);

    ASSERT_LE(shortTermLoudness, -22.9f);
    ASSERT_GE(shortTermLoudness, -23.1f);

    ASSERT_LE(integratedLoudness, -22.9f);
    ASSERT_GE(integratedLoudness, -23.1f);
}

TEST(EBU3341_Test_Set, Test_2)
{
    const std::string testFileName = "seq-3341-2-16bit.wav";

    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);

    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }

    LUFS::LoudnessMeter momentaryMeter(createStereoConfig(), false, std::chrono::milliseconds(400));
    LUFS::LoudnessMeter shortTermMeter(createStereoConfig(), false, std::chrono::milliseconds(3000));
    LUFS::LoudnessMeter integratedMeter(createStereoConfig(), true);
    momentaryMeter.prepare();
    shortTermMeter.prepare();
    integratedMeter.prepare();

    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        momentaryMeter.process(buffer);
        shortTermMeter.process(buffer);
        integratedMeter.process(buffer);
    };

    processFile(*audioFile, audioBufferCallback);

    const float momentaryLoudness = momentaryMeter.getLoudness();
    const float shortTermLoudness = shortTermMeter.getLoudness();
    const float integratedLoudness = integratedMeter.getLoudness();

    ASSERT_LE(momentaryLoudness, -32.9f);
    ASSERT_GE(momentaryLoudness, -33.1f);

    ASSERT_LE(shortTermLoudness, -32.9f);
    ASSERT_GE(shortTermLoudness, -33.1f);

    ASSERT_LE(integratedLoudness, -32.9f);
    ASSERT_GE(integratedLoudness, -33.1f);
}

TEST(EBU3341_Test_Set, Test_3)
{
    const std::string testFileName = "seq-3341-3-16bit-v02.wav";

    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);

    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }

    LUFS::LoudnessMeter integratedMeter(createStereoConfig(), true);
    integratedMeter.prepare();

    const auto audioBufferCallback = [&integratedMeter](const std::vector<std::vector<float>>& buffer)
    {
        integratedMeter.process(buffer);
    };

    processFile(*audioFile, audioBufferCallback);

    const float integratedLoudness = integratedMeter.getLoudness();

    ASSERT_LE(integratedLoudness, -22.9f);
    ASSERT_GE(integratedLoudness, -23.1f);
}

TEST(EBU3341_Test_Set, Test_4)
{
    const std::string testFileName = "seq-3341-4-16bit-v02.wav";

    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);

    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }

    LUFS::LoudnessMeter integratedMeter(createStereoConfig(), true);
    integratedMeter.prepare();

    const auto audioBufferCallback = [&integratedMeter](const std::vector<std::vector<float>>& buffer)
    {
        integratedMeter.process(buffer);
    };

    processFile(*audioFile, audioBufferCallback);

    const float integratedLoudness = integratedMeter.getLoudness();

    ASSERT_LE(integratedLoudness, -22.9f);
    ASSERT_GE(integratedLoudness, -23.1f);
}

TEST(EBU3341_Test_Set, Test_5)
{
    const std::string testFileName = "seq-3341-5-16bit-v02.wav";

    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);

    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }

    LUFS::LoudnessMeter integratedMeter(createStereoConfig(), true);
    integratedMeter.prepare();

    const auto audioBufferCallback = [&integratedMeter](const std::vector<std::vector<float>>& buffer)
    {
        integratedMeter.process(buffer);
    };

    processFile(*audioFile, audioBufferCallback);

    const float integratedLoudness = integratedMeter.getLoudness();

    ASSERT_LE(integratedLoudness, -22.9f);
    ASSERT_GE(integratedLoudness, -23.1f);
}

TEST(EBU3341_TEST_SET, Test_6)
{
    const std::string testFileName = "seq-3341-6-5channels-16bit.wav";

    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);

    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }

    LUFS::LoudnessMeter integratedMeter(create5point0Config(), true);
    integratedMeter.prepare();

    const auto audioBufferCallback = [&integratedMeter](const std::vector<std::vector<float>>& buffer)
    {
        integratedMeter.process(buffer);
    };

    processFile(*audioFile, audioBufferCallback);

    const float integratedLoudness = integratedMeter.getLoudness();

    ASSERT_LE(integratedLoudness, -22.9f);
    ASSERT_GE(integratedLoudness, -23.1f);
}

TEST(EBU3341_Test_Set, Test_7)
{
    const std::string testFileName = "seq-3341-7_seq-3342-5-24bit.wav";

    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);

    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }

    LUFS::LoudnessMeter integratedMeter(createStereoConfig(), true);
    integratedMeter.prepare();

    const auto audioBufferCallback = [&integratedMeter](const std::vector<std::vector<float>>& buffer)
    {
        integratedMeter.process(buffer);
    };

    processFile(*audioFile, audioBufferCallback);

    const float integratedLoudness = integratedMeter.getLoudness();

    ASSERT_LE(integratedLoudness, -22.9f);
    ASSERT_GE(integratedLoudness, -23.1f);
}