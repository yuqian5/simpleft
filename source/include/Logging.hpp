#ifndef FT_LOGGING_H
#define FT_LOGGING_H

#include <cstddef>
#include <iostream>
#include <mutex>
#include <string>

class Logging {
public:
    // Status lines. Each is prefixed with a Unicode symbol coloured by
    // severity. When the destination stream is not a TTY (pipe, file
    // redirect) the ANSI codes are stripped so logs read cleanly in
    // `tee`/`less`/CI.
    static void logInfo(const std::string &msg);     // → cyan,   stdout
    static void logSuccess(const std::string &msg);  // ✓ green,  stdout
    static void logWarning(const std::string &msg);  // ⚠ yellow, stderr
    static void logError(const std::string &msg);    // ✗ red,    stderr

    // Three-phase progress bar. Start once with the total byte count,
    // call Update on every chunk, Finish once after the last chunk to
    // emit the final state and terminate the line.
    //
    // Display when stdout is a TTY:
    //   [█████████░░░░░░░░░░░░░░░░░░░░░] 30% · 312 KB / 1.0 MB · 2.4 MB/s · 0:18
    //
    // When stdout is not a TTY, Update calls are silent; Start and Finish
    // each emit a single line so logs stay grep-able.
    static void logProgressStart(size_t total);
    static void logProgressUpdate(size_t bytesDone);
    static void logProgressFinish();

private:
    static std::mutex logMutex;
};

#endif //FT_LOGGING_H
