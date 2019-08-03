cmake_minimum_required(VERSION 3.14)
project(LANFileTransfer)

set(CMAKE_CXX_STANDARD 17)

add_executable(LANFileTransfer main.cpp mystruct.h TX.cpp TX.h)