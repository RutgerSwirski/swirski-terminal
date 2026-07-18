#include "screen_manager.hpp"

#include "home_screen.hpp"

#include "notifications_screen.hpp"
#include "notification_screen.hpp"
#include "music_screen.hpp"
#include "games_screen.hpp"
#include "pong_screen.hpp"
#include "blackjack_screen.hpp"
#include "settings_screen.hpp"

#include "status_bar.hpp"
#include "swirski_ui.hpp"

#include <string>

namespace swirski::screens::manager
{

    namespace // variables in here only exist here
    {
        lv_display_t *displayHandle = nullptr;

        lv_obj_t *applicationRoot = nullptr;
        lv_obj_t *contentRoot = nullptr;
        lv_obj_t *pageRoot = nullptr;
        lv_obj_t *statusBarRoot = nullptr;

        Screen currentScreen = Screen::Home;
        std::string currentNotificationId;

        constexpr int32_t STATUS_BAR_HEIGHT = 35;

        void renderNotification()
        {
            notification_screen::render(
                currentNotificationId);
        }

        struct ScreenDefinition
        {
            Screen id;
            const char *title;
            void (*render)();
            void (*handleInput)(swirski::input::input_action);
        };

        constexpr ScreenDefinition screens[]{
            {Screen::Home, "SWIRSKI OS", home::render, home::handleInput},
            {Screen::Notifications, "NOTIFICATIONS", notifications_screen::render, notifications_screen::handleInput},
            {Screen::Notification, "NOTIFICATION", renderNotification, notification_screen::handleInput},
            {Screen::Music, "MUSIC", music_screen::render, music_screen::handleInput},
            {Screen::Games, "GAMES", games_screen::render, games_screen::handleInput},
            {Screen::Pong, "PONG", pong_screen::render, pong_screen::handleInput},
            {Screen::Blackjack, "BLACKJACK", blackjack_screen::render, blackjack_screen::handleInput},
            {Screen::Settings, "SETTINGS", settings_screen::render, settings_screen::handleInput}};

        const ScreenDefinition *findScreen(Screen screen)
        {
            for (const ScreenDefinition &definition : screens)
            {
                if (definition.id == screen)
                {
                    return &definition;
                }
            }

            return nullptr;
        }

        void createApplicationShell()
        {
            if (applicationRoot != nullptr)
            {
                return;
            }

            applicationRoot =
                lv_obj_create(lv_screen_active());

            lv_obj_remove_style_all(
                applicationRoot);

            const int32_t screenWidth =
                lv_display_get_horizontal_resolution(
                    displayHandle);

            const int32_t screenHeight =
                lv_display_get_vertical_resolution(
                    displayHandle);

            lv_obj_set_size(
                applicationRoot,
                screenWidth,
                screenHeight);

            swirski::ui::swirski_ui::styleAppRoot(
                applicationRoot);

            lv_obj_clear_flag(
                applicationRoot,
                LV_OBJ_FLAG_SCROLLABLE);

            statusBarRoot =
                lv_obj_create(applicationRoot);

            lv_obj_remove_style_all(
                statusBarRoot);

            lv_obj_set_pos(
                statusBarRoot,
                0,
                0);

            lv_obj_set_size(
                statusBarRoot,
                screenWidth,
                STATUS_BAR_HEIGHT);

            lv_obj_set_style_bg_opa(
                statusBarRoot,
                LV_OPA_TRANSP,
                LV_PART_MAIN);

            lv_obj_clear_flag(
                statusBarRoot,
                LV_OBJ_FLAG_SCROLLABLE);

            swirski::ui::status_bar::create(
                statusBarRoot);

            contentRoot =
                lv_obj_create(applicationRoot);

            lv_obj_remove_style_all(
                contentRoot);

            lv_obj_set_pos(
                contentRoot,
                0,
                STATUS_BAR_HEIGHT);

            lv_obj_set_size(
                contentRoot,
                screenWidth,
                screenHeight - STATUS_BAR_HEIGHT);

            lv_obj_set_style_bg_opa(
                contentRoot,
                LV_OPA_TRANSP,
                LV_PART_MAIN);

            lv_obj_clear_flag(
                contentRoot,
                LV_OBJ_FLAG_SCROLLABLE);
        }

        void clearCurrentPage()
        {
            if (pageRoot == nullptr)
            {
                return;
            }

            lv_obj_delete(pageRoot);
            pageRoot = nullptr;
        }

    }

    void initialise(lv_display_t *display)
    {
        displayHandle = display;
        lv_display_set_default(displayHandle);

        createApplicationShell();
    }

    lv_obj_t *createPageRoot()
    {
        createApplicationShell();
        clearCurrentPage();

        pageRoot =
            lv_obj_create(contentRoot);

        lv_obj_remove_style_all(
            pageRoot);

        lv_obj_set_pos(
            pageRoot,
            0,
            0);

        lv_obj_set_size(
            pageRoot,
            LV_PCT(100),
            LV_PCT(100));

        lv_obj_set_style_bg_opa(
            pageRoot,
            LV_OPA_TRANSP,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            pageRoot,
            LV_OBJ_FLAG_SCROLLABLE);

        return pageRoot;
    }

    Screen getCurrentScreen()
    {
        return currentScreen;
    }

    void showNotificationScreen(
        std::string notificationId)
    {
        currentNotificationId = notificationId;
        showScreen(Screen::Notification);
    }

    void showScreen(Screen screen)
    {
        const ScreenDefinition *definition =
            findScreen(screen);

        if (definition == nullptr)
        {
            return;
        }

        currentScreen = screen;
        swirski::ui::status_bar::setTitle(
            definition->title);
        definition->render();
    }

    void handleInput(
        swirski::input::input_action action)
    {
        const ScreenDefinition *definition =
            findScreen(currentScreen);

        if (definition != nullptr)
        {
            definition->handleInput(action);
        }
    }
}
