#include "keyboard.hpp"

#include <array>
#include <cstddef>

#include "keyboard_service.hpp"
#include "lvgl.h"
#include "screen_manager.hpp"
#include "swirski_ui.hpp"

namespace swirski::ui::keyboard
{
    namespace
    {
        constexpr std::array<std::array<const char *, 26>, 4> characterPages{{
            {{
            "A", "B", "C", "D", "E", "F",
            "G", "H", "I", "J", "K", "L",
            "M", "N", "O", "P", "Q", "R",
            "S", "T", "U", "V", "W", "X",
            "Y", "Z"}},
            {{
            "a", "b", "c", "d", "e", "f",
            "g", "h", "i", "j", "k", "l",
            "m", "n", "o", "p", "q", "r",
            "s", "t", "u", "v", "w", "x",
            "y", "z"}},
            {{
            "0", "1", "2", "3", "4", "5",
            "6", "7", "8", "9", "!", "@",
            "#", "$", "%", "^", "&", "*",
            "(", ")", "-", "_", "=", "+",
            "[", "]"}},
            {{
            "0", "1", "2", "3", "4", "5",
            "6", "7", "8", "9", "{", "}",
            "\\", "|", ";", ":", "'", "\"",
            ",", ".", "<", ">", "/", "?",
            "`", "~"}}
        }};

        constexpr std::size_t modeKeyIndex = 26;
        constexpr std::size_t spaceKeyIndex = 27;
        constexpr std::size_t deleteKeyIndex = 28;
        constexpr std::size_t doneKeyIndex = 29;
        constexpr std::size_t keyCount = 30;

        std::array<lv_obj_t *, keyCount> keyLabels{};
        lv_obj_t *textLabel = nullptr;
        std::size_t selectedKeyIndex = 0;
        std::size_t characterPageIndex = 0;
        SubmitHandler submitHandler = nullptr;
        bool textIsObscured = false;
        swirski::screens::manager::Screen returnScreen =
            swirski::screens::manager::Screen::Home;

        void updateText()
        {
            const std::string &text =
                swirski::services::keyboard_service::getText();

            const std::string visibleText =
                textIsObscured
                    ? std::string(text.size(), '*')
                    : text;

            lv_label_set_text(
                textLabel,
                text.empty()
                    ? "Type something..."
                    : visibleText.c_str());

            lv_obj_set_style_text_color(
                textLabel,
                text.empty()
                    ? swirski_ui::color::textMuted()
                    : swirski_ui::color::text(),
                LV_PART_MAIN);
        }

        void updateSelection()
        {
            for (std::size_t i = 0; i < modeKeyIndex; ++i)
            {
                lv_label_set_text(
                    keyLabels[i],
                    characterPages[characterPageIndex][i]);
            }

            lv_label_set_text(
                keyLabels[modeKeyIndex],
                characterPageIndex == 0
                    ? "abc"
                : characterPageIndex == 1
                    ? "123"
                : characterPageIndex == 2
                    ? "+=?"
                    : "ABC");
            lv_label_set_text(keyLabels[spaceKeyIndex], "Space");
            lv_label_set_text(keyLabels[deleteKeyIndex], "Del");
            lv_label_set_text(keyLabels[doneKeyIndex], "Done");

            for (std::size_t i = 0; i < keyLabels.size(); ++i)
            {
                const bool selected = i == selectedKeyIndex;

                lv_obj_set_style_bg_color(
                    keyLabels[i],
                    selected
                        ? swirski_ui::color::accent()
                        : swirski_ui::color::surface(),
                    LV_PART_MAIN);

                lv_obj_set_style_text_color(
                    keyLabels[i],
                    selected
                        ? swirski_ui::color::surface()
                        : swirski_ui::color::text(),
                    LV_PART_MAIN);
            }
        }

