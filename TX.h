#include <fstream>

#include "mystruct.h"

class TX {
public:
    TX(CMDARGS cmd){
        info = cmd;
        main();
    }
    ~TX()= default;

private:
    CMDARGS info;


    int main();
    int send();
    int pack();
    int getFile();
};

