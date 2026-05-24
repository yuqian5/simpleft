#include "../../include/cli/PassphraseFlag.hpp"

#include <iostream>
#include <termios.h>
#include <unistd.h>

#include "../../include/Logging.hpp"

// Read a passphrase from stdin without echoing it.
//
// TTY path: disable the ECHO bit on stdin's termios, prompt, read a line,
// restore the original termios. The restore MUST run on every exit path —
// otherwise the user's shell is left with echo off after sft exits. We
// don't currently install a signal handler, so SIGINT during the prompt
// will leave the terminal in no-echo state; the user can recover with
// `stty echo` or `reset`. Acceptable tradeoff for a tool this small.
//
// Non-TTY path: when stdin is a pipe or redirected file the termios calls
// are meaningless (and would fail), so we just read a single line. This is
// what enables `echo pw | sft ...` and `sft ... < passfile`.
std::string promptPassphrase() {
    if (!isatty(STDIN_FILENO)) {
        std::string pw;
        std::getline(std::cin, pw);
        return pw;
    }

    struct termios oldTerm{};
    if (tcgetattr(STDIN_FILENO, &oldTerm) != 0) {
        Logging::logError("Failed to read terminal settings");
        exit(1);
    }
    struct termios newTerm = oldTerm;
    newTerm.c_lflag &= ~(tcflag_t) ECHO;
    // TCSAFLUSH discards any already-typed-but-unread characters, so an
    // attacker who pre-fills the input buffer can't smuggle bytes into the
    // passphrase.
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newTerm) != 0) {
        Logging::logError("Failed to disable terminal echo");
        exit(1);
    }

    // Prompt on stderr so it's not captured if stdout is redirected.
    std::cerr << "Passphrase: " << std::flush;
    std::string pw;
    std::getline(std::cin, pw);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldTerm);
    // Move past the (suppressed) newline the user pressed.
    std::cerr << std::endl;

    return pw;
}
