#include "notification_toast.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <mutex>

#include "lvgl.h"
#include "notification_service.hpp"
#include "screen_manager.hpp"
#include "swirski_ui.hpp"

namespace swirski::ui::notification_toast
{
    namespace
    {
        constexpr std::uint32_t TOAST_VISIBLE_MS = 4500;
        constexpr std::uint32_t SYNC_STALE_MS = 3500;

        lv_obj_t *toastRoot = nullptr;
        std::uint32_t toastShownAt = 0;
        std::uint32_t toastVisibleMs = TOAST_VISIBLE_MS;

        std::optional<int> requestedSyncPercent;
        std::optional<int> displayedSyncPercent;
        std::uint32_t syncUpdatedAt = 0;
        bool clearSyncRequested = false;

        struct PendingMessage
        {
            std::string title;
            std::string body;
            std::uint32_t durationMs;
        };

        std::optional<PendingMessage> pendingMessage;
        std::mutex pendingMessageMutex;

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
            toastVisibleMs = TOAST_VISIBLE_MS;
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
            toastVisibleMs = TOAST_VISIBLE_MS;

            displayedSyncPercent =
                percent;
        }

        void showMessageToast(
            const PendingMessage &message)
        {
            clearToast();

            toastRoot =
                swirski::ui::swirski_ui::createToast(
                    message.title.c_str(),
                    message.body.c_str(),
                    260,
                    58);

            toastShownAt = lv_tick_get();
            toastVisibleMs = message.durationMs;
        }
    }

    void requestMessage(
        const char *title,
        const char *body,
        std::uint32_t durationMs)
    {
        std::lock_guard<std::mutex> lock(
            pendingMessageMutex);

        pendingMessage = PendingMessage{
            title == nullptr ? "Swirski OS" : title,
            body == nullptr ? "" : body,
            durationMs};
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
        std::optional<PendingMessage> message;

        {
            std::lock_guard<std::mutex> lock(
                pendingMessageMutex);
            message = std::move(pendingMessage);
            pendingMessage.reset();
        }

        if (message)
        {
            requestedSyncPercent.reset();
            swirski::screens::manager::wake();
            showMessageToast(*message);
        }

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
            swirski::screens::manager::wake();
            showToast(*notification);
        }
        else if (requestedSyncPercent)
        {
            if (
                !displayedSyncPercent ||
                *displayedSyncPercent !=
                    *requestedSyncPercent)
            {
                if (!displayedSyncPercent)
                {
                    swirski::screens::manager::wake();
                }

                showSyncToast(*requestedSyncPercent);
            }
        }

        if (
            !requestedSyncPercent &&
            toastRoot != nullptr &&
            lv_tick_elaps(toastShownAt) >= toastVisibleMs)
        {
            clearToast();
        }
    }
}
