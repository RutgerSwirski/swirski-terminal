#include "screen_manager.hpp"

#include "home_screen.hpp"

#include "notifications_screen.hpp"

#include "status_bar.hpp"

namespace swirski::screens::manager
{

    namespace // variables in here only exist here
    {
        lv_display_t *displayHandle = nullptr;

        lv_obj_t *screenRoot = nullptr;

        Screen currentScreen = Screen::Home;
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

    lv_obj_t *createScreenRoot()
    {
        clearCurrentScreen();

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

        swirski::ui::status_bar::create(screenRoot);

        return screenRoot;
    }

    Screen getCurrentScreen()
    {
        return currentScreen;
    }

    void showScreen(Screen screen)
    {

        currentScreen = screen;

        switch (screen)
        {
        case Screen::Home:
            swirski::screens::home::render();
            break;
        case Screen::Notifications:

            swirski::screens::notifications::render();

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
