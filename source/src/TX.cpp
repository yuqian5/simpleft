#include <thread>
#include "../include/TX.hpp"
#include "../include/Packet.hpp"
#include "../include/Logging.hpp"

TX::TX(CMD_ARGS cmd) {
    // copy commandline arguments
    cmdArgs = std::move(cmd);
    filePath = cmdArgs.filePath;

    socketSetup();
    transmit();
}

TX::~TX() {
    shutdown(this->connectFd, O_RDWR);
    close(this->connectFd);
}

void TX::connectNow(sockaddr_in &serverAddr4, int &connectFd) {
    //connect, timeout after 600s (10min)
    int tries_left = 30;
    Logging::logInfo("Searching for receiver");

    while (tries_left > 0) {
        //struct setup
        if (connect(connectFd, (struct sockaddr *) &serverAddr4, sizeof(serverAddr4)) < 0) {
            tries_left -= 1;
            if (!tries_left) {
                Logging::logError("Connection failed, no receiver found");
                exit(1);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds (500));
            }
        } else {
            Logging::logInfo("Connected");
            break;
        }
    }
}

void TX::connectNow(sockaddr_in6 &serverAddr6, int &connectFd) {
    //connect, timeout after 600s (10min)
    int tries_left = 30;
    Logging::logInfo("Searching for receiver");

    while (tries_left > 0) {
        //struct setup
        if (connect(connectFd, (struct sockaddr *) &serverAddr6, sizeof(serverAddr6)) < 0) {
            tries_left -= 1;
            if (!tries_left) {
                Logging::logError("Connection failed, no receiver found");
                exit(1);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds (500));
            }
        } else {
            std::cout << "Connected" << std::endl;
            break;
        }
    }
}

void TX::socketSetup() {
    // check protocol standard
    if (cmdArgs.standard == "ipv4") { // ipv4
        // initialize socket
        this->connectFd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&this->serverAddr4, '0', sizeof(this->serverAddr4));

        this->serverAddr4.sin_family = AF_INET;
        this->serverAddr4.sin_port = htons(cmdArgs.port);
        if (inet_pton(AF_INET, this->cmdArgs.ip.c_str(), &(this->serverAddr4.sin_addr)) != 1) {
            Logging::logError("Connection failed, inet_pton error");
            exit(1);
        }
        // connect
        connectNow(std::ref(this->serverAddr4), std::ref(this->connectFd));
    } else { // ipv6
        // initialize socket
        this->connectFd = socket(AF_INET6, SOCK_STREAM, 0);
        memset(&this->serverAddr6, '0', sizeof(this->serverAddr6));

        this->serverAddr6.sin6_family = AF_INET6;
        this->serverAddr6.sin6_port = htons(this->cmdArgs.port);
        if (inet_pton(AF_INET6, this->cmdArgs.ip.c_str(), &(this->serverAddr6.sin6_addr)) != 1) {
            Logging::logError("Connection failed, inet_pton error");
            exit(1);
        }
        // connect
        connectNow(std::ref(this->serverAddr6), std::ref(this->connectFd));
    }
}

void TX::transmit() {
    // pack file
    Logging::logInfo("Packaging file(s)");
    if (!packFile(cmdArgs.filePath)) {
        Logging::logError("No such file or directory");
        exit(0);
    }

    char inboundDataBuffer[16];
    memset(inboundDataBuffer, 0, sizeof(inboundDataBuffer));

    // get file cmdArgs
    FILE *inputFd;

    struct stat fileInfo = {};
    if (stat(".ft_temp_pack.tar.gz", &fileInfo) == 0) {
        //open file
        inputFd = fopen(".ft_temp_pack.tar.gz", "rb");
        if (inputFd == nullptr) {
            Logging::logError("Unable to open packed file");
            exit(1);
        }
    } else {
        Logging::logError("Unable to gather packaged file statistics");
        exit(1);
    }

    // send file
    Logging::logInfo("Sending file(s)");
    unsigned char outboundDataBuffer[2048];
    memset(outboundDataBuffer, 0, sizeof(outboundDataBuffer));

    size_t fileSize = fileInfo.st_size;

    int sentCount = 0;

    while (fileSize != 0) {
        // read chunk from file
        auto readSize = MAX_PACKET_SIZE > fileSize ? fileSize : MAX_PACKET_SIZE;
        fread(outboundDataBuffer, readSize, 1, inputFd);
        fileSize -= readSize;

        // generate packet
        auto packet = Packet::serialize(outboundDataBuffer, readSize);
        memset(outboundDataBuffer, 0, sizeof(outboundDataBuffer)); // reset outboundDataBuffer

        // send packet
        send(this->connectFd, packet.get(), readSize + 4, 0);

        // wait for read receipt
        recv(this->connectFd, inboundDataBuffer, 2, 0);
        memset(inboundDataBuffer, 0, sizeof(inboundDataBuffer));

        // update progress bar
        sentCount++;
        if (sentCount % 500 == 0) {
            Logging::logProgress(fileInfo.st_size - fileSize, fileInfo.st_size, false);
        }
    }

    // send end packet
    auto terminationPacket = Packet::generateTerminationPacket();
    send(this->connectFd, terminationPacket.get(), 4, 0);

    // log complete
    Logging::logProgress(1, 1, true);

    // close file
    fclose(inputFd);

    // remove file
    deletePackedFile();
}

