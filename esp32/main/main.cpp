#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "driver/gpio.h"

#include "app.hpp"
#include "./inputs/rotary_encoder.hpp"
#include "./inputs/push_buttons.hpp"
#include "app_constants.hpp"
#include "services/date_time.hpp"
#include "screen_manager.hpp"
#include "notifications_screen.hpp"
#include "ui/status_bar.hpp"
#include "input.hpp"

#include "ble_transport.hpp"

namespace
{

    constexpr spi_host_device_t LCD_HOST = SPI2_HOST;

    constexpr gpio_num_t LCD_PIN_MOSI = GPIO_NUM_6;
    constexpr gpio_num_t LCD_PIN_SCLK = GPIO_NUM_7;
    constexpr gpio_num_t LCD_PIN_CS = GPIO_NUM_5;
    constexpr gpio_num_t LCD_PIN_DC = GPIO_NUM_4;
    constexpr gpio_num_t LCD_PIN_RST = GPIO_NUM_3;

    constexpr int LCD_WIDTH = 320;
    constexpr int LCD_HEIGHT = 240;

    lv_display_t *displayHandle = nullptr;
}

void initialiseDisplay()
{
    constexpr size_t drawBufferHeight = 20;

    const size_t transferBufferSize =
        LCD_WIDTH *
        drawBufferHeight *
        sizeof(uint16_t);

    ESP_LOGI(swirski::TAG, "Initialising SPI bus");

    spi_bus_config_t busConfig{};

    busConfig.sclk_io_num = LCD_PIN_SCLK;
    busConfig.mosi_io_num = LCD_PIN_MOSI;
    busConfig.miso_io_num = -1;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;
    busConfig.max_transfer_sz =
        static_cast<int>(transferBufferSize);

    ESP_ERROR_CHECK(
        spi_bus_initialize(
            LCD_HOST,
            &busConfig,
            SPI_DMA_CH_AUTO));

    ESP_LOGI(swirski::TAG, "Creating LCD SPI interface");

    esp_lcd_panel_io_handle_t ioHandle = nullptr;

    esp_lcd_panel_io_spi_config_t ioConfig{};

    ioConfig.cs_gpio_num = LCD_PIN_CS;
    ioConfig.dc_gpio_num = LCD_PIN_DC;
    ioConfig.spi_mode = 0;
    ioConfig.pclk_hz = 40 * 1000 * 1000;
    ioConfig.trans_queue_depth = 10;
    ioConfig.lcd_cmd_bits = 8;
    ioConfig.lcd_param_bits = 8;

    ESP_ERROR_CHECK(
        esp_lcd_new_panel_io_spi(
            static_cast<esp_lcd_spi_bus_handle_t>(LCD_HOST),
            &ioConfig,
            &ioHandle));

    ESP_LOGI(swirski::TAG, "Creating ILI9341 panel");

    esp_lcd_panel_handle_t panelHandle = nullptr;

    esp_lcd_panel_dev_config_t panelConfig{};

    panelConfig.reset_gpio_num = LCD_PIN_RST;
    panelConfig.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
    panelConfig.bits_per_pixel = 16;

    ESP_ERROR_CHECK(
        esp_lcd_new_panel_ili9341(
            ioHandle,
            &panelConfig,
            &panelHandle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panelHandle, true));

    ESP_LOGI(swirski::TAG, "Registering display with LVGL");

    lvgl_port_display_cfg_t displayConfig{};

    displayConfig.io_handle = ioHandle;
    displayConfig.panel_handle = panelHandle;
    displayConfig.buffer_size = LCD_WIDTH * drawBufferHeight;
    displayConfig.double_buffer = false;
    displayConfig.hres = LCD_WIDTH;
    displayConfig.vres = LCD_HEIGHT;
    displayConfig.monochrome = false;

    displayConfig.rotation.swap_xy = true;
    displayConfig.rotation.mirror_x = true;
    displayConfig.rotation.mirror_y = true;

    displayConfig.color_format = LV_COLOR_FORMAT_RGB565;

    displayConfig.flags.buff_dma = true;
    displayConfig.flags.swap_bytes = true;

    displayHandle = lvgl_port_add_disp(&displayConfig);

    if (displayHandle == nullptr)
    {
        ESP_LOGE(swirski::TAG, "Could not register display with LVGL");
        return;
    }

    ESP_LOGI(swirski::TAG, "Display initialised");
}

// use C linkage
extern "C" void app_main()
{
    ESP_LOGI(swirski::TAG, "Starting Swirski OS");

    const lvgl_port_cfg_t lvglConfig =
        ESP_LVGL_PORT_INIT_CONFIG();

    ESP_ERROR_CHECK(
        lvgl_port_init(&lvglConfig));

    vTaskDelay(1);

    initialiseDisplay();

    vTaskDelay(1);

    swirski::inputs::rotary_encoder::initialise();
    swirski::inputs::push_buttons::initialise();
    swirski::service::date_time::initialise(0);

    swirski::transport::ble::BleTransport bleTransport;

    bleTransport.initialise();

    vTaskDelay(1);

    if (lvgl_port_lock(0))
    {
        swirski::app::createInterface(displayHandle);

        swirski::ui::status_bar::updateClock();

        lvgl_port_unlock();
    }

    vTaskDelay(1);

    while (true)
    {

        bleTransport.update();

        if (lvgl_port_lock(0))
        {
            swirski::ui::status_bar::updateSystemState();

            if (
                swirski::screens::manager::getCurrentScreen() ==
                swirski::screens::manager::Screen::Notifications)
            {
                swirski::screens::notifications_screen::
                    refreshIfNeeded();
            }

            lvgl_port_unlock();
        }

        swirski::inputs::rotary_encoder::poll();

        if (swirski::inputs::push_buttons::backPressed())
        {
            ESP_LOGI(swirski::TAG, "Back button pressed");

            if (lvgl_port_lock(20))
            {
                swirski::input::handleInput(
                    swirski::input::input_action::Back);

                lvgl_port_unlock();
            }
        }

        if (swirski::service::date_time::update())
        {
            if (lvgl_port_lock(20))
            {
                swirski::ui::status_bar::updateClock();
                lvgl_port_unlock();
            }
        }

        vTaskDelay(1);
    }
}
