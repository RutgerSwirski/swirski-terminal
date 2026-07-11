#include "screen.hpp"

namespace swirski::screen
{

    namespace // variables in here only exist here
    {
        lv_display_t *displayHandle = nullptr;

        Screen currentScreen = Screen::Home;

        lv_obj_t *screenRoot = nullptr;

        lv_obj_t *currentScreenObj = nullptr;

    }

    void initialise(lv_display_t *display)
    {
        displayHandle = display;
        lv_display_set_default(displayHandle);
    }

    void clearCurrentScreen()
    {
        if (screenRoot != nullptr)
        {
            lv_obj_delete(screenRoot);
            screenRoot = nullptr;
        }
    }

    void createScreenRoot()
    {
        screenRoot = lv_obj_create(lv_screen_active());

        lv_obj_remove_style_all(screenRoot);

        lv_obj_set_size(
            screenRoot,
            LV_PCT(100),
            LV_PCT(100));

        lv_obj_set_style_bg_color(
            screenRoot,
            lv_color_hex(0x00283d),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            screenRoot,
            LV_OPA_COVER,
            LV_PART_MAIN);
    }

    void showHomeScreen()
    {
        createScreenRoot();

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

    void showScreen(Screen screen)
    {
        if (currentScreenObj != nullptr)
        {
            lv_obj_delete(currentScreenObj);
        }

        currentScreen = screen;

        switch (screen)
        {
        case Screen::Home:
            showHomeScreen();
            break;
        case Screen::Notifications:
            // showNotificationsScreen();
            break;
        case Screen::Music:
            // showMusicScreen();
            break;
        case Screen::Studio:
            // showStudioScreen();
            break;
        case Screen::Settings:
            // showSettingsScreen();
            break;
        }
    }

}
