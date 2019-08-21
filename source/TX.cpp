#include "TX.h"

//TODO : serialize data with boost library before sending, enable transfers between machines with different endian.
//TODO : add encryption, enable secure transfer (RSA), prevent MITM attack

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
    cout << "File Name: " << filePath << endl;

    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) == 0) {
        cout << "File Size: " << fileInfo.st_size << endl;
        //open file
        fdin = fopen(filePath.c_str(), "rb");
        if (fdin == nullptr) {
            std::cerr << "an error occurred while opening file " << errno << std::endl;
            exit(1);
        }
    } else {
        std::cerr << "cannot get file info" << std::endl;
        exit(1);
    }

    //send filename length
    string fileName; // just the file name, path removed
    fileName = extractFilename();

    int length = fileName.length();
    if (length > 999) {
        std::cerr << "filepath too long" << std::endl;
        exit(1);
    } else {
        string length_str = to_string(length);
        int diff = 3 - length_str.length();
        for (int i = 0; i < diff; i++) {
            length_str = "0" + length_str;
        }
        send(txSocket, length_str.c_str(), length_str.length(), 0);
    }

    //send filename
    send(txSocket, fileName.c_str(), fileName.length(), 0);

    //send sha256 sum
    string result, result_temp;
    result_temp = shasum(filePath);
    result = result_temp.substr(0, result_temp.find(' '));
    std::cout << "SHA256 SUM: " << result << std::endl;
    send(txSocket, result.c_str(), result.length(), 0);

    //send fileSize
    string size_str = to_string(fileInfo.st_size);
    int diff = 13 - (int) size_str.length();
    for (int i = 0; i < diff; i++) {
        size_str = "0" + size_str;
    }
    send(txSocket, size_str.c_str(), size_str.length(), 0);

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

string TX::extractFilename() {
    unsigned short index = filePath.length();
    for ( ; index > 0; --index) {
        if(filePath[index] == '/'){
            ++index;
            return filePath.substr(index, filePath.length());
        }
    }
    return filePath;
}

