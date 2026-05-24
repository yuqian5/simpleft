// Argv parser. The CLI has a small fixed grammar:
//
//   sft rx -p <port> -k
//   sft tx (-ip4 <addr> | -ip6 <addr>) -p <port> -k <path>
//
// Rules worth knowing before editing:
//   * `rx` / `tx` are positional tokens, not flags - they can appear anywhere.
//   * `-k` is a boolean; the passphrase is read from the terminal after parsing.
//   * `-ip4` / `-ip6` and `-p` each consume the next argv token as their value
//     (POSIX-getopt semantics - we do not validate that the value isn't itself
//     a flag, so `sft rx -p -k` is a user error that surfaces downstream).
//   * The file path is a single positional, accepted in any position (not just
//     last) because the loop tracks it explicitly.

#include "../../include/cli/ArgParser.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/stat.h>

#include "../../include/cli/IpFlag.hpp"
#include "../../include/cli/PassphraseFlag.hpp"
#include "../../include/cli/PortFlag.hpp"
#include "../../include/Logging.hpp"

static void usage() {
    Logging::logError("Usage: sft rx -p <port> -k [-loop]");
    Logging::logError("       sft tx (-ip4 <addr> | -ip6 <addr>) -p <port> -k <path>");
    Logging::logError("Try 'sft -h' for more information.");
}

// Plain stdout help, no ANSI colour. Printed for `-h` / `--help` and
// followed by exit(0). Distinct from usage() (red-colored, on errors).
static void printHelp() {
    std::cout <<
        "sft - simple peer-to-peer encrypted file transfer\n"
        "\n"
        "Usage:\n"
        "  sft rx -p <port> -k [-loop]\n"
        "  sft tx (-ip4 <addr> | -ip6 <addr>) -p <port> -k <path>\n"
        "\n"
        "Modes:\n"
        "  rx              Receive mode: listen for an incoming transfer.\n"
        "  tx              Transmit mode: connect to a receiver and send <path>.\n"
        "\n"
        "Flags:\n"
        "  -ip4 <addr>     IPv4 address of the peer (default 127.0.0.1).\n"
        "  -ip6 <addr>     IPv6 address of the peer.\n"
        "  -p <port>       TCP port (default 8203, range 1024-65535).\n"
        "  -k              Prompt for the passphrase on the controlling terminal\n"
        "                  with echo disabled. If stdin is piped, reads one line\n"
        "                  plainly. Alternatively, set SFT_PASSPHRASE in the\n"
        "                  environment to provide the passphrase non-interactively.\n"
        "  -loop           rx-only: after each transfer, keep listening for the\n"
        "                  next one. Connection-level failures (bad handshake,\n"
        "                  wrong passphrase) are non-fatal in this mode.\n"
        "  -h, --help      Show this help and exit.\n"
        "\n"
        "Encryption is mandatory. Traffic is end-to-end encrypted with\n"
        "Curve25519 + XSalsa20-Poly1305; both sides must agree on the same\n"
        "passphrase or the receiver aborts on the first packet.\n"
        "\n"
        "The path may be a file or a directory; it is tar-packed before sending.\n"
        "\n"
        "Project: https://github.com/yuqian5/simpleft\n";
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

        if (arg == "-h" || arg == "--help") {
            printHelp();
            exit(0);
        }

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
        } else if (arg == "-loop") {
            cmdArgs.loop = true;
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

    if (cmdArgs.loop && cmdArgs.mode != 1) {
        Logging::logError("-loop is only valid in rx mode");
        usage();
        exit(1);
    }

    // Source the passphrase. With -k, prompt interactively (after argv has
    // fully validated, so a typo doesn't waste the user's typing). Without
    // -k, fall back to the SFT_PASSPHRASE env var so the tool is scriptable
    // in headless contexts. The env var is unset immediately after copying
    // so it does not leak to any child process (notably the `tar` exec).
    if (kSet) {
        cmdArgs.passphrase = promptPassphrase();
        if (cmdArgs.passphrase.empty()) {
            Logging::logError("Passphrase cannot be empty");
            exit(1);
        }
    } else {
        const char *env = std::getenv("SFT_PASSPHRASE");
        if (env == nullptr || env[0] == '\0') {
            Logging::logError("Encryption is mandatory; pass -k to enter a passphrase or set SFT_PASSPHRASE");
            exit(1);
        }
        cmdArgs.passphrase = env;
        unsetenv("SFT_PASSPHRASE");
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
