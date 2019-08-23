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
    std::string ip; // ip stored as a string
    int port; // port number stored as a unsigned short int
    std::string filePath; // path to the file, or directory if -d is indicated
    bool dir; // indicate of filePath points to a folder
    bool enc; // indicate if encryption is need
};

// get the sha265 sum of any file
std::string shasum(const std::string &fileName);

// check port number, if legal, return port number, else print error and return -1
int checkPort(const std::string &port_str);

// check IP addr, if valid, return true, else print error and return false
bool checkIP(const std::string &IP_str);

// parse argv
CMDARGS parseInput(char *cmd[], int argc);

// tar a directory is -d is provide in the argv
int packDir(const std::string &path);

// unpack .tar.gz
void unpackDir();