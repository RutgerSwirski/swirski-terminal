
#pragma once

#include <string>
#include <vector>

namespace swirski::services::notifications_service
{
    struct Notification
    {
        std::string id;
        std::string appName;
        std::string title;
        std::string body;
    };

    void setNotifications(std::vector<Notification> notifications);

    void addNotification(Notification notification);

    std::vector<Notification> getNotifications();

    Notification getNotificationById(std::string id);

}