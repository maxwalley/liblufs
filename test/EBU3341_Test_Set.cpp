#include "gtest/gtest.h"

#include <filesystem>
#include <cassert>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "FixedTermLoudnessMeter.h"
#include "IntegratedLoudnessMeter.h"
#include "TruePeakMeter.h"

std::optional<std::filesystem::path> getTestContentPath()
{
    const std::filesystem::path path = std::filesystem::current_path() / "../_deps/ebuloudnesstestset-src";
    std::cout << path << std::endl;

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
    
    if(!drwav_init_file(audioFile.get(), (const char*)filePath.c_str(), nullptr))
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

void processFile(drwav& file, std::function<void(const std::vector<std::vector<float>>&)> audioBufferCallback, int maxBufferSize)
{
    assert(audioBufferCallback);
    
    std::vector<float> interleavedBuffer(file.channels * maxBufferSize);
    std::vector<std::vector<float>> deinterleavedBuffer(file.channels);
    uint64_t numFramesRead = maxBufferSize;
    
    while(numFramesRead >= maxBufferSize)
    {
        numFramesRead = drwav_read_pcm_frames_f32(&file, maxBufferSize, interleavedBuffer.data());
        
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
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize, 0.0f);
    
    LUFS::FixedTermLoudnessMeter momentaryMeter(createStereoConfig(), std::chrono::milliseconds(400));
    LUFS::FixedTermLoudnessMeter shortTermMeter(createStereoConfig(), std::chrono::milliseconds(3000));
    LUFS::IntegratedLoudnessMeter integratedMeter(createStereoConfig());
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        const size_t numSamples = buffer[0].size();
        
        interleavedBuffer.resize(numChannels * numSamples);
        
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            const float* channelData = buffer[channelIndex].data();
            
            for(int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = *channelData;
                ++channelData;
            }
        }
        
        momentaryMeter.process(interleavedBuffer);
        shortTermMeter.process(interleavedBuffer);
        integratedMeter.process(interleavedBuffer);
    };
    
    processFile(*audioFile, audioBufferCallback, maxBufferSize);
    
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
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<const float*> channelBuffers{numChannels, nullptr};
    
    LUFS::FixedTermLoudnessMeter momentaryMeter(createStereoConfig(), std::chrono::milliseconds(400));
    LUFS::FixedTermLoudnessMeter shortTermMeter(createStereoConfig(), std::chrono::milliseconds(3000));
    LUFS::IntegratedLoudnessMeter integratedMeter(createStereoConfig());
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
        {
            return channelData.data();
        });
        
        momentaryMeter.process(channelBuffers, buffer[0].size());
        shortTermMeter.process(channelBuffers, buffer[0].size());
        integratedMeter.process(channelBuffers, buffer[0].size());
    };
    
    processFile(*audioFile, audioBufferCallback, 1024);
    
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
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize, 0.0f);
    
    LUFS::IntegratedLoudnessMeter integratedMeter(createStereoConfig());
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        const size_t numSamples = buffer[0].size();
        
        interleavedBuffer.resize(numChannels * numSamples);
        
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            const float* channelData = buffer[channelIndex].data();
            
            for(int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = *channelData;
                ++channelData;
            }
        }
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        integratedMeter.process(interleavedBuffer);
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
    };
    
    processFile(*audioFile, audioBufferCallback, 1024);
    
    std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
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
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<const float*> channelBuffers{numChannels, nullptr};
    
    LUFS::IntegratedLoudnessMeter integratedMeter(createStereoConfig());
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
        {
            return channelData.data();
        });
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        integratedMeter.process(channelBuffers, buffer[0].size());
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
    };
    
    processFile(*audioFile, audioBufferCallback, 1024);
    
    std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
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
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize, 0.0f);
    
    LUFS::IntegratedLoudnessMeter integratedMeter(createStereoConfig());
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        const size_t numSamples = buffer[0].size();
        
        interleavedBuffer.resize(numChannels * numSamples);
        
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            const float* channelData = buffer[channelIndex].data();
            
            for(int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = *channelData;
                ++channelData;
            }
        }
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        integratedMeter.process(interleavedBuffer);
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
    };
    
    processFile(*audioFile, audioBufferCallback, 1024);
    
    std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    const float integratedLoudness = integratedMeter.getLoudness();
    
    ASSERT_LE(integratedLoudness, -22.9f);
    ASSERT_GE(integratedLoudness, -23.1f);
}

