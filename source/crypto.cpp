#include "crypto.h"

crypto::crypto() = default;

crypto::~crypto() = default;

int crypto::checkPrime(unsigned long long int prime) {
    unsigned long long int sr = sqrt(prime);
    for (unsigned long long int i = 2; i <= sr; i += 2) {
        if (prime % i == 0) {
            return 0;
        }
    }
    return 1;
}

int crypto::checkPRoot(unsigned long long int alpha, unsigned long long int q) {

}

unsigned long long int crypto::getAlpha(unsigned long long int q) {
    // create random number generator, multiple seed is created to gather enough entropy
    uniform_int_distribution<long long unsigned int> uniform_dist(0, q);
    random_device seed1;
    mt19937_64 seed2(seed1());
    seed2.discard(700000);
    mt19937_64 seed3(uniform_dist(seed2));
    seed3.discard(700000);
    mt19937_64 seed4(uniform_dist(seed3));
    seed4.discard(700000);
    mt19937_64 seed5(uniform_dist(seed4));
    seed5.discard(700000);

    long long unsigned int alpha;
    while (true) {
        alpha = uniform_dist(seed5);
        if (checkPRoot(alpha, q)) {
            return alpha;
        }
    }
}

unsigned long long int crypto::getQ() {
    // create random number generator, multiple seed is created to gather enough entropy
    uniform_int_distribution<long long unsigned int> uniform_dist(0, 0xFFFFFFFFFFFFFFFF);
    random_device seed1;
    mt19937_64 seed2(seed1());
    seed2.discard(700000);
    mt19937_64 seed3(uniform_dist(seed2));
    seed3.discard(700000);
    mt19937_64 seed4(uniform_dist(seed3));
    seed4.discard(700000);
    mt19937_64 seed5(uniform_dist(seed4));
    seed5.discard(700000);

    long long unsigned int q;
    while (true) {
        q = uniform_dist(seed5);
        if (checkPrime(q)) {
            return q;
        }
    }
}
