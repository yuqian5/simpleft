#include <cstring>
#include "Transceiver.hpp"

bool Transceiver::packFile(const std::string &path) {
    char readBuf[512];
    memset(readBuf, 0, sizeof(readBuf));

    std::string result;

    std::string cmd = "tar -zcf tempFilePackage.tar.gz " + path + " 2>&1";
    FILE *pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    fgets(readBuf, sizeof(readBuf), pipe);
    result += readBuf;
    pclose(pipe);

    // check if tar operation failed, if yes, remove temp file and return false
    if (result.find("No such file") != std::string::npos) {
        pipe = popen("rm tempFilePackage.tar.gz", "r");
        pclose(pipe);
        return false;
    }
    return true;
}

void Transceiver::unpackFile() {
    FILE *pipe = popen("tar -xzf tempFilePackage.tar.gz", "r");
    pclose(pipe);
}

void Transceiver::deleteFile() {
    FILE *pipe = popen("rm tempFilePackage.tar.gz", "r");
    pclose(pipe);
}

std::string Transceiver::shasum() noexcept(false) {
    char readBuf[256];
    memset(readBuf, 0, sizeof(readBuf));

    std::string result;

    // run shasum
    std::string cmd = "shasum -a 256 tempFilePackage.tar.gz";
    FILE *pipe = popen(cmd.c_str(), "r");

    // get output
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    } else {
        fgets(readBuf, sizeof(readBuf), pipe);
        result = readBuf;
    }
    pclose(pipe);

    // check if operation was successful
    if (result.length() < 64) {
        throw std::runtime_error("Unable to open file");
    }

    // return shasum
    return result;
}

