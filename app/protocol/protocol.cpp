#include "protocol.hpp"
#include "message.hpp"

#include <iostream>
#include <optional>
#include <string>

#include <ArduinoJson.h>

#include "date_time.hpp"
#include "music_service.hpp"
#include "notification_service.hpp"
#include "wifi_service.hpp"

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

        if (rawType == "notification.removed")
        {
            return swirski::protocol::MessageType::NotificationRemoved;
        }

        if (rawType == "notifications.snapshot")
        {
            return swirski::protocol::MessageType::NotificationsSnapshot;
        }

        if (rawType == "music.state")
        {
            return swirski::protocol::MessageType::MusicState;
        }

        if (rawType == "wifi.scan.request")
        {
            return swirski::protocol::MessageType::WifiScanRequest;
        }

        if (rawType == "wifi.networks")
        {
            return swirski::protocol::MessageType::WifiNetworks;
        }

        if (rawType == "wifi.configure")
        {
            return swirski::protocol::MessageType::WifiConfigure;
        }

        if (rawType == "wifi.disconnect")
        {
            return swirski::protocol::MessageType::WifiDisconnect;
        }

        if (rawType == "wifi.status")
        {
            return swirski::protocol::MessageType::WifiStatus;
        }

        if (rawType == "wifi.internet.test")
        {
            return swirski::protocol::MessageType::WifiInternetTest;
        }

        if (rawType == "disconnect.requested")
        {
            return swirski::protocol::MessageType::DisconnectRequested;
        }

        return swirski::protocol::MessageType::Unknown;
    }

    std::optional<swirski::protocol::Message> parseMessage(
        JsonObjectConst document)
    {
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

        swirski::protocol::Message message;

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

        message.type =
            parseMessageType(
                document["type"].as<std::string>());

        message.id =
            document["id"].as<std::string>();

        return message;
    }
}

namespace swirski::protocol
{
    namespace
    {
        const char *wifiStateName(
            services::wifi_service::ConnectionState state)
        {
            using State = services::wifi_service::ConnectionState;

            switch (state)
            {
            case State::Unavailable:
                return "unavailable";
            case State::Disconnected:
                return "disconnected";
            case State::Connecting:
                return "connecting";
            case State::Connected:
                return "connected";
            case State::Failed:
                return "failed";
            }

            return "unavailable";
        }

        const char *internetTestStateName(
            services::wifi_service::InternetTestState state)
        {
            using State =
                services::wifi_service::InternetTestState;

            switch (state)
            {
            case State::Idle:
                return "idle";
            case State::Testing:
                return "testing";
            case State::Success:
                return "success";
            case State::Failed:
                return "failed";
            }

            return "idle";
        }

        std::string serializeDocument(JsonDocument &document)
        {
            std::string output;
            serializeJson(document, output);
            return output;
        }
    }

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

    std::string createWifiNetworksMessage()
    {
        JsonDocument document;
        document["version"] = 1;
        document["type"] = "wifi.networks";
        document["id"] =
            "terminal-wifi-networks-" +
            std::to_string(
                services::wifi_service::getRevision());

        JsonArray networkArray =
            document["payload"]["networks"].to<JsonArray>();

        for (const auto &network :
             services::wifi_service::getNetworks())
        {
            JsonObject item = networkArray.add<JsonObject>();
            item["ssid"] = network.ssid;
            item["signalStrength"] = network.signalStrength;
            item["secured"] = network.secured;
        }

        return serializeDocument(document);
    }

