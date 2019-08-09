# LANFileTransfer

## About
This program transfers a file of your choosing (up to 1TB) to another machine connected to LAN using TCP socket. The integrity of the file is checked with SHA265 automatically.

Please understand that the TCP protocol have relatively weak checksum. Therefore, an undetected corruption during the transfer is entirely possible. Hence, the larger the file, the more likely that it will get corrupted during the transfer.
 [Check here for more information](https://stackoverflow.com/questions/3830206/can-a-tcp-checksum-fail-to-detect-an-error-if-yes-how-is-this-dealt-with)

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
 *      lanft -rx -ip [your IP addr] -p [port to receive from]
 * send:
 *      lanft -tx -ip [receive side IP] -p [port to send to] [path to file]
 
## Compatibility
This program will run on **MacOS** and **most Linux** without a hitch. Unfortunately, it is 100% not supported on window. That is, unless your are using WSL. 