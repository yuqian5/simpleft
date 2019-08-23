#include "misc.h"

// get the sha265 sum of any file
std::string shasum(const std::string &fileName) {
    char buffer[256];
    std::string result;
    std::string cmd = "shasum -a 256 ";
    cmd += fileName;
    FILE *pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    fgets(buffer, sizeof(buffer), pipe);
    result += buffer;
    pclose(pipe);
    return result;
}

// check port number, if legal, return port number, else print error and exit
int checkPort(const std::string &port_str) {
    int port;
    try {
        port = stoi(port_str);
        if (port < 1023 || port > 65535) {
            throw std::range_error("port number out of range 1024<=port<=65535");
        }
    }
    catch (const std::invalid_argument &em) {
        std::cerr << "Invalid argument: " << "port number must be a int in base 10" << std::endl;
        exit(1);
    }
    catch (const std::range_error &em) {
        return -1;
    }
    return port;
}

// check IP addr, if valid, return true, else print error and exit
bool checkIP(const std::string &IP_str) {
    try {
        if (regex_match(IP_str, std::regex(
                "\\b(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\b"))) { // validate ip
            return true;
        }
        throw std::domain_error("IP address inputted not valid");
    }
    catch (std::domain_error &em) {
        std::cerr << "Input Error: " << em.what() << std::endl;
        return false;
    }

}

CMDARGS parseInput(char *cmd[], int argc) {
    CMDARGS args = {}; // to be returned

    // convert argv to a string, then filter using find
    std::string argv_str;
    for (int i = 1; i < argc; i++) {
        argv_str += " ";
        argv_str += cmd[i];
    }

    // get mode
    int mode = 0;
    if (argv_str.find("rx") != std::string::npos) { // check if only rx exist in argv
        if (argv_str.find("tx") == std::string::npos) { // tx does not exist in argv, mode is rx
            mode = 1;
        } else { // tx and rx both present in argv, fatal error
            std::cerr << "Program can be run in either tx mode or rx mode. Not Both." << std::endl;
            exit(1);
        }
    } else if (argv_str.find("tx") != std::string::npos) { // check if only tx exist in argv
        if (argv_str.find("rx") == std::string::npos) { // rx is not in argv, mode is tx
            mode = 2;
        } else { // tx and rx both present in argv, fatal error
            std::cerr << "Program can be run in either tx mode or rx mode. Not Both." << std::endl;
            exit(1);
        }
    } else { // no mode argument found, fatal error, exit
        std::cerr << "A mode (tx/rx) must be specified in the commandline arguments" << std::endl;
    }
    args.mode = mode;

    // get ip
    int start_index = 0;
    start_index = argv_str.find("ip");
    if (start_index != std::string::npos) { // if ip is found in argv
        int end_index = argv_str.substr(start_index + 3, argv_str.length()).find(' ');
        args.ip = argv_str.substr(start_index + 3, end_index);
        // check if ip is valid
        if (!checkIP(args.ip)) { // ip not valid, fatal error, exit
            std::cerr << "No Valid ip address is provided, ip address must be a ipv4 address" << std::endl;
            exit(1);
        }
    } else { // ip is not provided, fatal error, exit
        std::cerr << "Missing ip address" << std::endl;
        exit(1);
    }

    // get port
    start_index = argv_str.find("-p");
    if (start_index != std::string::npos) { // if -p exist in argv
        int end_index = argv_str.substr(start_index + 3, argv_str.length()).find(' ');
        std::string port_str = argv_str.substr(start_index + 3, end_index);
        // check if port is valid
        args.port = checkPort(port_str);
        if (args.port < 0) { // no valid port provided, fatal error, exit
            std::cerr << "No Valid port number is provided, port number must be between 1024 and 65535" << std::endl;
            exit(1);
        }
    } else { // port is not provided, fatal error, exit
        std::cerr << "Missing port number" << std::endl;
        exit(1);
    }

    // get dir option and get filePath, filePath is always the last argument in argv
    if (args.mode == 2) {
        args.dir = argv_str.find("-d") != std::string::npos;
        args.filePath = cmd[argc - 1];
    }

    return args;
}

//TODO: unable to detect is taring has failed, need fix
int packDir(std::string path) {
    char buffer[256];
    std::string result;
    std::string cmd = "tar -zcf lanft_temp.tar.gz " + path + " 2>&1";
    FILE *pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    fgets(buffer, 256, pipe);
    result += buffer;
    if (result.find("No such file") != std::string::npos) {
        return 0;
    }
    pclose(pipe);
    return 1;
}

void unpackDir() {
    system("tar -xzf lanft_temp.tar.gz");
    system("rm lanft_temp.tar.gz");
}