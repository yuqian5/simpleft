# simpleft
[![CMake on linux and macos](https://github.com/yuqian5/simpleft/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=master)](https://github.com/yuqian5/simpleft/actions/workflows/cmake-multi-platform.yml)

## Menu
- [About](#description)
- [Install](#install)
    - [Homebrew](#homebrew)
    - [Compile from source](#compile-from-source)
- [Usage](#usage)

## Description
**simpleft** is a lightweight command-line tool designed to simplify p2p file sharing. No server, no configurations, no hassle.

## Install
### Homebrew
* 
        brew tap yuqian5/simpleft
        brew install simpleft

### Compile from source
* 
        cmake CMakeLists.txt
        make
        make install

## Usage
- To receive files:
    ```
    sft rx -p [port]
    ```
- To send files over IPv4:
    ```
    sft tx -ip4 [IPv4 address] -p [port] [path to file]
    ```
- To send files over IPv6:
    ```
    sft tx -ip6 [IPv6 address] -p [port] [path to file]
    ```

Note: The port and IP address are optional parameters. If not specified, the program will default to port 8203 and IP address 127.0.0.1 (localhost).

