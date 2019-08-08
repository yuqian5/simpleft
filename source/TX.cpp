#include "TX.h"
#include <netdb.h>


void TX::socketSetup() {
    //struct setup
    address.sin_family = AF_INET;
    address.sin_port = htons(info.port);
    if(inet_pton(AF_INET, info.ip.c_str(), &(address.sin_addr)) != 1){
        std::cerr << "inet_pton failed" << std::endl;
        exit(1);
    }

    //init socket
    txSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(txSocket < 0){
        std::cerr << "socket() failed" << std::endl;
        exit(1);
    }
    //connect
    if(connect(txSocket, (struct sockaddr *)&address, sizeof(address)) < 0){
        std::cerr << "connection failed " << errno << std::endl;
        exit(1);
    }
    std::cout << "Socket Connected" << std::endl;
}

void TX::transmit() {
    int fdin;
    string fileName = "testFile.txt";
    cout << "Title: " << fileName << endl;

    struct stat fileInfo;
    if (stat(fileName.c_str(), &fileInfo) == 0) {
        cout << "File Size: " << fileInfo.st_size << endl;

        fdin = open(fileName.c_str(), O_RDONLY);
        if(fdin == -1){
            std::cerr << "an error occurred while opening file " << errno << std::endl;
            exit(1);
        }
    }else{
        std::cerr << "cannot get file info" << std::endl;
        exit(1);
    }

    //send filename
    send(txSocket, fileName.c_str(), strlen(fileName.c_str()), 0);
    usleep(200);

    //send filesize
    send(txSocket, to_string(fileInfo.st_size).c_str(), strlen(to_string(fileInfo.st_size).c_str()), 0);
    usleep(200);

    //send sha256 sum
    char buffer[256];
    string result;
    string cmd = "shasum -a 256 ";
    cmd += fileName;
    FILE* pipe = popen(cmd.c_str() , "r");

    if(!pipe){
        throw std::runtime_error("popen() failed!");
    }

    fgets(buffer, sizeof(buffer), pipe);
    result += buffer;
    pclose(pipe);
    std::cout << "SHA256 SUM: " << result << std::endl;
    send(txSocket, result.c_str(), strlen(result.c_str()), 0);
    usleep(200);


    //send file
    char toSend[2048];
    while(read(fdin, toSend, 2048)){
        send(txSocket, toSend, strlen(toSend), 0);

        memset(toSend, 0, sizeof(toSend));
    }
    close(fdin);
}


