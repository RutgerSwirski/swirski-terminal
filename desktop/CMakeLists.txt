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
    ../app/screen/screen.cpp
    ../app/input/input.cpp
    inputs/keyboard.cpp


)

target_include_directories(
    swirski_os_desktop
    PRIVATE
        ../app
        ../app/screen
        ../app/input
        inputs
        
)

target_link_libraries(
    swirski_os_desktop
    PRIVATE
        lvgl::lvgl
        SDL2::SDL2
)