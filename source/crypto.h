#pragma once

#include <random>
#include <cmath>

using namespace std;

class crypto {
public:
    unsigned long long int getQ();

    unsigned long long int getAlpha(unsigned long long int q);

    crypto();

    ~crypto();

private:
    int checkPrime(unsigned long long int num);

    int checkPRoot(unsigned long long int alpha, unsigned long long int q);
};
