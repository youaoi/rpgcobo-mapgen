cmake_minimum_required(VERSION 3.16)
project(mapimggen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(mapimggen
    main.cpp
    CaveMapImageGenerator.cpp
    DungeonMapImageGenerator.cpp
    WorldMapImageGenerator.cpp
    utils.cpp
)

target_include_directories(mapimggen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(mapimggen PRIVATE /W4 /permissive-)
else()
    target_compile_options(mapimggen PRIVATE -Wall -Wextra -Wpedantic)
endif()
