
#pragma once

#include <string>
#include <vector>
#include <optional>

namespace swirski::services::notification_service
{
    struct Notification
    {
        std::string id;
        std::string appName;
        std::string title;
        std::string body;
    };

    void setNotifications(std::vector<Notification> notifications);

    bool addNotification(Notification notification);

    std::vector<Notification> getNotifications();

    std::optional<Notification> getNotificationById(const std::string &notificationId);

    bool removeNotificationById(const std::string &notificationId);

}