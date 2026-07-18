#pragma once

#include <cstdint>

#include "lvgl.h"
#include "fonts/swirski_fonts.hpp"

namespace swirski::ui::swirski_ui
{
    namespace color
    {
        inline lv_color_t background()
        {
            return lv_color_hex(0xF7F6EF);
        }

        inline lv_color_t surface()
        {
            return lv_color_hex(0xFFFFFF);
        }

        inline lv_color_t surfaceMuted()
        {
            return lv_color_hex(0xF7F6EF);
        }

        inline lv_color_t surfaceSoft()
        {
            return lv_color_hex(0xF6F2E8);
        }

        inline lv_color_t text()
        {
            return lv_color_hex(0x111111);
        }

        inline lv_color_t textMuted()
        {
            return lv_color_hex(0x4B5563);
        }

        inline lv_color_t accent()
        {
            return lv_color_hex(0x0057FF);
        }

        inline lv_color_t accentBright()
        {
            return lv_color_hex(0xFF3131);
        }

        inline lv_color_t accentWarm()
        {
            return lv_color_hex(0xFFD400);
        }

        inline lv_color_t ink()
        {
            return lv_color_hex(0x111111);
        }
    }

    namespace space
    {
        constexpr std::int32_t xs = 3;
        constexpr std::int32_t sm = 6;
        constexpr std::int32_t md = 8;
        constexpr std::int32_t lg = 12;
        constexpr std::int32_t xl = 16;
    }

    namespace radius
    {
        constexpr std::int32_t sm = 0;
    }

    inline void styleAppRoot(lv_obj_t *object)
    {
        lv_obj_set_style_text_font(
            object,
            &swirski_font_14,
            LV_PART_MAIN);

        lv_obj_set_style_bg_color(
            object,
            color::background(),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            object,
            LV_OPA_COVER,
            LV_PART_MAIN);
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

        lv_obj_set_style_bg_opa(
            object,
            LV_OPA_COVER,
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
            3,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            object,
            color::ink(),
            LV_PART_MAIN);

        lv_obj_set_style_radius(
            object,
            radius::sm,
            LV_PART_MAIN);

        lv_obj_set_style_pad_all(
            object,
            space::md,
            LV_PART_MAIN);

        lv_obj_set_style_shadow_width(
            object,
            2,
            LV_PART_MAIN);

        lv_obj_set_style_shadow_offset_x(
            object,
            5,
            LV_PART_MAIN);

        lv_obj_set_style_shadow_offset_y(
            object,
            5,
            LV_PART_MAIN);

        lv_obj_set_style_shadow_color(
            object,
            color::accent(),
            LV_PART_MAIN);

        lv_obj_set_style_shadow_opa(
            object,
            LV_OPA_COVER,
            LV_PART_MAIN);
    }

    inline void styleScrollbar(lv_obj_t *object)
    {
        lv_obj_set_style_width(
            object,
            0,
            LV_PART_SCROLLBAR);

        lv_obj_set_style_bg_opa(
            object,
            0,
            LV_PART_SCROLLBAR);
    }

    inline lv_obj_t *createCard(
        lv_obj_t *parent,
        std::int32_t height)
    {
        lv_obj_t *card = lv_obj_create(parent);

        lv_obj_set_width(
            card,
            LV_PCT(95));

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

        lv_obj_clear_flag(
            card,
            LV_OBJ_FLAG_SCROLLABLE);

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

        lv_obj_set_style_text_font(
            label,
            &swirski_font_14,
            LV_PART_MAIN);

        return label;
    }

