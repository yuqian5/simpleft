#include <iostream>
#include <cstring>
#include <stdexcept>
#include <regex>

#include "TX.h"
#include "RX.h"
using namespace std;

#include "mystruct.h"

/*
 * command line argument template:
 * receive:
 *      ./ftp -rx -ip [your IP addr] -p [port to receive from]
 * send:
 *      ./ftp -tx -ip [receive side IP] -p [port to send to]
 */

// check port number, if legal, return port number, else print error and exit
int checkPort(const string& port_str){
    int port;
    try{
        port = stoi(port_str);
        if(port < 1023 || port > 65535){
            throw range_error("port number out of range 1024<=port<=65535");
        }
    }
    catch(const std::invalid_argument& em){
        std::cerr << "Invalid argument: " << "port number must be a int in base 10" << std::endl;
        exit(1);
    }
    catch(const std::range_error& em) {
        std::cerr << "Invalid argument: " << em.what() << std::endl;
        exit(1);
    }
    return port;
}

// check IP addr, if valid, return true, else print error and exit
bool checkIP(const string& IP_str){
    try{
        if (regex_match(IP_str, std::regex("\\b(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\b"))){ // validate ip
            return true;
        }
        throw std::domain_error("IP address inputted not valid");
    }
    catch(domain_error& em){
        std::cerr << "Input Error: " << em.what() << std::endl;
        exit(1);
    }

}

// process usr input, if invalid input found, print error message and exit
// if all input are valid, convert and load input into struct CMDARGS and return it
CMDARGS usrInputValid(char* cmd[], int argc){
    // basic commandline format check
    if(argc == 6){
        if(strcmp(cmd[1],"-rx")==0||strcmp(cmd[1],"-tx")==0){
            if(strcmp(cmd[2],"-ip")==0){
                if(strcmp(cmd[4],"-p")==0){
                    goto LOADARG;
                }
            }
        }
    }
    std::cerr << "Invalid argument: Please see README.md for proper commandline argument usage" << endl;
    exit(1);

    // convert and load arguments
    LOADARG:
        CMDARGS info = {};
        if(strcmp(cmd[1], "-rx") == 0){
            info.mode = 1;
        }else if(strcmp(cmd[1], "-tx") == 0){
            info.mode = 2;
        }
        info.port = checkPort(cmd[5]);
        if(checkIP(cmd[3])){
            info.ip = cmd[3];
        }

        return info;
}

int main(int argc, char* argv[]) {
    CMDARGS info = usrInputValid(argv, argc);

    if(info.mode == 1){
        cout << "Receiving on port " << info.port << endl;
        RX rx(info);
    }else{
        cout << "Sending to " << info.ip << " on port " << info.port << endl;
        TX tx(info);
    }

    return 0;
}