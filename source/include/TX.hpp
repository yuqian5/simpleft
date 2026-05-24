#ifndef FT_TX_HPP
#define FT_TX_HPP

#include <arpa/inet.h>ß

#include "cli/CmdArgs.hpp"
#include "Transceiver.hpp"
#include "Logging.hpp"

class TX : protected Transceiver {
public:
    explicit TX(CMD_ARGS cmd);

    ~TX();

private:
    CMD_ARGS cmdArgs;

    int connectFd;

    struct sockaddr_in6 serverAddr6;
    struct sockaddr_in serverAddr4;
    std::string filePath;

    void socketSetup(); // setup and connect to RX side

    void transmit() const; // sending the file

    static void connectNow(sockaddr_in6 &serverAddr6, int &connectFd);

    static void connectNow(sockaddr_in &serverAddr4, int &connectFd);
};

#endif //FT_TX_HPP

