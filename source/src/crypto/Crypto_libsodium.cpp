// libsodium implementation of Crypto.hpp.
//
// Primitives:
//   * X25519 for ephemeral key agreement (crypto_scalarmult_base + crypto_scalarmult)
//   * BLAKE2b-256 to mix the passphrase into the derived key (crypto_generichash)
//   * XChaCha20-Poly1305-IETF AEAD for per-packet encryption
//     (crypto_aead_xchacha20poly1305_ietf_{encrypt,decrypt})
//
// Wire frame layout is the same as the other backends:
//   [NONCE_SIZE nonce] [MAC_SIZE tag] [N bytes ciphertext]    = 40 + N bytes
//
// This backend IS byte-compatible with the monocypher backend on the data
// plane (both use IETF XChaCha20-Poly1305 with the same KDF, and produce
// identical key/nonce/ciphertext/tag bytes given identical inputs), so a
// monocypher TX can talk to a libsodium RX and vice versa. The tweetnacl
// backend is XSalsa20-based and is NOT interoperable with either.
//
// libsodium's `crypto_secretbox_xchacha20poly1305_easy` would have been the
// shorter call but it uses the NaCl-secretbox MAC layout (Poly1305 over
// ciphertext only), not the IETF AEAD MAC (Poly1305 over
// pad16(AAD)||pad16(ct)||u64_le(len_AAD)||u64_le(len_ct)), and would have
// MAC-failed against monocypher.
//
// Unlike the vendored backends, this one depends on a system-installed
// libsodium (find_package via pkg-config in CMakeLists.txt). That is the
// only deviation in this codebase from the "no system third-party libs"
// rule, and it is justified only because libsodium is ~50k LOC with an
// autotools build system - vendoring it would dwarf the rest of the repo.

#include "../../include/Crypto.hpp"

#include <cstdlib>
#include <cstring>
#include <vector>

#include "../../include/sft_constants.hpp"

#include <sodium.h>

// Compile-time assertions that libsodium's primitive sizes match the
// constants in sft_constants.hpp. If a future libsodium changes these,
// the build will fail loudly rather than silently producing garbage.
static_assert(crypto_scalarmult_BYTES == 32,
              "libsodium X25519 output is not 32 bytes");
static_assert(crypto_scalarmult_SCALARBYTES == SECRETKEY_SIZE,
              "libsodium X25519 secret key size mismatch");
static_assert(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES == NONCE_SIZE,
              "XChaCha20-Poly1305-IETF nonce size mismatch");
static_assert(crypto_aead_xchacha20poly1305_ietf_ABYTES == MAC_SIZE,
              "XChaCha20-Poly1305-IETF tag size mismatch");
static_assert(crypto_aead_xchacha20poly1305_ietf_KEYBYTES == SHARED_KEY_SIZE,
              "XChaCha20-Poly1305-IETF key size mismatch");

namespace {

// libsodium requires sodium_init() before any other call. It scans CPU
// feature flags for hardware AES/SHA/etc and is idempotent on subsequent
// calls. A global object's constructor runs before main() starts, so by
// the time TX/RX call any Crypto:: method, init has already happened.
struct LibsodiumInit {
    LibsodiumInit() {
        if (sodium_init() < 0) {
            std::abort();
        }
    }
};
const LibsodiumInit g_init;

} // namespace

void Crypto::generateKeypair(unsigned char *pk, unsigned char *sk) {
    // X25519: random 32 bytes is a valid secret key (clamping happens
    // inside crypto_scalarmult_*). randombytes_buf is libsodium's
    // CSPRNG wrapper - on Linux/macOS it pulls from getrandom / arc4random.
    randombytes_buf(sk, SECRETKEY_SIZE);
    crypto_scalarmult_base(pk, sk);
}

void Crypto::deriveSharedKey(const unsigned char *my_sk,
                             const unsigned char *peer_pk,
                             const std::string &passphrase,
                             unsigned char *out_key) {
    unsigned char shared[crypto_scalarmult_BYTES];
    // Raw X25519 output - non-uniform, must be hashed before use.
    if (crypto_scalarmult(shared, my_sk, peer_pk) != 0) {
        // crypto_scalarmult only fails on degenerate peer keys (all-zero
        // result). Treat as fatal - we cannot derive a usable session key.
        std::abort();
    }

    std::vector<unsigned char> buf(sizeof(shared) + passphrase.size());
    std::memcpy(buf.data(), shared, sizeof(shared));
    std::memcpy(buf.data() + sizeof(shared), passphrase.data(), passphrase.size());

    // BLAKE2b with explicit 32-byte output, no key. Same primitive as the
    // monocypher backend - the two derive the same out_key given identical
    // shared/passphrase inputs, even though their AEADs are still mutually
    // incompatible.
    crypto_generichash(out_key, SHARED_KEY_SIZE, buf.data(), buf.size(),
                       nullptr, 0);

    sodium_memzero(shared, sizeof(shared));
    sodium_memzero(buf.data(), buf.size());
}

std::unique_ptr<unsigned char[]> Crypto::encryptPacket(
        const unsigned char *plaintext, size_t len,
        const unsigned char *key,
        size_t &out_len) {
    out_len = NONCE_SIZE + MAC_SIZE + len;
    auto packet = std::make_unique<unsigned char[]>(out_len);

    // Fresh random 24-byte nonce. With XChaCha20's 192-bit nonce, random
    // collision is negligible (~2^96 messages) so no counter is needed.
    randombytes_buf(packet.get(), NONCE_SIZE);

    // IETF AEAD output layout: [ciphertext (len)][tag (16)] - the tag
    // trails the ciphertext, unlike secretbox which prepends the MAC. Our
    // wire format places the tag BEFORE the ciphertext, so we point the
    // _detached variant's two output buffers separately and skip the
    // trailing-tag layout entirely.
    unsigned long long mac_len = 0;
    crypto_aead_xchacha20poly1305_ietf_encrypt_detached(
            packet.get() + NONCE_SIZE + MAC_SIZE,  // c (ciphertext only)
            packet.get() + NONCE_SIZE,             // mac
            &mac_len,
            plaintext, len,
            nullptr, 0,                            // no AAD
            nullptr,                               // nsec (unused)
            packet.get(),                          // nonce
            key);

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

    int rc = crypto_aead_xchacha20poly1305_ietf_decrypt_detached(
            plain.get(),
            nullptr,                               // nsec (unused)
            in + NONCE_SIZE + MAC_SIZE,            // ciphertext
            out_len,
            in + NONCE_SIZE,                       // mac
            nullptr, 0,                            // no AAD
            in,                                    // nonce
            key);
    if (rc != 0) {
        out_len = 0;
        return nullptr;
    }
    return plain;
}

void Crypto::zeroize(void *ptr, size_t len) {
    // libsodium's sodium_memzero is the audited equivalent of our volatile
    // byte-loop in the tweetnacl backend - prefer it.
    sodium_memzero(ptr, len);
}
