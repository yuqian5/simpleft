cmake_minimum_required(VERSION 3.5)
project(lanft)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif (NOT CMAKE_BUILD_TYPE)

add_executable(lanft main.cpp TX.h TX.cpp RX.h RX.cpp misc.h misc.cpp Transceiver.cpp Transceiver.hpp)

# install
install(TARGETS lanft DESTINATION bin)
