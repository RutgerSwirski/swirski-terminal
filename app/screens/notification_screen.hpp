#pragma once

#include "input.hpp"

#include <string>

namespace swirski::screens::notification_screen
{

    void render(const std::string &notificationId);

    void refreshIfNeeded();

    void handleInput(swirski::input::input_action action);

}
