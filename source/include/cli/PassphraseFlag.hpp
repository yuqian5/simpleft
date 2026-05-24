#ifndef SFT_CLI_PASSPHRASEFLAG_HPP
#define SFT_CLI_PASSPHRASEFLAG_HPP

#include <string>

/**
 * read a passphrase from stdin. When stdin is a TTY the prompt is shown on
 * stderr with terminal echo disabled; otherwise a single line is read from
 * stdin (so `echo pw | sft ...` works for scripting).
 */
std::string promptPassphrase();

#endif //SFT_CLI_PASSPHRASEFLAG_HPP
