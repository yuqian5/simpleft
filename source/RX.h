#include <fstream>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <cstdlib>
#include <utility>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
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
        shutdown(rxSocket, 0);
        shutdown(newRxSocket, 0);
    }

private:
    CMDARGS info;
    int rxSocket, newRxSocket;
    struct sockaddr_in address;
    struct sockaddr_storage receive_storage;


    void socketSetup(); // setup, then listen and accept connection from TX side
    void receive();
    static bool verify(string fileName, string shasum);
};
