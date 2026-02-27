cmake_minimum_required(VERSION 3.10)

project(qrd C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build: Enabling AddressSanitizer")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.c")

add_executable(qrd ${SOURCES})
