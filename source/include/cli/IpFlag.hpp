#ifndef SFT_CLI_IPFLAG_HPP
#define SFT_CLI_IPFLAG_HPP

#include <string>

/**
 * validate that IP_str is a well-formed IPv4 or IPv6 address.
 */
bool checkIP(const std::string &IP_str);

#endif //SFT_CLI_IPFLAG_HPP
