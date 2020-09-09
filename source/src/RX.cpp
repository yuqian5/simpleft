#include "../include/RX.hpp"

RX::RX(CMDARGS &cmd) {
    // copy commandline arguments
    this->info = cmd;

    // initialize socket
    socketSetup();

    // receive file
    receive();
}

RX::~RX() {
    // shutdown sockets
    shutdown(this->listenfd, O_RDWR);
    close(this->listenfd);
    shutdown(this->connectfd, O_RDWR);
    close(this->connectfd);
}

void RX::socketSetup() {
    // init socket
    this->listenfd = socket(AF_INET6, SOCK_STREAM, 0);
    memset(&this->serverAddr, 0, sizeof(this->serverAddr));

    this->serverAddr.sin6_family = AF_INET6;
    this->serverAddr.sin6_port = htons(this->info.port);
    this->serverAddr.sin6_addr = in6addr_any;

    // set to accept ipv4 protocol
    int no = 0;
    setsockopt(this->listenfd, IPPROTO_IPV6, IPV6_V6ONLY, (void *) &no, sizeof(no));

    //bind socket
    bind(this->listenfd, (struct sockaddr *) &this->serverAddr, sizeof(this->serverAddr));

    //listen
    listen(this->listenfd, BACKLOG);
    std::cout << "Connecting...";

    //accept
    socklen_t addrSize = sizeof dataStorage;
    this->connectfd = accept(this->listenfd, (struct sockaddr *) &this->dataStorage, &addrSize);
    std::cout << "Connected\n";
}

void RX::receive() {
    std::string FileName;
    std::string shasum;
    char readBuf[2048];
    memset(readBuf, 0, sizeof(readBuf));
    int fileSize = 0;

    //get fileName
    recv(this->connectfd, readBuf, sizeof(readBuf), 0);
    FileName = readBuf;
    memset(readBuf, 0, sizeof(readBuf)); // reset readBuf

    //send confirmation
    write(this->connectfd, "1", 1);

    //get sha265 sum
    recv(this->connectfd, readBuf, sizeof(readBuf), 0);
    shasum = readBuf;
    memset(readBuf, 0, sizeof(readBuf)); // reset readBuf

    //send confirmation
    write(this->connectfd, "1", 1);

    //get fileSize
    recv(this->connectfd, readBuf, sizeof(readBuf), 0);
    try {
        fileSize = std::stoi(readBuf, nullptr, 10);
    } catch (std::invalid_argument &e) {
        std::cerr << "ERROR: Error converting fileSize size" << std::endl;
        exit(1);
    }

    //send confirmation
    write(this->connectfd, "1", 1);

    //create file using the fileName received from socket
    FILE *fdout;
    fdout = fopen("tempFilePackage.tar.gz", "wb");
    if (fdout == nullptr) {
        perror("ERROR: Error occurred while creating file");
        exit(1);
    }

    //receive until the other side does a orderly shutdown
    while (true) {
        int chunkSize = 2048;
        if (chunkSize > fileSize) {
            chunkSize = fileSize;
        } else {
            fileSize -= chunkSize;
        }
        //no more message if recvRET become 0, stop receiving
        int recvRET = recv(this->connectfd, readBuf, chunkSize, 0);
        if (recvRET == 0) {
            break;
        }
        fwrite(readBuf, chunkSize, 1, fdout);
        memset(readBuf, 0, sizeof(readBuf)); // reset readBuf
    }

    fclose(fdout);

    if (verify(shasum)) {
        std::cout << FileName << " Received And Verified" << std::endl;
    } else {
        std::cout << "DANGER: File Was Received But Cannot Be Verified, File Deleted" << std::endl;
        // remove file
        deleteFile();
    }

    std::cout << "\nUnpacking File...";
    unpackFile();
    deleteFile();
    std::cout << "Done\n";
}

//verifies the file received by checking it sha265 sum against the one transferred.
bool RX::verify(const std::string &sum) {
    //check shasum
    std::string result;
    result = shasum();

    if (result != sum) {
        std::cerr << "File Verification Failed" << std::endl;
        return false;
    }
    return true;
}





