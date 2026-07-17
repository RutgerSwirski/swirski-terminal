#pragma once

#include "lvgl.h"

namespace swirski::ui::status_bar
{
    void create(lv_obj_t *parent);

    void setTitle(const char *title);

    void updateClock();

    void updateSystemState();
}
