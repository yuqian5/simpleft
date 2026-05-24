cmake_minimum_required(VERSION 3.16)
project(sft
        VERSION 0.1.0
        DESCRIPTION "Peer-to-peer encrypted file transfer"
        LANGUAGES CXX C)

# Strict C++20: no GNU extensions (-std=c++20, not -std=gnu++20), and the
# build fails outright on a compiler that can't actually deliver C++20
# rather than silently falling back.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# `tar` is invoked at runtime via fork+execvp (see Transceiver::packFile)
# and must exist on PATH on both sides. Failing at configure time is far
# better than discovering this at transfer time.
find_program(TAR tar)
if(NOT TAR)
    message(FATAL_ERROR "tar not found in PATH (required at runtime by Transceiver::packFile)")
endif()

# --- Crypto backend selection ------------------------------------------------
# Each backend lives in src/crypto/Crypto_<name>.cpp and implements every
# method declared in include/Crypto.hpp. To add a fourth backend, drop in
# a new .cpp and extend the conditional below.
#
# Default selection: libsodium if a system copy is detected AND the host is
# x86_64. Why arch-gated: libsodium ships hand-tuned SSE2/AVX2/AVX-512 paths
# for ChaCha20 + Poly1305 on x86 and dominates there (~2+ GB/s), but on
# AArch64 (Apple Silicon, ARM servers) it falls back to the portable C
# reference implementations - no NEON paths shipped in 1.0.x. On those
# hosts monocypher wins by ~10% because it gets compiled into the binary
# with full LTO instead of going through a dylib. Users can always override
# with -DSFT_CRYPTO_BACKEND=...

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SFT_PROBE_LIBSODIUM QUIET libsodium)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    set(_sft_host_has_libsodium_simd TRUE)
else()
    set(_sft_host_has_libsodium_simd FALSE)
endif()

if(SFT_PROBE_LIBSODIUM_FOUND AND _sft_host_has_libsodium_simd)
    set(_sft_default_backend "libsodium")
    message(STATUS "sft: detected system libsodium ${SFT_PROBE_LIBSODIUM_VERSION} on x86_64 - default backend is libsodium")
elseif(SFT_PROBE_LIBSODIUM_FOUND)
    set(_sft_default_backend "monocypher")
    message(STATUS "sft: detected system libsodium ${SFT_PROBE_LIBSODIUM_VERSION}, but host is ${CMAKE_SYSTEM_PROCESSOR} where libsodium has no SIMD path - defaulting to monocypher (faster here); override with -DSFT_CRYPTO_BACKEND=libsodium if you want it anyway")
else()
    set(_sft_default_backend "monocypher")
endif()

set(SFT_CRYPTO_BACKEND "${_sft_default_backend}" CACHE STRING
        "Crypto backend to link (monocypher | libsodium)")
set_property(CACHE SFT_CRYPTO_BACKEND PROPERTY STRINGS monocypher libsodium)

# Per-backend outputs consumed below by add_executable / target_*.
set(SFT_CRYPTO_SOURCES)
set(SFT_CRYPTO_INCLUDE_DIRS)
set(SFT_CRYPTO_LINK_TARGETS)

if(SFT_CRYPTO_BACKEND STREQUAL "monocypher")
    set(SFT_CRYPTO_SOURCES
            src/crypto/Crypto_monocypher.cpp
            lib/monocypher/monocypher.c)
    set(SFT_CRYPTO_INCLUDE_DIRS lib/monocypher)

elseif(SFT_CRYPTO_BACKEND STREQUAL "libsodium")
    # Only backend that uses a system-installed library. Vendoring
    # libsodium would dwarf this codebase; using the system copy is the
    # pragmatic compromise. IMPORTED_TARGET (CMake 3.6+) gives us a
    # `PkgConfig::LIBSODIUM` target that carries headers, link search
    # paths, and the -lsodium flag - no FIND_LIBRARY dance needed.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSODIUM REQUIRED IMPORTED_TARGET libsodium)
    set(SFT_CRYPTO_SOURCES src/crypto/Crypto_libsodium.cpp)
    set(SFT_CRYPTO_LINK_TARGETS PkgConfig::LIBSODIUM)

else()
    message(FATAL_ERROR
            "Unknown SFT_CRYPTO_BACKEND: ${SFT_CRYPTO_BACKEND} "
            "(expected monocypher or libsodium)")
endif()

message(STATUS "sft crypto backend: ${SFT_CRYPTO_BACKEND}")

# --- Target ------------------------------------------------------------------
# Headers are intentionally not enumerated here - target_include_directories
# below makes them visible to source files, and most IDEs index them
# automatically through the include path.

add_executable(sft
        src/main.cpp
        src/TX.cpp
        src/RX.cpp
        src/Transceiver.cpp
        src/Packet.cpp
        src/Logging.cpp
        src/NetworkUtility.cpp
        src/cli/ArgParser.cpp
        src/cli/IpFlag.cpp
        src/cli/PortFlag.cpp
        src/cli/PassphraseFlag.cpp
        ${SFT_CRYPTO_SOURCES})

target_include_directories(sft PRIVATE
        include
        ${SFT_CRYPTO_INCLUDE_DIRS})

# Per-target warnings, scoped by build config via generator expressions.
# Debug builds get strict warnings; Release stays quiet so vendored C in
# lib/monocypher and lib/tweetnacl doesn't drown the output.
target_compile_options(sft PRIVATE
        $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:CXX>>:-Wall -Wextra>
        $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C>>:-Wall>)

if(SFT_CRYPTO_LINK_TARGETS)
    target_link_libraries(sft PRIVATE ${SFT_CRYPTO_LINK_TARGETS})
endif()

# Use GNUInstallDirs so `cmake --install` honours distro/host conventions
# (e.g. /usr/bin vs /usr/local/bin) instead of always landing in <prefix>/bin.
include(GNUInstallDirs)
install(TARGETS sft RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
