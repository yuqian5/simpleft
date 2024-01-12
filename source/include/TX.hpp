#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

#include "misc.hpp"
#include "Transceiver.hpp"

#ifndef FT_TX_HPP
#define FT_TX_HPP

class TX : protected Transceiver {
public:
    explicit TX(CMD_ARGS cmd);

    ~TX();

private:
    const size_t MAX_PACKET_SIZE = 1500;

    CMD_ARGS cmdArgs;

    int connectFd;

    struct sockaddr_in6 serverAddr6;
    struct sockaddr_in serverAddr4;
    std::string filePath;

    void socketSetup(); // setup and connect to RX side
    void transmit(); // sending the file

    static void connectNow(sockaddr_in6 &serverAddr6, int &connectFd);

    static void connectNow(sockaddr_in &serverAddr4, int &connectFd);
};

#endif //FT_TX_HPP

