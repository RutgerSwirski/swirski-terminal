#include "protocol.hpp"
#include "message.hpp"

#include <iostream>
#include <optional>
#include <string>

#include <ArduinoJson.h>

#include "notification_service.hpp"

namespace
{
    swirski::protocol::MessageType parseMessageType(
        const std::string &rawType)
    {
        if (rawType == "ping")
        {
            return swirski::protocol::MessageType::Ping;
        }

        if (rawType == "pong")
        {
            return swirski::protocol::MessageType::Pong;
        }

        if (rawType == "notification.received")
        {
            return swirski::protocol::MessageType::NotificationReceived;
        }

        return swirski::protocol::MessageType::Unknown;
    }

}

namespace swirski::protocol
{
    std::string createPongMessage(
        const std::string &messageId)
    {
        JsonDocument document;

        document["version"] = 1;
        document["type"] = "pong";
        document["id"] = messageId;

        std::string output;
        serializeJson(document, output);

        return output;
    }

    std::optional<Message> parseMessage(
        const std::string &rawMessage)
    {
        JsonDocument document;

        const DeserializationError error =
            deserializeJson(document, rawMessage);

        if (error)
        {
            std::cerr
                << "JSON parsing failed: "
                << error.c_str()
                << std::endl;

            return std::nullopt;
        }

        if (!document.is<JsonObject>())
        {
            std::cerr
                << "Protocol message must be a JSON object"
                << std::endl;

            return std::nullopt;
        }

        if (!document["version"].is<int>())
        {
            std::cerr
                << "Protocol message has no valid version"
                << std::endl;

            return std::nullopt;
        }

        if (!document["type"].is<const char *>())
        {
            std::cerr
                << "Protocol message has no valid type"
                << std::endl;

            return std::nullopt;
        }

        if (!document["id"].is<const char *>())
        {
            std::cerr
                << "Protocol message has no valid ID"
                << std::endl;

            return std::nullopt;
        }

        Message message;

        message.version =
            document["version"].as<int>();

        if (message.version != 1)
        {
            std::cerr
                << "Unsupported protocol version: "
                << message.version
                << std::endl;

            return std::nullopt;
        }

        const std::string rawType =
            document["type"].as<std::string>();

        message.type =
            parseMessageType(rawType);

        message.id =
            document["id"].as<std::string>();

        if (message.type == MessageType::NotificationReceived)
        {
            if (!document["payload"].is<JsonObject>())
            {
                std::cerr
                    << "Notification message has no valid payload"
                    << std::endl;

                return std::nullopt;
            }

            const JsonObject payload =
                document["payload"].as<JsonObject>();

            if (!payload["notificationId"].is<const char *>() ||
                !payload["appName"].is<const char *>() ||
                !payload["title"].is<const char *>() ||
                !payload["body"].is<const char *>())
            {
                std::cerr
                    << "Notification payload is invalid"
                    << std::endl;

                return std::nullopt;
            }

            const char *notificationId =
                payload["notificationId"].as<const char *>();

            const char *appName =
                payload["appName"].as<const char *>();

            const char *title =
                payload["title"].as<const char *>();

            const char *body =
                payload["body"].as<const char *>();

            if (notificationId == nullptr)
            {
                std::cerr << "notificationId is null" << std::endl;
                return std::nullopt;
            }

            if (appName == nullptr)
            {
                std::cerr << "appName is null" << std::endl;
                return std::nullopt;
            }

            if (title == nullptr)
            {
                std::cerr << "title is null" << std::endl;
                return std::nullopt;
            }

            if (body == nullptr)
            {
                std::cerr << "body is null" << std::endl;
                return std::nullopt;
            }

            message.payload.notificationId = notificationId;
            message.payload.appName = appName;
            message.payload.title = title;
            message.payload.body = body;
        }

        return message;
    }

    std::optional<std::string> handleIncomingMessage(
        const std::string &rawMessage)
    {

        std::cout << "1. Message parsed" << std::endl;

        const auto message =
            parseMessage(rawMessage);

        if (!message)
        {
            return std::nullopt;
        }

        switch (message->type)
        {
        case MessageType::Ping:
            return createPongMessage(message->id);

        case MessageType::Pong:
            std::cout
                << "Received pong"
                << std::endl;

            return std::nullopt;

        case MessageType::NotificationReceived:
        {

            std::cout << "2. Creating notification" << std::endl;

            swirski::services::notification_service::Notification newNotification{
                .id = message->payload.notificationId,
                .appName = message->payload.appName,
                .title = message->payload.title,
                .body = message->payload.body};

            std::cout << "3. Notification created" << std::endl;

            swirski::services::notification_service::addNotification(
                newNotification);

            std::cout << "4. Notification added" << std::endl;

            return std::nullopt;
        }

        case MessageType::Unknown:
            std::cerr
                << "Unknown protocol message type"
                << std::endl;

            return std::nullopt;
        }

        return std::nullopt;
    }
}