CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(sft)

# set cpp standard
SET(CMAKE_CXX_STANDARD 20)

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
        include/Transceiver.hpp src/Transceiver.cpp
        include/Packet.hpp src/Packet.cpp
        include/Logging.hpp src/Logging.cpp
        include/NetworkUtility.hpp src/NetworkUtility.cpp
        include/sft_constants.hpp
        include/cli/CmdArgs.hpp
        include/cli/ArgParser.hpp        src/cli/ArgParser.cpp
        include/cli/IpFlag.hpp           src/cli/IpFlag.cpp
        include/cli/PortFlag.hpp         src/cli/PortFlag.cpp
        include/cli/PassphraseFlag.hpp   src/cli/PassphraseFlag.cpp
        lib/tweetnacl/tweetnacl.c
        lib/tweetnacl/randombytes.c
)

TARGET_INCLUDE_DIRECTORIES(sft PRIVATE lib/tweetnacl)

# install
INSTALL(TARGETS sft DESTINATION bin)

