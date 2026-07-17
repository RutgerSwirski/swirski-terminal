#pragma once

#include <ArduinoJson.h>

#include <cstdint>
#include <string>

namespace swirski::services::music_service
{
    struct MusicState
    {
        std::string appName;
        std::string title;
        std::string artist;
        bool isPlaying = false;
        std::uint64_t durationMs = 0;
        std::uint64_t positionMs = 0;
        std::uint64_t receivedAtMs = 0;
    };

    extern int revision;

    MusicState getState();
    void setState(MusicState state);
    void handleMusicState(JsonObjectConst payload);
}
