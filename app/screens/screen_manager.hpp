#pragma once

#include "input.hpp"
#include "lvgl.h"

#include <string>

namespace swirski::screens::manager
{
    using DisplayPowerHandler = void (*)(bool on);

    enum class Screen
    {
        Home,
        Notifications,
        Music,
        Games,
        Pong,
        Blackjack,
        Studio,
        Settings,
        Notification
    };

    void showScreen(Screen screen);
    void showNotificationScreen(std::string notificationId);
    void handleInput(swirski::input::input_action action);

    void initialise(lv_display_t *display);

    lv_obj_t *createPageRoot();

    Screen getCurrentScreen();

    void setDisplayPowerHandler(DisplayPowerHandler handler);
    void updatePowerState();
    bool wake();

}
