set(APP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../app")

idf_component_register(
    SRCS
        "main.cpp"

        "inputs/rotary_encoder.cpp"
        "inputs/push_buttons.cpp"

        "ble_transport/ble_transport.cpp"

        "${APP_DIR}/app.cpp"
        
        "${APP_DIR}/input/input.cpp"

        "${APP_DIR}/ui/status_bar.cpp"

        "${APP_DIR}/screens/home_screen.cpp"
        "${APP_DIR}/screens/notifications_screen.cpp"
        "${APP_DIR}/screens/screen_manager.cpp"
        "${APP_DIR}/screens/notification_screen.cpp"
        
        "${APP_DIR}/services/date_time.cpp"
        "${APP_DIR}/services/notification_service.cpp"

        "${APP_DIR}/protocol/protocol.cpp"

        "${APP_DIR}/state/system_state.cpp"


    INCLUDE_DIRS
        "."
        "inputs"
        "ble_transport"
        "${APP_DIR}"
        "${APP_DIR}/screens"
        "${APP_DIR}/input"
        "${APP_DIR}/ui"
        "${APP_DIR}/services"
        "${APP_DIR}/transport"
        "${APP_DIR}/protocol"
        "${APP_DIR}/state"

    PRIV_REQUIRES
        esp_driver_gpio
        esp_driver_spi
        esp_lcd
        esp_lcd_ili9341
        esp_lvgl_port
        lvgl
)