#include "../include/NetworkUtility.hpp"

bool NetworkUtility::recvAll(int socketFd, unsigned char *buffer, size_t bufferSize, size_t bytes) {
    if (bufferSize < bytes) {
        throw std::runtime_error("Requiring more bytes than the buffer size");
    }

    size_t byteReceived = 0;
    while(byteReceived != bytes) {
        size_t count = recv(socketFd, buffer + byteReceived, bytes - byteReceived, 0);
        if (count == 0) {
            return false;
        }
        byteReceived += count;
    }

    return true;
}

bool NetworkUtility::sendAll(int socketFd, unsigned char *buffer, size_t bufferSize, size_t bytes) {
    if (bytes == 0) {
        bytes = bufferSize;
    }

    size_t byteSent = 0;
    while(byteSent != bytes) {
        size_t count = send(socketFd, buffer + byteSent, bytes - byteSent, 0);
        if (count == 0) {
            return false;
        }
        byteSent += count;
    }

    return true;
}
