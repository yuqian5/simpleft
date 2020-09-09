#include "../include/TX.hpp"

TX::TX(CMDARGS cmd) {
    // copy commandline arguments
    info = std::move(cmd);
    filePath = info.filePath;

    socketSetup();
    transmit();
}

TX::~TX() {
    shutdown(this->connectfd, O_RDWR);
    close(this->connectfd);
}

void TX::connectNow(sockaddr_in &serverAddr4, int &connectfd) {
    //connect, timeout after 600s (10min)
    int tries_left = 60;
    std::cout << "Connecting...";
    while (tries_left) {
        //struct setup
        if (connect(connectfd, (struct sockaddr *) &serverAddr4, sizeof(serverAddr4)) < 0) {
            std::cerr << '.';
            tries_left -= 1;
            if (!tries_left) {
                std::cerr << "\nConnection Failed, No Receiver Found" << std::endl;
                exit(1);
            } else {
                sleep(1);
            }
        } else {
            std::cout << "Connected" << std::endl;
            break;
        }
    }
}

void TX::connectNow(sockaddr_in6 &serverAddr6, int &connectfd) {
    //connect, timeout after 600s (10min)
    int tries_left = 60;
    std::cout << "Connecting...";
    while (tries_left) {
        //struct setup
        if (connect(connectfd, (struct sockaddr *) &serverAddr6, sizeof(serverAddr6)) < 0) {
            std::cout << '.';
            tries_left -= 1;
            if (!tries_left) {
                std::cerr << "\nconnection failed, no receiver found" << std::endl;
                exit(1);
            } else {
                sleep(1);
            }
        } else {
            std::cout << "Connected" << std::endl;
            break;
        }
    }
}

void TX::socketSetup() {
    // check protocol standard
    if (info.standard == "ipv4") { // ipv4
        // initialize socket
        this->connectfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&this->serverAddr4, '0', sizeof(this->serverAddr4));

        this->serverAddr4.sin_family = AF_INET;
        this->serverAddr4.sin_port = htons(info.port);
        if (inet_pton(AF_INET, this->info.ip.c_str(), &(this->serverAddr4.sin_addr)) != 1) {
            std::cerr << "ERROR: inet_pton failed" << std::endl;
            exit(1);
        }
        // connect
        connectNow(std::ref(this->serverAddr4), std::ref(this->connectfd));
    } else { // ipv6
        // initialize socket
        this->connectfd = socket(AF_INET6, SOCK_STREAM, 0);
        memset(&this->serverAddr6, '0', sizeof(this->serverAddr6));

        this->serverAddr6.sin6_family = AF_INET6;
        this->serverAddr6.sin6_port = htons(this->info.port);
        if (inet_pton(AF_INET6, this->info.ip.c_str(), &(this->serverAddr6.sin6_addr)) != 1) {
            std::cerr << "ERROR: inet_pton failed" << std::endl;
            exit(1);
        }
        // connect
        connectNow(std::ref(this->serverAddr6), std::ref(this->connectfd));
    }
}

void TX::transmit() {
    if (!packFile(info.filePath)) {
        std::cerr << "ERROR: No such file or directory" << std::endl;
        exit(0);
    }

    char readBuf[16];
    memset(readBuf, 0, sizeof(readBuf));

    // get file info
    FILE *fdin;

    struct stat fileInfo = {};
    if (stat("tempFilePackage.tar.gz", &fileInfo) == 0) {
        //open file
        fdin = fopen("tempFilePackage.tar.gz", "rb");
        if (fdin == nullptr) {
            std::cerr << "ERROR: Cannot Get File Info" << std::endl;
            exit(1);
        }
    } else {
        std::cerr << "ERROR: Cannot Get File Info" << std::endl;
        exit(1);
    }

    // get filename
    std::string fileName; // just the file name, path removed
    fileName = extractFilename();

    // send filename
    send(this->connectfd, fileName.c_str(), fileName.length(), 0);

    // wait for confirmation
    recv(this->connectfd, readBuf, sizeof(readBuf), 0);
    memset(readBuf, 0, sizeof(readBuf));

    // send sha256 sum
    std::string sum;
    sum = shasum();
    send(this->connectfd, sum.c_str(), sum.length(), 0);

    // wait for confirmation
    recv(this->connectfd, readBuf, sizeof(readBuf), 0);
    memset(readBuf, 0, sizeof(readBuf));

    // send fileSize
    std::string size_str = std::to_string(fileInfo.st_size);
    int diff = 13 - (int) size_str.length();
    for (int i = 0; i < diff; i++) {
        size_str.insert(0, "0");
    }
    send(this->connectfd, size_str.c_str(), size_str.length(), 0);

    // wait for confirmation
    recv(this->connectfd, readBuf, sizeof(readBuf), 0);
    memset(readBuf, 0, sizeof(readBuf));

    // send file
    unsigned char outBoundData[2048];
    int chunkSize = 2048;
    int fileSize = fileInfo.st_size;

    while (fileSize != 0) {
        if (chunkSize > fileSize) {
            chunkSize = fileSize;
            fileSize = 0;
        } else {
            fileSize -= chunkSize;
        }
        fread(outBoundData, chunkSize, 1, fdin);

        send(this->connectfd, outBoundData, chunkSize, 0);
        memset(outBoundData, 0, sizeof(outBoundData));
    }
    fclose(fdin);

    // remove file
    deleteFile();

    // prompt complete
    std::cout << fileName << " Sent To " << info.ip << std::endl;
}

std::string TX::extractFilename() {
    for (unsigned short index = filePath.length(); index > 0; --index) {
        if (filePath[index] == '/') {
            ++index;
            return filePath.substr(index, filePath.length());
        }
    }
    return filePath;
}

