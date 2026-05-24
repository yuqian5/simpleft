// Implementation note - NaCl's zero-padding API:
//
//   crypto_secretbox(c, m, mlen, n, k) requires that m starts with
//   crypto_secretbox_ZEROBYTES (32) bytes of zeros, and writes
//   crypto_secretbox_BOXZEROBYTES (16) zeros followed by a 16-byte
//   Poly1305 MAC and then the ciphertext.
//
//   crypto_secretbox_open is the symmetric inverse and returns -1 if
//   the MAC does not validate.
//
// The padding is a relic of the underlying primitive and is easy to get
// wrong. This file is the only place in the codebase that has to deal
// with it; everywhere else uses our compact wire format:
//
//   [NONCE_SIZE nonce] [MAC_SIZE tag] [N bytes ciphertext]    = 40 + N bytes

#include "../include/Crypto.hpp"

#include <cstring>
#include <vector>

#include "../include/sft_constants.hpp"

extern "C" {
#include "tweetnacl.h"

// tweetnacl.h calls randombytes() but does not declare it - the embedder
// (us, in lib/tweetnacl/randombytes.c) is expected to supply it.
void randombytes(unsigned char *x, unsigned long long xlen);
}

void Crypto::generateKeypair(unsigned char *pk, unsigned char *sk) {
    crypto_box_keypair(pk, sk);
}

void Crypto::deriveSharedKey(const unsigned char *my_sk,
                             const unsigned char *peer_pk,
                             const std::string &passphrase,
                             unsigned char *out_key) {
    unsigned char shared[crypto_box_BEFORENMBYTES];
    crypto_box_beforenm(shared, peer_pk, my_sk);

    // Hash `shared || passphrase` and take the leading 32 bytes as the key.
    // SHA-512 output is 64 bytes; using only the first 32 is standard.
    std::vector<unsigned char> buf(crypto_box_BEFORENMBYTES + passphrase.size());
    std::memcpy(buf.data(), shared, crypto_box_BEFORENMBYTES);
    std::memcpy(buf.data() + crypto_box_BEFORENMBYTES,
                passphrase.data(), passphrase.size());

    unsigned char hash[crypto_hash_BYTES];
    crypto_hash(hash, buf.data(), buf.size());

    std::memcpy(out_key, hash, SHARED_KEY_SIZE);

    zeroize(shared, sizeof(shared));
    zeroize(buf.data(), buf.size());
    zeroize(hash, sizeof(hash));
}

std::unique_ptr<unsigned char[]> Crypto::encryptPacket(
        const unsigned char *plaintext, size_t len,
        const unsigned char *key,
        size_t &out_len) {
    const size_t padded = crypto_secretbox_ZEROBYTES + len;
    auto in = std::make_unique<unsigned char[]>(padded);
    auto rawOut = std::make_unique<unsigned char[]>(padded);

    // NaCl input contract: 32 leading zeros, then plaintext.
    std::memset(in.get(), 0, crypto_secretbox_ZEROBYTES);
    std::memcpy(in.get() + crypto_secretbox_ZEROBYTES, plaintext, len);

    out_len = NONCE_SIZE + MAC_SIZE + len;
    auto packet = std::make_unique<unsigned char[]>(out_len);

    // Fresh random nonce. 24 bytes is large enough that random collision
    // is negligible (~2^96 messages before risk), so we do not need a counter.
    randombytes(packet.get(), NONCE_SIZE);

    crypto_secretbox(rawOut.get(), in.get(), padded, packet.get(), key);

    // rawOut layout: [16 zeros][16 MAC][len ciphertext]. Drop the leading
    // zeros and copy the rest after the nonce in our wire packet.
    std::memcpy(packet.get() + NONCE_SIZE,
                rawOut.get() + crypto_secretbox_BOXZEROBYTES,
                MAC_SIZE + len);

    zeroize(in.get(), padded);
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
    const size_t cipherLen = in_len - NONCE_SIZE; // MAC + ciphertext
    const size_t padded = crypto_secretbox_BOXZEROBYTES + cipherLen;

    auto padCipher = std::make_unique<unsigned char[]>(padded);
    std::memset(padCipher.get(), 0, crypto_secretbox_BOXZEROBYTES);
    std::memcpy(padCipher.get() + crypto_secretbox_BOXZEROBYTES,
                in + NONCE_SIZE, cipherLen);

    auto rawOut = std::make_unique<unsigned char[]>(padded);
    int rc = crypto_secretbox_open(rawOut.get(), padCipher.get(), padded,
                                   in, key);
    if (rc != 0) {
        return nullptr;
    }

    out_len = in_len - CRYPTO_OVERHEAD;
    auto plain = std::make_unique<unsigned char[]>(out_len);
    std::memcpy(plain.get(),
                rawOut.get() + crypto_secretbox_ZEROBYTES, out_len);

    zeroize(rawOut.get(), padded);
    return plain;
}

void Crypto::zeroize(void *ptr, size_t len) {
    // volatile prevents the compiler from optimising the write away as a
    // dead store. memset(...) alone is not safe because the buffer is
    // about to be freed.
    volatile unsigned char *p = (volatile unsigned char *) ptr;
    while (len-- > 0) *p++ = 0;
}
