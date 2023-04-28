add_executable(${PROJECT_NAME} main.c chip8.c)
include_directories(${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} raylib)
