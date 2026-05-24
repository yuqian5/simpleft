#ifndef SFT_CLI_ARGPARSER_HPP
#define SFT_CLI_ARGPARSER_HPP

#include "CmdArgs.hpp"

/**
 * parse argv into a CMD_ARGS. Exits the process on any fatal parse error.
 * @param cmd argv as passed to main
 * @param argc argc as passed to main (reference to match the historical signature)
 */
CMD_ARGS parseInput(char *cmd[], int &argc);

#endif //SFT_CLI_ARGPARSER_HPP
