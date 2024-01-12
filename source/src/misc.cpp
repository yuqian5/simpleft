#include <iomanip>
#include <sys/stat.h>
#include "../include/misc.hpp"
#include "../include/Logging.hpp"

bool checkPort(const std::string &port_str) {
    int port;
    try {
        port = stoi(port_str, nullptr, 10);
        if (port < 1023 || port > 65535) {
            throw std::range_error("port number out of range");
        }
    } catch (const std::invalid_argument &e) {
        Logging::logError("Invalid port number, port must be between 1024 and 65535, using default port: 8203");
        return false;
    } catch (const std::range_error &e) {
        Logging::logError("Invalid port number, port must be between 1024 and 65535, using default port: 8203");
        return false;
    }
    return true;
}

bool checkIP(const std::string &IP_str) {
    try {
        std::string pattern = R"(((^\s*((([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]))\s*$)|(^\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?\s*$)))";
        std::regex regex = std::regex(pattern);
        if (regex_match(IP_str, regex)) { // validate ip
            return true;
        }
        throw std::domain_error("Invalid IP Address");
    } catch (std::domain_error &e) {
        Logging::logError("Invalid IP Address, using default ip: 127.0.0.1");
        return false;
    }
}

bool extractIP(const std::string &argv_str, CMD_ARGS &cmdArgs) {
    unsigned long start_index;

    // check if ipv4 is provided
    start_index = argv_str.find("ip4");
    if (start_index != std::string::npos) { // if ip4 is found in argv
        // extract serverAddr
        unsigned long end_index = argv_str.substr(start_index + 4, argv_str.length()).find(' ');
        cmdArgs.ip = argv_str.substr(start_index + 4, end_index);

        // check if ip is valid
        if (!checkIP(cmdArgs.ip)) { // ip not valid, fatal error, exit
            return false;
        }
        cmdArgs.standard = "ipv4";
        return true;
    }
    // check if ipv6 is provided
    start_index = argv_str.find("ip6");
    if (start_index != std::string::npos) { // if ip6 is found in argv
        // extract serverAddr
        unsigned long end_index = argv_str.substr(start_index + 4, argv_str.length()).find(' ');
        cmdArgs.ip = argv_str.substr(start_index + 4, end_index);

        // check if ip is valid
        if (!checkIP(cmdArgs.ip)) { // ip not valid, fatal error, exit
            return false;
        }
        cmdArgs.standard = "ipv6";
        return true;
    }
    return false;
}

bool extractPort(const std::string &argv_str, CMD_ARGS &cmdArgs) {
    unsigned long start_index;

    start_index = argv_str.find("-p");
    if (start_index != std::string::npos) { // if -p exist in argv
        // extract port
        unsigned long end_index = argv_str.substr(start_index + 3, argv_str.length()).find(' ') + start_index + 3;
        std::string port_str = argv_str.substr(start_index + 3, end_index);
        if (checkPort(port_str)) {
            cmdArgs.port = std::stoi(port_str, nullptr, 10);
        } else {
            Logging::logWarning("Invalid port number, port must be between 1024 and 65535, using default port: 8203");
            return false;
        }
    } else { // port is not provided, fatal error, exit
        Logging::logWarning("Port not provided, using default port: 8203");
        return false;
    }
    return true;
}

bool extractMode(const std::string &argv_str, CMD_ARGS &cmdArgs) {
    int mode;
    if (argv_str.find("rx") != std::string::npos) { // check if only rx exist in argv
        if (argv_str.find("tx") == std::string::npos) { // tx does not exist in argv, mode is rx
            mode = 1;
        } else { // tx and rx both present in argv, fatal error
            Logging::logError("Program can be run in either tx mode or rx mode. Not Both.");
            return false;
        }
    } else if (argv_str.find("tx") != std::string::npos) { // check if only tx exist in argv
        if (argv_str.find("rx") == std::string::npos) { // rx is not in argv, mode is tx
            mode = 2;
        } else { // tx and rx both present in argv, fatal error
            Logging::logError("Program can be run in either tx mode or rx mode. Not Both.");
            return false;
        }
    } else { // no mode argument found, fatal error, exit
        Logging::logError("No mode (tx/rx) is provided");
        return false;
    }
    cmdArgs.mode = mode;
    return true;
}

CMD_ARGS parseInput(char *cmd[], int &argc) {
    CMD_ARGS cmdArgs = {}; // to be returned

    // convert argv to a string, then filter using find
    std::string argv_str;
    for (int i = 1; i < argc; i++) {
        argv_str += " ";
        argv_str += cmd[i];
    }

    // extract mode
    if (!extractMode(std::ref(argv_str), std::ref(cmdArgs))) {
        exit(0);
    }

    // extract ip if tx
    if (cmdArgs.mode == 2) {
        if (!extractIP(std::ref(argv_str), std::ref(cmdArgs))) {
            Logging::logWarning("IP not provided, using default ip: 127.0.0.1");
            cmdArgs.ip = "127.0.0.1";
            cmdArgs.standard = "ipv4";
        }
    }

    // extract port
    if (!extractPort(std::ref(argv_str), std::ref(cmdArgs))) {
        cmdArgs.port = 8203;
    }

    // get filePath, filePath is always the last argument in argv
    if (cmdArgs.mode == 2) {
        cmdArgs.filePath = cmd[argc - 1];

        struct stat buffer{};
        if (stat(cmdArgs.filePath.c_str(), &buffer) != 0) {
            Logging::logError("No such file or directory");
            exit(0);
        }
    }

    // return parse argument
    return cmdArgs;
}



