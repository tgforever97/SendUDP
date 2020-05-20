cmake_minimum_required(VERSION 3.15)
project(SendUDP)

set(CMAKE_CXX_STANDARD 11)

# log setting
set(LOG_FILE_NAME "application.log")
set(SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG)
configure_file(
        "./cmake/config.h.in"
        "${PROJECT_SOURCE_DIR}/include/config.h"
        @ONLY
)

add_executable(SendUDP main.cpp include/UdpServer.h include/UdpClient.h src/UdpServer.cpp src/UdpClient.cpp)

target_include_directories( ${PROJECT_NAME}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )

target_link_libraries( ${PROJECT_NAME}
        PRIVATE
        )
