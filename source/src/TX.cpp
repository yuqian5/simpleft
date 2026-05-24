#include "../include/TX.hpp"

TX::TX(CMD_ARGS cmd) {
    // copy commandline arguments
    cmdArgs = std::move(cmd);
    filePath = cmdArgs.filePath;

    socketSetup();
    handshake();
    transmit();
}

TX::~TX() {
    // wipe key material before freeing the object's storage
    Crypto::zeroize(this->sharedKey, sizeof(this->sharedKey));
    shutdown(this->connectFd, O_RDWR);
    close(this->connectFd);
}

// TX side of the handshake. RX writes its public key first, so we recv
// before generating our own keypair - that way we don't waste entropy
// generating an ephemeral key just to discover the peer hung up.
void TX::handshake() {
    Logging::logInfo("Performing handshake");

    unsigned char peerPk[PUBKEY_SIZE];
    if (!NetworkUtility::recvAll(this->connectFd, peerPk, PUBKEY_SIZE, PUBKEY_SIZE)) {
        Logging::logError("Handshake failed - did not receive peer public key");
        exit(1);
    }

    unsigned char myPk[PUBKEY_SIZE];
    unsigned char mySk[SECRETKEY_SIZE];
    Crypto::generateKeypair(myPk, mySk);

    if (!NetworkUtility::sendAll(this->connectFd, myPk, PUBKEY_SIZE)) {
        Crypto::zeroize(mySk, sizeof(mySk));
        Logging::logError("Handshake failed - could not send public key");
        exit(1);
    }

    Crypto::deriveSharedKey(mySk, peerPk, cmdArgs.passphrase, this->sharedKey);
    Crypto::zeroize(mySk, sizeof(mySk));

    Logging::logInfo("Handshake complete");
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
        if (connect(connectFd, reinterpret_cast<sockaddr*>(&serverAddr6), sizeof(serverAddr6)) < 0) {
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
        memset(&this->serverAddr4, 0, sizeof(this->serverAddr4));

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
        memset(&this->serverAddr6, 0, sizeof(this->serverAddr6));

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

void TX::transmit() const {
    // pack file into tar ball
    Logging::logInfo("Packaging file(s)");
    if (!packFile(cmdArgs.filePath)) {
        Logging::logError("No such file or directory");
        exit(0);
    }

    char readReceiptBuffer[READ_RECEIPT_SIZE];

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
    unsigned char outboundDataBuffer[MAX_PACKET_SIZE] = {};

    size_t fileSize = fileInfo.st_size;

    while (fileSize != 0) {
        // read chunk from file
        auto readSize = MAX_PACKET_SIZE > fileSize ? fileSize : MAX_PACKET_SIZE;
        fread(outboundDataBuffer, readSize, 1, inputFd);
        fileSize -= readSize;

        // encrypt + frame this chunk
        size_t frameLen = 0;
        auto frame = Packet::serialize(outboundDataBuffer, readSize, this->sharedKey, frameLen);
        memset(outboundDataBuffer, 0, sizeof(outboundDataBuffer)); // reset outboundDataBuffer

        // send packet
        auto success = NetworkUtility::sendAll(this->connectFd, frame.get(), frameLen);
        if (!success) {
            Logging::logError("Fatal error - Broken Socket");
            exit(1);
        }

        // wait for read receipt
        recv(this->connectFd, readReceiptBuffer, READ_RECEIPT_SIZE, 0);

        // update progress bar
        Logging::logProgress(fileInfo.st_size - fileSize, fileInfo.st_size, false);
    }

    // end-of-stream marker: encrypted empty-payload frame. Authenticated by
    // the same MAC mechanism as data frames, so an attacker cannot inject a
    // fake one to truncate the transfer.
    size_t termLen = 0;
    auto terminationPacket = Packet::generateTerminationPacket(this->sharedKey, termLen);
    NetworkUtility::sendAll(this->connectFd, terminationPacket.get(), termLen);

    // log complete
    Logging::logProgress(1, 1, true);

    // close file
    fclose(inputFd);

    // remove file
    deletePackedFile();
}

