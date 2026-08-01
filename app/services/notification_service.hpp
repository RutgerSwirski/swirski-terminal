
#pragma once

#include <ArduinoJson.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace swirski::services::notification_service
{
    struct Notification
    {
        std::string id;
        std::string packageName;
        std::string appName;
        std::string title;
        std::string body;
        std::int64_t postedAt = 0;
        bool ongoing = false;
    };

    enum class UpsertResult
    {
        Inserted,
        Updated,
        Unchanged
    };

    extern int revision;

    using AlertHandler = void (*)();
    void setAlertHandler(AlertHandler handler);

    void setNotifications(std::vector<Notification> notifications);

    bool addNotification(Notification notification);

    const std::vector<Notification> &getNotifications();

    const Notification *getNotificationById(const std::string &notificationId);

    bool removeNotificationById(const std::string &notificationId);

    void setSnapshot(std::vector<Notification> snapshot);

    UpsertResult upsert(const Notification &notification);

    void handleNotificationsSnapshot(JsonObjectConst payload);

    std::optional<UpsertResult>
    handleNotificationUpserted(
        JsonObjectConst payload);

    std::optional<Notification> takePendingToastNotification();

}
