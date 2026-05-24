#ifndef FT_RX_HPP
#define FT_RX_HPP

#include <sys/socket.h>
#include <arpa/inet.h>ß

#include "cli/CmdArgs.hpp"
#include "Transceiver.hpp"

class RX : protected Transceiver {
public:
    explicit RX(CMD_ARGS &cmd);

    ~RX();

private:
    CMD_ARGS cmdArgs;

    int listenFd;
    int connectFd;

    struct sockaddr_in6 serverAddr;
    struct sockaddr_storage dataStorage;

    /**
     * setup, then listen and accept connection from TX side
     */
    void socketSetup();

    /**
     * receive file name, file sha, file data. Write file data to a file created with the same file name
     */
    void receive();
};

#endif //FT_RX_HPP
