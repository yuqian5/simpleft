#include <iostream>

#include "../include/TX.hpp"
#include "../include/RX.hpp"
#include "../include/misc.hpp"
#include "../include/Logging.hpp"

int main(int argc, char *argv[]) {
    //get and sanitize cmd arguments store in struct commandline args
    CMD_ARGS info = parseInput(argv, argc);

    //mode 1 = rx, 2 = tx
    if (info.mode == 1) {
        // prompt
        Logging::logInfo("Receiving on port " + std::to_string(info.port));

        // receive
        RX rx(std::ref(info));
    } else {
        // prompt
        Logging::logInfo("Sending to " + info.ip + " on port " + std::to_string(info.port));
        Logging::logInfo("File to transport " + info.filePath);

        // transmit
        TX tx(info);
    }

    // exit
    return 0;
}