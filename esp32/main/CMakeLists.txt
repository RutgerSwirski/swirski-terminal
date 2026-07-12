set(APP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../app")

idf_component_register(
    SRCS
        "main.cpp"
        "inputs/rotary_encoder.cpp"
        "${APP_DIR}/app.cpp"
        "${APP_DIR}/screens/home_screen.cpp"
        "${APP_DIR}/screens/screen_manager.cpp"
        "${APP_DIR}/input/input.cpp"

    INCLUDE_DIRS
        "."
        "inputs"
        "${APP_DIR}"
        "${APP_DIR}/screens"
        "${APP_DIR}/input"

    PRIV_REQUIRES
        esp_driver_gpio
        esp_driver_spi
        esp_lcd
        esp_lcd_ili9341
        esp_lvgl_port
        lvgl
)