TEST(EBU3341_Test_Set, Test_6)
{
    const std::string testFileName = "seq-3341-6-5channels-16bit.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 5;
    
    std::vector<const float*> channelBuffers{numChannels, nullptr};
    
    LUFS::IntegratedLoudnessMeter integratedMeter(create5point0Config());
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
        {
            return channelData.data();
        });
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        integratedMeter.process(channelBuffers, buffer[0].size());
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
    };
    
    processFile(*audioFile, audioBufferCallback, 1024);
    
    std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
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
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize, 0.0f);
    
    LUFS::IntegratedLoudnessMeter integratedMeter(createStereoConfig());
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        const size_t numSamples = buffer[0].size();
        
        interleavedBuffer.resize(numChannels * numSamples);
        
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            const float* channelData = buffer[channelIndex].data();
            
            for(int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = *channelData;
                ++channelData;
            }
        }
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        integratedMeter.process(interleavedBuffer);
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
    };
    
    processFile(*audioFile, audioBufferCallback, 1024);
    
    std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    const float integratedLoudness = integratedMeter.getLoudness();
    
    ASSERT_LE(integratedLoudness, -22.9f);
    ASSERT_GE(integratedLoudness, -23.1f);
}

TEST(EBU3341_Test_Set, Test_8)
{
    const std::string testFileName = "seq-3341-2011-8_seq-3342-6-24bit-v02.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize, 0.0f);
    
    LUFS::IntegratedLoudnessMeter integratedMeter(createStereoConfig());
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        const size_t numSamples = buffer[0].size();
        
        interleavedBuffer.resize(numChannels * numSamples);
        
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            const float* channelData = buffer[channelIndex].data();
            
            for(int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = *channelData;
                ++channelData;
            }
        }
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        integratedMeter.process(interleavedBuffer);
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
    };
    
    processFile(*audioFile, audioBufferCallback, 1024);
    
    std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    const float integratedLoudness = integratedMeter.getLoudness();
    
    ASSERT_LE(integratedLoudness, -22.9f);
    ASSERT_GE(integratedLoudness, -23.1f);
}

TEST(EBU3341_Test_Set, Test_9)
{
    const std::string testFileName = "seq-3341-9-24bit.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    const int constantTimeThresholdSamples = 48000 * 3;
    
    std::vector<const float*> channelBuffers{numChannels, nullptr};
    
    LUFS::FixedTermLoudnessMeter shortTermMeter(createStereoConfig(), std::chrono::milliseconds(3000));
    
    std::chrono::microseconds longestProcessDuration(0);
    
    size_t totalNumSamplesProcessed = 0;
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        const int numSamples = buffer[0].size();
        
        std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
        {
            return channelData.data();
        });
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        shortTermMeter.process(channelBuffers, numSamples);
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        totalNumSamplesProcessed += numSamples;
        
        //Stabibility Check
        if(totalNumSamplesProcessed >= constantTimeThresholdSamples)
        {
            const float shortTermLoudness = shortTermMeter.getLoudness();
            
            ASSERT_LE(shortTermLoudness, -22.9f);
            ASSERT_GE(shortTermLoudness, -23.1f);
        }
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
    };
    
    processFile(*audioFile, audioBufferCallback, 1024);
    
    std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
}

