#pragma once

#include "lvgl.h"

#include <string>

namespace swirski::screens::manager
{
    enum class Screen
    {
        Home,
        Notifications,
        Music,
        Games,
        Studio,
        Settings,
        Notification
    };

    void showScreen(Screen screen);
    void showNotificationScreen(std::string notificationId);

    void initialise(lv_display_t *display);

    lv_obj_t *createPageRoot();

    Screen getCurrentScreen();

}
