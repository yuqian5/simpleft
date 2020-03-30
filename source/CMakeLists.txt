CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(lanft)

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

# find shasum, required
FIND_PROGRAM(SHASUM shasum)
IF (NOT SHASUM)
    MESSAGE(FATAL_ERROR "shasum not found")
    RETURN()
ENDIF (NOT SHASUM)

# add project file
ADD_EXECUTABLE(lanft main.cpp TX.hpp TX.cpp RX.hpp RX.cpp misc.hpp misc.cpp Transceiver.hpp Transceiver.cpp)

# install
INSTALL(TARGETS lanft DESTINATION bin)

