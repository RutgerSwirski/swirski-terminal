#include "music_service.hpp"

#include <chrono>
#include <iostream>
#include <utility>

#include "display_text.hpp"

namespace swirski::services::music_service
{
    int revision = 0;

    namespace
    {
        std::uint64_t nowMs()
        {
            const auto now =
                std::chrono::steady_clock::now()
                    .time_since_epoch();

            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<
                    std::chrono::milliseconds>(
                    now)
                    .count());
        }

        MusicState currentState{
            "Music",
            "Nothing playing",
            "Connect your phone",
            false,
            0,
            0,
            nowMs()};
    }

    MusicState getState()
    {
        MusicState state =
            currentState;

        if (
            state.isPlaying &&
            state.durationMs > 0)
        {
            const std::uint64_t elapsedMs =
                nowMs() - state.receivedAtMs;

            state.positionMs += elapsedMs;

            if (state.positionMs > state.durationMs)
            {
                state.positionMs =
                    state.durationMs;
            }
        }

        return state;
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
            display_text::normalize(
                payload["appName"] |
                "Music");

        state.title =
            display_text::normalize(
                payload["title"] |
                "Nothing playing");

        state.artist =
            display_text::normalize(
                payload["artist"] |
                "");

        state.isPlaying =
            payload["isPlaying"] |
            false;

        state.durationMs =
            payload["durationMs"] |
            0;

        state.positionMs =
            payload["positionMs"] |
            0;

        state.receivedAtMs =
            nowMs();

        setState(
            std::move(state));

        std::cout
            << "Applied music state"
            << std::endl;
    }
}
