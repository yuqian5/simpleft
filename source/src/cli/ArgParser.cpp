// Argv parser. The CLI has a small fixed grammar:
//
//   sft rx -p <port> -k
//   sft tx (-ip4 <addr> | -ip6 <addr>) -p <port> -k <path>
//
// Rules worth knowing before editing:
//   * `rx` / `tx` are positional tokens, not flags — they can appear anywhere.
//   * `-k` is a boolean; the passphrase is read from the terminal after parsing.
//   * `-ip4` / `-ip6` and `-p` each consume the next argv token as their value
//     (POSIX-getopt semantics — we do not validate that the value isn't itself
//     a flag, so `sft rx -p -k` is a user error that surfaces downstream).
//   * The file path is a single positional, accepted in any position (not just
//     last) because the loop tracks it explicitly.

#include "../../include/cli/ArgParser.hpp"

#include <string>
#include <sys/stat.h>

#include "../../include/cli/IpFlag.hpp"
#include "../../include/cli/PassphraseFlag.hpp"
#include "../../include/cli/PortFlag.hpp"
#include "../../include/Logging.hpp"

static void usage() {
    Logging::logError("Usage: sft rx -p <port> -k");
    Logging::logError("       sft tx (-ip4 <addr> | -ip6 <addr>) -p <port> -k <path>");
}

// Pull the token after argv[i] as `flag`'s value, advancing i. Aborts if the
// flag is at the end of argv with nothing after it.
static std::string nextValue(char *cmd[], int argc, int &i, const std::string &flag) {
    if (i + 1 >= argc) {
        Logging::logError(flag + " requires a value");
        usage();
        exit(1);
    }
    return cmd[++i];
}

CMD_ARGS parseInput(char *cmd[], int &argc) {
    CMD_ARGS cmdArgs = {};
    // defaults used when the corresponding flag is absent or invalid
    cmdArgs.port = 8203;
    cmdArgs.ip = "127.0.0.1";
    cmdArgs.standard = "ipv4";

    bool modeSet = false;
    bool kSet = false;
    std::string positional;
    bool positionalSet = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = cmd[i];

        if (arg == "rx" || arg == "tx") {
            if (modeSet) {
                Logging::logError("Program can be run in either tx mode or rx mode. Not both.");
                exit(1);
            }
            cmdArgs.mode = (arg == "rx") ? 1 : 2;
            modeSet = true;
        } else if (arg == "-ip4" || arg == "-ip6") {
            std::string value = nextValue(cmd, argc, i, arg);
            if (!checkIP(value)) {
                exit(1);
            }
            cmdArgs.ip = value;
            cmdArgs.standard = (arg == "-ip4") ? "ipv4" : "ipv6";
        } else if (arg == "-p") {
            std::string value = nextValue(cmd, argc, i, arg);
            // checkPort already logs on failure; on failure we silently keep
            // the default port set above.
            if (checkPort(value)) {
                cmdArgs.port = std::stoi(value);
            }
        } else if (arg == "-k") {
            kSet = true;
        } else {
            // Anything not matching a known flag is treated as the file path.
            // Exactly one positional is permitted.
            if (positionalSet) {
                Logging::logError("Unexpected extra argument: " + arg);
                usage();
                exit(1);
            }
            positional = arg;
            positionalSet = true;
        }
    }

    if (!modeSet) {
        Logging::logError("No mode (tx/rx) is provided");
        usage();
        exit(1);
    }

    if (!kSet) {
        Logging::logError("Encryption is mandatory; pass -k to enter a passphrase");
        exit(1);
    }

    // Prompt only after the rest of argv has validated, so a typo doesn't
    // waste the user's effort entering a passphrase.
    cmdArgs.passphrase = promptPassphrase();
    if (cmdArgs.passphrase.empty()) {
        Logging::logError("Passphrase cannot be empty");
        exit(1);
    }

    if (cmdArgs.mode == 2) {
        if (!positionalSet) {
            Logging::logError("tx mode requires a file path");
            usage();
            exit(1);
        }
        cmdArgs.filePath = positional;

        // Probe early so the user sees "no such file" before we open a socket.
        struct stat buffer{};
        if (stat(cmdArgs.filePath.c_str(), &buffer) != 0) {
            Logging::logError("No such file or directory");
            exit(1);
        }
    } else if (positionalSet) {
        Logging::logError("rx mode does not take a file path");
        usage();
        exit(1);
    }

    return cmdArgs;
}
