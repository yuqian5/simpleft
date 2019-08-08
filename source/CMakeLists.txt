cmake_minimum_required(VERSION 3.14)
project(LANFileTransfer)

set(CMAKE_CXX_STANDARD 17)

add_executable(LANFileTransfer main.cpp mystruct.h TX.cpp TX.h RX.cpp RX.h)

find_package(OpenSSL REQUIRED)
target_link_libraries(LANFileTransfer OpenSSL::SSL)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/testFile.txt
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