TEST(EBU3341_Test_Set, Test_10)
{
    for(int testIndex = 0; testIndex < 20; ++testIndex)
    {
        const std::string testFileName = "seq-3341-10-" + std::to_string(testIndex + 1) + "-24bit.wav";
        
        std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
        
        if(!audioFile)
        {
            FAIL() << "Failed to open test file: " << testFileName;
        }
        
        const int maxBufferSize = 1024;
        const int numChannels = 2;
        
        std::vector<const float*> channelBuffers{numChannels, nullptr};
        
        LUFS::FixedTermLoudnessMeter shortTermMeter(createStereoConfig(), std::chrono::milliseconds(3000));
        
        std::chrono::microseconds longestProcessDuration(0);
        float maxLoudness = std::numeric_limits<float>::lowest();
        
        const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
        {
            const int numSamples = buffer[0].size();
            
            std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
            {
                return channelData.data();
            });
            
            const auto startTime = std::chrono::high_resolution_clock::now();
            
            shortTermMeter.process(channelBuffers, numSamples);
            
            const auto endTime = std::chrono::high_resolution_clock::now();
            
            if(endTime - startTime > longestProcessDuration)
            {
                longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            }
            
            maxLoudness = std::max(maxLoudness, shortTermMeter.getLoudness());
        };
        
        processFile(*audioFile, audioBufferCallback, 1024);
        
        ASSERT_LE(maxLoudness, -22.9f) << testFileName;
        ASSERT_GE(maxLoudness, -23.1f) << testFileName;
        
        std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
    }
}

TEST(EBU3341_Test_Set, Test_11)
{
    const std::string testFileName = "seq-3341-11-24bit.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    size_t segmentNumSamples = 48000 * 6;
    
    std::vector<const float*> channelBuffers{numChannels, nullptr};
    
    LUFS::FixedTermLoudnessMeter shortTermMeter(createStereoConfig(), std::chrono::milliseconds(3000));
    
    std::chrono::microseconds longestProcessDuration(0);
    
    size_t currentSegmentNumSamplesProcessed = 0;
    float currentMaxShortTermLoudness = std::numeric_limits<float>::lowest();
    int segmentIndex = 0;
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        const int numSamples = buffer[0].size();
        
        std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
        {
            return channelData.data();
        });
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        shortTermMeter.process(channelBuffers, numSamples);
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        const float shortTermLoudness = shortTermMeter.getLoudness();
        
        currentMaxShortTermLoudness = std::max(currentMaxShortTermLoudness, shortTermLoudness);
        
        currentSegmentNumSamplesProcessed += numSamples;
        
        if(currentSegmentNumSamplesProcessed >= segmentNumSamples)
        {
            ASSERT_LE(currentMaxShortTermLoudness, -37.9f + segmentIndex);
            ASSERT_GE(currentMaxShortTermLoudness, -38.1f + segmentIndex);
            
            currentMaxShortTermLoudness = std::numeric_limits<float>::lowest();
            
            currentSegmentNumSamplesProcessed -= segmentNumSamples;
            
            ++segmentIndex;
        }
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
    };
    
    //Round down to hit even buffers for 2 min file
    processFile(*audioFile, audioBufferCallback, 1000);
    
    std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
}

TEST(EBU3341_Test_Set, Test_12)
{
    const std::string testFileName = "seq-3341-12-24bit.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    const int constantTimeThresholdSamples = 48000;
    
    std::vector<const float*> channelBuffers{numChannels, nullptr};
    
    LUFS::FixedTermLoudnessMeter momentaryMeter(createStereoConfig(), std::chrono::milliseconds(400));
    
    std::chrono::microseconds longestProcessDuration(0);
    
    size_t totalNumSamplesProcessed = 0;
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        const int numSamples = buffer[0].size();
        
        std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
        {
            return channelData.data();
        });
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        momentaryMeter.process(channelBuffers, numSamples);
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        totalNumSamplesProcessed += numSamples;
        
        //Stabibility Check
        if(totalNumSamplesProcessed >= constantTimeThresholdSamples)
        {
            const float shortTermLoudness = momentaryMeter.getLoudness();
            
            ASSERT_LE(shortTermLoudness, -22.9f);
            ASSERT_GE(shortTermLoudness, -23.1f);
        }
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
    };
    
    processFile(*audioFile, audioBufferCallback, 1024);
    
    std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
}

