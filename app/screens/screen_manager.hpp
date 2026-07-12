#pragma once

#include "lvgl.h"

namespace swirski::screens::manager
{
    enum class Screen
    {
        Home,
        Notifications,
        Music,
        Studio,
        Settings
    };

    void showScreen(Screen screen);

    void initialise(lv_display_t *display);

    lv_obj_t *createScreenRoot();

    Screen getCurrentScreen();

}