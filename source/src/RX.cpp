#include "../include/RX.hpp"

#include <memory>

RX::RX(CMD_ARGS &cmd) {
    this->cmdArgs = cmd;
    this->listenFd = -1;
    this->connectFd = -1;

    // One-time setup: bind and listen. The same listening socket is reused
    // across iterations in loop mode.
    setupListen();

    do {
        // Per-transfer: accept a connection, do the crypto handshake, then
        // receive the file. Any step returning false means "this peer didn't
        // work out"; we close the connection and either exit (one-shot mode)
        // or wait for the next peer (loop mode).
        bool ok = acceptConnection() && handshake() && receive();
        closeConnection();

        if (!ok) {
            if (!cmdArgs.loop) {
                exit(1);
            }
            Logging::logWarning("Transfer aborted; waiting for next connection");
        } else if (cmdArgs.loop) {
            Logging::logInfo("Waiting for next transfer (Ctrl-C to stop)");
        }
    } while (cmdArgs.loop);
}

RX::~RX() {
    // wipe key material before freeing the object's storage
    Crypto::zeroize(this->sharedKey, sizeof(this->sharedKey));

    if (this->listenFd != -1) {
        shutdown(this->listenFd, O_RDWR);
        close(this->listenFd);
    }
    if (this->connectFd != -1) {
        shutdown(this->connectFd, O_RDWR);
        close(this->connectFd);
    }
}

