cmake_minimum_required(VERSION 3.10)
project(virtual_remote_linux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(virtual_remote_server
    src/server_main.cpp
    src/udp_receiver.cpp
    src/virtual_gamepad.cpp
    src/controller_engine.cpp
    src/discovery_service.cpp
)

target_include_directories(virtual_remote_server PRIVATE include)
