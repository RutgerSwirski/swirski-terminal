
#pragma once

#include "input.hpp"
namespace swirski::screens::notifications_screen
{
    void render();

    void refreshIfNeeded();

    void handleInput(swirski::input::input_action action);
}
