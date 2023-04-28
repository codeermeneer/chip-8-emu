cmake_minimum_required(VERSION 3.15)
project(chip-8-emu)

set(RAYLIB_VERSION 4.2.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

set(CMAKE_C_STANDARD 11)

add_executable(${PROJECT_NAME} main.c chip8.c)
include_directories(${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} raylib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
add_custom_target(copy_resources
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_MODULE_PATH}/copy-resources.cmake
)
add_dependencies(${PROJECT_NAME} copy_resources)
