#include "../../include/cli/PortFlag.hpp"

#include <stdexcept>

#include "../../include/Logging.hpp"

// Accept ports in [1024, 65535]. The lower bound excludes well-known
// privileged ports (0-1023) so sft can run unprivileged on both sides.
// On invalid input we log a warning and let the caller fall back to the
// default port (8203).
bool checkPort(const std::string &port_str) {
    try {
        int port = std::stoi(port_str, nullptr, 10);
        if (port < 1024 || port > 65535) {
            throw std::range_error("port number out of range");
        }
    } catch (const std::invalid_argument &) {
        Logging::logWarning("Invalid port number, port must be between 1024 and 65535, using default port: 8203");
        return false;
    } catch (const std::range_error &) {
        Logging::logWarning("Invalid port number, port must be between 1024 and 65535, using default port: 8203");
        return false;
    }
    return true;
}
