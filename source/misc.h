#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

#pragma once

// struct which stores all info pass from commandline
struct CMDARGS{
    int mode; // rx = 1, tx = 2
    std::string ip;
    unsigned short int port;
};

// get the sha265 sum of any file
std::string shasum(std::string fileName);


