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

        std::string fallbackTitleForNotification(
            const swirski::services::notification_service::
                Notification &notification)
        {
            if (!notification.title.empty())
            {
                return notification.title;
            }

            return "New notification";
        }

        void showToast(
            const swirski::services::notification_service::
                Notification &notification)
        {
            clearToast();

            const std::string appName =
                notification.appName.empty()
                    ? "Notification"
                    : notification.appName;

            const std::string title =
                fallbackTitleForNotification(notification);

            const std::string body =
                notification.body;

            toastRoot =
                swirski::ui::swirski_ui::createToast(
                    appName.c_str(),
                    title.c_str(),
                    body.c_str(),
                    300,
                    88);

            toastShownAt =
                lv_tick_get();
        }

        void showSyncToast(
            int percent)
        {
            clearToast();

            const std::string progress =
                std::to_string(percent) + "%";

            toastRoot =
                swirski::ui::swirski_ui::createToast(
                    "Syncing",
                    progress.c_str(),
                    220,
                    58);

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

    bool dismiss()
    {
        if (
            toastRoot == nullptr &&
            !requestedSyncPercent &&
            !displayedSyncPercent)
        {
            return false;
        }

        requestedSyncPercent.reset();
        clearSyncRequested =
            false;

        clearToast();

        return true;
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
