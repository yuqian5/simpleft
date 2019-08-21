#include "RX.h"

//TODO : deserialize data before writing, enable transfers between machines with different endian.
//TODO : add encryption, enable secure transfer (RSA), prevent MITM attack

void RX::socketSetup() {
    //struct setup
    address.sin_family = AF_INET;
    address.sin_port = htons(info.port);
    if (inet_pton(AF_INET, info.ip.c_str(), &(address.sin_addr)) != 1) {
        std::cerr << "inet_pton failed" << std::endl;
        exit(1);
    }

    //init socket
    rxSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (rxSocket < 0) {
        std::cerr << "socket() failed" << std::endl;
        exit(1);
    }

    //bind socket
    if (bind(rxSocket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        std::cerr << "bind failed" << std::endl;
        exit(1);
    }

    //listen
    if (listen(rxSocket, BACKLOG) != 0) {
        std::cerr << "listen failed" << std::endl;
        exit(1);
    }

    //accept
    socklen_t addr_size = sizeof receive_storage;
    newRxSocket = accept(rxSocket, (struct sockaddr *) &receive_storage, &addr_size);
    std::cout << "Socket Connected" << std::endl;
}

void RX::receive() {
    string FileName;
    string shasum;
    char newMsg[2048];
    int fileName_length;
    int fileSize;

    //get fileName length
    recv(newRxSocket, newMsg, 3, 0);
    try { fileName_length = stoi(newMsg, nullptr, 10); }
    catch (invalid_argument &e) {
        std::cerr << "error converting filename size" << std::endl;
        exit(1);
    }
    memset(newMsg, 0, sizeof(newMsg)); // reset newMsg

    //get fileName
    recv(newRxSocket, newMsg, fileName_length, 0);
    FileName = newMsg;
    memset(newMsg, 0, sizeof(newMsg)); // reset newMsg

    //get sha265 sum
    recv(newRxSocket, newMsg, 64, 0);
    shasum = newMsg;
    memset(newMsg, 0, sizeof(newMsg)); // reset newMsg

    //get fileSize
    recv(newRxSocket, newMsg, 13, 0);
    try { fileSize = stoi(newMsg, nullptr, 10); }
    catch (invalid_argument &e) {
        std::cerr << "error converting fileSize size" << std::endl;
        exit(1);
    }

    //print out info
    cout << "File Name: " << FileName << endl;
    cout << "File Size: " << fileSize << endl;
    cout << "SHA265 Sum: " << shasum << endl;

    //create file using the fileName received from socket
    FILE *fdout;
    fdout = fopen(FileName.c_str(), "wb");
    if (fdout == nullptr) {
        std::cerr << "an error occurred while opening file " << errno << std::endl;
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
        int recvRET = recv(newRxSocket, newMsg, chunkSize, 0);
        if (recvRET == 0) {
            break;
        }
        fwrite(newMsg, chunkSize,1 ,fdout);
        memset(newMsg, 0, sizeof(newMsg)); // reset newMsg
    }

    fclose(fdout);

    //change file permission so it can be read without sudo privilege
    string cmd = "chmod 666 ";
    cmd += FileName;
    system(cmd.c_str());

    if (verify(FileName, shasum)) {
        cout << "File Received and Verified" << endl;
    } else {
        cout << "File was received but cannot be verified" << endl;
    }
}

//verifies the file received by checking it sha265 sum against the one transferred.
bool RX::verify(const string &fileName, const string &sum) {
    //check shasum
    string result, result_temp;
    result_temp = shasum(fileName);
    result = result_temp.substr(0, result_temp.find(' '));

    if (result != sum) {
        std::cerr << "File Verification failed" << std::endl;
        return false;
    }
    return true;
}
