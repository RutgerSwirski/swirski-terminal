#include "max17048.hpp"

#include "max17048_conversion.hpp"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "system_state.hpp"

#include <array>
#include <cstdint>

namespace swirski::hardware::max17048
{
    namespace
    {
        constexpr char tag[] = "max17048";

        constexpr i2c_port_num_t I2C_PORT = I2C_NUM_0;
        constexpr gpio_num_t I2C_PIN_SDA = GPIO_NUM_1;
        constexpr gpio_num_t I2C_PIN_SCL = GPIO_NUM_2;

        constexpr std::uint16_t DEVICE_ADDRESS = 0x36;
        constexpr std::uint32_t I2C_FREQUENCY_HZ = 100'000;
        constexpr std::uint8_t VCELL_REGISTER = 0x02;
        constexpr std::uint8_t SOC_REGISTER = 0x04;
        constexpr std::int64_t READ_INTERVAL_US = 5'000'000;
        constexpr int TRANSACTION_TIMEOUT_MS = 100;

        i2c_master_bus_handle_t busHandle = nullptr;
        i2c_master_dev_handle_t deviceHandle = nullptr;

        bool firstRead = true;
        std::int64_t lastReadAtUs = 0;
        std::uint32_t failedReadCount = 0;

        std::uint16_t decodeBigEndian(
            std::uint8_t high,
            std::uint8_t low)
        {
            return static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(high) << 8U) |
                low);
        }

        esp_err_t readRegister(
            std::uint8_t registerAddress,
            std::uint16_t &value)
        {
            std::array<std::uint8_t, 2> registerValue{};

            const esp_err_t result =
                i2c_master_transmit_receive(
                    deviceHandle,
                    &registerAddress,
                    sizeof(registerAddress),
                    registerValue.data(),
                    registerValue.size(),
                    TRANSACTION_TIMEOUT_MS);

            if (result == ESP_OK)
            {
                value = decodeBigEndian(
                    registerValue[0],
                    registerValue[1]);
            }

            return result;
        }

        void reportReadFailure(esp_err_t result)
        {
            ++failedReadCount;

            if (failedReadCount == 1 || failedReadCount % 12 == 0)
            {
                ESP_LOGW(
                    tag,
                    "Could not read gauge at 0x%02x: %s",
                    DEVICE_ADDRESS,
                    esp_err_to_name(result));
            }

            swirski::state::system::setBatteryMeasurement(
                std::nullopt,
                std::nullopt);
        }
    }

    void initialise()
    {
        ESP_LOGI(
            tag,
            "Initialising on SDA GPIO%d, SCL GPIO%d, address 0x%02x",
            I2C_PIN_SDA,
            I2C_PIN_SCL,
            DEVICE_ADDRESS);

        i2c_master_bus_config_t busConfig{};
        busConfig.i2c_port = I2C_PORT;
        busConfig.sda_io_num = I2C_PIN_SDA;
        busConfig.scl_io_num = I2C_PIN_SCL;
        busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
        busConfig.glitch_ignore_cnt = 7;
        busConfig.flags.enable_internal_pullup = true;

        esp_err_t result =
            i2c_new_master_bus(
                &busConfig,
                &busHandle);

        if (result != ESP_OK)
        {
            ESP_LOGE(
                tag,
                "Could not initialise I2C bus: %s",
                esp_err_to_name(result));
            return;
        }

        i2c_device_config_t deviceConfig{};
        deviceConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        deviceConfig.device_address = DEVICE_ADDRESS;
        deviceConfig.scl_speed_hz = I2C_FREQUENCY_HZ;

        result =
            i2c_master_bus_add_device(
                busHandle,
                &deviceConfig,
                &deviceHandle);

        if (result != ESP_OK)
        {
            ESP_LOGE(
                tag,
                "Could not register gauge: %s",
                esp_err_to_name(result));
            deviceHandle = nullptr;
            return;
        }

        update();
    }

    void update()
    {
        if (deviceHandle == nullptr)
        {
            return;
        }

        const std::int64_t nowUs = esp_timer_get_time();

        if (
            !firstRead &&
            nowUs - lastReadAtUs < READ_INTERVAL_US)
        {
            return;
        }

        firstRead = false;
        lastReadAtUs = nowUs;

        std::uint16_t rawVoltage = 0;
        std::uint16_t rawPercentage = 0;

        esp_err_t result =
            readRegister(VCELL_REGISTER, rawVoltage);

        if (result == ESP_OK)
        {
            result = readRegister(SOC_REGISTER, rawPercentage);
        }

        if (result != ESP_OK)
        {
            reportReadFailure(result);
            return;
        }

        const std::uint16_t millivolts =
            voltageMillivoltsFromRaw(rawVoltage);
        const std::uint8_t percentage =
            percentageFromRaw(rawPercentage);

        if (failedReadCount > 0)
        {
            ESP_LOGI(tag, "Gauge communication restored");
        }

        failedReadCount = 0;

        swirski::state::system::setBatteryMeasurement(
            millivolts,
            percentage);

        ESP_LOGD(
            tag,
            "Battery: %u mV, %u%%",
            static_cast<unsigned>(millivolts),
            static_cast<unsigned>(percentage));
    }
}
