#include <iostream>

using namespace std;

#include "TX.h"
#include "RX.h"
#include "misc.h"


int main(int argc, char *argv[]) {
    //get and sanitize cmd arguments store in struct CMDARGS
    CMDARGS info = parseInput(argv, argc);

    //mode 1 = rx, 2 = tx
    if (info.mode == 1) {
        cout << "Receiving on port " << info.port << endl;
        RX rx(info);
    } else {
        if (info.dir) { // if the path points to a directory, pack it into a tar.gz
            if (!packDir(info.filePath)) {
                std::cerr << "Unable to tar directory, make sure you have the correct path" << std::endl;
                exit(1);
            }
            info.filePath = "lanft_temp.tar.gz";
        }
        cout << "Sending " << info.filePath << " to " << info.ip << " on port " << info.port << endl;
        TX tx(info);

        // rm the tar.gz file
        system("rm lanft_temp.tar.gz");
    }

    return 0;
}