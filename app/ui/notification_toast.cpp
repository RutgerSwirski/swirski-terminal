#include "notification_toast.hpp"

#include <cstdint>
#include <optional>
#include <string>

#include "lvgl.h"
#include "notification_service.hpp"
#include "swirski_ui.hpp"

namespace swirski::ui::notification_toast
{
    namespace
    {
        constexpr std::uint32_t TOAST_VISIBLE_MS = 4500;
        constexpr std::uint32_t SYNC_STALE_MS = 3500;

        lv_obj_t *toastRoot = nullptr;
        std::uint32_t toastShownAt = 0;

        std::optional<int> requestedSyncPercent;
        std::optional<int> displayedSyncPercent;
        std::uint32_t syncUpdatedAt = 0;
        bool clearSyncRequested = false;

        void clearToast()
        {
            if (toastRoot == nullptr)
            {
                return;
            }

            lv_obj_delete(toastRoot);
            toastRoot = nullptr;
            displayedSyncPercent.reset();
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

            lv_obj_clear_flag(
                toastRoot,
                LV_OBJ_FLAG_SCROLLABLE);

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

        void showSyncToast(
            int percent)
        {
            clearToast();

            toastRoot =
                lv_obj_create(lv_layer_top());

            lv_obj_set_size(
                toastRoot,
                220,
                58);

            lv_obj_align(
                toastRoot,
                LV_ALIGN_TOP_MID,
                0,
                8);

            lv_obj_add_flag(
                toastRoot,
                LV_OBJ_FLAG_FLOATING);

            swirski::ui::swirski_ui::styleCard(toastRoot);

            lv_obj_clear_flag(
                toastRoot,
                LV_OBJ_FLAG_SCROLLABLE);

            swirski::ui::swirski_ui::createLabel(
                toastRoot,
                "Syncing",
                swirski::ui::swirski_ui::TextTone::Accent,
                0,
                18);

            const std::string progress =
                std::to_string(percent) + "%";

            swirski::ui::swirski_ui::createLabel(
                toastRoot,
                progress.c_str(),
                swirski::ui::swirski_ui::TextTone::Default,
                24,
                22);

            toastShownAt =
                lv_tick_get();

            displayedSyncPercent =
                percent;
        }
    }

    void requestSyncProgress(
        int percent)
    {
        if (percent < 0)
        {
            percent = 0;
        }

        if (percent > 100)
        {
            percent = 100;
        }

        requestedSyncPercent =
            percent;

        clearSyncRequested =
            false;

        syncUpdatedAt =
            lv_tick_get();
    }

    void clearSyncProgress()
    {
        requestedSyncPercent.reset();
        clearSyncRequested =
            true;
    }

    void update()
    {
        if (clearSyncRequested)
        {
            clearSyncRequested =
                false;

            if (displayedSyncPercent)
            {
                clearToast();
            }
        }

        if (
            requestedSyncPercent &&
            lv_tick_elaps(syncUpdatedAt) >= SYNC_STALE_MS)
        {
            requestedSyncPercent.reset();
            clearToast();
        }

        const auto notification =
            swirski::services::notification_service::
                takePendingToastNotification();

        if (notification)
        {
            requestedSyncPercent.reset();
            showToast(*notification);
        }
        else if (requestedSyncPercent)
        {
            if (
                !displayedSyncPercent ||
                *displayedSyncPercent !=
                    *requestedSyncPercent)
            {
                showSyncToast(*requestedSyncPercent);
            }
        }

        if (
            !requestedSyncPercent &&
            toastRoot != nullptr &&
            lv_tick_elaps(toastShownAt) >= TOAST_VISIBLE_MS)
        {
            clearToast();
        }
    }
}
