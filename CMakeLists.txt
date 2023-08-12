cmake_minimum_required(VERSION 3.25)
project(obs-studio-plugin-unix-socket-control)

add_compile_options(
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:DEBUG>:-O0>
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
