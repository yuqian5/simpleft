#include <fstream>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#include "misc.h"

class TX {
public:
    explicit TX(CMDARGS cmd) {
        // copy commandline arguments
        info = std::move(cmd);
        fileName = info.filePath;

        socketSetup();
        transmit();
    }

    ~TX() {
        shutdown(txSocket, 0);
    }

private:
    CMDARGS info;
    int txSocket;
    struct sockaddr_in address;
    string fileName;

    void socketSetup(); // setup and connect to RX side
    void transmit();
};

