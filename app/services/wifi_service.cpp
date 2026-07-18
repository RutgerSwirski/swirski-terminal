#include "wifi_service.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "ping/ping_sock.h"
#endif

namespace swirski::services::wifi_service
{
    namespace
    {
        constexpr std::size_t maxNetworks = 8;

        ConnectionState connectionState =
            ConnectionState::Unavailable;
        bool scanning = false;
        std::int32_t signalStrength = -127;
        InternetTestState internetTestState =
            InternetTestState::Idle;
        std::uint32_t internetLatencyMs = 0;
        std::string connectedSsid;
        std::vector<Network> networks;
        std::uint32_t revision = 0;

#ifdef ESP_PLATFORM
        std::atomic_bool scanFinished{false};
        std::atomic_bool connectionSucceeded{false};
        std::atomic_bool connectionFailed{false};
        std::atomic_bool ignoreNextDisconnect{false};
        std::atomic_bool pingReplyReceived{false};
        std::atomic_int internetTestResult{0};
        std::atomic_uint32_t pendingInternetLatencyMs{0};

        using Clock = std::chrono::steady_clock;
        constexpr std::chrono::seconds signalUpdateInterval{5};
        Clock::time_point lastSignalUpdate = Clock::now();

        void updateSignalStrength()
        {
            wifi_ap_record_t accessPoint{};

            if (esp_wifi_sta_get_ap_info(&accessPoint) != ESP_OK)
            {
                return;
            }

            const int previousLevel =
                signalBarsForRssi(signalStrength);
            signalStrength = accessPoint.rssi;

            if (signalBarsForRssi(signalStrength) != previousLevel)
            {
                revision++;
            }
        }

        void handlePingSuccess(
            esp_ping_handle_t handle,
            void *)
        {
            std::uint32_t latencyMs = 0;
            esp_ping_get_profile(
                handle,
                ESP_PING_PROF_TIMEGAP,
                &latencyMs,
                sizeof(latencyMs));

            pendingInternetLatencyMs = latencyMs;
            pingReplyReceived = true;
        }

        void handlePingEnd(
            esp_ping_handle_t handle,
            void *)
        {
            internetTestResult =
                pingReplyReceived ? 1 : 2;
            esp_ping_delete_session(handle);
        }

        void runInternetTest(void *)
        {
            addrinfo hints{};
            hints.ai_family = AF_UNSPEC;

            addrinfo *addressInfo = nullptr;

            if (
                getaddrinfo(
                    "swirski.studio",
                    nullptr,
                    &hints,
                    &addressInfo) != 0 ||
                addressInfo == nullptr)
            {
                internetTestResult = 2;
                vTaskDelete(nullptr);
                return;
            }

            ip_addr_t targetAddress{};

            if (addressInfo->ai_family == AF_INET)
            {
                const auto *address =
                    reinterpret_cast<sockaddr_in *>(
                        addressInfo->ai_addr);
                inet_addr_to_ip4addr(
                    ip_2_ip4(&targetAddress),
                    &address->sin_addr);
            }
            else if (addressInfo->ai_family == AF_INET6)
            {
                const auto *address =
                    reinterpret_cast<sockaddr_in6 *>(
                        addressInfo->ai_addr);
                inet6_addr_to_ip6addr(
                    ip_2_ip6(&targetAddress),
                    &address->sin6_addr);
            }
            else
            {
                freeaddrinfo(addressInfo);
                internetTestResult = 2;
                vTaskDelete(nullptr);
                return;
            }

            freeaddrinfo(addressInfo);

            esp_ping_config_t config =
                ESP_PING_DEFAULT_CONFIG();
            config.target_addr = targetAddress;
            config.count = 1;
            config.timeout_ms = 2000;

            esp_ping_callbacks_t callbacks{};
            callbacks.on_ping_success = handlePingSuccess;
            callbacks.on_ping_end = handlePingEnd;

            esp_ping_handle_t pingHandle = nullptr;

            if (
                esp_ping_new_session(
                    &config,
                    &callbacks,
                    &pingHandle) != ESP_OK ||
                esp_ping_start(pingHandle) != ESP_OK)
            {
                if (pingHandle != nullptr)
                {
                    esp_ping_delete_session(pingHandle);
                }

                internetTestResult = 2;
            }

            vTaskDelete(nullptr);
        }

