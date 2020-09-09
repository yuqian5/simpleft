#include "../include/misc.hpp"

bool checkPort(const std::string &port_str) {
    int port;
    try {
        port = stoi(port_str, nullptr, 10);
        if (port < 1023 || port > 65535) {
            throw std::range_error("port number out of range, 1024<=port<=65535");
        }
    } catch (const std::invalid_argument &e) {
        std::cerr << "ERROR: Invalid Argument: " << "port number must be a int in base 10" << std::endl;
        return false;
    } catch (const std::range_error &e) {
        std::cerr << "ERROR: Invalid Argument: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool checkIP(const std::string &IP_str) {
    try {
        std::string pattern = "((^\\s*((([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]))\\s*$)|(^\\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:)))(%.+)?\\s*$))";
        std::regex regex = std::regex(pattern);
        if (regex_match(IP_str, regex)) { // validate ip
            return true;
        }
        throw std::domain_error("Invalid IP Address");
    } catch (std::domain_error &e) {
        std::cerr << "ERROR: Input Error: " << e.what() << std::endl;
        return false;
    }
}

bool extractIP(const std::string &argv_str, CMDARGS &cmdargs) {
    int start_index = 0;

    // check if ipv4 is provided
    start_index = argv_str.find("ip4");
    if (start_index != std::string::npos) { // if ip4 is found in argv
        // extract serverAddr
        int end_index = argv_str.substr(start_index + 4, argv_str.length()).find(' ');
        cmdargs.ip = argv_str.substr(start_index + 4, end_index);

        // check if ip is valid
        if (!checkIP(cmdargs.ip)) { // ip not valid, fatal error, exit
            return false;
        }
        cmdargs.standard = "ipv4";
        return true;
    }
    // check if ipv6 is provided
    start_index = argv_str.find("ip6");
    if (start_index != std::string::npos) { // if ip6 is found in argv
        // extract serverAddr
        int end_index = argv_str.substr(start_index + 4, argv_str.length()).find(' ');
        cmdargs.ip = argv_str.substr(start_index + 4, end_index);

        // check if ip is valid
        if (!checkIP(cmdargs.ip)) { // ip not valid, fatal error, exit
            return false;
        }
        cmdargs.standard = "ipv6";
        return true;
    }
    return false;
}

bool extractPort(const std::string &argv_str, CMDARGS &cmdargs) {
    int start_index = 0;

    start_index = argv_str.find("-p");
    if (start_index != std::string::npos) { // if -p exist in argv
        // extract port
        int end_index = argv_str.substr(start_index + 3, argv_str.length()).find(' ') + start_index + 3;
        std::string port_str = argv_str.substr(start_index + 3, end_index);
        if (checkPort(port_str)) {
            cmdargs.port = std::stoi(port_str, nullptr, 10);
        } else {
            std::cerr << "ERROR: No valid port number is provided, port number must be between 1024 and 65535"
                      << std::endl;
            return false;
        }
    } else { // port is not provided, fatal error, exit
        std::cerr << "ERROR: No valid port number is provided, port number must be between 1024 and 65535" << std::endl;
        return false;
    }
    return true;
}

bool extractMode(const std::string &argv_str, CMDARGS &cmdargs) {
    int mode = 0;
    if (argv_str.find("rx") != std::string::npos) { // check if only rx exist in argv
        if (argv_str.find("tx") == std::string::npos) { // tx does not exist in argv, mode is rx
            mode = 1;
        } else { // tx and rx both present in argv, fatal error
            std::cerr << "ERROR: Program can be run in either tx mode or rx mode. Not Both." << std::endl;
            return false;
        }
    } else if (argv_str.find("tx") != std::string::npos) { // check if only tx exist in argv
        if (argv_str.find("rx") == std::string::npos) { // rx is not in argv, mode is tx
            mode = 2;
        } else { // tx and rx both present in argv, fatal error
            std::cerr << "ERROR: Program can be run in either tx mode or rx mode. Not Both." << std::endl;
            return false;
        }
    } else { // no mode argument found, fatal error, exit
        std::cerr << "ERROR: A mode (tx/rx) must be specified in the commandline arguments" << std::endl;
    }
    cmdargs.mode = mode;
    return true;
}

CMDARGS parseInput(char *cmd[], int &argc) {
    CMDARGS cmdargs = {}; // to be returned

    // convert argv to a string, then filter using find
    std::string argv_str;
    for (int i = 1; i < argc; i++) {
        argv_str += " ";
        argv_str += cmd[i];
    }

    // extract mode
    if (!extractMode(std::ref(argv_str), std::ref(cmdargs))) {
        exit(0);
    }

    // extract ip if tx
    if (cmdargs.mode == 2) {
        if (!extractIP(std::ref(argv_str), std::ref(cmdargs))) {
            exit(0);
        }
    }

    // extract port
    if (!extractPort(std::ref(argv_str), std::ref(cmdargs))) {
        exit(0);
    }

    // get dir option and get filePath, filePath is always the last argument in argv
    if (cmdargs.mode == 2) {
        cmdargs.directory = argv_str.find("-d") != std::string::npos;
        cmdargs.filePath = cmd[argc - 1];
    }

    // return parse argument
    return cmdargs;
}

