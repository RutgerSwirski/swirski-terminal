#include "notification_toast.hpp"

#include <cstdint>
#include <string>

#include "lvgl.h"
#include "notification_service.hpp"
#include "swirski_ui.hpp"

namespace swirski::ui::notification_toast
{
    namespace
    {
        constexpr std::uint32_t TOAST_VISIBLE_MS = 4500;

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

            swirski::ui::swirski_ui::styleCard(toastRoot);

            const std::string appName =
                notification.appName.empty()
                    ? "Notification"
                    : notification.appName;

            swirski::ui::swirski_ui::createLabel(
                toastRoot,
                appName.c_str(),
                swirski::ui::swirski_ui::TextTone::Accent,
                0,
                18);

            const std::string title =
                titleForNotification(notification);

            swirski::ui::swirski_ui::createLabel(
                toastRoot,
                title.c_str(),
                swirski::ui::swirski_ui::TextTone::Default,
                26,
                22);

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
