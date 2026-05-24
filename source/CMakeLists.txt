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

# Pick which crypto backend to compile. Each backend lives in
# src/crypto/Crypto_<name>.cpp and implements every method of Crypto.hpp.
# To add a new backend (e.g. monocypher, libsodium), drop in a new .cpp and
# extend the conditional below.
SET(SFT_CRYPTO_BACKEND "monocypher" CACHE STRING "Crypto backend to link (monocypher | tweetnacl)")
SET_PROPERTY(CACHE SFT_CRYPTO_BACKEND PROPERTY STRINGS monocypher tweetnacl)

IF (SFT_CRYPTO_BACKEND STREQUAL "monocypher")
    SET(SFT_CRYPTO_SOURCES
            src/crypto/Crypto_monocypher.cpp
            lib/monocypher/monocypher.c)
    SET(SFT_CRYPTO_INCLUDE_DIRS lib/monocypher)
ELSEIF (SFT_CRYPTO_BACKEND STREQUAL "tweetnacl")
    SET(SFT_CRYPTO_SOURCES
            src/crypto/Crypto_tweetnacl.cpp
            lib/tweetnacl/tweetnacl.c
            lib/tweetnacl/randombytes.c)
    SET(SFT_CRYPTO_INCLUDE_DIRS lib/tweetnacl)
ELSE ()
    MESSAGE(FATAL_ERROR "Unknown SFT_CRYPTO_BACKEND: ${SFT_CRYPTO_BACKEND} (expected monocypher or tweetnacl)")
ENDIF ()

MESSAGE(STATUS "sft crypto backend: ${SFT_CRYPTO_BACKEND}")

# add project file
ADD_EXECUTABLE(sft
        src/main.cpp
        include/TX.hpp src/TX.cpp
        include/RX.hpp src/RX.cpp
        include/Transceiver.hpp src/Transceiver.cpp
        include/Packet.hpp src/Packet.cpp
        include/Crypto.hpp
        include/Logging.hpp src/Logging.cpp
        include/NetworkUtility.hpp src/NetworkUtility.cpp
        include/sft_constants.hpp
        include/cli/CmdArgs.hpp
        include/cli/ArgParser.hpp        src/cli/ArgParser.cpp
        include/cli/IpFlag.hpp           src/cli/IpFlag.cpp
        include/cli/PortFlag.hpp         src/cli/PortFlag.cpp
        include/cli/PassphraseFlag.hpp   src/cli/PassphraseFlag.cpp
        ${SFT_CRYPTO_SOURCES}
)

TARGET_INCLUDE_DIRECTORIES(sft PRIVATE ${SFT_CRYPTO_INCLUDE_DIRS})

# install
INSTALL(TARGETS sft DESTINATION bin)

