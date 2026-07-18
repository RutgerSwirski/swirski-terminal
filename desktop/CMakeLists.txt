cmake_minimum_required(VERSION 3.22)

project(
    swirski_os_desktop
    LANGUAGES C CXX
)


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

include(FetchContent)

FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG v9.4.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

set(USE_TLS OFF CACHE BOOL "" FORCE)
set(USE_ZLIB OFF CACHE BOOL "" FORCE)
set(USE_WS OFF CACHE BOOL "" FORCE)
set(USE_TEST OFF CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v12.0.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(ixwebsocket)

FetchContent_Declare(
    ArduinoJson
    GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
    GIT_TAG v7.4.3
)

FetchContent_MakeAvailable(ArduinoJson)

# Point LVGL at the desktop-specific configuration.
set(
    LV_BUILD_CONF_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h"
    CACHE PATH
    ""
    FORCE
)

# We don't need the large LVGL example/demo libraries.
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(lvgl)

add_executable(
    swirski_os_desktop

    main.cpp

    ../app/app.cpp

    ../app/input/input.cpp

    ../app/ui/status_bar.cpp
    ../app/ui/keyboard.cpp

    ../app/screens/screen_manager.cpp
    ../app/screens/home_screen.cpp
    ../app/screens/notifications_screen.cpp
    ../app/screens/notification_screen.cpp
    ../app/screens/music_screen.cpp
    ../app/screens/games_screen.cpp
    ../app/screens/pong_screen.cpp
    ../app/screens/blackjack_screen.cpp
    ../app/screens/settings_screen.cpp

    ../app/services/date_time.cpp
    ../app/services/notification_service.cpp
    ../app/services/music_service.cpp
    ../app/services/settings_service.cpp
    ../app/services/keyboard_service.cpp

    ../app/protocol/protocol.cpp

    ../app/state/system_state.cpp


    inputs/keyboard.cpp

    websocket_transport/websocket_transport.cpp
)

target_include_directories(
    swirski_os_desktop
    PRIVATE
        ../app
        ../app/input
        ../app/ui
        ../app/screens
        ../app/services
        ../app/transport
        ../app/protocol
        ../app/state
        ../app/models
        inputs
        websocket_transport
)

target_link_libraries(
    swirski_os_desktop
    PRIVATE
        lvgl::lvgl
        SDL2::SDL2
        ixwebsocket::ixwebsocket
        ArduinoJson
)

include(CTest)

if(BUILD_TESTING)
    add_executable(
        swirski_os_tests
        tests.cpp
        ../app/protocol/protocol.cpp
        ../app/services/date_time.cpp
        ../app/services/music_service.cpp
        ../app/services/notification_service.cpp
        ../app/services/keyboard_service.cpp
    )

    target_include_directories(
        swirski_os_tests
        PRIVATE
            ../app/protocol
            ../app/services
    )

    target_link_libraries(
        swirski_os_tests
        PRIVATE
            lvgl::lvgl
            ArduinoJson
    )

    add_test(
        NAME swirski_os_tests
        COMMAND swirski_os_tests
    )
endif()
