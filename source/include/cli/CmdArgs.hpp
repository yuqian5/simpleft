#ifndef SFT_CLI_CMDARGS_HPP
#define SFT_CLI_CMDARGS_HPP

#include <string>

/**
 * stores all command-line arguments parsed from argv
 */
struct CMD_ARGS {
    int mode; // rx = 1, tx = 2
    std::string ip; // ip stored as a string
    std::string standard; // ipv4 or ipv6
    int port; // port number
    std::string filePath; // path to the file, or directory
    std::string passphrase; // shared secret, mixed into the handshake
    bool loop; // rx-only: after each transfer, keep accepting new connections
};

#endif //SFT_CLI_CMDARGS_HPP
