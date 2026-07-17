#include "protocol.hpp"
#include "message.hpp"

#include <iostream>
#include <optional>
#include <string>

#include <ArduinoJson.h>

#include "date_time.hpp"
#include "music_service.hpp"
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

        if (rawType == "time.sync")
        {
            return swirski::protocol::MessageType::TimeSync;
        }

        if (rawType == "notification.received")
        {
            return swirski::protocol::MessageType::NotificationReceived;
        }

        if (rawType == "notifications.snapshot")
        {
            return swirski::protocol::MessageType::NotificationsSnapshot;
        }

        if (rawType == "music.state")
        {
            return swirski::protocol::MessageType::MusicState;
        }

        if (rawType == "disconnect.requested")
        {
            return swirski::protocol::MessageType::DisconnectRequested;
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

        return message;
    }

    std::optional<std::string> handleIncomingMessage(
        const std::string &rawMessage)
    {

        std::cout << "1. Message parsed" << std::endl;

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

        case MessageType::TimeSync:
        {
            JsonObjectConst payload =
                document["payload"].as<JsonObjectConst>();

            if (!payload["unixTimeSeconds"].is<long>())
            {
                std::cerr
                    << "Invalid time.sync payload"
                    << std::endl;

                return std::nullopt;
            }

            const long unixTimeSeconds =
                payload["unixTimeSeconds"].as<long>();

            const long timezoneOffsetMinutes =
                payload["timezoneOffsetMinutes"] | 0;

            swirski::service::date_time::setFromTimestamp(
                unixTimeSeconds +
                timezoneOffsetMinutes * 60);

            std::cout
                << "Applied date/time sync"
                << std::endl;

            return std::nullopt;
        }

        case MessageType::NotificationReceived:
        {
            swirski::services::notification_service::
                handleNotificationReceived(
                    document["payload"].as<JsonObjectConst>());

            return std::nullopt;
        }

        case MessageType::NotificationsSnapshot:
            swirski::services::notification_service::
                handleNotificationsSnapshot(
                    document["payload"].as<JsonObjectConst>());

            return std::nullopt;

        case MessageType::MusicState:
            swirski::services::music_service::
                handleMusicState(
                    document["payload"].as<JsonObjectConst>());

            return std::nullopt;

        case MessageType::DisconnectRequested:
            return std::nullopt;

        case MessageType::Unknown:
            std::cerr
                << "Unknown protocol message type"
                << std::endl;

            return std::nullopt;
        }

        return std::nullopt;
    }
}
