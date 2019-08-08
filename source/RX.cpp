#include "RX.h"
#include <netdb.h>

#define BACKLOG 10


void RX::socketSetup() {
    //struct setup
    address.sin_family = AF_INET;
    address.sin_port = htons(info.port);
    if(inet_pton(AF_INET, info.ip.c_str(), &(address.sin_addr)) != 1){
        std::cerr << "inet_pton failed" << std::endl;
        exit(1);
    }

    //init socket
    rxSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(rxSocket < 0){
        std::cerr << "socket() failed" << std::endl;
        exit(1);
    }

    //bind socket
    if(bind(rxSocket, (struct sockaddr *)&address, sizeof(address))<0){
        std::cerr << "bind failed" << std::endl;
        exit(1);
    }

    //listen
    if(listen(rxSocket, BACKLOG) != 0){
        std::cerr << "listen failed" << std::endl;
        exit(1);
    }

    //accept
    socklen_t addr_size = sizeof receive_storage;
    newRxSocket = accept(rxSocket, (struct sockaddr *)&receive_storage, &addr_size);
    std::cout << "Socket Connected" << std::endl;
}

void RX::receive() {
    string title;
    int fileSize = 0;
    string shasum;
    char newMsg[2048];

    //get title
    recv(newRxSocket, newMsg, sizeof(newMsg), 0);
    title = newMsg;
    memset(newMsg, 0, sizeof(newMsg)); // reset newMsg

    //get file size
    recv(newRxSocket, newMsg, sizeof(newMsg), 0);
    try{fileSize = stoi(newMsg, nullptr, 10);}
    catch(invalid_argument &e){std::cerr << "size error: " << e.what() << std::endl;exit(1);}
    memset(newMsg, 0, sizeof(newMsg)); // reset newMsg

    //get sha265 sum
    recv(newRxSocket, newMsg, sizeof(newMsg), 0);
    shasum = newMsg;
    memset(newMsg, 0, sizeof(newMsg)); // reset newMsg

    cout << "Title: " << title << endl;
    cout << "File Size: " << fileSize << endl;
    cout << "SHA265 Sum: " << shasum << endl;

    int fdout = open(title.c_str(), O_CREAT|O_WRONLY);
    if(!fdout){
        std::cerr << "File cannot be created" << std::endl;
        exit(1);
    }

    while (true) {
        //no more message if recvRET become 0, stop receiving
        int recvRET = recv(newRxSocket, newMsg, sizeof(newMsg), 0);

        if(recvRET == 0){
            break;
        }

        write(fdout, newMsg, sizeof(newMsg));

        memset(newMsg, 0, sizeof(newMsg)); // reset newMsg
    }

    if(verify(title, shasum)){
        cout << "File Received and Verified" << endl;
    }else{
        cout << "File was received but cannot be verified" << endl;
    }
}

bool RX::verify(const string fileName, const string shasum) {
    //check shasum
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

    if(result != shasum){
        cout << result << endl << shasum << endl;

        std::cerr << "File Verification failed" << std::endl;
        return false;
    }
    return true;
}
