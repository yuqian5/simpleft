#include "../include/Logging.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <unistd.h>

namespace {

// TTY detection - cached on first call so we're not syscalling on every log line.
bool stdoutIsTty() {
    static const bool cached = isatty(fileno(stdout));
    return cached;
}
bool stderrIsTty() {
    static const bool cached = isatty(fileno(stderr));
    return cached;
}

// Wrap `s` in `code...reset` only if the target is a TTY. ANSI escapes
// inside log files are noise; modern tools strip them automatically.
std::string paint(const char *code, const std::string &s, bool tty) {
    if (!tty) return s;
    return std::string(code) + s + "\033[0m";
}

void emitLine(std::ostream &out, const char *color, const char *symbol,
              const std::string &msg, bool tty) {
    out << paint(color, symbol, tty) << " " << msg << "\n";
    out.flush();
}

// "1.5 MB", "300 KB", "42 B"
std::string humanBytes(double bytes) {
    constexpr double KB = 1024.0, MB = KB * 1024.0, GB = MB * 1024.0;
    char buf[32];
    if (bytes >= GB)      std::snprintf(buf, sizeof(buf), "%.2f GB", bytes / GB);
    else if (bytes >= MB) std::snprintf(buf, sizeof(buf), "%.1f MB", bytes / MB);
    else if (bytes >= KB) std::snprintf(buf, sizeof(buf), "%.0f KB", bytes / KB);
    else                  std::snprintf(buf, sizeof(buf), "%.0f B",  bytes);
    return buf;
}

// "1:23" or "1:02:45" for longer durations. "--:--" when unknown.
std::string humanDuration(double seconds) {
    if (seconds < 0 || std::isnan(seconds) || std::isinf(seconds) || seconds > 99 * 3600) {
        return "--:--";
    }
    int total = static_cast<int>(seconds);
    int h = total / 3600;
    int m = (total / 60) % 60;
    int s = total % 60;
    char buf[16];
    if (h > 0) std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    else       std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}

// Single block-character progress bar of `width` cells.
std::string makeBar(double pct, int width) {
    if (pct < 0) pct = 0;
    if (pct > 1) pct = 1;
    int filled = static_cast<int>(pct * width);
    std::string out;
    out.reserve(width * 3);  // UTF-8 block char is 3 bytes
    out += "[";
    for (int i = 0; i < width; i++) out += (i < filled ? "█" : "░");
    out += "]";
    return out;
}

// Progress state. Lives at file scope so the three-phase API (Start/Update/
// Finish) can share it across calls without making the public class hold it.
struct ProgressState {
    bool active = false;
    size_t total = 0;
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point lastUpdate;
} progressState;

void renderProgress(size_t done, bool finalFrame) {
    if (!progressState.active) return;

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - progressState.start).count();
    double pct = progressState.total > 0
                 ? static_cast<double>(done) / static_cast<double>(progressState.total)
                 : (finalFrame ? 1.0 : 0.0);
    // Average speed from session start is more stable than instantaneous
    // for transfers of any meaningful size, and trivially correct on the
    // final frame.
    double speed = elapsed > 0.001 ? done / elapsed : 0;
    double remaining = (progressState.total > done && speed > 0)
                       ? (progressState.total - done) / speed
                       : (finalFrame ? 0 : -1);

    std::ostringstream line;
    line << makeBar(pct, 30) << " "
         << static_cast<int>(pct * 100) << "%  "
         << humanBytes(static_cast<double>(done))   << " / "
         << humanBytes(static_cast<double>(progressState.total)) << "  "
         << humanBytes(speed) << "/s  ETA " << humanDuration(remaining);

    if (stdoutIsTty()) {
        // Carriage-return + clear-to-end-of-line, then redraw. Cyan arrow
        // matches the rest of the info-level styling.
        std::cout << "\r\033[K" << paint("\033[36m", "→", true) << " " << line.str();
        if (finalFrame) std::cout << "\n";
        std::cout.flush();
    } else if (finalFrame) {
        // Not a TTY: emit only the final state so log files stay readable.
        std::cout << "→ " << line.str() << "\n";
        std::cout.flush();
    }
}

}  // anonymous namespace

std::mutex Logging::logMutex;

void Logging::logInfo(const std::string &msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    emitLine(std::cout, "\033[36m", "→", msg, stdoutIsTty());  // → cyan
}

void Logging::logSuccess(const std::string &msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    emitLine(std::cout, "\033[32m", "✓", msg, stdoutIsTty());  // ✓ green
}

void Logging::logWarning(const std::string &msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    emitLine(std::cerr, "\033[33m", "⚠", msg, stderrIsTty());  // ⚠ yellow
}

void Logging::logError(const std::string &msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    emitLine(std::cerr, "\n\033[1;31m", "✗", msg, stderrIsTty()); // ✗ bold red
}

void Logging::logProgressStart(size_t total) {
    std::lock_guard<std::mutex> lock(logMutex);
    progressState.active = true;
    progressState.total = total;
    progressState.start = std::chrono::steady_clock::now();
    progressState.lastUpdate = progressState.start;
    renderProgress(0, false);
}

void Logging::logProgressUpdate(size_t bytesDone) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (!progressState.active) return;
    // Throttle to ~5 frames/sec so the redraw doesn't dominate the transfer.
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - progressState.lastUpdate).count() < 200) {
        return;
    }
    progressState.lastUpdate = now;
    renderProgress(bytesDone, false);
}

void Logging::logProgressFinish() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (!progressState.active) return;
    renderProgress(progressState.total, true);
    progressState.active = false;
}
