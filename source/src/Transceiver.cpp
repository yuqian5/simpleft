#include "../include/Transceiver.hpp"

bool Transceiver::packFile(const std::string &path) {
    char cmdOutputBuf[512];
    memset(cmdOutputBuf, 0, sizeof(cmdOutputBuf));

    std::string result;

    std::string cmd = "tar -cf .ft_temp_pack.tar.gz " + path + " 2>&1";
    FILE *pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    fgets(cmdOutputBuf, sizeof(cmdOutputBuf), pipe);
    result += cmdOutputBuf;
    pclose(pipe);

    // check if tar operation failed, if yes, remove temp file and return false
    if (result.find("No such file") != std::string::npos) {
        pipe = popen("rm .ft_temp_pack.tar.gz", "r");
        pclose(pipe);
        return false;
    }
    return true;
}

void Transceiver::unpackFile() {
    FILE *pipe = popen("tar -xf .ft_temp_pack_buffer.tar.gz", "r");
    pclose(pipe);
}

void Transceiver::deletePackedFile() {
    FILE *pipe = popen("rm .ft_temp_pack.tar.gz", "r");
    pclose(pipe);
}

void Transceiver::deletePackedBufferFile() {
    FILE *pipe = popen("rm .ft_temp_pack_buffer.tar.gz", "r");
    pclose(pipe);
}

