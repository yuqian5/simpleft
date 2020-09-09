#include <iostream>

#include "../include/TX.hpp"
#include "../include/RX.hpp"
#include "../include/misc.hpp"

int main(int argc, char *argv[]) {
    //get and sanitize cmd arguments store in struct CMDARGS
    CMDARGS info = parseInput(argv, argc);

    //mode 1 = rx, 2 = tx
    if (info.mode == 1) {
        // prompt
        std::cout << "Receiving on port " << info.port << " - ";

        // receive
        RX rx(std::ref(info));
    } else {
        // prompt
        std::cout << "Sending " << info.filePath << " to " << info.ip << " on port " << info.port << std::endl;

        // transmit
        TX tx(info);
    }

    // exit
    return 0;
}