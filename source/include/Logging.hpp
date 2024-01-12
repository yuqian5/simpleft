#include <iostream>
#include <iomanip>

#ifndef FT_LOGGING_H
#define FT_LOGGING_H

class Logging {
private:
    static std::mutex logMutex;
    static clock_t lastLogTime;

public:
    static void logInfo(const std::string &msg);

    static void logError(const std::string &msg);

    static void logWarning(const std::string &msg);

    static void logSuccess(const std::string &msg);

    static void log(const std::string &msg, const std::string &color);

    static void logProgress(size_t progress, size_t total, bool ignoreTimeLimit = false);

    static void logDeleteLine();

};

#endif //FT_LOGGING_H