        void handleWifiEvent(
            void *,
            esp_event_base_t eventBase,
            std::int32_t eventId,
            void *)
        {
            if (
                eventBase == WIFI_EVENT &&
                eventId == WIFI_EVENT_SCAN_DONE)
            {
                scanFinished = true;
            }
            else if (
                eventBase == WIFI_EVENT &&
                eventId == WIFI_EVENT_STA_DISCONNECTED)
            {
                if (!ignoreNextDisconnect.exchange(false))
                {
                    connectionFailed = true;
                }
            }
            else if (
                eventBase == IP_EVENT &&
                eventId == IP_EVENT_STA_GOT_IP)
            {
                connectionSucceeded = true;
            }
        }

        void readScanResults()
        {
            std::uint16_t count = 0;
            esp_wifi_scan_get_ap_num(&count);
            count = std::min<std::uint16_t>(count, 20);

            std::vector<wifi_ap_record_t> records(count);

            if (
                count > 0 &&
                esp_wifi_scan_get_ap_records(
                    &count,
                    records.data()) != ESP_OK)
            {
                count = 0;
            }

            networks.clear();

            for (std::uint16_t i = 0; i < count; ++i)
            {
                const char *ssidText =
                    reinterpret_cast<const char *>(records[i].ssid);
                const std::string ssid(
                    ssidText,
                    strnlen(ssidText, sizeof(records[i].ssid)));

                if (
                    ssid.empty() ||
                    std::any_of(
                        networks.begin(),
                        networks.end(),
                        [&ssid](const Network &network)
                        {
                            return network.ssid == ssid;
                        }))
                {
                    continue;
                }

                networks.push_back(
                    {ssid,
                     records[i].rssi,
                     records[i].authmode != WIFI_AUTH_OPEN});

                if (networks.size() == maxNetworks)
                {
                    break;
                }
            }

            std::sort(
                networks.begin(),
                networks.end(),
                [](const Network &left, const Network &right)
                {
                    return left.signalStrength > right.signalStrength;
                });
        }
#endif
    }

    void initialise()
    {
#ifdef ESP_PLATFORM
        if (esp_netif_init() != ESP_OK)
        {
            return;
        }

        const esp_err_t eventLoopResult =
            esp_event_loop_create_default();

        if (
            eventLoopResult != ESP_OK &&
            eventLoopResult != ESP_ERR_INVALID_STATE)
        {
            return;
        }

        esp_netif_create_default_wifi_sta();

        wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

        if (esp_wifi_init(&config) != ESP_OK)
        {
            return;
        }

        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            handleWifiEvent,
            nullptr);
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            handleWifiEvent,
            nullptr);

        esp_wifi_set_storage(WIFI_STORAGE_FLASH);
        esp_wifi_set_mode(WIFI_MODE_STA);

        if (esp_wifi_start() != ESP_OK)
        {
            return;
        }

        connectionState = ConnectionState::Disconnected;
        revision++;

        wifi_config_t savedConfig{};

        if (
            esp_wifi_get_config(
                WIFI_IF_STA,
                &savedConfig) == ESP_OK &&
            savedConfig.sta.ssid[0] != 0)
        {
            connectedSsid = reinterpret_cast<const char *>(
                savedConfig.sta.ssid);
            connectionState = ConnectionState::Connecting;
            esp_wifi_connect();
            revision++;
        }
