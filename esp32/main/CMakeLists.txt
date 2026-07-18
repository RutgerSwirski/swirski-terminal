set(APP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../app")

idf_component_register(
    SRCS
        "main.cpp"

        "inputs/rotary_encoder.cpp"
        "inputs/push_buttons.cpp"

        "settings/hardware_settings.cpp"

        "ble_transport/ble_transport.cpp"

        "${APP_DIR}/app.cpp"
        
        "${APP_DIR}/input/input.cpp"

        "${APP_DIR}/ui/status_bar.cpp"
        "${APP_DIR}/ui/notification_toast.cpp"
        "${APP_DIR}/ui/keyboard.cpp"
        "${APP_DIR}/ui/fonts/swirski_font_12.c"
        "${APP_DIR}/ui/fonts/swirski_font_14.c"

        "${APP_DIR}/screens/home_screen.cpp"
        "${APP_DIR}/screens/notifications_screen.cpp"
        "${APP_DIR}/screens/screen_manager.cpp"
        "${APP_DIR}/screens/notification_screen.cpp"
        "${APP_DIR}/screens/music_screen.cpp"
        "${APP_DIR}/screens/games_screen.cpp"
        "${APP_DIR}/screens/pong_screen.cpp"
        "${APP_DIR}/screens/blackjack_screen.cpp"
        "${APP_DIR}/screens/settings_screen.cpp"
        "${APP_DIR}/screens/wifi_screen.cpp"
        
        "${APP_DIR}/services/date_time.cpp"
        "${APP_DIR}/services/notification_service.cpp"
        "${APP_DIR}/services/music_service.cpp"
        "${APP_DIR}/services/settings_service.cpp"
        "${APP_DIR}/services/keyboard_service.cpp"
        "${APP_DIR}/services/wifi_service.cpp"
        "${APP_DIR}/services/firmware_update.cpp"

        "${APP_DIR}/protocol/protocol.cpp"

        "${APP_DIR}/state/system_state.cpp"


    INCLUDE_DIRS
        "."
        "inputs"
        "settings"
        "ble_transport"
        "${APP_DIR}"
        "${APP_DIR}/screens"
        "${APP_DIR}/input"
        "${APP_DIR}/ui"
        "${APP_DIR}/services"
        "${APP_DIR}/transport"
        "${APP_DIR}/protocol"
        "${APP_DIR}/state"
        "${APP_DIR}/models"

    PRIV_REQUIRES
        esp_driver_gpio
        esp_driver_spi
        esp_pm
        esp_lcd
        esp_lcd_ili9341
        esp_lvgl_port
        lvgl
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        lwip
        esp_https_ota
        mbedtls
        app_update
        bootloader_support
)