void RX::setupListen() {
    this->listenFd = socket(AF_INET6, SOCK_STREAM, 0);
    memset(&this->serverAddr, 0, sizeof(this->serverAddr));

    this->serverAddr.sin6_family = AF_INET6;
    this->serverAddr.sin6_port = htons(this->cmdArgs.port);
    this->serverAddr.sin6_addr = in6addr_any;

    // accept ipv4 too
    int no = 0;
    setsockopt(this->listenFd, IPPROTO_IPV6, IPV6_V6ONLY, (void *) &no, sizeof(no));

    // Allow restart while a previous binding is still in TIME_WAIT - matters
    // most for -loop where the user may want to bounce the process and bring
    // it back up immediately on the same port.
    int yes = 1;
    setsockopt(this->listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    int result = bind(this->listenFd, (struct sockaddr *) &this->serverAddr, sizeof(this->serverAddr));
    if (result < 0) {
        Logging::logError("Failed to bind socket");
        exit(1);
    }

    listen(this->listenFd, BACKLOG);
    Logging::logInfo("Listening for connection");
}

bool RX::acceptConnection() {
    socklen_t addrSize = sizeof dataStorage;
    this->connectFd = accept(this->listenFd, (struct sockaddr *) &this->dataStorage, &addrSize);
    if (this->connectFd < 0) {
        Logging::logError("Failed to accept connection");
        return false;
    }

    // Disable Nagle. The data plane streams large frames back-to-back with no
    // small follow-ups; Nagle's coalescing only adds delayed-ACK stalls.
    int nodelay = 1;
    setsockopt(this->connectFd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    Logging::logInfo("Connected");
    return true;
}

void RX::closeConnection() {
    if (this->connectFd != -1) {
        shutdown(this->connectFd, O_RDWR);
        close(this->connectFd);
        this->connectFd = -1;
    }
}

// RX side of the handshake. RX writes its public key first because it's
// the side that accepted the connection - the caller (TX) is waiting on
// us to make the first move, mirroring the TCP roles.
bool RX::handshake() {
    Logging::logInfo("Performing handshake");

    unsigned char myPk[PUBKEY_SIZE];
    unsigned char mySk[SECRETKEY_SIZE];
    Crypto::generateKeypair(myPk, mySk);

    if (!NetworkUtility::sendAll(this->connectFd, myPk, PUBKEY_SIZE)) {
        Crypto::zeroize(mySk, sizeof(mySk));
        Logging::logError("Handshake failed - could not send public key");
        return false;
    }

    unsigned char peerPk[PUBKEY_SIZE];
    if (!NetworkUtility::recvAll(this->connectFd, peerPk, PUBKEY_SIZE, PUBKEY_SIZE)) {
        Crypto::zeroize(mySk, sizeof(mySk));
        Logging::logError("Handshake failed - did not receive peer public key");
        return false;
    }

    Crypto::deriveSharedKey(mySk, peerPk, cmdArgs.passphrase, this->sharedKey);
    Crypto::zeroize(mySk, sizeof(mySk));

    Logging::logInfo("Handshake complete");
    return true;
}

// Read one encrypted frame from the socket, decrypt, and return the
// plaintext. Returns an empty unique_ptr (with plainLen unchanged) on any
// failure - caller logs and returns false.
static std::unique_ptr<unsigned char[]> readOneFrame(
        int connectFd, const unsigned char *key,
        size_t &plainLen, const char *whatForLog) {
    unsigned char header[PACKET_HEADER_SIZE];
    if (!NetworkUtility::recvAll(connectFd, header, PACKET_HEADER_SIZE, PACKET_HEADER_SIZE)) {
        Logging::logError(std::string("Fatal Error - ") + whatForLog + " header lost");
        return nullptr;
    }
    unsigned int bodyLen = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
    if (bodyLen < CRYPTO_OVERHEAD || bodyLen > MAX_PACKET_SIZE + CRYPTO_OVERHEAD) {
        Logging::logError(std::string("Fatal Error - implausible ") + whatForLog + " size");
        return nullptr;
    }
    auto body = std::make_unique<unsigned char[]>(bodyLen);
    if (!NetworkUtility::recvAll(connectFd, body.get(), bodyLen, bodyLen)) {
        Logging::logError(std::string("Fatal Error - broken socket reading ") + whatForLog);
        return nullptr;
    }
    auto plain = Crypto::decryptPacket(body.get(), bodyLen, key, plainLen);
    if (!plain) {
        Logging::logError("Fatal Error - decryption failed (wrong passphrase or tampered data)");
        return nullptr;
    }
    return plain;
}

bool RX::receive() {
    // RAII wrapper so every error path closes the buffer file
    std::unique_ptr<FILE, decltype(&fclose)> fdout(
            fopen(".ft_temp_pack_buffer.tar.gz", "wb"), &fclose);
    if (!fdout) {
        Logging::logError("Unable to create buffer file");
        // filesystem failure is fatal regardless of loop mode
        exit(1);
    }

    // First frame after the handshake is the 8-byte file-size preamble.
    // Used only for the progress display; the authoritative end-of-stream
    // is still the encrypted empty-payload frame at the end.
    size_t preambleLen = 0;
    auto preamble = readOneFrame(this->connectFd, this->sharedKey,
                                  preambleLen, "size preamble");
    if (!preamble || preambleLen != 8) {
        return false;
    }
    size_t totalSize = 0;
    for (int i = 0; i < 8; i++) {
        totalSize = (totalSize << 8) | preamble[i];
    }

    Logging::logInfo("Receiving package");
    Logging::logProgressStart(totalSize);

    size_t received = 0;
    while (true) {
        size_t plainLen = 0;
        auto plain = readOneFrame(this->connectFd, this->sharedKey, plainLen, "data");
        if (!plain) {
            return false;
        }

        // empty plaintext == end-of-stream marker
        if (plainLen == 0) {
            break;
        }

        // write decrypted payload to the buffer file
        fwrite(plain.get(), plainLen, 1, fdout.get());

        received += plainLen;
        Logging::logProgressUpdate(received);
    }
    Logging::logProgressFinish();

    // flush+close before tar so the extracted file sees the complete tarball
    fdout.reset();

    Logging::logInfo("Unpacking File");
    unpackFile();
    deletePackedBufferFile();

    Logging::logSuccess("Transfer complete");
    return true;
}
