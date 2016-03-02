cmake_minimum_required (VERSION 2.6)
project (Chip8pp)

set ( CMAKE_CXX_FLAGS "-std=c++11 -g" )

find_package (Threads)

# Detect and add SFML
set(CMAKE_MODULE_PATH "." ${CMAKE_MODULE_PATH})
find_package(SFML 2 REQUIRED system window graphics audio)

add_library(chip8cpu SHARED chip8cpu.cpp)
add_library(chip8SFML SHARED chip8SFML.cpp)
add_executable(Chip8pp main.cpp)
target_link_libraries (Chip8pp chip8cpu chip8SFML ${CMAKE_THREAD_LIBS_INIT} ${SFML_LIBRARIES})

