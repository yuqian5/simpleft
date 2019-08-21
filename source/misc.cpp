#include "misc.h"

// get the sha265 sum of any file
std::string shasum(std::string fileName){
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
int checkPort(const std::string& port_str){
    int port;
    try{
        port = stoi(port_str);
        if(port < 1023 || port > 65535){
            throw std::range_error("port number out of range 1024<=port<=65535");
        }
    }
    catch(const std::invalid_argument& em){
        std::cerr << "Invalid argument: " << "port number must be a int in base 10" << std::endl;
        exit(1);
    }
    catch(const std::range_error& em) {
        std::cerr << "Invalid argument: " << em.what() << std::endl;
        exit(1);
    }
    return port;
}

// check IP addr, if valid, return true, else print error and exit
bool checkIP(const std::string& IP_str){
    try{
        if (regex_match(IP_str, std::regex("\\b(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\b"))){ // validate ip
            return true;
        }
        throw std::domain_error("IP address inputted not valid");
    }
    catch(std::domain_error& em){
        std::cerr << "Input Error: " << em.what() << std::endl;
        exit(1);
    }

}

CMDARGS parseCMDInput(char **cmd, int argc){
    // basic commandline format check
    if(argc == 7 || argc == 6){
        if(strcmp(cmd[1],"-rx")==0||strcmp(cmd[1],"-tx")==0){
            if(strcmp(cmd[2],"-ip")==0){
                if(strcmp(cmd[4],"-p")==0){
                    goto LOADARG;
                }
            }
        }
    }
    std::cerr << "Invalid argument: Please see README.md for proper commandline argument usage" << std::endl;
    exit(1);

    // convert and load arguments
    LOADARG:
    CMDARGS info = {};
    //load mode
    if(strcmp(cmd[1], "-rx") == 0){
        info.mode = 1;
    }else if(strcmp(cmd[1], "-tx") == 0){
        info.mode = 2;
    }
    //load ip
    if(checkIP(cmd[3])){
        info.ip = cmd[3];
    }
    //load port
    info.port = checkPort(cmd[5]);
    //load file path
    if(info.mode == 2){
        info.filePath = cmd[6];

    }

    return info;
}

//TODO : parseInput incomplete
CMDARGS parseInput(char* cmd[], int argc){
    int c;

    while(true){
        int opt_index = 1;
        static struct option long_options[] = {
                {"rx", 0, 0, 0 },
                {"tx", 0, 0, 0 },
                {"ip", 1, 0, 0 },
                {"p" , 1, 0, 0 },
                {0   , 0, 0, 0}
        };

        c = getopt_long(argc, cmd, "rx:tx:ip:p",
                        long_options, &opt_index);
        if(c == -1){
            break;
        }
    }


}