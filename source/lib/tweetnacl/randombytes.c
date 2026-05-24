/*
 * TweetNaCl declares `randombytes` but deliberately leaves it for the
 * embedder to define. This is the project's implementation, reading from
 * /dev/urandom.
 *
 * /dev/urandom (not /dev/random): on modern Linux/macOS both draw from the
 * same CSPRNG once the pool is initialised at boot. urandom never blocks,
 * which matters because randombytes() is called from the crypto hot path.
 *
 * Failure policy is abort(). Returning an error would force every caller in
 * tweetnacl.c to thread the failure through, and "we can't get entropy" is
 * not a recoverable condition for a tool that exists to encrypt traffic.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

void randombytes(unsigned char *x, unsigned long long xlen) {
    int fd;
    do {
        fd = open("/dev/urandom", O_RDONLY);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
        abort();
    }

    while (xlen > 0) {
        // read() on /dev/urandom is capped at 32 MiB on Linux and may
        // short-read; loop in 1 MiB chunks and let the outer loop reassemble.
        unsigned long long want = xlen > 1048576ULL ? 1048576ULL : xlen;
        ssize_t got = read(fd, x, (size_t) want);
        if (got < 0) {
            if (errno == EINTR) continue;
            abort();
        }
        if (got == 0) {
            // EOF from /dev/urandom should never happen; treat as fatal.
            abort();
        }
        x += got;
        xlen -= (unsigned long long) got;
    }

    close(fd);
}
