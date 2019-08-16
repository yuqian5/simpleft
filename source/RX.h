#include <stdio.h>
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
#include <netdb.h>

using namespace std;

#include "misc.h"

#define BACKLOG 10

class RX {
public:
    explicit RX(CMDARGS cmd) {
        // copy commandline arguments
        info = std::move(cmd);

        socketSetup();
        receive();
    }

    ~RX() {
        shutdown(rxSocket, 0);
        shutdown(newRxSocket, 0);
    }

private:
    CMDARGS info;
    int rxSocket, newRxSocket;
    struct sockaddr_in address;
    struct sockaddr_storage receive_storage;

    // setup, then listen and accept connection from TX side
    void socketSetup();

    // receive file name, file sha, file data. Write file data to a file created with the same file name
    void receive();

    // generates sharsum of the file created, compare it to the shasum received
    static bool verify(const string &fileName, const string &sum);
};
