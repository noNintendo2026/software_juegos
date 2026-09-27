cmake_minimum_required(VERSION 3.14)

project(halSimulator LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)
include_directories(include)

add_executable(halSimulator src/main.c)
target_link_libraries(halSimulator PRIVATE ${SDL2_LIBRARIES})
