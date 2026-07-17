#include "notification_service.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <ArduinoJson.h>

namespace swirski::services::notification_service
{
    int revision = 0;

    namespace
    {

        std::vector<Notification> notifications = {

            {"1",
             "com.whatsapp",
             "WhatsApp",
             "Anna",
             "Are you still coming tonight?",
             0},
            {"2",
             "com.spotify.music",
             "Spotify",
             "Now playing",
             "Once in a Lifetime — Talking Heads",
             0},
            {"3",
             "com.google.android.calendar",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes",
             0},
            {"4",
             "com.google.android.calendar",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes",
             0},
            {"5",
             "com.google.android.calendar",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes",
             0},
            {"6",
             "com.google.android.calendar",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes",
             0},
            {"7",
             "com.google.android.calendar",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes",
             0},
            {"8",
             "com.google.android.calendar",
             "Calendar",
             "Design meeting",
             "Starts in 10 minutes",
             0}};

        std::optional<Notification> pendingToastNotification;

        bool looksLikePackageName(
            const std::string &value)
        {
            return value.find('.') != std::string::npos;
        }

        bool isGenericAndroidLabel(
            const std::string &appName,
            const std::string &packageName)
        {
            return packageName != "android" &&
                   appName == "Android";
        }

        bool isGenericPackageSegment(
            const std::string &segment)
        {
            return segment == "android";
        }

        std::string readableNameFromPackageName(
            const std::string &packageName)
        {
            const std::size_t lastDot =
                packageName.find_last_of('.');

            std::string name =
                lastDot == std::string::npos
                    ? packageName
                    : packageName.substr(lastDot + 1);

            if (isGenericPackageSegment(name))
            {
                const std::size_t previousDot =
                    packageName.find_last_of(
                        '.',
                        lastDot == std::string::npos
                            ? std::string::npos
                            : lastDot - 1);

                if (
                    lastDot != std::string::npos &&
                    previousDot != std::string::npos)
                {
                    name =
                        packageName.substr(
                            previousDot + 1,
                            lastDot - previousDot - 1);
                }
            }

            for (char &character : name)
            {
                if (
                    character == '_' ||
                    character == '-')
                {
                    character = ' ';
                }
            }

            bool capitalizeNext = true;

            for (char &character : name)
            {
                if (character == ' ')
                {
                    capitalizeNext = true;
                    continue;
                }

                if (capitalizeNext)
                {
                    character =
                        static_cast<char>(
                            std::toupper(
                                static_cast<unsigned char>(
                                    character)));

                    capitalizeNext = false;
                }
            }

            return name.empty()
                       ? packageName
                       : name;
        }
    }

    void setNotifications(std::vector<Notification> incomingNotifications)
    {
        notifications = incomingNotifications;
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

    bool addNotification(Notification notification)
    {
        notifications.push_back(notification);
        return true;
    }

    bool removeNotificationById(const std::string &id)
    {
        for (auto it = notifications.begin(); it != notifications.end(); ++it)
        {
            if (it->id == id)
            {
                notifications.erase(it);
                return true;
            }
        }
        return false;
    }

    void setSnapshot(std::vector<Notification> snapshot)
    {
        notifications = std::move(snapshot);

        revision += 1;
    }

    void upsert(
        Notification notification)
    {
        const auto existing =
            std::find_if(
                notifications.begin(),
                notifications.end(),
                [&notification](
                    const Notification &current)
                {
                    return current.id ==
                           notification.id;
                });

        if (existing != notifications.end())
        {
            *existing =
                std::move(notification);
        }
        else
        {
            notifications.insert(
                notifications.begin(),
                std::move(notification));
        }

        revision += 1;
    }

    std::optional<Notification>
    parseNotification(
        JsonObjectConst object)
    {
        const char *id =
            object["id"];

        if (id == nullptr)
        {
            return std::nullopt;
        }

        Notification notification;

        notification.id =
            id;

        notification.packageName =
            object["packageName"] |
            "";

        notification.appName =
            object["appName"] |
            "";

        if (
            notification.appName.empty() ||
            looksLikePackageName(notification.appName) ||
            isGenericAndroidLabel(
                notification.appName,
                notification.packageName))
        {
            notification.appName =
                readableNameFromPackageName(
                    notification.packageName.empty()
                        ? notification.appName
                        : notification.packageName);
        }

        notification.title =
            object["title"] |
            "";

        notification.body =
            object["body"] |
            "";

        notification.postedAt =
            object["postedAt"] |
            0;

        return notification;
    }

    void handleNotificationsSnapshot(
        JsonObjectConst payload)
    {
        JsonArrayConst notificationObjects =
            payload["notifications"]
                .as<JsonArrayConst>();

        std::vector<Notification> snapshot;

        snapshot.reserve(
            notificationObjects.size());

        for (
            JsonObjectConst object :
            notificationObjects)
        {
            const auto notification =
                parseNotification(object);

            if (notification)
            {
                snapshot.push_back(
                    *notification);
            }
        }

        setSnapshot(
            std::move(snapshot));

        std::cout
            << "Applied notification snapshot"
            << std::endl;
    }

    void handleNotificationReceived(
        JsonObjectConst payload)
    {
        JsonObjectConst object =
            payload["notification"]
                .as<JsonObjectConst>();

        const auto notification =
            parseNotification(object);

        if (!notification)
        {
            std::cerr
                << "Invalid notification.received payload"
                << std::endl;

            return;
        }

        upsert(*notification);

        pendingToastNotification =
            *notification;

        std::cout
            << "Applied incoming notification: "
            << notification->id
            << std::endl;
    }

    std::optional<Notification> takePendingToastNotification()
    {
        auto notification =
            std::move(pendingToastNotification);

        pendingToastNotification.reset();

        return notification;
    }

}
