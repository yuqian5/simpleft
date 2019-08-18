#include "misc.h"

// function that get the sharsun 256 of a file
std::string shasum(std::string fileName) {
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