TEST(EBU3341_Test_Set, Test_13)
{
    for(int testIndex = 0; testIndex < 20; ++testIndex)
    {
        const std::string testFileName = "seq-3341-10-" + std::to_string(testIndex + 1) + "-24bit.wav";
        
        std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
        
        if(!audioFile)
        {
            FAIL() << "Failed to open test file: " << testFileName;
        }
        
        const int maxBufferSize = 1024;
        const int numChannels = 2;
        
        std::vector<const float*> channelBuffers{numChannels, nullptr};
        
        LUFS::FixedTermLoudnessMeter momentaryMeter(createStereoConfig(), std::chrono::milliseconds(400));
        
        std::chrono::microseconds longestProcessDuration(0);
        float maxLoudness = std::numeric_limits<float>::lowest();
        
        const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
        {
            const int numSamples = buffer[0].size();
            
            std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
            {
                return channelData.data();
            });
            
            const auto startTime = std::chrono::high_resolution_clock::now();
            
            momentaryMeter.process(channelBuffers, numSamples);
            
            const auto endTime = std::chrono::high_resolution_clock::now();
            
            if(endTime - startTime > longestProcessDuration)
            {
                longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            }
            
            maxLoudness = std::max(maxLoudness, momentaryMeter.getLoudness());
        };
        
        processFile(*audioFile, audioBufferCallback, 1024);
        
        ASSERT_LE(maxLoudness, -22.9f) << testFileName;
        ASSERT_GE(maxLoudness, -23.1f) << testFileName;
        
        std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
    }
}

TEST(EBU3341_Test_Set, Test_14)
{
    const std::string testFileName = "seq-3341-14-24bit.wav.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int numChannels = 2;
    
    //800ms
    size_t segmentNumSamples = 38400;
    
    //10ms - This test only seems to work with a very fast update frequency
    size_t inputBufferSize = 48;
    size_t buffersPerSegment = segmentNumSamples / inputBufferSize;
    
    LUFS::FixedTermLoudnessMeter momentaryMeter{createStereoConfig(), std::chrono::milliseconds{400}};
    
    std::chrono::microseconds longestProcessDuration{0};
    
    std::vector<float> interleavedBuffer(numChannels * inputBufferSize);
    
    for(int segmentIndex = 0; segmentIndex < 20; ++segmentIndex)
    {
        float maxMomentaryLoudness = std::numeric_limits<float>::lowest();
        
        for(int bufferIndex = 0; bufferIndex < buffersPerSegment; ++bufferIndex)
        {
            const int numSamplesRead = drwav_read_pcm_frames_f32(audioFile.get(), inputBufferSize, interleavedBuffer.data());
            
            EXPECT_EQ(numSamplesRead, inputBufferSize);
            
            const auto startTime = std::chrono::high_resolution_clock::now();
            
            momentaryMeter.process(interleavedBuffer);
            
            const auto endTime = std::chrono::high_resolution_clock::now();
            
            maxMomentaryLoudness = std::max(maxMomentaryLoudness, momentaryMeter.getLoudness());
            
            if(endTime - startTime > longestProcessDuration)
            {
                longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            }
        }
        
        ASSERT_LE(maxMomentaryLoudness, -37.9f + segmentIndex);
        ASSERT_GE(maxMomentaryLoudness, -38.1f + segmentIndex);
    }
    
    std::cout << "Longest meter processing time: " << longestProcessDuration.count() << " microseconds" << std::endl;
}

TEST(EBU3341_Test_Set, Test_15)
{
    const std::string testFileName = "seq-3341-15-24bit.wav.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<const float*> channelBuffers{numChannels, nullptr};
    LUFS::TruePeakMeter meter(48000.0, numChannels, maxBufferSize);
    
    float maxTruePeak = std::numeric_limits<float>::lowest();
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
        {
            return channelData.data();
        });
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        meter.process(channelBuffers, buffer[0].size());
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
        
        maxTruePeak = std::max(meter.getTruePeak(), maxTruePeak);
    };
    
    processFile(*audioFile, audioBufferCallback, maxBufferSize);
    
    std::cout << "Longest true peak calculation: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    ASSERT_LE(maxTruePeak, -5.8f);
    ASSERT_GE(maxTruePeak, -6.4f);
}