        void finish()
        {
            if (submitHandler != nullptr)
            {
                submitHandler(
                    swirski::services::keyboard_service::getText());
            }

            swirski::screens::manager::showScreen(returnScreen);
        }

        void activateSelectedKey()
        {
            if (selectedKeyIndex < modeKeyIndex)
            {
                swirski::services::keyboard_service::addCharacter(
                    characterPages[characterPageIndex]
                                  [selectedKeyIndex][0]);
            }
            else if (selectedKeyIndex == modeKeyIndex)
            {
                characterPageIndex =
                    (characterPageIndex + 1) % characterPages.size();
                updateSelection();
            }
            else if (selectedKeyIndex == spaceKeyIndex)
            {
                swirski::services::keyboard_service::addSpace();
            }
            else if (selectedKeyIndex == deleteKeyIndex)
            {
                swirski::services::keyboard_service::removeLastCharacter();
            }
            else
            {
                finish();
                return;
            }

            updateText();
        }
    }

    void open(
        const std::string &initialText,
        SubmitHandler onSubmit,
        bool obscureText)
    {
        returnScreen = swirski::screens::manager::getCurrentScreen();
        submitHandler = onSubmit;
        selectedKeyIndex = 0;
        characterPageIndex = 0;
        textIsObscured = obscureText;
        swirski::services::keyboard_service::begin(initialText);
        swirski::screens::manager::showScreen(
            swirski::screens::manager::Screen::Keyboard);
    }

    void render()
    {
        lv_obj_t *pageRoot =
            swirski::screens::manager::createPageRoot();

        textLabel = lv_label_create(pageRoot);
        lv_obj_set_pos(textLabel, 8, 4);
        lv_obj_set_size(textLabel, 304, 28);
        lv_label_set_long_mode(textLabel, LV_LABEL_LONG_DOT);
        lv_obj_set_style_bg_color(
            textLabel,
            swirski_ui::color::surface(),
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(textLabel, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(textLabel, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(
            textLabel,
            swirski_ui::color::ink(),
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(textLabel, 4, LV_PART_MAIN);

        constexpr int columns = 6;
        constexpr int keyWidth = 46;
        constexpr int keyHeight = 27;
        constexpr int gap = 4;

        for (std::size_t i = 0; i < keyLabels.size(); ++i)
        {
            lv_obj_t *key = lv_label_create(pageRoot);
            keyLabels[i] = key;

            lv_label_set_text(key, "");
            lv_obj_set_pos(
                key,
                8 + static_cast<int>(i % columns) * (keyWidth + gap),
                38 + static_cast<int>(i / columns) * (keyHeight + gap));
            lv_obj_set_size(key, keyWidth, keyHeight);
            lv_obj_set_style_bg_opa(key, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_width(key, 2, LV_PART_MAIN);
            lv_obj_set_style_border_color(
                key,
                swirski_ui::color::ink(),
                LV_PART_MAIN);
            lv_obj_set_style_pad_top(key, 4, LV_PART_MAIN);
            lv_obj_set_style_text_align(key, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        }

        updateText();
        updateSelection();
    }

    void handleInput(swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
            selectedKeyIndex = selectedKeyIndex == 0
                ? keyLabels.size() - 1
                : selectedKeyIndex - 1;
            updateSelection();
            break;

        case swirski::input::input_action::Next:
            selectedKeyIndex =
                (selectedKeyIndex + 1) % keyLabels.size();
            updateSelection();
            break;

        case swirski::input::input_action::Confirm:
            activateSelectedKey();
            break;

        case swirski::input::input_action::Back:
            if (swirski::services::keyboard_service::getText().empty())
            {
                swirski::screens::manager::showScreen(returnScreen);
            }
            else
            {
                swirski::services::keyboard_service::removeLastCharacter();
                updateText();
            }
            break;

        case swirski::input::input_action::Home:
            swirski::screens::manager::showScreen(
                swirski::screens::manager::Screen::Home);
            break;
        }
    }
}
