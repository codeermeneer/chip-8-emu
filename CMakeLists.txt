cmake_minimum_required(VERSION 3.15)
project(chip-8-emu)

find_package(raylib 3.0 REQUIRED)

set(CMAKE_C_STANDARD 11)

add_executable(${PROJECT_NAME} main.c chip8.c)
include_directories(${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} raylib)
