#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <regex>
#include <getopt.h>

#pragma once

// struct which stores all info pass from commandline
struct CMDARGS {
    int mode; // rx = 1, tx = 2
    std::string ip;
    unsigned short int port;
    std::string filePath;
};

// get the sha265 sum of any file
std::string shasum(std::string fileName);

// check port number, if legal, return port number, else print error and exit
int checkPort(const std::string& port_str);

// check IP addr, if valid, return true, else print error and exit
bool checkIP(const std::string& IP_str);

//parse argv (InProgress...)
CMDARGS parseInput(char* cmd[], int argc);

//parse argv (InUse... Soon to be replaced)
CMDARGS parseCMDInput(char **cmd, int argc);