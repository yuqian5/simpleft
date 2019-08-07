#include "RX.h"
#include <netdb.h>

#define BACKLOG 10


void RX::socketSetup() {
    //struct setup
    address.sin_family = AF_INET;
    address.sin_port = htons(info.port);
    if(inet_pton(AF_INET, info.ip.c_str(), &(address.sin_addr)) != 1){
        std::cerr << "inet_pton failed" << std::endl;
        exit(1);
    }

    //init socket
    rxSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(rxSocket < 0){
        std::cerr << "socket() failed" << std::endl;
        exit(1);
    }

    //bind socket
    if(bind(rxSocket, (struct sockaddr *)&address, sizeof(address))<0){
        std::cerr << "bind failed" << std::endl;
        exit(1);
    }

    //listen
    if(listen(rxSocket, BACKLOG) != 0){
        std::cerr << "listen failed" << std::endl;
        exit(1);
    }

    //accept
    socklen_t addr_size = sizeof receive_storage;
    newRxSocket = accept(rxSocket, (struct sockaddr *)&receive_storage, &addr_size);
    std::cout << "Socket Connected" << std::endl;
}

void RX::receive() {
    char a[1000];
    read(newRxSocket, a, sizeof(a));
    cout << a << endl;
}
