#include "music_service.hpp"
#include "notification_service.hpp"
#include "protocol.hpp"
#include "date_time.hpp"
#include "keyboard_service.hpp"
#include "ble_security.hpp"
#include "wifi_service.hpp"
#include "weather_service.hpp"
#include "display_text.hpp"

#include <ArduinoJson.h>

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using Notification =
        swirski::services::notification_service::Notification;

    void check(
        bool condition,
        const char *expression)
    {
        if (!condition)
        {
            throw std::runtime_error(expression);
        }
    }

#define CHECK(expression) check((expression), #expression)

    Notification makeNotification(
        std::string id,
        std::string title)
    {
        return {
            std::move(id),
            "com.example.app",
            "Example",
            std::move(title),
            "Body",
            1000};
    }

    std::uint64_t steadyNowMs()
    {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<
                std::chrono::milliseconds>(
                std::chrono::steady_clock::now()
                    .time_since_epoch())
                .count());
    }

    void validProtocolMessageParses()
    {
        const auto message =
            swirski::protocol::parseMessage(
                R"({"version":1,"type":"ping","id":"ping-1"})");

        CHECK(message.has_value());
        CHECK(message->type == swirski::protocol::MessageType::Ping);
        CHECK(message->id == "ping-1");
    }

    void malformedJsonIsRejected()
    {
        CHECK(!swirski::protocol::parseMessage("{").has_value());
    }

    void unsupportedProtocolVersionIsRejected()
    {
        CHECK(!swirski::protocol::parseMessage(
                   R"({"version":2,"type":"ping","id":"ping-2"})")
                   .has_value());
    }

    void pingCreatesPongResponse()
    {
        const auto result =
            swirski::protocol::handleIncomingMessage(
                R"({"version":1,"type":"ping","id":"ping-3"})");

        CHECK(result.response.has_value());
        CHECK(result.pingReceived);

        JsonDocument response;
        CHECK(!deserializeJson(response, *result.response));
        CHECK(response["type"] == "pong");
        CHECK(response["id"] == "ping-3");
    }

    void disconnectSetsResultFlag()
    {
        const auto result =
            swirski::protocol::handleIncomingMessage(
                R"({"version":1,"type":"disconnect.requested","id":"disconnect-1"})");

        CHECK(result.disconnectRequested);
        CHECK(!result.response.has_value());
    }

    void snapshotReplacesNotifications()
    {
        swirski::services::notification_service::setSnapshot(
            {makeNotification("1", "First"),
             makeNotification("2", "Second")});

        const auto &notifications =
            swirski::services::notification_service::getNotifications();

        CHECK(notifications.size() == 2);
        CHECK(notifications[0].id == "1");
        CHECK(notifications[1].id == "2");
    }

    void upsertReplacesExistingNotification()
    {
        swirski::services::notification_service::setSnapshot(
            {makeNotification("2", "Second"),
             makeNotification("1", "Old")});

        swirski::services::notification_service::upsert(
            makeNotification("1", "Updated"));

        const auto &notifications =
            swirski::services::notification_service::getNotifications();

        CHECK(notifications.size() == 2);
        CHECK(notifications[0].id == "1");
        CHECK(notifications[0].title == "Updated");
    }

    void notificationLookupAndRemovalWork()
    {
        swirski::services::notification_service::setSnapshot(
            {makeNotification("1", "First")});

        const auto *notification =
            swirski::services::notification_service::getNotificationById("1");

        CHECK(notification != nullptr);
        CHECK(notification->title == "First");
        CHECK(swirski::services::notification_service::removeNotificationById("1"));
        CHECK(swirski::services::notification_service::getNotificationById("1") == nullptr);
        CHECK(!swirski::services::notification_service::removeNotificationById("missing"));
    }

    void notificationStorageIsCapped()
    {
        std::vector<Notification> notifications;

        for (int index = 0; index < 45; ++index)
        {
            notifications.push_back(
                makeNotification(
                    std::to_string(index),
                    "Notification"));
        }

        swirski::services::notification_service::setSnapshot(
            std::move(notifications));

        CHECK(
            swirski::services::notification_service::
                getNotifications()
                    .size() == 40);
    }

    void notificationRemovalMessageUpdatesService()
    {
        swirski::services::notification_service::setSnapshot(
            {makeNotification("remove-me", "Temporary")});

        swirski::protocol::handleIncomingMessage(
            R"({"version":1,"type":"notification.removed","id":"remove-1","payload":{"id":"remove-me"}})");

        CHECK(
            swirski::services::notification_service::
                getNotificationById("remove-me") == nullptr);
    }

    void packageNameGetsReadableFallback()
    {
        swirski::protocol::handleIncomingMessage(
            R"({"version":1,"type":"notifications.snapshot","id":"snapshot-1","payload":{"notifications":[{"id":"1","packageName":"com.soundcloud.android","appName":"Android","title":"Track","body":"Artist"}]}})");

        const auto &notifications =
            swirski::services::notification_service::getNotifications();

        CHECK(notifications.size() == 1);
        CHECK(notifications[0].appName == "Soundcloud");
    }

    void musicMessageUpdatesState()
    {
        swirski::protocol::handleIncomingMessage(
            R"({"version":1,"type":"music.state","id":"music-1","payload":{"appName":"Spotify","title":"Track","artist":"Artist","isPlaying":false,"durationMs":120000,"positionMs":30000}})");

        const auto music =
            swirski::services::music_service::getState();

        CHECK(music.appName == "Spotify");
        CHECK(music.title == "Track");
        CHECK(music.artist == "Artist");
        CHECK(!music.isPlaying);
        CHECK(music.durationMs == 120000);
        CHECK(music.positionMs == 30000);
    }

    void weatherMessageUpdatesSnapshot()
    {
        swirski::protocol::handleIncomingMessage(
            R"({"version":1,"type":"weather.snapshot","id":"weather-1","payload":{"location":"Current location","updatedAtMs":1784332107616,"current":{"temperatureC":21,"condition":"Cloudy"},"forecast":[{"day":"SUN","condition":"Cloudy","lowC":14,"highC":22},{"day":"MON","condition":"Rain","lowC":12,"highC":19}]}})");

        const auto &weather =
            swirski::services::weather_service::getSnapshot();

        CHECK(swirski::services::weather_service::hasData());
        CHECK(weather.location == "Current location");
        CHECK(weather.temperatureC == 21);
        CHECK(weather.condition == "Cloudy");
        CHECK(weather.updatedAtMs == 1784332107616ULL);
        CHECK(weather.forecastCount == 2);
        CHECK(weather.forecast[1].day == "MON");
        CHECK(weather.forecast[1].highC == 19);
    }

    void invalidWeatherMessageIsIgnored()
    {
        const int revision =
            swirski::services::weather_service::getRevision();

        swirski::protocol::handleIncomingMessage(
            R"({"version":1,"type":"weather.snapshot","id":"weather-2","payload":{"current":{"condition":"Rain"},"forecast":[]}})");

        CHECK(
            swirski::services::weather_service::getRevision() ==
            revision);
    }

    void timeSyncKeepsUtcAndOffsetSeparate()
    {
        swirski::service::date_time::initialise(0);
        CHECK(!swirski::service::date_time::hasValidTime());

        swirski::protocol::handleIncomingMessage(
            R"({"version":1,"type":"time.sync","id":"time-1","payload":{"unixTimeSeconds":1000,"timezoneOffsetMinutes":60}})");

        CHECK(swirski::service::date_time::getTimestamp() == 1000);
        CHECK(swirski::service::date_time::getLocalTimestamp() == 4600);
        CHECK(swirski::service::date_time::hasValidTime());
    }

    void pausedMusicPositionStaysStill()
    {
        swirski::services::music_service::setState(
            {"Music", "Track", "Artist", false, 10000, 2500, 0});

        CHECK(swirski::services::music_service::getState().positionMs == 2500);
    }

    void playingMusicPositionStopsAtDuration()
    {
        swirski::services::music_service::setState(
            {"Music", "Track", "Artist", true, 1000, 900, steadyNowMs() - 500});

        CHECK(swirski::services::music_service::getState().positionMs == 1000);
    }

    void keyboardTextCanBeEdited()
    {
        using namespace swirski::services::keyboard_service;

        begin("HI");
        addSpace();
        addCharacter('A');
        CHECK(getText() == "HI A");

        removeLastCharacter();
        CHECK(getText() == "HI ");
    }

    void keyboardTextIsLimited()
    {
        using namespace swirski::services::keyboard_service;

        begin(std::string(40, 'A'));
        CHECK(getText().size() == 40);

        begin(std::string(70, 'A'));
        CHECK(getText().size() == 63);

        addCharacter('B');
        CHECK(getText().size() == 63);
    }

    void displayTextRemovesEmoji()
    {
        CHECK(
            swirski::services::display_text::normalize(
                "Hello \xF0\x9F\x90\xB6 world \xF0\x9F\x94\xA5") ==
            "Hello world");
    }

    void bleSecurityRequiresAllProtections()
    {
        using swirski::transport::ble_security::isSecure;

        CHECK(isSecure(true, true, true));
        CHECK(!isSecure(false, true, true));
        CHECK(!isSecure(true, false, true));
        CHECK(!isSecure(true, true, false));
    }

    void wifiConfigureDoesNotEchoPassword()
    {
        const auto result =
            swirski::protocol::handleIncomingMessage(
                R"({"version":1,"type":"wifi.configure","id":"wifi-1","payload":{"ssid":"Home","password":"very-secret"}})");

        CHECK(result.response.has_value());
        CHECK(result.response->find("very-secret") == std::string::npos);
        CHECK(result.response->find("wifi.status") != std::string::npos);
    }

    void invalidWifiCredentialsAreRejected()
    {
        const auto result =
            swirski::protocol::handleIncomingMessage(
                R"({"version":1,"type":"wifi.configure","id":"wifi-2","payload":{"ssid":"","password":"password"}})");

        CHECK(!result.response.has_value());

        const auto missingPassword =
            swirski::protocol::handleIncomingMessage(
                R"({"version":1,"type":"wifi.configure","id":"wifi-3","payload":{"ssid":"Home"}})");

        CHECK(!missingPassword.response.has_value());
    }

    void wifiStatusContainsExpectedFields()
    {
        JsonDocument status;
        CHECK(!deserializeJson(
            status,
            swirski::protocol::createWifiStatusMessage()));

        CHECK(status["type"] == "wifi.status");
        CHECK(status["payload"]["state"].is<const char *>());
        CHECK(status["payload"]["scanning"].is<bool>());
        CHECK(status["payload"]["signalStrength"].is<int>());
        CHECK(status["payload"]["internetTest"].is<const char *>());
        CHECK(status["payload"]["internetLatencyMs"].is<unsigned int>());
    }

    void wifiDisconnectReturnsDisconnectedStatus()
    {
        const auto result =
            swirski::protocol::handleIncomingMessage(
                R"({"version":1,"type":"wifi.disconnect","id":"wifi-4"})");

        CHECK(result.response.has_value());
        CHECK(
            swirski::services::wifi_service::getConnectionState() ==
            swirski::services::wifi_service::ConnectionState::Disconnected);

        JsonDocument status;
        CHECK(!deserializeJson(status, *result.response));
        CHECK(status["payload"]["state"] == "disconnected");
    }

    void wifiSignalBarsUseStableThresholds()
    {
        using swirski::services::wifi_service::signalBarsForRssi;

        CHECK(signalBarsForRssi(-127) == 0);
        CHECK(signalBarsForRssi(-71) == 1);
        CHECK(signalBarsForRssi(-70) == 2);
        CHECK(signalBarsForRssi(-56) == 2);
        CHECK(signalBarsForRssi(-55) == 3);
    }

    void internetTestReturnsWifiStatus()
    {
        const auto result =
            swirski::protocol::handleIncomingMessage(
                R"({"version":1,"type":"wifi.internet.test","id":"internet-1"})");

        CHECK(result.response.has_value());
        CHECK(result.response->find("wifi.status") != std::string::npos);
        CHECK(result.response->find("internetTest") != std::string::npos);
    }

    struct Test
    {
        const char *name;
        void (*run)();
    };
}

