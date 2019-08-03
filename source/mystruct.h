#include <cstring>

// struct which stores all info pass from commandline
struct CMDARGS{
    int mode; // rx = 1, tx = 2
    std::string ip;
    int port;
};
