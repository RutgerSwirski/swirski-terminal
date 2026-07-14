#pragma once

#include "input.hpp"

#include <string>

namespace swirski::screens::notification_screen
{

    void render(std::string notificationId);

    void handleInput(swirski::input::input_action action);

}