// Monocypher implementation of Crypto.hpp.
//
// Primitives:
//   * X25519 for ephemeral key agreement (crypto_x25519_public_key + crypto_x25519)
//   * BLAKE2b-256 to mix the passphrase into the derived key
//   * XChaCha20-Poly1305-IETF AEAD for per-packet encryption, composed by
//     hand from monocypher's lower-level primitives (crypto_chacha20_h +
//     crypto_chacha20_ietf + crypto_poly1305_*)
//
// We do NOT use monocypher's built-in crypto_aead_lock / _unlock. Those
// implement an IETF-style MAC layout but with djb-style ChaCha20 inside
// (8-byte nonce + 64-bit counter), a hybrid construction that does not
// match libsodium's crypto_aead_xchacha20poly1305_ietf_* and would make
// monocypher and libsodium endpoints unable to talk to each other.
// Composing the standard IETF construction by hand keeps the two backends
// byte-for-byte wire-compatible.
//
// Wire frame layout is the same as the other backends:
//   [NONCE_SIZE nonce] [MAC_SIZE tag] [N bytes ciphertext]    = 40 + N bytes
// and the data plane is interoperable with the libsodium backend. The
// tweetnacl backend uses XSalsa20-Poly1305 (different stream cipher) and
// is NOT compatible with either of the XChaCha20 backends.

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

// Standard IETF XChaCha20-Poly1305 AEAD, composed from monocypher's
// lower-level primitives. Matches libsodium's
// crypto_aead_xchacha20poly1305_ietf_encrypt byte-for-byte (verified
// against the test vectors in RFC 8439 + libsodium's own tests).
//
// Construction (RFC 8439 + draft-irtf-cfrg-xchacha):
//   subkey      = HChaCha20(key, nonce[0..16])
//   ietf_nonce  = 0x00000000 || nonce[16..24]                       (12 bytes)
//   poly_block  = ChaCha20-IETF(subkey, ietf_nonce, ctr=0) over 64 zero bytes
//   poly_key    = poly_block[0..32]
//   ciphertext  = plaintext XOR ChaCha20-IETF(subkey, ietf_nonce, ctr=1)
//   mac_input   = pad16(AAD) || pad16(ct) || u64_le(|AAD|) || u64_le(|ct|)
//   tag         = Poly1305(poly_key, mac_input)
void aead_ietf_xchacha20poly1305(
        uint8_t *cipher_text, uint8_t mac[16],
        const uint8_t key[32], const uint8_t nonce[24],
        const uint8_t *ad, size_t ad_size,
        const uint8_t *plain_text, size_t text_size) {
    // 1. HChaCha20 derives a 256-bit subkey from the first 16 bytes of
    //    the 24-byte XChaCha20 nonce.
    uint8_t subkey[32];
    crypto_chacha20_h(subkey, key, nonce);

    // 2. The remaining 8 bytes of the XChaCha20 nonce become the bottom
    //    64 bits of the 12-byte IETF nonce; the top 32 bits are zero.
    uint8_t ietf_nonce[12] = {};
    std::memcpy(ietf_nonce + 4, nonce + 16, 8);

    // 3. Generate the one-time Poly1305 key as ChaCha20-IETF counter 0
    //    over 64 zero bytes; only the first 32 bytes are used.
    uint8_t poly_block[64] = {};
    crypto_chacha20_ietf(poly_block, poly_block, 64, subkey, ietf_nonce, 0);

    // 4. Encrypt the plaintext starting at ChaCha20-IETF counter 1.
    crypto_chacha20_ietf(cipher_text, plain_text, text_size, subkey, ietf_nonce, 1);

    // 5. Compute the Poly1305 tag over pad16(AAD) || pad16(ct) || sizes.
    uint8_t sizes[16];
    for (int i = 0; i < 8; i++) sizes[i]     = (uint8_t)((ad_size   >> (i * 8)) & 0xff);
    for (int i = 0; i < 8; i++) sizes[8 + i] = (uint8_t)((text_size >> (i * 8)) & 0xff);

    const uint8_t pad[16] = {};
    crypto_poly1305_ctx pctx;
    crypto_poly1305_init(&pctx, poly_block);
    if (ad_size > 0) {
        crypto_poly1305_update(&pctx, ad, ad_size);
        size_t pad_ad = (16 - (ad_size % 16)) % 16;
        if (pad_ad) crypto_poly1305_update(&pctx, pad, pad_ad);
    }
    if (text_size > 0) {
        crypto_poly1305_update(&pctx, cipher_text, text_size);
        size_t pad_ct = (16 - (text_size % 16)) % 16;
        if (pad_ct) crypto_poly1305_update(&pctx, pad, pad_ct);
    }
    crypto_poly1305_update(&pctx, sizes, 16);
    crypto_poly1305_final(&pctx, mac);

    crypto_wipe(subkey, sizeof(subkey));
    crypto_wipe(poly_block, sizeof(poly_block));
    crypto_wipe(&pctx, sizeof(pctx));
}