#endif
    }

    void update()
    {
#ifdef ESP_PLATFORM
        if (scanFinished.exchange(false))
        {
            readScanResults();
            scanning = false;
            revision++;
        }

        if (connectionSucceeded.exchange(false))
        {
            connectionState = ConnectionState::Connected;
            updateSignalStrength();
            revision++;
        }

        if (connectionFailed.exchange(false))
        {
            connectionState = ConnectionState::Failed;
            signalStrength = -127;
            revision++;
        }

        if (
            connectionState == ConnectionState::Connected &&
            Clock::now() - lastSignalUpdate >= signalUpdateInterval)
        {
            lastSignalUpdate = Clock::now();
            updateSignalStrength();
        }

        const int testResult =
            internetTestResult.exchange(0);

        if (
            testResult != 0 &&
            connectionState == ConnectionState::Connected)
        {
            internetTestState =
                testResult == 1
                    ? InternetTestState::Success
                    : InternetTestState::Failed;
            internetLatencyMs =
                pendingInternetLatencyMs;
            revision++;
        }
#endif
    }

    void scan()
    {
#ifdef ESP_PLATFORM
        if (
            connectionState == ConnectionState::Unavailable ||
            scanning)
        {
            return;
        }

        wifi_scan_config_t config{};
        config.show_hidden = false;

        if (esp_wifi_scan_start(&config, false) == ESP_OK)
        {
            scanning = true;
            revision++;
        }
#endif
    }

    void startInternetTest()
    {
#ifdef ESP_PLATFORM
        if (
            connectionState != ConnectionState::Connected ||
            internetTestState == InternetTestState::Testing)
        {
            return;
        }

        internetTestState = InternetTestState::Testing;
        internetLatencyMs = 0;
        pingReplyReceived = false;
        pendingInternetLatencyMs = 0;
        internetTestResult = 0;
        revision++;

        if (
            xTaskCreate(
                runInternetTest,
                "internet_test",
                4096,
                nullptr,
                2,
                nullptr) != pdPASS)
        {
            internetTestState = InternetTestState::Failed;
            revision++;
        }
#else
        internetTestState = InternetTestState::Failed;
        revision++;
#endif
    }

    void disconnect()
    {
#ifdef ESP_PLATFORM
        ignoreNextDisconnect = true;

        if (esp_wifi_disconnect() != ESP_OK)
        {
            ignoreNextDisconnect = false;
        }

        wifi_config_t emptyConfig{};
        esp_wifi_set_config(WIFI_IF_STA, &emptyConfig);
#endif

        connectedSsid.clear();
        signalStrength = -127;
        connectionState = ConnectionState::Disconnected;
        internetTestState = InternetTestState::Idle;
        internetLatencyMs = 0;
        revision++;
    }

    void connect(
        const std::string &ssid,
        const std::string &password)
    {
#ifdef ESP_PLATFORM
        wifi_config_t config{};

        const std::size_t ssidLength =
            std::min(ssid.size(), sizeof(config.sta.ssid) - 1);
        const std::size_t passwordLength =
            std::min(password.size(), sizeof(config.sta.password) - 1);

        std::memcpy(config.sta.ssid, ssid.data(), ssidLength);
        std::memcpy(
            config.sta.password,
            password.data(),
            passwordLength);
        config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        config.sta.pmf_cfg.capable = true;
        config.sta.pmf_cfg.required = false;

        ignoreNextDisconnect =
            connectionState == ConnectionState::Connected ||
            connectionState == ConnectionState::Connecting;

        if (esp_wifi_disconnect() != ESP_OK)
        {
            ignoreNextDisconnect = false;
        }

        if (
            esp_wifi_set_config(WIFI_IF_STA, &config) != ESP_OK ||
            esp_wifi_connect() != ESP_OK)
        {
            connectionState = ConnectionState::Failed;
        }
        else
        {
            connectedSsid = ssid;
            connectionState = ConnectionState::Connecting;
        }

        revision++;
#else
        (void)ssid;
        (void)password;
#endif
    }

    ConnectionState getConnectionState()
    {
        return connectionState;
    }

    bool isScanning()
    {
        return scanning;
    }

    std::int32_t getSignalStrength()
    {
        return signalStrength;
    }

    InternetTestState getInternetTestState()
    {
        return internetTestState;
    }

    std::uint32_t getInternetLatencyMs()
    {
        return internetLatencyMs;
    }

    const std::string &getConnectedSsid()
    {
        return connectedSsid;
    }

    const std::vector<Network> &getNetworks()
    {
        return networks;
    }

    std::uint32_t getRevision()
    {
        return revision;
    }
}
