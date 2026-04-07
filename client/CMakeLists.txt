cmake_minimum_required(VERSION 3.16)
project(activity_monitor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11 xtst xi)
find_package(CURL REQUIRED)

add_executable(activity_monitor
    src/main.c
    src/state.c
    src/time_utils.c
    src/json_payload.c
    src/http_client.c
    src/xrecord_listener.c
)

target_include_directories(activity_monitor PRIVATE
    include
    ${X11_INCLUDE_DIRS}
)

target_compile_options(activity_monitor PRIVATE ${X11_CFLAGS_OTHER})

target_link_libraries(activity_monitor PRIVATE
    ${X11_LIBRARIES}
    CURL::libcurl
    pthread
    m
)
