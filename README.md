# LANFT

## About
**lanft** is a lightweight command-line tool designed to simplify p2p file sharing within a network environment. 
It does not require a centralized server or any complex configuration. It is designed to be simple and easy to use.

## Install/Uninstall
* Compile and Install
* 
        cmake CMakeLists.txt
        make
        make install

## Usage
- To receive files:
    ```
    lanft rx -p [port]
    ```
- To send files over IPv4:
    ```
    lanft tx -ip4 [IPv4 address] -p [port] [path to file]
    ```
- To send files over IPv6:
    ```
    lanft tx -ip6 [IPv6 address] -p [port] [path to file]
    ```

Note: The port and IP address are optional parameters. If not specified, the program will default to port 8203 and IP address 127.0.0.1 (localhost).

