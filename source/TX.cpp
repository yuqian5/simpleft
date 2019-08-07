#include "TX.h"
#include <netdb.h>
#include <string.h>


void TX::socketSetup() {
    //struct setup
    address.sin_family = AF_INET;
    address.sin_port = htons(info.port);
    if(inet_pton(AF_INET, info.ip.c_str(), &(address.sin_addr)) != 1){
        std::cerr << "inet_pton failed" << std::endl;
        exit(1);
    }

    //init socket
    txSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(txSocket < 0){
        std::cerr << "socket() failed" << std::endl;
        exit(1);
    }
    //connect
    if(connect(txSocket, (struct sockaddr *)&address, sizeof(address)) < 0){
        std::cerr << "connection failed " << errno << std::endl;
        exit(1);
    }
    std::cout << "Socket Connected" << std::endl;
}

void TX::transmit() {
    char *hello = "Hello from client";
    char buffer[1024] = {0};

    send(txSocket , hello , strlen(hello) , 0 );
}


