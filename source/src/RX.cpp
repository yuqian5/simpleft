#include "../include/RX.hpp"

RX::RX(CMD_ARGS &cmd) {
    // copy commandline arguments
    this->cmdArgs = cmd;

    // initialize socket
    socketSetup();

    // crypto handshake before any data flows
    handshake();

    // receive file
    receive();
}

RX::~RX() {
    // wipe key material before freeing the object's storage
    Crypto::zeroize(this->sharedKey, sizeof(this->sharedKey));
    // shutdown sockets
    shutdown(this->listenFd, O_RDWR);
    close(this->listenFd);
    shutdown(this->connectFd, O_RDWR);
    close(this->connectFd);
}

// RX side of the handshake. RX writes its public key first because it's
// the side that accepted the connection — the caller (TX) is waiting on
// us to make the first move, mirroring the TCP roles.
void RX::handshake() {
    Logging::logInfo("Performing handshake");

    unsigned char myPk[PUBKEY_SIZE];
    unsigned char mySk[SECRETKEY_SIZE];
    Crypto::generateKeypair(myPk, mySk);

    if (!NetworkUtility::sendAll(this->connectFd, myPk, PUBKEY_SIZE)) {
        Crypto::zeroize(mySk, sizeof(mySk));
        Logging::logError("Handshake failed - could not send public key");
        exit(1);
    }

    unsigned char peerPk[PUBKEY_SIZE];
    if (!NetworkUtility::recvAll(this->connectFd, peerPk, PUBKEY_SIZE, PUBKEY_SIZE)) {
        Crypto::zeroize(mySk, sizeof(mySk));
        Logging::logError("Handshake failed - did not receive peer public key");
        exit(1);
    }

    Crypto::deriveSharedKey(mySk, peerPk, cmdArgs.passphrase, this->sharedKey);
    Crypto::zeroize(mySk, sizeof(mySk));

    Logging::logInfo("Handshake complete");
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

    //receive until the peer sends an encrypted empty-payload frame
    while (true) {
        // receive packet size header
        auto success = NetworkUtility::recvAll(this->connectFd, packetSizeBuf, PACKET_HEADER_SIZE, PACKET_HEADER_SIZE);
        if (!success) {
            Logging::logError("Fatal Error - Packet Header Lost");
            exit(1);
        }

        // length covers nonce + MAC + ciphertext; the smallest valid frame
        // is an empty payload (CRYPTO_OVERHEAD bytes)
        unsigned int packetSize = (packetSizeBuf[0] << 24) | (packetSizeBuf[1] << 16) | (packetSizeBuf[2] << 8) | packetSizeBuf[3];
        if (packetSize < CRYPTO_OVERHEAD || packetSize > MAX_PACKET_SIZE + CRYPTO_OVERHEAD) {
            Logging::logError("Fatal Error - implausible packet size");
            exit(1);
        }

        // receive the encrypted body
        auto packetBuf = std::make_unique<unsigned char[]>(packetSize);
        success = NetworkUtility::recvAll(this->connectFd, packetBuf.get(), packetSize, packetSize);
        if (!success) {
            Logging::logError("Fatal Error - Broken Socket");
            exit(1);
        }

        // decrypt + verify MAC. A failure here means either wire tampering
        // or a passphrase mismatch — both are fatal.
        size_t plainLen = 0;
        auto plain = Crypto::decryptPacket(packetBuf.get(), packetSize, this->sharedKey, plainLen);
        if (!plain) {
            Logging::logError("Fatal Error - decryption failed (wrong passphrase or tampered data)");
            exit(1);
        }

        // empty plaintext == end-of-stream marker
        if (plainLen == 0) {
            Logging::logInfo("File received");
            break;
        }

        // send read receipt (plaintext lock-step pacing signal; not security-critical)
        send(this->connectFd, READ_RECEIPT, READ_RECEIPT_SIZE, 0);

        // write decrypted payload to the buffer file
        fwrite(plain.get(), plainLen, 1, fdout);
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





