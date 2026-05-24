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

bool RX::receive() {
    Logging::logInfo("Receiving package");

    // RAII wrapper so every error path closes the buffer file
    std::unique_ptr<FILE, decltype(&fclose)> fdout(
            fopen(".ft_temp_pack_buffer.tar.gz", "wb"), &fclose);
    if (!fdout) {
        Logging::logError("Unable to create buffer file");
        // filesystem failure is fatal regardless of loop mode
        exit(1);
    }

    unsigned char packetSizeBuf[PACKET_HEADER_SIZE];
    memset(packetSizeBuf, 0, sizeof(packetSizeBuf));

    while (true) {
        // receive packet size header
        if (!NetworkUtility::recvAll(this->connectFd, packetSizeBuf, PACKET_HEADER_SIZE, PACKET_HEADER_SIZE)) {
            Logging::logError("Fatal Error - Packet Header Lost");
            return false;
        }

        // length covers nonce + MAC + ciphertext; the smallest valid frame
        // is an empty payload (CRYPTO_OVERHEAD bytes)
        unsigned int packetSize = (packetSizeBuf[0] << 24) | (packetSizeBuf[1] << 16) | (packetSizeBuf[2] << 8) | packetSizeBuf[3];
        if (packetSize < CRYPTO_OVERHEAD || packetSize > MAX_PACKET_SIZE + CRYPTO_OVERHEAD) {
            Logging::logError("Fatal Error - implausible packet size");
            return false;
        }

        // receive the encrypted body
        auto packetBuf = std::make_unique<unsigned char[]>(packetSize);
        if (!NetworkUtility::recvAll(this->connectFd, packetBuf.get(), packetSize, packetSize)) {
            Logging::logError("Fatal Error - Broken Socket");
            return false;
        }

        // decrypt + verify MAC. A failure here means either wire tampering
        // or a passphrase mismatch - both abort this transfer.
        size_t plainLen = 0;
        auto plain = Crypto::decryptPacket(packetBuf.get(), packetSize, this->sharedKey, plainLen);
        if (!plain) {
            Logging::logError("Fatal Error - decryption failed (wrong passphrase or tampered data)");
            return false;
        }

        // empty plaintext == end-of-stream marker
        if (plainLen == 0) {
            Logging::logInfo("File received");
            break;
        }

        // send read receipt (plaintext lock-step pacing signal; not security-critical)
        send(this->connectFd, READ_RECEIPT, READ_RECEIPT_SIZE, 0);

        // write decrypted payload to the buffer file
        fwrite(plain.get(), plainLen, 1, fdout.get());
    }

    // flush+close before tar so the extracted file sees the complete tarball
    fdout.reset();

    Logging::logInfo("Unpacking File");
    unpackFile();
    deletePackedBufferFile();

    Logging::logInfo("File transfer complete");
    return true;
}
