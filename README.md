# LANFileTransfer

## About
This program transfers a file/directory of your choosing to another machine connected to LAN using TCP socket. The integrity of the file is checked with SHA265 automatically.

## Install/Uninstall
* Compile and Install

        make
        make install
    
* Uninstall

        make uninstall
        
* Default install path:
    
        /usr/local/bin/
    
    


## Usage
 * receive:
 *      lanft -rx -ip [your IP addr] -p [port]
 * send:
 *      lanft -tx -ip [receiver side IP addr] -p [port] [path/file]
 * send directory:
 *      lanft -tx -d -ip [receiver side IP addr] -p [port] [path/file]
 * note:
 *      -ip, -p and -tx/-rx is always required to run this CLI program
 
## Compatibility
This program will run on **MacOS** and **most Linux** without a hitch. Unfortunately, it is 100% not supported on window. That is, unless your are using WSL. 