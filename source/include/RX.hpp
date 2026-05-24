#ifndef FT_RX_HPP
#define FT_RX_HPP

#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

#include "cli/CmdArgs.hpp"
#include "Transceiver.hpp"
#include "Crypto.hpp"
#include "Logging.hpp"
#include "NetworkUtility.hpp"
#include "sft_constants.hpp"

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

    unsigned char sharedKey[SHARED_KEY_SIZE]{};

    /**
     * setup, then listen and accept connection from TX side
     */
    void socketSetup();

    /**
     * exchange pubkeys with TX and derive sharedKey
     */
    void handshake();

    /**
     * receive file name, file sha, file data. Write file data to a file created with the same file name
     */
    void receive();
};

#endif //FT_RX_HPP
