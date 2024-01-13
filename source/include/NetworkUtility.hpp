#ifndef SFT_NETWORKUTILITY_HPP
#define SFT_NETWORKUTILITY_HPP

#include <cstdio>
#include <sys/socket.h>
#include <stdexcept>

class NetworkUtility {
public:
    static bool recvAll(int socketFd, unsigned char *buffer, size_t bufferSize, size_t bytes);

    static bool sendAll(int socketFd, unsigned char *buffer, size_t bufferSize, size_t bytes = 0);
};


#endif //SFT_NETWORKUTILITY_HPP
