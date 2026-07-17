#include "music_service.hpp"

#include <iostream>
#include <utility>

namespace swirski::services::music_service
{
    int revision = 0;

    namespace
    {
        MusicState currentState{
            "Music",
            "Nothing playing",
            "Connect your phone",
            false};
    }

    MusicState getState()
    {
        return currentState;
    }

    void setState(
        MusicState state)
    {
        currentState =
            std::move(state);

        revision += 1;
    }

    void handleMusicState(
        JsonObjectConst payload)
    {
        MusicState state;

        state.appName =
            payload["appName"] |
            "Music";

        state.title =
            payload["title"] |
            "Nothing playing";

        state.artist =
            payload["artist"] |
            "";

        state.isPlaying =
            payload["isPlaying"] |
            false;

        setState(
            std::move(state));

        std::cout
            << "Applied music state"
            << std::endl;
    }
}
