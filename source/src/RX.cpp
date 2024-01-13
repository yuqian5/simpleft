#include "../include/RX.hpp"

RX::RX(CMD_ARGS &cmd) {
    // copy commandline arguments
    this->cmdArgs = cmd;

    // initialize socket
    socketSetup();

    // receive file
    receive();
}

RX::~RX() {
    // shutdown sockets
    shutdown(this->listenFd, O_RDWR);
    close(this->listenFd);
    shutdown(this->connectFd, O_RDWR);
    close(this->connectFd);
}

void RX::socketSetup() {
    // init socket
    this->listenFd = socket(AF_INET6, SOCK_STREAM, 0);
    memset(&this->serverAddr, 0, sizeof(this->serverAddr));

    this->serverAddr.sin6_family = AF_INET6;
    this->serverAddr.sin6_port = htons(this->cmdArgs.port);
    this->serverAddr.sin6_addr = in6addr_any;

    // set to accept ipv4 protocol
    int no = 0;
    setsockopt(this->listenFd, IPPROTO_IPV6, IPV6_V6ONLY, (void *) &no, sizeof(no));

    //bind socket
    int result = bind(this->listenFd, (struct sockaddr *) &this->serverAddr, sizeof(this->serverAddr));
    if (result < 0) {
        Logging::logError("Failed to bind socket");
        exit(1);
    }

    //listen
    listen(this->listenFd, BACKLOG);
    Logging::logInfo("Listening for connection");

    //accept
    socklen_t addrSize = sizeof dataStorage;
    this->connectFd = accept(this->listenFd, (struct sockaddr *) &this->dataStorage, &addrSize);
    Logging::logInfo("Connected");
}

void RX::receive() {
    // log receiving
    Logging::logInfo("Receiving package");

    unsigned char packetSizeBuf[PACKET_HEADER_SIZE];
    memset(packetSizeBuf, 0, sizeof(packetSizeBuf));

    //create file using the fileName received from socket
    FILE *fdout;
    fdout = fopen(".ft_temp_pack_buffer.tar.gz", "wb");
    if (fdout == nullptr) {
        Logging::logError("Unable to create buffer file");
        exit(1);
    }

    //receive until the other side does an orderly shutdown
    while (true) {
        // receive packet size header
        auto result = NetworkUtility::recvAll(this->connectFd, packetSizeBuf, PACKET_HEADER_SIZE, PACKET_HEADER_SIZE);
        if (result) {
            Logging::logError("Fatal Error - Packet Header Lost");
            exit(1);
        }

        // convert packet size header to int
        unsigned int packetSize = (packetSizeBuf[0] << 24) | (packetSizeBuf[1] << 16) | (packetSizeBuf[2] << 8) | packetSizeBuf[3];
        if (packetSize == 0xFFFFFFFF) { // if packetSize is 0xFFFFFFFF, it means file transfer is complete
            Logging::logInfo("File received");
            break;
        }

        // receive packet
        unsigned char packetBuf[packetSize];
        result = NetworkUtility::recvAll(this->connectFd, packetBuf, packetSize, packetSize);
        if (result) {
            Logging::logError("Fatal Error - Broken Socket");
            exit(1);
        }

        // send read receipt
        send(this->connectFd, "OK", 2, 0);

        // write to received buffer to file
        fwrite(packetBuf, packetSize, 1, fdout);
    }

    // close file
    fclose(fdout);

    // un-package file
    Logging::logInfo("Unpacking File");
    unpackFile();

    // delete temp buffer file
    deletePackedBufferFile();

    // log complete
    Logging::logInfo("File transfer complete");
}





