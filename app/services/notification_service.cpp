
#include <vector>
#include "notification_service.hpp"
#include <string>

#include <optional>

namespace swirski::services::notification_service
{

    namespace
    {

        std::vector<Notification> notifications = {

            {"1",
             "WhatsApp",
             "Anna",
             "Are you still coming tonight?"},
            {"2",
             "Spotify",
             "Now playing",
             "Once in a Lifetime — Talking Heads"},
            {"3",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes"},
            {"4",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes"},
            {"5",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes"},
            {"6",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes"},
            {"7",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes"},
            {"8",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes"}};
    }

    void setNotifications(std::vector<Notification> incomingNotifications)
    {
        notifications = incomingNotifications;
    }

    void addNotification(Notification notification)
    {
        notifications.push_back(notification);
    }

    std::vector<Notification> getNotifications()
    {
        return notifications;
    }

    std::optional<Notification> getNotificationById(const std::string &id)
    {
        for (auto notification : notifications)
        {
            if (notification.id == id)
            {
                return notification;
            }
        }
        return {};
    }

}