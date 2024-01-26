cmake_minimum_required(VERSION 3.25)

set(obs_usc_VERSION_MAJOR 1)
set(obs_usc_VERSION_MINOR 2)
set(obs_usc_VERSION_PATCH 0)
set(obs_usc_VERSION_STR "${obs_usc_VERSION_MAJOR}.${obs_usc_VERSION_MINOR}.${obs_usc_VERSION_PATCH}")

project(obs-studio-plugin-unix-socket-control VERSION "${obs_usc_VERSION_STR}")


add_compile_options(
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:DEBUG>:-Og>
)

if(NOT DEFINED CMAKE_BUILD_TYPE OR NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
    message("Set build type to Debug by default")
endif()

set(unix-socket-control_SOURCES
    src/plugin.c
    src/socket.c
)

add_library(unix-socket-control MODULE
    ${unix-socket-control_SOURCES})

find_package(libobs REQUIRED)
target_link_libraries(unix-socket-control PRIVATE OBS::libobs)

find_package(obs-frontend-api REQUIRED)
target_link_libraries(unix-socket-control PRIVATE OBS::obs-frontend-api)

add_executable(unix-socket-control-client src/client.c)
