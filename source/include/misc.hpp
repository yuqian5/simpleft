#ifndef SFT_MISC_HPP
#define SFT_MISC_HPP

#include <iostream>
#include <regex>
#include <iomanip>
#include <sys/stat.h>

#include "Logging.hpp"

/**
 * struct which stores all cmdArgs passed from commandline
 */
struct CMD_ARGS {
    int mode; // rx = 1, tx = 2
    std::string ip; // ip stored as a string
    std::string standard; // ipv4 or ipv6
    int port; // port number
    std::string filePath; // path to the file, or directory
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
 * @param cmdArgs CMD_ARGS
 * @return true if success, false otherwise
 */
bool extractIP(const std::string &argv_str, CMD_ARGS &cmdArgs);

/**
* extract port number from command line argument
* @param argv_str std::string
* @param cmdArgs CMD_ARGS
* @return true if success, false otherwise
*/
bool extractPort(const std::string &argv_str, CMD_ARGS &cmdArgs);

/**
* extract mode from command line argument
* @param argv_str std::string
* @param cmdArgs CMD_ARGS
* @return true if success, false otherwise
*/
bool extractMode(const std::string &argv_str, CMD_ARGS &cmdArgs);

/**
 * parse command line argument
 * @param cmd char**
 * @param argc int
 * @return CMD_ARGS
 */
CMD_ARGS parseInput(char *cmd[], int &argc);

#endif //SFT_MISC_HPP
