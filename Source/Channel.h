#pragma once

namespace LUFS 
{

enum class StandardChannel
{
    Left,
    Right,
    Centre,
    LeftSurround,
    RightSurround
};

struct Channel
{
    float azimuth;
    float elevation;

    float getWeighting()
    {
        if(elevation < 30.0f && azimuth >= 60.0f && azimuth <= 120.0f)
        {
            return 1.41f;
        }

        return 1.0f
    }

    static Channel createStandard(StandardChannel channelType)
    {
        Channel createdChannel{0.0f, 0.0f};

        switch(channelType)
        {
            case StandardChannel::Left:
            case StandardChannel::Right:
                createdChannel.azimuth = 30.0f;

            case StandardChannel::LeftSurround:
            case StandardChannel::RightSurround:
                createdChannel.azimuth = 110.0f;
        }

        return createdChannel;
    }
};

}