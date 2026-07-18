#include "wifi_service.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>

#ifdef ESP_PLATFORM
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#endif

namespace swirski::services::wifi_service
{
    namespace
    {
        constexpr std::size_t maxNetworks = 8;

        ConnectionState connectionState =
            ConnectionState::Unavailable;
        bool scanning = false;
        std::string connectedSsid;
        std::vector<Network> networks;
        std::uint32_t revision = 0;

#ifdef ESP_PLATFORM
        std::atomic_bool scanFinished{false};
        std::atomic_bool connectionSucceeded{false};
        std::atomic_bool connectionFailed{false};
        std::atomic_bool ignoreNextDisconnect{false};

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
            revision++;
        }

        if (connectionFailed.exchange(false))
        {
            connectionState = ConnectionState::Failed;
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