    inline lv_obj_t *createBadge(
        lv_obj_t *parent,
        const char *text)
    {
        lv_obj_t *badge = lv_label_create(parent);

        lv_label_set_text(
            badge,
            text);

        lv_label_set_long_mode(
            badge,
            LV_LABEL_LONG_DOT);

        lv_obj_set_width(
            badge,
            LV_SIZE_CONTENT);

        lv_obj_set_height(
            badge,
            18);

        lv_obj_set_style_max_width(
            badge,
            LV_PCT(64),
            LV_PART_MAIN);

        lv_obj_set_style_bg_color(
            badge,
            color::accentWarm(),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            badge,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            badge,
            2,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            badge,
            color::ink(),
            LV_PART_MAIN);

        lv_obj_set_style_radius(
            badge,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_pad_left(
            badge,
            space::sm,
            LV_PART_MAIN);

        lv_obj_set_style_pad_right(
            badge,
            space::sm,
            LV_PART_MAIN);

        lv_obj_set_style_shadow_width(
            badge,
            2,
            LV_PART_MAIN);

        lv_obj_set_style_shadow_offset_x(
            badge,
            2,
            LV_PART_MAIN);

        lv_obj_set_style_shadow_offset_y(
            badge,
            2,
            LV_PART_MAIN);

        lv_obj_set_style_shadow_color(
            badge,
            color::ink(),
            LV_PART_MAIN);

        lv_obj_set_style_shadow_opa(
            badge,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_text_color(
            badge,
            color::ink(),
            LV_PART_MAIN);

        lv_obj_set_style_text_font(
            badge,
            &swirski_font_12,
            LV_PART_MAIN);

        lv_obj_align(
            badge,
            LV_ALIGN_TOP_LEFT,
            0,
            0);

        return badge;
    }

    inline lv_obj_t *createToast(
        const char *title,
        const char *body,
        std::int32_t width,
        std::int32_t height)
    {
        lv_obj_t *toast =
            lv_obj_create(lv_layer_top());

        lv_obj_set_size(
            toast,
            width,
            height);

        lv_obj_align(
            toast,
            LV_ALIGN_TOP_MID,
            0,
            space::md);

        lv_obj_add_flag(
            toast,
            LV_OBJ_FLAG_FLOATING);

        styleCard(toast);

        lv_obj_clear_flag(
            toast,
            LV_OBJ_FLAG_SCROLLABLE);

        createLabel(
            toast,
            title,
            TextTone::Accent,
            0,
            18);

        createLabel(
            toast,
            body,
            TextTone::Default,
            26,
            22);

        return toast;
    }

    inline lv_obj_t *createToast(
        const char *title,
        const char *subtitle,
        const char *body,
        std::int32_t width,
        std::int32_t height)
    {
        lv_obj_t *toast =
            createToast(
                title,
                subtitle,
                width,
                height);

        createLabel(
            toast,
            body,
            TextTone::Muted,
            48,
            18);

        return toast;
    }

    inline void styleMenuItem(
        lv_obj_t *label,
        bool selected)
    {
        lv_obj_set_width(
            label,
            LV_PCT(100));

        lv_obj_set_style_pad_top(
            label,
            space::xs,
            LV_PART_MAIN);

        lv_obj_set_style_pad_bottom(
            label,
            space::xs,
            LV_PART_MAIN);

        lv_obj_set_style_pad_left(
            label,
            space::md,
            LV_PART_MAIN);

        lv_obj_set_style_pad_right(
            label,
            space::md,
            LV_PART_MAIN);

        lv_obj_set_style_text_font(
            label,
            &swirski_font_12,
            LV_PART_MAIN);

        lv_obj_set_style_text_color(
            label,
            selected ? color::surface() : color::text(),
            LV_PART_MAIN);

        lv_obj_set_style_bg_color(
            label,
            selected ? color::accent() : color::surface(),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            label,
            selected ? LV_OPA_COVER : LV_OPA_TRANSP,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            label,
            selected ? 2 : 0,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            label,
            color::ink(),
            LV_PART_MAIN);
    }

    inline void removeShadow(lv_obj_t *object)
    {
        lv_obj_set_style_shadow_width(
            object,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_shadow_opa(
            object,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
    }
}
