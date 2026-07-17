#pragma once

#include "input.hpp"

namespace swirski::screens::pong_screen
{
    void render();

    void handleInput(swirski::input::input_action action);
}
