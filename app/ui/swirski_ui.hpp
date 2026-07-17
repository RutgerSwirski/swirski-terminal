#pragma once

#include <cstdint>

#include "lvgl.h"

namespace swirski::ui::swirski_ui
{
    namespace color
    {
        inline lv_color_t background()
        {
            return lv_color_hex(0xF6F2E8);
        }

        inline lv_color_t surface()
        {
            return lv_color_hex(0x10394A);
        }

        inline lv_color_t surfaceMuted()
        {
            return lv_color_hex(0x00283D);
        }

        inline lv_color_t text()
        {
            return lv_color_hex(0xFFFFFF);
        }

        inline lv_color_t textMuted()
        {
            return lv_color_hex(0x8FAAB7);
        }

        inline lv_color_t accent()
        {
            return lv_color_hex(0x00CC88);
        }

        inline lv_color_t accentBright()
        {
            return lv_color_hex(0x00FF00);
        }
    }

    namespace space
    {
        constexpr std::int32_t xs = 3;
        constexpr std::int32_t sm = 6;
        constexpr std::int32_t md = 8;
        constexpr std::int32_t lg = 12;
    }

    namespace radius
    {
        constexpr std::int32_t sm = 6;
    }

    enum class TextTone
    {
        Default,
        Muted,
        Accent,
        Selected
    };

    inline void stylePanel(lv_obj_t *object)
    {
        lv_obj_set_style_bg_color(
            object,
            color::surfaceMuted(),
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            object,
            0,
            LV_PART_MAIN);
    }

    inline void styleCard(lv_obj_t *object)
    {
        lv_obj_set_style_bg_color(
            object,
            color::surface(),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            object,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            object,
            1,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            object,
            color::accent(),
            LV_PART_MAIN);

        lv_obj_set_style_radius(
            object,
            radius::sm,
            LV_PART_MAIN);

        lv_obj_set_style_pad_all(
            object,
            space::md,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            object,
            LV_OBJ_FLAG_SCROLLABLE);
    }

    inline lv_obj_t *createCard(
        lv_obj_t *parent,
        std::int32_t height)
    {
        lv_obj_t *card = lv_obj_create(parent);

        lv_obj_set_width(
            card,
            LV_PCT(100));

        lv_obj_set_height(
            card,
            height);

        lv_obj_set_flex_grow(
            card,
            0);

        lv_obj_set_scrollbar_mode(
            card,
            LV_SCROLLBAR_MODE_OFF);

        styleCard(card);

        return card;
    }

    inline lv_obj_t *createLabel(
        lv_obj_t *parent,
        const char *text,
        TextTone tone,
        std::int32_t y,
        std::int32_t height)
    {
        lv_obj_t *label = lv_label_create(parent);

        lv_label_set_text(
            label,
            text);

        lv_obj_set_size(
            label,
            LV_PCT(100),
            height);

        lv_label_set_long_mode(
            label,
            LV_LABEL_LONG_DOT);

        lv_obj_align(
            label,
            LV_ALIGN_TOP_LEFT,
            0,
            y);

        lv_obj_set_style_text_color(
            label,
            tone == TextTone::Accent
                ? color::accent()
            : tone == TextTone::Selected
                ? color::accentBright()
            : tone == TextTone::Muted
                ? color::textMuted()
                : color::text(),
            LV_PART_MAIN);

        return label;
    }
}
