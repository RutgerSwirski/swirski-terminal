#pragma once

#include <ArduinoJson.h>

#include <string>

namespace swirski::services::music_service
{
    struct MusicState
    {
        std::string appName;
        std::string title;
        std::string artist;
        bool isPlaying = false;
    };

    extern int revision;

    MusicState getState();
    void setState(MusicState state);
    void handleMusicState(JsonObjectConst payload);
}