TEST(EBU3341_Test_Set, Test_16)
{
    const std::string testFileName = "seq-3341-16-24bit.wav.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize);
    LUFS::TruePeakMeter meter(48000.0, numChannels, maxBufferSize);
    
    float maxTruePeak = std::numeric_limits<float>::lowest();
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            for(int sampleIndex = 0; sampleIndex < buffer[0].size(); ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = buffer[channelIndex][sampleIndex];
            }
        }
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        meter.process({interleavedBuffer.data(), interleavedBuffer.size()});
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
        
        maxTruePeak = std::max(meter.getTruePeak(), maxTruePeak);
    };
    
    processFile(*audioFile, audioBufferCallback, maxBufferSize);
    
    std::cout << "Longest true peak calculation: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    ASSERT_LE(maxTruePeak, -5.8f);
    ASSERT_GE(maxTruePeak, -6.4f);
}

TEST(EBU3341_Test_Set, Test_17)
{
    const std::string testFileName = "seq-3341-17-24bit.wav.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<const float*> channelBuffers{numChannels, nullptr};
    LUFS::TruePeakMeter meter(48000.0, numChannels, maxBufferSize);
    
    float maxTruePeak = std::numeric_limits<float>::lowest();
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
        {
            return channelData.data();
        });
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        meter.process(channelBuffers, buffer[0].size());
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
        
        maxTruePeak = std::max(meter.getTruePeak(), maxTruePeak);
    };
    
    processFile(*audioFile, audioBufferCallback, maxBufferSize);
    
    std::cout << "Longest true peak calculation: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    ASSERT_LE(maxTruePeak, -5.8f);
    ASSERT_GE(maxTruePeak, -6.4f);
}

TEST(EBU3341_Test_Set, Test_18)
{
    const std::string testFileName = "seq-3341-18-24bit.wav.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize);
    LUFS::TruePeakMeter meter(48000.0, numChannels, maxBufferSize);
    
    float maxTruePeak = std::numeric_limits<float>::lowest();
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            for(int sampleIndex = 0; sampleIndex < buffer[0].size(); ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = buffer[channelIndex][sampleIndex];
            }
        }
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        meter.process({interleavedBuffer.data(), interleavedBuffer.size()});
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
        
        maxTruePeak = std::max(meter.getTruePeak(), maxTruePeak);
    };
    
    processFile(*audioFile, audioBufferCallback, maxBufferSize);
    
    std::cout << "Longest true peak calculation: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    ASSERT_LE(maxTruePeak, -5.8f);
    ASSERT_GE(maxTruePeak, -6.4f);
}

TEST(EBU3341_Test_Set, Test_19)
{
    const std::string testFileName = "seq-3341-19-24bit.wav.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<const float*> channelBuffers{numChannels, nullptr};
    LUFS::TruePeakMeter meter(48000.0, numChannels, maxBufferSize);
    
    float maxTruePeak = std::numeric_limits<float>::lowest();
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        std::transform(buffer.begin(), buffer.end(), channelBuffers.begin(), [](const std::vector<float>& channelData)
        {
            return channelData.data();
        });
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        meter.process(channelBuffers, buffer[0].size());
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
        
        maxTruePeak = std::max(meter.getTruePeak(), maxTruePeak);
    };
    
    processFile(*audioFile, audioBufferCallback, maxBufferSize);
    
    std::cout << "Longest true peak calculation: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    ASSERT_LE(maxTruePeak, 3.2f);
    ASSERT_GE(maxTruePeak, 2.6f);
}

