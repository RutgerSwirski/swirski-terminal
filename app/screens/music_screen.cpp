#include "music_screen.hpp"

#include <iostream>
#include <string>

#include "lvgl.h"

#include "music_service.hpp"
#include "screen_manager.hpp"
#include "swirski_ui.hpp"

namespace swirski::screens::music_screen
{
    namespace
    {
        int renderedMusicRevision = -1;

        std::string playingText(
            bool isPlaying)
        {
            return isPlaying
                       ? "Playing"
                       : "Paused";
        }
    }

    void render()
    {
        const auto music =
            swirski::services::music_service::
                getState();

        lv_obj_t *pageRoot =
            swirski::screens::manager::
                createPageRoot();

        lv_obj_t *container =
            swirski::ui::swirski_ui::createCard(
                pageRoot,
                170);

        lv_obj_set_width(
            container,
            280);

        lv_obj_align(
            container,
            LV_ALIGN_TOP_MID,
            0,
            18);

        swirski::ui::swirski_ui::createBadge(
            container,
            music.appName.empty()
                ? "Music"
                : music.appName.c_str());

        const std::string title =
            music.title.empty()
                ? "Nothing playing"
                : music.title;

        swirski::ui::swirski_ui::createLabel(
            container,
            title.c_str(),
            swirski::ui::swirski_ui::TextTone::Default,
            34,
            26);

        const std::string artist =
            music.artist.empty()
                ? "Unknown artist"
                : music.artist;

        swirski::ui::swirski_ui::createLabel(
            container,
            artist.c_str(),
            swirski::ui::swirski_ui::TextTone::Muted,
            66,
            22);

        lv_obj_t *separator =
            lv_obj_create(container);

        lv_obj_set_size(
            separator,
            LV_PCT(100),
            2);

        lv_obj_align(
            separator,
            LV_ALIGN_TOP_LEFT,
            0,
            98);

        lv_obj_set_style_bg_color(
            separator,
            swirski::ui::swirski_ui::color::accentWarm(),
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            separator,
            0,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            separator,
            LV_OBJ_FLAG_SCROLLABLE);

        const std::string status =
            playingText(
                music.isPlaying);

        swirski::ui::swirski_ui::createLabel(
            container,
            status.c_str(),
            music.isPlaying
                ? swirski::ui::swirski_ui::TextTone::Accent
                : swirski::ui::swirski_ui::TextTone::Muted,
            112,
            22);

        renderedMusicRevision =
            swirski::services::music_service::
                revision;
    }

    void refreshIfNeeded()
    {
        if (
            renderedMusicRevision ==
            swirski::services::music_service::
                revision)
        {
            return;
        }

        render();
    }

    void handleInput(
        swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Back:
            swirski::screens::manager::showScreen(
                swirski::screens::manager::Screen::Home);
            break;

        case swirski::input::input_action::Previous:
        case swirski::input::input_action::Next:
        case swirski::input::input_action::Confirm:
        case swirski::input::input_action::Home:
            std::cout
                << "Music input not implemented yet"
                << std::endl;
            break;
        }
    }
}
