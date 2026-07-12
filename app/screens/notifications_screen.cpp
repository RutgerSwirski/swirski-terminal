
#include "notifications_screen.hpp"

#include "screen_manager.hpp"

#include "lvgl.h"

namespace swirski::screens::notifications
{

    void render()
    {

        lv_obj_t *screenRoot = swirski::screens::manager::createScreenRoot();

        lv_obj_t *title =
            lv_label_create(screenRoot);

        lv_label_set_text(title, "Notifications");

        lv_obj_set_style_text_color(
            title,
            lv_color_white(),
            LV_PART_MAIN);

        lv_obj_align(
            title,
            LV_ALIGN_TOP_MID,
            0,
            15);
    }
}