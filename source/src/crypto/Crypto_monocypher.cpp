// Monocypher implementation of Crypto.hpp.
//
// Primitives:
//   * X25519 for ephemeral key agreement (crypto_x25519_public_key + crypto_x25519)
//   * BLAKE2b-256 to mix the passphrase into the derived key
//   * XChaCha20-Poly1305 AEAD for per-packet encryption
//
// Wire frame layout is identical to the TweetNaCl backend:
//   [NONCE_SIZE nonce] [MAC_SIZE tag] [N bytes ciphertext]    = 40 + N bytes
// but the stream cipher (XChaCha20 here vs XSalsa20 in TweetNaCl) differs,
// so the two backends are NOT wire-compatible. Both endpoints must be
// built with the same SFT_CRYPTO_BACKEND.

#include "../../include/Crypto.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "../../include/sft_constants.hpp"

extern "C" {
#include "monocypher.h"
}

namespace {

// Read `n` cryptographically-strong random bytes from /dev/urandom. Aborts
// on failure - "we can't get entropy" is not a recoverable condition for a
// tool whose job is to encrypt traffic. Mirrors lib/tweetnacl/randombytes.c
// but kept local so the monocypher backend has no cross-backend dependency.
void randomBytes(uint8_t *buf, size_t n) {
    int fd;
    do {
        fd = open("/dev/urandom", O_RDONLY);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
        std::abort();
    }
    while (n > 0) {
        // Linux caps /dev/urandom reads at 32 MiB and may short-read; loop
        // in 1 MiB chunks. In practice nothing in sft asks for more than
        // 32 bytes at a time, but keep the loop for robustness.
        size_t want = n > 1048576u ? 1048576u : n;
        ssize_t got = read(fd, buf, want);
        if (got < 0) {
            if (errno == EINTR) continue;
            std::abort();
        }
        if (got == 0) {
            std::abort();
        }
        buf += got;
        n -= static_cast<size_t>(got);
    }
    close(fd);
}

} // namespace

void Crypto::generateKeypair(unsigned char *pk, unsigned char *sk) {
    // X25519 accepts any 32 random bytes as a secret key - clamping is
    // applied internally by crypto_x25519_public_key / crypto_x25519.
    randomBytes(sk, SECRETKEY_SIZE);
    crypto_x25519_public_key(pk, sk);
}

void Crypto::deriveSharedKey(const unsigned char *my_sk,
                             const unsigned char *peer_pk,
                             const std::string &passphrase,
                             unsigned char *out_key) {
    // Raw X25519 output is not uniformly random; hash it (mixed with the
    // passphrase) to derive the actual symmetric key. BLAKE2b-256 gives
    // us exactly SHARED_KEY_SIZE bytes, no truncation needed.
    unsigned char shared[32];
    crypto_x25519(shared, my_sk, peer_pk);

    std::vector<unsigned char> buf(sizeof(shared) + passphrase.size());
    std::memcpy(buf.data(), shared, sizeof(shared));
    std::memcpy(buf.data() + sizeof(shared), passphrase.data(), passphrase.size());

    crypto_blake2b(out_key, SHARED_KEY_SIZE, buf.data(), buf.size());

    crypto_wipe(shared, sizeof(shared));
    crypto_wipe(buf.data(), buf.size());
}

std::unique_ptr<unsigned char[]> Crypto::encryptPacket(
        const unsigned char *plaintext, size_t len,
        const unsigned char *key,
        size_t &out_len) {
    out_len = NONCE_SIZE + MAC_SIZE + len;
    auto packet = std::make_unique<unsigned char[]>(out_len);

    // Fresh random 24-byte nonce. With XChaCha20's 192-bit nonce, random
    // collision is negligible (~2^96 messages) so no counter is needed.
    randomBytes(packet.get(), NONCE_SIZE);

    // crypto_aead_lock writes the ciphertext and MAC to separate output
    // pointers - no zero-padding contract, encrypt directly into the wire
    // packet. Layout: packet[0..24]=nonce, packet[24..40]=MAC, packet[40..]=ct.
    crypto_aead_lock(
            packet.get() + NONCE_SIZE + MAC_SIZE, // cipher_text
            packet.get() + NONCE_SIZE,            // mac
            key,
            packet.get(),                         // nonce
            nullptr, 0,                           // no associated data
            plaintext, len);

    return packet;
}

std::unique_ptr<unsigned char[]> Crypto::decryptPacket(
        const unsigned char *in, size_t in_len,
        const unsigned char *key,
        size_t &out_len) {
    out_len = 0;
    if (in_len < CRYPTO_OVERHEAD) {
        return nullptr;
    }
    out_len = in_len - CRYPTO_OVERHEAD;
    auto plain = std::make_unique<unsigned char[]>(out_len);

    int rc = crypto_aead_unlock(
            plain.get(),
            in + NONCE_SIZE,                      // mac
            key,
            in,                                   // nonce
            nullptr, 0,                           // no associated data
            in + NONCE_SIZE + MAC_SIZE, out_len); // cipher_text
    if (rc != 0) {
        out_len = 0;
        return nullptr;
    }
    return plain;
}

void Crypto::zeroize(void *ptr, size_t len) {
    // monocypher's crypto_wipe is the same idea (volatile write that the
    // compiler can't elide) but vendored and audited - prefer it over
    // hand-rolling.
    crypto_wipe(ptr, len);
}