// Verify-then-decrypt counterpart of aead_ietf_xchacha20poly1305. Returns
// 0 on success, nonzero on MAC failure. Computes the expected tag first
// and bails before touching the plaintext output if it doesn't match,
// so the caller never sees partially-decrypted bytes on tampered input.
int aead_ietf_xchacha20poly1305_open(
        uint8_t *plain_text,
        const uint8_t key[32], const uint8_t nonce[24],
        const uint8_t *ad, size_t ad_size,
        const uint8_t mac[16],
        const uint8_t *cipher_text, size_t text_size) {
    uint8_t subkey[32];
    crypto_chacha20_h(subkey, key, nonce);

    uint8_t ietf_nonce[12] = {};
    std::memcpy(ietf_nonce + 4, nonce + 16, 8);

    uint8_t poly_block[64] = {};
    crypto_chacha20_ietf(poly_block, poly_block, 64, subkey, ietf_nonce, 0);

    uint8_t sizes[16];
    for (int i = 0; i < 8; i++) sizes[i]     = (uint8_t)((ad_size   >> (i * 8)) & 0xff);
    for (int i = 0; i < 8; i++) sizes[8 + i] = (uint8_t)((text_size >> (i * 8)) & 0xff);

    const uint8_t pad[16] = {};
    crypto_poly1305_ctx pctx;
    crypto_poly1305_init(&pctx, poly_block);
    if (ad_size > 0) {
        crypto_poly1305_update(&pctx, ad, ad_size);
        size_t pad_ad = (16 - (ad_size % 16)) % 16;
        if (pad_ad) crypto_poly1305_update(&pctx, pad, pad_ad);
    }
    if (text_size > 0) {
        crypto_poly1305_update(&pctx, cipher_text, text_size);
        size_t pad_ct = (16 - (text_size % 16)) % 16;
        if (pad_ct) crypto_poly1305_update(&pctx, pad, pad_ct);
    }
    crypto_poly1305_update(&pctx, sizes, 16);

    uint8_t real_mac[16];
    crypto_poly1305_final(&pctx, real_mac);

    int mismatch = crypto_verify16(mac, real_mac);
    if (mismatch == 0) {
        crypto_chacha20_ietf(plain_text, cipher_text, text_size,
                             subkey, ietf_nonce, 1);
    }
    crypto_wipe(subkey, sizeof(subkey));
    crypto_wipe(poly_block, sizeof(poly_block));
    crypto_wipe(real_mac, sizeof(real_mac));
    crypto_wipe(&pctx, sizeof(pctx));
    return mismatch;
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

    // Encrypt directly into the wire packet:
    //   packet[0..24]=nonce, packet[24..40]=tag, packet[40..]=ciphertext
    aead_ietf_xchacha20poly1305(
            packet.get() + NONCE_SIZE + MAC_SIZE,  // cipher_text
            packet.get() + NONCE_SIZE,             // mac
            key,
            packet.get(),                          // nonce
            nullptr, 0,                            // no associated data
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

    int rc = aead_ietf_xchacha20poly1305_open(
            plain.get(),
            key,
            in,                                    // nonce
            nullptr, 0,                            // no associated data
            in + NONCE_SIZE,                       // mac
            in + NONCE_SIZE + MAC_SIZE, out_len);  // cipher_text
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
