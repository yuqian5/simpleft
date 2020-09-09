# LANFT

## About
Transfers a file/directory to another machine connected to LAN using TCP socket. The integrity of the file is checked with SHA265 automatically.

## Install/Uninstall
* Compile and Install

        cmake CMakeLists.txt
        make
        make install

## Usage
 * receive:
 *      lanft rx -p [port]
 * send over ipv4:
 *      lanft tx -ip4 [ipv4 address] -p [port] [path to file]
 * send over ipv6:
 *      lanft tx -ip6 [ipv4 address] -p [port] [path to file]
 
## Compatibility
 *      Most Linux
 *      MacOS
