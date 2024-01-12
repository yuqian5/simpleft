#include "../include/Logging.hpp"

std::mutex Logging::logMutex = std::mutex();
clock_t Logging::lastLogTime = clock();

void Logging::log(const std::string &msg, const std::string &color) {
    logMutex.lock();

    time_t rawTime;
    struct tm * timeInfo;

    time ( &rawTime );
    timeInfo = localtime ( &rawTime );
    std::cout << color;
    std::cout << std::put_time(timeInfo, "[%Y/%m/%d %H:%M:%S] ") << msg << std::endl;
    std::cout << "\033[0m";

    logMutex.unlock();
}

void Logging::logInfo(const std::string &msg) {
    log("\033[1;96m" + msg, "\033[1;96m");
}

void Logging::logError(const std::string &msg) {
    log("\033[1;91m" + msg, "\033[1;91m");
}

void Logging::logWarning(const std::string &msg) {
    log("\033[1;93m" + msg, "\033[1;93m");
}

void Logging::logSuccess(const std::string &msg) {
    log("\033[1;92m" + msg, "\033[1;92m");
}

void Logging::logProgress(size_t progress, size_t total, bool ignoreTimeLimit) {
    // only update progress every 1 seconds
    if (!ignoreTimeLimit) {
        auto currentTime = clock();
        if ((double) (currentTime - lastLogTime) / CLOCKS_PER_SEC < 1) {
            return;
        }
        lastLogTime = currentTime;
    }

    logDeleteLine();

    logMutex.lock();

    auto percentage = ((double) progress / (double) total);

    std::string msg = "Progress: [";

    for (int i = 0; i < percentage * 30; i++) {
        msg += "#";
    }

    for (int i = 0; i < 30 - percentage * 30; i++) {
        msg += " ";
    }

    msg += "] " + std::to_string((int) (percentage * 100)) + "%";

    std::cout << msg << std::flush;

    logMutex.unlock();
}

void Logging::logDeleteLine() {
    logMutex.lock();

    for (int i = 0; i < 47; i++) {
        std::cout << "\b";
    }

    std::cout << std::flush;

    logMutex.unlock();
}
