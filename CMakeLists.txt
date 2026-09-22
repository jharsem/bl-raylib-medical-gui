cmake_minimum_required(VERSION 3.16)
project(raymed LANGUAGES C)

set(CMAKE_C_STANDARD 11)                # MSVC needs C11 mode for compound literals
set(CMAKE_C_STANDARD_REQUIRED ON)

option(RAYMED_USE_SYSTEM_RAYLIB "Use an installed raylib when available" ON)
option(RAYMED_BUILD_DEMO "Build the demo program" ON)

# raymed itself is header-only
add_library(raymed INTERFACE)
target_include_directories(raymed INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT RAYMED_BUILD_DEMO)
    return()
endif()

if(RAYMED_USE_SYSTEM_RAYLIB)
    find_package(raylib 5.5 QUIET)
endif()

if(NOT raylib_FOUND)
    message(STATUS "raymed: fetching raylib 5.5")
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(demo demo.c)
target_link_libraries(demo PRIVATE raymed raylib)

if(MSVC)
    target_compile_definitions(demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(demo PRIVATE /W3)
else()
    target_compile_options(demo PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-unused-result)
    if(NOT WIN32)
        target_link_libraries(demo PRIVATE m)
    endif()
endif()

if(APPLE)
    target_link_libraries(demo PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()
