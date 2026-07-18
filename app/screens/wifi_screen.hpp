#pragma once

#include "input.hpp"

namespace swirski::screens::wifi_screen
{
    void render();
    void refreshIfNeeded();
    void handleInput(swirski::input::input_action action);
}