TEST(EBU3341_Test_Set, Test_20)
{
    const std::string testFileName = "seq-3341-20-24bit.wav.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize);
    LUFS::TruePeakMeter meter(48000.0, numChannels, maxBufferSize);
    
    float maxTruePeak = std::numeric_limits<float>::lowest();
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            for(int sampleIndex = 0; sampleIndex < buffer[0].size(); ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = buffer[channelIndex][sampleIndex];
            }
        }
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        meter.process({interleavedBuffer.data(), interleavedBuffer.size()});
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
        
        maxTruePeak = std::max(meter.getTruePeak(), maxTruePeak);
    };
    
    processFile(*audioFile, audioBufferCallback, maxBufferSize);
    
    std::cout << "Longest true peak calculation: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    ASSERT_LE(maxTruePeak, 0.2f);
    ASSERT_GE(maxTruePeak, -0.4f);
}

TEST(EBU3341_Test_Set, Test_21)
{
    const std::string testFileName = "seq-3341-21-24bit.wav.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize);
    LUFS::TruePeakMeter meter(48000.0, numChannels, maxBufferSize);
    
    float maxTruePeak = std::numeric_limits<float>::lowest();
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            for(int sampleIndex = 0; sampleIndex < buffer[0].size(); ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = buffer[channelIndex][sampleIndex];
            }
        }
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        meter.process({interleavedBuffer.data(), interleavedBuffer.size()});
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
        
        maxTruePeak = std::max(meter.getTruePeak(), maxTruePeak);
    };
    
    processFile(*audioFile, audioBufferCallback, maxBufferSize);
    
    std::cout << "Longest true peak calculation: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    ASSERT_LE(maxTruePeak, 0.2f);
    ASSERT_GE(maxTruePeak, -0.4f);
}

TEST(EBU3341_Test_Set, Test_22)
{
    const std::string testFileName = "seq-3341-22-24bit.wav.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize);
    LUFS::TruePeakMeter meter(48000.0, numChannels, maxBufferSize);
    
    float maxTruePeak = std::numeric_limits<float>::lowest();
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            for(int sampleIndex = 0; sampleIndex < buffer[0].size(); ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = buffer[channelIndex][sampleIndex];
            }
        }
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        meter.process({interleavedBuffer.data(), interleavedBuffer.size()});
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
        
        maxTruePeak = std::max(meter.getTruePeak(), maxTruePeak);
    };
    
    processFile(*audioFile, audioBufferCallback, maxBufferSize);
    
    std::cout << "Longest true peak calculation: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    ASSERT_LE(maxTruePeak, 0.2f);
    ASSERT_GE(maxTruePeak, -0.4f);
}

TEST(EBU3341_Test_Set, Test_23)
{
    const std::string testFileName = "seq-3341-23-24bit.wav.wav";
    
    std::unique_ptr<drwav> audioFile = openTestFile(testFileName);
    
    if(!audioFile)
    {
        FAIL() << "Failed to open test file: " << testFileName;
    }
    
    const int maxBufferSize = 1024;
    const int numChannels = 2;
    
    std::vector<float> interleavedBuffer(numChannels * maxBufferSize);
    LUFS::TruePeakMeter meter(48000.0, numChannels, maxBufferSize);
    
    float maxTruePeak = std::numeric_limits<float>::lowest();
    
    std::chrono::microseconds longestProcessDuration(0);
    
    const auto audioBufferCallback = [&](const std::vector<std::vector<float>>& buffer)
    {
        for(int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
        {
            for(int sampleIndex = 0; sampleIndex < buffer[0].size(); ++sampleIndex)
            {
                interleavedBuffer[sampleIndex * numChannels + channelIndex] = buffer[channelIndex][sampleIndex];
            }
        }
        
        const auto startTime = std::chrono::high_resolution_clock::now();
        
        meter.process({interleavedBuffer.data(), interleavedBuffer.size()});
        
        const auto endTime = std::chrono::high_resolution_clock::now();
        
        if(endTime - startTime > longestProcessDuration)
        {
            longestProcessDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }
        
        maxTruePeak = std::max(meter.getTruePeak(), maxTruePeak);
    };
    
    processFile(*audioFile, audioBufferCallback, maxBufferSize);
    
    std::cout << "Longest true peak calculation: " << longestProcessDuration.count() << " microseconds" << std::endl;
    
    ASSERT_LE(maxTruePeak, 0.2f);
    ASSERT_GE(maxTruePeak, -0.4f);
}