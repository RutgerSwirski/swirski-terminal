#pragma once

#include "input.hpp"

namespace swirski::screens::music_screen
{
    void render();
    void refreshIfNeeded();
    void handleInput(swirski::input::input_action action);
}
