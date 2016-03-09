cmake_minimum_required (VERSION 2.6)
project (Chip8pp)

set ( CMAKE_CXX_FLAGS "-std=c++11 -g" )

find_package (Threads)

# Detect and add SFML
set(CMAKE_MODULE_PATH "." ${CMAKE_MODULE_PATH})
find_package(SFML 2 REQUIRED system window graphics audio)

add_executable(Chip8pp main.cpp chip8cpu.cpp chip8SFML.cpp)
target_link_libraries (Chip8pp ${CMAKE_THREAD_LIBS_INIT} ${SFML_LIBRARIES})

