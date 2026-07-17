
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
    };

    extern int revision;

    void setNotifications(std::vector<Notification> notifications);

    bool addNotification(Notification notification);

    std::vector<Notification> getNotifications();

    std::optional<Notification> getNotificationById(const std::string &notificationId);

    bool removeNotificationById(const std::string &notificationId);

    void setSnapshot(std::vector<Notification> snapshot);

    void upsert(Notification notification);

    void handleNotificationsSnapshot(JsonObjectConst payload);

    void handleNotificationReceived(JsonObjectConst payload);

}
