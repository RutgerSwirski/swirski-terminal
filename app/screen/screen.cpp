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
