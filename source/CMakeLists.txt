CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(sft)

# set cpp standard
SET(CMAKE_CXX_STANDARD 17)

# set compile flag
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

# default build type, release
IF (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release")
ENDIF (NOT CMAKE_BUILD_TYPE)

# find tar, required
FIND_PROGRAM(TAR tar)
IF (NOT TAR)
    MESSAGE(FATAL_ERROR "tar not found")
    RETURN()
ENDIF (NOT TAR)

# add project file
ADD_EXECUTABLE(sft
        src/main.cpp
        include/TX.hpp src/TX.cpp
        include/RX.hpp src/RX.cpp
        include/misc.hpp src/misc.cpp
        include/Transceiver.hpp src/Transceiver.cpp
        include/Packet.hpp src/Packet.cpp
        include/Logging.hpp src/Logging.cpp)

# install
INSTALL(TARGETS sft DESTINATION bin)

