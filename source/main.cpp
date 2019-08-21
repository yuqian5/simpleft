#include <iostream>
#include <cstring>
#include <stdexcept>
#include "TX.h"
#include "RX.h"
#include "misc.h"

using namespace std;

/*
 * command line argument template:
 * receive:
 *      ./lanft -rx -ip [your IP addr] -p [port to receive from]
 * send:
 *      ./lanft -tx -ip [receive side IP] -p [port to send to] [path to file]
 */


// process usr input, if invalid input found, print error message and exit
// if all input are valid, convert and load input into struct CMDARGS and return it


int main(int argc, char* argv[]) {
    //get and sanitize cmd arguments store in struct CMDARGS
    CMDARGS info = parseCMDInput(argv, argc);

    //mode 1 = rx, 2 = tx
    if(info.mode == 1){
        cout << "Receiving on port " << info.port << endl;
        RX rx(info);
    }else{
        cout << "Sending " << info.filePath << " to " << info.ip << " on port " << info.port << endl;
        TX tx(info);
    }

    return 0;
}