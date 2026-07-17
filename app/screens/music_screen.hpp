#pragma once

#include "input.hpp"
#include "transport.hpp"

namespace swirski::screens::music_screen
{
    void setTransport(swirski::transport::Transport *transport);
    void render();
    void refreshIfNeeded();
    void handleInput(swirski::input::input_action action);
}
