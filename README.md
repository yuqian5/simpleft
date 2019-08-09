# LANFileTransfer

##Install/Uninstall
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
This program will run on MacOS and Ubuntu without a hitch. Unfortunately, it is 100% not supported on window. That is, unless your are using WSL. 