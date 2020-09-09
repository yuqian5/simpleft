#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "misc.hpp"
#include "Transceiver.hpp"

#define BACKLOG 1

#ifndef LANFT_RX_HPP
#define LANFT_RX_HPP

class RX : protected Transceiver {
public:
    explicit RX(CMDARGS &cmd);

    ~RX();

private:
    CMDARGS info;
    int listenfd;
    int connectfd;
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

    /**
     * generates shasum of the file created, compare it to the shasum received
     * @param fileName std::string
     * @param sum std::string
     * @return true if sum match, false otherwise
     */
    bool verify(const std::string &sum);
};

#endif //LANFT_RX_HPP
