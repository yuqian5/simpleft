#include <fstream>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <cstdlib>
#include <utility>
#include <netinet/in.h>
using namespace std;

#include "mystruct.h"

class TX {
public:
    explicit TX(CMDARGS cmd){
        // copy commandline arguments
        info = std::move(cmd);

        //init socket
        txSocket = socket(AF_INET, SOCK_STREAM, 0);
        main();
    }
    ~TX()= default;

private:
    CMDARGS info;
    int txSocket;


    int main();
    int send();
    int pack();
    int getFile();
};

