#include "music_service.hpp"
#include "notification_service.hpp"
#include "protocol.hpp"
#include "date_time.hpp"
#include "keyboard_service.hpp"

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
        {"time sync keeps UTC and offset separate", timeSyncKeepsUtcAndOffsetSeparate},
        {"paused music position stays still", pausedMusicPositionStaysStill},
        {"playing music position stops at duration", playingMusicPositionStopsAtDuration},
        {"keyboard text can be edited", keyboardTextCanBeEdited},
        {"keyboard text is limited", keyboardTextIsLimited}};

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
