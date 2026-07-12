
#include "home_screen.hpp"

#include "screen_manager.hpp"

namespace swirski::screens::Home
{

    void render()
    {
        // TODO
        lv_obj_t *screenRoot = swirski::screens::Manager::createScreenRoot();

        lv_obj_t *title =
            lv_label_create(screenRoot);

        lv_label_set_text(title, "SWIRSKI OS");

        lv_obj_set_style_text_color(
            title,
            lv_color_white(),
            LV_PART_MAIN);

        lv_obj_align(
            title,
            LV_ALIGN_TOP_MID,
            0,
            15);

        // create a flex container

        lv_obj_t *flexContainer = lv_obj_create(screenRoot);

        lv_obj_set_layout(flexContainer, LV_LAYOUT_FLEX);

        lv_obj_set_flex_flow(flexContainer, LV_FLEX_FLOW_COLUMN);

        lv_obj_set_size(flexContainer, 200, 150);

        lv_obj_set_style_bg_color(flexContainer, lv_color_hex(0x00283d), LV_PART_MAIN);

        lv_obj_set_flex_align(flexContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

        lv_obj_set_style_pad_row(flexContainer, 10, LV_PART_MAIN);

        lv_obj_set_style_pad_column(flexContainer, 10, LV_PART_MAIN);

        lv_obj_align(
            flexContainer,
            LV_ALIGN_CENTER,
            0,
            0);

        // create new label Notifications
        lv_obj_t *notifications =
            lv_label_create(flexContainer);

        lv_label_set_text(notifications, "> Notifications");

        lv_obj_set_style_text_color(
            notifications,
            lv_color_white(),
            LV_PART_MAIN);

        // create new label Music
        lv_obj_t *music =
            lv_label_create(flexContainer);

        lv_label_set_text(music, "Music");

        lv_obj_set_style_text_color(
            music,
            lv_color_white(),
            LV_PART_MAIN);

        // create new label Studio
        lv_obj_t *studio =
            lv_label_create(flexContainer);

        lv_label_set_text(studio, "Studio");

        lv_obj_set_style_text_color(
            studio,
            lv_color_white(),
            LV_PART_MAIN);

        // create new label Settings
        lv_obj_t *settings =
            lv_label_create(flexContainer);

        lv_label_set_text(settings, "Settings");

        lv_obj_set_style_text_color(
            settings,
            lv_color_white(),
            LV_PART_MAIN);
    }
}