#include "TX.h"

//TODO: Read file in binary mode, serialized it, then send. Therefore, the program is capable of sending any type of file

void TX::socketSetup() {
    //struct setup
    address.sin_family = AF_INET;
    address.sin_port = htons(info.port);
    if (inet_pton(AF_INET, info.ip.c_str(), &(address.sin_addr)) != 1) {
        std::cerr << "inet_pton failed" << std::endl;
        exit(1);
    }

    //init socket
    txSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (txSocket < 0) {
        std::cerr << "socket() failed" << std::endl;
        exit(1);
    }
    //connect
    if (connect(txSocket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        std::cerr << "connection failed " << errno << std::endl;
        exit(1);
    }
    std::cout << "Socket Connected" << std::endl;
}

void TX::transmit() {
    FILE *fdin;
    cout << "File Name: " << fileName << endl;

    struct stat fileInfo;
    if (stat(fileName.c_str(), &fileInfo) == 0) {
        cout << "File Size: " << fileInfo.st_size << endl;
        //open file
        fdin = fopen(fileName.c_str(), "rb");
        if (fdin == nullptr) {
            std::cerr << "an error occurred while opening file " << errno << std::endl;
            exit(1);
        }
    } else {
        std::cerr << "cannot get file info" << std::endl;
        exit(1);
    }

    //send filename length
    int length = strlen(fileName.c_str());
    if (length > 999) {
        std::cerr << "filepath too long" << std::endl;
        exit(1);
    } else {
        string length_str = to_string(length);
        int diff = 3 - length_str.length();
        for (int i = 0; i < diff; i++) {
            length_str = "0" + length_str;
        }
        send(txSocket, length_str.c_str(), strlen(length_str.c_str()), 0);
    }

    //send filename
    // TODO: fix when enter a path such as /usr/file, fileName should be file not /usr/file
    send(txSocket, fileName.c_str(), strlen(fileName.c_str()), 0);

    //send sha256 sum
    string result;
    result = shasum(fileName);
    std::cout << "SHA256 SUM: " << result << std::endl;
    send(txSocket, result.c_str(), strlen(result.c_str()), 0);

    //send fileSize
    string size_str = to_string(fileInfo.st_size);
    int diff = 13 - (int) size_str.length();
    for (int i = 0; i < diff; i++) {
        size_str = "0" + size_str;
    }
    send(txSocket, size_str.c_str(), strlen(size_str.c_str()), 0);

    //send file
    unsigned char toSend[2048];
    int chunkSize = 2048;
    int fileSize = fileInfo.st_size;

    while(fileSize != 0){
        if (chunkSize > fileSize) {
            chunkSize = fileSize;
            fileSize = 0;
        }else{
            fileSize-=chunkSize;
        }
        fread(toSend, chunkSize, 1, fdin);

        send(txSocket, toSend, chunkSize, 0);
        memset(toSend, 0, sizeof(toSend));
    }
    fclose(fdin);
}


