#include "../include/Transceiver.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdexcept>

bool Transceiver::packFile(const std::string &path) {
    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("fork() failed");
    }

    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execlp("tar", "tar", "-cf", ".ft_temp_pack.tar.gz", path.c_str(), nullptr);
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        throw std::runtime_error("waitpid() failed");
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return true;
    }

    unlink(".ft_temp_pack.tar.gz");
    return false;
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