int main()
{
    const std::vector<Test> tests{
        {"valid protocol message parses", validProtocolMessageParses},
        {"malformed JSON is rejected", malformedJsonIsRejected},
        {"unsupported protocol version is rejected", unsupportedProtocolVersionIsRejected},
        {"ping creates pong response", pingCreatesPongResponse},
        {"disconnect sets result flag", disconnectSetsResultFlag},
        {"snapshot replaces notifications", snapshotReplacesNotifications},
        {"upsert replaces existing notification", upsertReplacesExistingNotification},
        {"notification lookup and removal work", notificationLookupAndRemovalWork},
        {"notification storage is capped", notificationStorageIsCapped},
        {"notification removal message updates service", notificationRemovalMessageUpdatesService},
        {"package name gets readable fallback", packageNameGetsReadableFallback},
        {"music message updates state", musicMessageUpdatesState},
        {"weather message updates snapshot", weatherMessageUpdatesSnapshot},
        {"invalid weather message is ignored", invalidWeatherMessageIsIgnored},
        {"time sync keeps UTC and offset separate", timeSyncKeepsUtcAndOffsetSeparate},
        {"paused music position stays still", pausedMusicPositionStaysStill},
        {"playing music position stops at duration", playingMusicPositionStopsAtDuration},
        {"keyboard text can be edited", keyboardTextCanBeEdited},
        {"keyboard text is limited", keyboardTextIsLimited},
        {"display text removes emoji", displayTextRemovesEmoji},
        {"BLE security requires encryption, authentication and bonding", bleSecurityRequiresAllProtections},
        {"Wi-Fi configure does not echo password", wifiConfigureDoesNotEchoPassword},
        {"invalid Wi-Fi credentials are rejected", invalidWifiCredentialsAreRejected},
        {"Wi-Fi status contains expected fields", wifiStatusContainsExpectedFields},
        {"Wi-Fi disconnect returns disconnected status", wifiDisconnectReturnsDisconnectedStatus},
        {"Wi-Fi signal bars use stable thresholds", wifiSignalBarsUseStableThresholds},
        {"internet test returns Wi-Fi status", internetTestReturnsWifiStatus}};

    int failures = 0;

    for (const Test &test : tests)
    {
        try
        {
            test.run();
            std::cout << "PASS: " << test.name << '\n';
        }
        catch (const std::exception &error)
        {
            failures += 1;
            std::cerr
                << "FAIL: " << test.name
                << " (" << error.what() << ")\n";
        }
    }

    std::cout
        << tests.size() - failures
        << "/"
        << tests.size()
        << " tests passed\n";

    return failures == 0 ? 0 : 1;
}
