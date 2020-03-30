#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "misc.h"
#include "Transceiver.hpp"

#ifndef LANFT_TX_HPP
#define LANFT_TX_HPP

class TX : protected Transceiver {
public:
    explicit TX(CMDARGS cmd);

    ~TX();

private:
    CMDARGS info;

    int connectfd;
    struct sockaddr_in6 serverAddr6;
    struct sockaddr_in serverAddr4;
    std::string filePath;

    void socketSetup(); // setup and connect to RX side
    void transmit(); // sending the file
    static void connectNow(sockaddr_in6 &serverAddr6, int &connectfd);

    static void connectNow(sockaddr_in &serverAddr4, int &connectfd);

    std::string extractFilename(); // extract file name from file path if needed
};

#endif //LANFT_TX_HPP

