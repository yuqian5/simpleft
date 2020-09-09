#include <iostream>
#include <regex>

#pragma once

/**
 * struct which stores all info passed from commandline
 */
struct CMDARGS {
    int mode; // rx = 1, tx = 2
    std::string ip; // ip stored as a string
    std::string standard; // ipv4 or ipv6
    int port; // port number
    std::string filePath; // path to the file, or directory
    bool directory; // indicate of filePath points to a folder
};

/**
 * check if the port number provided is valid
 * @param port_str std::string port number
 * @return true if port number provided is valid, false otherwise
 */
bool checkPort(const std::string &port_str);

/**
 * check if ipv4 or ipv6 serverAddr provided is valid
 * @param IP_str std::string ip serverAddr
 * @return true of ip serverAddr provided is valid, false otherwise
 */
bool checkIP(const std::string &IP_str);

/**
 * extract ip serverAddr from command line argument
 * @param argv_str std::string
 * @param cmdargs CMDARGS
 * @return true if success, false otherwise
 */
bool extractIP(const std::string &argv_str, CMDARGS &cmdargs);

/**
* extract port number from command line argument
* @param argv_str std::string
* @param cmdargs CMDARGS
* @return true if success, false otherwise
*/
bool extractPort(const std::string &argv_str, CMDARGS &cmdargs);

/**
* extract mode from command line argument
* @param argv_str std::string
* @param cmdargs CMDARGS
* @return true if success, false otherwise
*/
bool extractMode(const std::string &argv_str, CMDARGS &cmdargs);

/**
 * parse command line argument
 * @param cmd char**
 * @param argc int
 * @return CMDARGS
 */
CMDARGS parseInput(char *cmd[], int &argc);

