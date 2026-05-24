#ifndef SFT_CLI_PORTFLAG_HPP
#define SFT_CLI_PORTFLAG_HPP

#include <string>

/**
 * validate that port_str is a decimal integer in [1024, 65535].
 */
bool checkPort(const std::string &port_str);

#endif //SFT_CLI_PORTFLAG_HPP