    std::string createWifiStatusMessage()
    {
        JsonDocument document;
        document["version"] = 1;
        document["type"] = "wifi.status";
        document["id"] =
            "terminal-wifi-status-" +
            std::to_string(
                services::wifi_service::getRevision());

        JsonObject payload =
            document["payload"].to<JsonObject>();
        payload["state"] = wifiStateName(
            services::wifi_service::getConnectionState());
        payload["ssid"] =
            services::wifi_service::getConnectedSsid();
        payload["scanning"] =
            services::wifi_service::isScanning();
        payload["signalStrength"] =
            services::wifi_service::getSignalStrength();
        payload["internetTest"] = internetTestStateName(
            services::wifi_service::getInternetTestState());
        payload["internetLatencyMs"] =
            services::wifi_service::getInternetLatencyMs();

        return serializeDocument(document);
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

        return ::parseMessage(
            document.as<JsonObjectConst>());
    }

    MessageResult handleIncomingMessage(
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

            return {};
        }

        const auto message =
            ::parseMessage(
                document.as<JsonObjectConst>());

        if (!message)
        {
            return {};
        }

        std::cout << "Message parsed" << std::endl;

        switch (message->type)
        {
        case MessageType::Ping:
            return {
                createPongMessage(message->id),
                false,
                true};

        case MessageType::Pong:
            std::cout
                << "Received pong"
                << std::endl;

            return {};

        case MessageType::TimeSync:
        {
            JsonObjectConst payload =
                document["payload"].as<JsonObjectConst>();

            if (!payload["unixTimeSeconds"].is<long>())
            {
                std::cerr
                    << "Invalid time.sync payload"
                    << std::endl;

                return {};
            }

            const long unixTimeSeconds =
                payload["unixTimeSeconds"].as<long>();

            const long timezoneOffsetMinutes =
                payload["timezoneOffsetMinutes"] | 0;

            swirski::service::date_time::setFromPhoneTime(
                unixTimeSeconds,
                timezoneOffsetMinutes);

            swirski::service::date_time::save();

            std::cout
                << "Applied date/time sync"
                << std::endl;

            return {};
        }

        case MessageType::NotificationReceived:
        {
            swirski::services::notification_service::
                handleNotificationReceived(
                    document["payload"].as<JsonObjectConst>());

            return {};
        }

        case MessageType::NotificationRemoved:
        {
            const char *notificationId =
                document["payload"]["id"];

            if (notificationId != nullptr)
            {
                swirski::services::notification_service::
                    removeNotificationById(notificationId);
            }

            return {};
        }

        case MessageType::NotificationsSnapshot:
            swirski::services::notification_service::
                handleNotificationsSnapshot(
                    document["payload"].as<JsonObjectConst>());

            return {};

        case MessageType::MusicState:
            swirski::services::music_service::
                handleMusicState(
                    document["payload"].as<JsonObjectConst>());

            return {};

        case MessageType::WifiScanRequest:
            services::wifi_service::scan();
            return {createWifiStatusMessage(), false};

        case MessageType::WifiConfigure:
        {
            JsonObjectConst payload =
                document["payload"].as<JsonObjectConst>();

            if (
                !payload["ssid"].is<const char *>() ||
                !payload["password"].is<const char *>())
            {
                std::cerr
                    << "Invalid wifi.configure payload"
                    << std::endl;
                return {};
            }

            const std::string ssid =
                payload["ssid"].as<std::string>();
            const std::string password =
                payload["password"].as<std::string>();

            if (
                ssid.empty() ||
                ssid.size() > 32 ||
                password.size() > 63)
            {
                std::cerr
                    << "Invalid Wi-Fi credential lengths"
                    << std::endl;
                return {};
            }

            services::wifi_service::connect(ssid, password);
            return {createWifiStatusMessage(), false};
        }

        case MessageType::WifiDisconnect:
            services::wifi_service::disconnect();
            return {createWifiStatusMessage(), false};

        case MessageType::WifiNetworks:
        case MessageType::WifiStatus:
            return {};

        case MessageType::WifiInternetTest:
            services::wifi_service::startInternetTest();
            return {createWifiStatusMessage(), false};

        case MessageType::DisconnectRequested:
            return {
                std::nullopt,
                true};

        case MessageType::Unknown:
            std::cerr
                << "Unknown protocol message type"
                << std::endl;

            return {};
        }

        return {};
    }
}
