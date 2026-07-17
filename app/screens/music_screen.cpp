#include "music_screen.hpp"

#include <ArduinoJson.h>
#include <cstdint>
#include <iostream>
#include <sstream>
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
        std::uint32_t lastProgressRenderAt = 0;
        int selectedControlIndex = 1;
        int nextCommandId = 1;
        swirski::transport::Transport *musicTransport = nullptr;

        std::string playingText(
            bool isPlaying)
        {
            return isPlaying
                       ? "Playing"
                       : "Paused";
        }

        std::string formatTime(
            std::uint64_t milliseconds)
        {
            const std::uint64_t totalSeconds =
                milliseconds / 1000;

            const std::uint64_t minutes =
                totalSeconds / 60;

            const std::uint64_t seconds =
                totalSeconds % 60;

            std::ostringstream output;

            output << minutes << ":";

            if (seconds < 10)
            {
                output << "0";
            }

            output << seconds;

            return output.str();
        }

        void styleControl(
            lv_obj_t *control,
            bool selected)
        {
            lv_obj_set_style_bg_color(
                control,
                selected
                    ? swirski::ui::swirski_ui::color::accent()
                    : swirski::ui::swirski_ui::color::surface(),
                LV_PART_MAIN);

            lv_obj_set_style_bg_opa(
                control,
                LV_OPA_COVER,
                LV_PART_MAIN);

            lv_obj_set_style_border_width(
                control,
                2,
                LV_PART_MAIN);

            lv_obj_set_style_border_color(
                control,
                swirski::ui::swirski_ui::color::ink(),
                LV_PART_MAIN);

            lv_obj_set_style_radius(
                control,
                0,
                LV_PART_MAIN);

            lv_obj_clear_flag(
                control,
                LV_OBJ_FLAG_SCROLLABLE);
        }

        void createControl(
            lv_obj_t *parent,
            const char *label,
            int index,
            std::int32_t x)
        {
            const bool selected =
                selectedControlIndex == index;

            lv_obj_t *control =
                lv_obj_create(parent);

            lv_obj_set_size(
                control,
                72,
                26);

            lv_obj_align(
                control,
                LV_ALIGN_TOP_LEFT,
                x,
                130);

            styleControl(
                control,
                selected);

            lv_obj_t *text =
                lv_label_create(control);

            lv_label_set_text(
                text,
                label);

            lv_obj_set_style_text_color(
                text,
                selected
                    ? lv_color_white()
                    : swirski::ui::swirski_ui::color::text(),
                LV_PART_MAIN);

            lv_obj_center(text);
        }

        void sendCommand(
            const char *action)
        {
            if (musicTransport == nullptr)
            {
                std::cout
                    << "Music command ignored: transport is not ready"
                    << std::endl;

                return;
            }

            JsonDocument document;
            const std::string commandId =
                "terminal-music-" +
                std::to_string(
                    nextCommandId++);

            document["version"] = 1;
            document["type"] = "music.command";
            document["id"] = commandId;
            document["payload"]["action"] = action;

            std::string message;
            serializeJson(document, message);

            musicTransport->send(message);

            std::cout
                << "Sent music command: "
                << action
                << std::endl;
        }
    }

    void setTransport(
        swirski::transport::Transport *transport)
    {
        musicTransport =
            transport;
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
                176);

        lv_obj_set_width(
            container,
            280);

        lv_obj_align(
            container,
            LV_ALIGN_TOP_MID,
            0,
            10);

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
            58,
            22);

        const std::uint64_t durationMs =
            music.durationMs;

        const std::uint64_t positionMs =
            music.positionMs;

        lv_obj_t *progress =
            lv_bar_create(container);

        lv_obj_set_size(
            progress,
            LV_PCT(100),
            10);

        lv_obj_align(
            progress,
            LV_ALIGN_TOP_LEFT,
            0,
            82);

        lv_bar_set_range(
            progress,
            0,
            durationMs > 0
                ? static_cast<int>(durationMs)
                : 100);

        lv_bar_set_value(
            progress,
            durationMs > 0
                ? static_cast<int>(positionMs)
                : 0,
            LV_ANIM_OFF);

        lv_obj_set_style_bg_color(
            progress,
            swirski::ui::swirski_ui::color::surfaceSoft(),
            LV_PART_MAIN);

        lv_obj_set_style_bg_color(
            progress,
            swirski::ui::swirski_ui::color::accentWarm(),
            LV_PART_INDICATOR);

        const std::string elapsedText =
            durationMs > 0
                ? formatTime(positionMs)
                : "--:--";

        const std::string remainingText =
            durationMs > 0
                ? "-" + formatTime(durationMs - positionMs)
                : "--:--";

        swirski::ui::swirski_ui::createLabel(
            container,
            elapsedText.c_str(),
            swirski::ui::swirski_ui::TextTone::Muted,
            96,
            18);

        lv_obj_t *remainingLabel =
            swirski::ui::swirski_ui::createLabel(
                container,
                remainingText.c_str(),
                swirski::ui::swirski_ui::TextTone::Muted,
                96,
                18);

        lv_obj_align(
            remainingLabel,
            LV_ALIGN_TOP_RIGHT,
            0,
            96);

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

        createControl(
            container,
            "Prev",
            0,
            0);

        createControl(
            container,
            music.isPlaying ? "Pause" : "Play",
            1,
            86);

        createControl(
            container,
            "Next",
            2,
            172);

        renderedMusicRevision =
            swirski::services::music_service::
                revision;

        lastProgressRenderAt =
            lv_tick_get();
    }

    void refreshIfNeeded()
    {
        const auto music =
            swirski::services::music_service::
                getState();

        if (
            renderedMusicRevision ==
                swirski::services::music_service::
                    revision &&
            (!music.isPlaying ||
             lv_tick_elaps(lastProgressRenderAt) < 1000))
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
            if (selectedControlIndex > 0)
            {
                selectedControlIndex -= 1;
                render();
            }
            break;

        case swirski::input::input_action::Next:
            if (selectedControlIndex < 2)
            {
                selectedControlIndex += 1;
                render();
            }
            break;

        case swirski::input::input_action::Confirm:
            switch (selectedControlIndex)
            {
            case 0:
                sendCommand("previous");
                break;
            case 1:
                sendCommand("playPause");
                break;
            case 2:
                sendCommand("next");
                break;
            }
            break;

        case swirski::input::input_action::Home:
            std::cout
                << "Music input not implemented yet"
                << std::endl;
            break;
        }
    }
}
