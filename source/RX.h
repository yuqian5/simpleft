#include <fstream>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <cstdlib>
#include <utility>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

#include "mystruct.h"

class RX {
public:
    explicit RX(CMDARGS cmd){
        // copy commandline arguments
        info = std::move(cmd);

        socketSetup();
        receive();
    }

    ~RX(){
        close(rxSocket);
    }

private:
    CMDARGS info;
    int rxSocket;
    struct sockaddr_in address;
    struct sockaddr_storage receive_storage;


    void socketSetup();
    void receive();
};
