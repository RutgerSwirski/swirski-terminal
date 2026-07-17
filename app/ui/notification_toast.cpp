#include "notification_toast.hpp"

#include <cstdint>
#include <string>

#include "lvgl.h"
#include "notification_service.hpp"

namespace swirski::ui::notification_toast
{
    namespace
    {
        constexpr std::uint32_t TOAST_VISIBLE_MS = 3000;

        lv_obj_t *toastRoot = nullptr;
        std::uint32_t toastShownAt = 0;

        void clearToast()
        {
            if (toastRoot == nullptr)
            {
                return;
            }

            lv_obj_delete(toastRoot);
            toastRoot = nullptr;
        }

        std::string titleForNotification(
            const swirski::services::notification_service::
                Notification &notification)
        {
            if (!notification.title.empty())
            {
                return notification.title;
            }

            if (!notification.body.empty())
            {
                return notification.body;
            }

            return "New notification";
        }

        void showToast(
            const swirski::services::notification_service::
                Notification &notification)
        {
            clearToast();

            toastRoot =
                lv_obj_create(lv_layer_top());

            lv_obj_set_size(
                toastRoot,
                300,
                72);

            lv_obj_align(
                toastRoot,
                LV_ALIGN_TOP_MID,
                0,
                8);

            lv_obj_add_flag(
                toastRoot,
                LV_OBJ_FLAG_FLOATING);

            lv_obj_clear_flag(
                toastRoot,
                LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_set_style_bg_color(
                toastRoot,
                lv_color_hex(0x10394A),
                LV_PART_MAIN);

            lv_obj_set_style_bg_opa(
                toastRoot,
                LV_OPA_COVER,
                LV_PART_MAIN);

            lv_obj_set_style_border_width(
                toastRoot,
                1,
                LV_PART_MAIN);

            lv_obj_set_style_border_color(
                toastRoot,
                lv_color_hex(0x00CC88),
                LV_PART_MAIN);

            lv_obj_set_style_radius(
                toastRoot,
                6,
                LV_PART_MAIN);

            lv_obj_set_style_pad_all(
                toastRoot,
                8,
                LV_PART_MAIN);

            lv_obj_t *appNameLabel =
                lv_label_create(toastRoot);

            const std::string appName =
                notification.appName.empty()
                    ? "Notification"
                    : notification.appName;

            lv_label_set_text(
                appNameLabel,
                appName.c_str());

            lv_obj_set_width(
                appNameLabel,
                LV_PCT(100));

            lv_label_set_long_mode(
                appNameLabel,
                LV_LABEL_LONG_DOT);

            lv_obj_set_style_text_color(
                appNameLabel,
                lv_color_hex(0x00CC88),
                LV_PART_MAIN);

            lv_obj_align(
                appNameLabel,
                LV_ALIGN_TOP_LEFT,
                0,
                0);

            lv_obj_t *titleLabel =
                lv_label_create(toastRoot);

            const std::string title =
                titleForNotification(notification);

            lv_label_set_text(
                titleLabel,
                title.c_str());

            lv_obj_set_width(
                titleLabel,
                LV_PCT(100));

            lv_label_set_long_mode(
                titleLabel,
                LV_LABEL_LONG_DOT);

            lv_obj_set_style_text_color(
                titleLabel,
                lv_color_white(),
                LV_PART_MAIN);

            lv_obj_align(
                titleLabel,
                LV_ALIGN_TOP_LEFT,
                0,
                26);

            toastShownAt =
                lv_tick_get();
        }
    }

    void update()
    {
        const auto notification =
            swirski::services::notification_service::
                takePendingToastNotification();

        if (notification)
        {
            showToast(*notification);
        }

        if (
            toastRoot != nullptr &&
            lv_tick_elaps(toastShownAt) >= TOAST_VISIBLE_MS)
        {
            clearToast();
        }
    }
}
