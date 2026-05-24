#ifndef SFT_CRYPTO_HPP
#define SFT_CRYPTO_HPP

#include <cstddef>
#include <memory>
#include <string>

/**
 * Backend-agnostic crypto facade. Exactly one backend implementation
 * (e.g. src/crypto/Crypto_tweetnacl.cpp) is linked at build time, selected
 * by the CMake option SFT_CRYPTO_BACKEND. Adding a new backend means
 * dropping a new .cpp under src/crypto/ that implements every method
 * declared below; nothing outside Crypto.hpp / Crypto_*.cpp needs to
 * change.
 *
 * Primitive contract (all backends must satisfy):
 *   * Curve25519 (or X25519) for ephemeral key agreement
 *   * SHA-512 to mix the passphrase into the derived key
 *   * An AEAD with 24-byte nonce and 16-byte tag (XSalsa20-Poly1305,
 *     XChaCha20-Poly1305, or equivalent) for per-packet encryption
 *
 * Wire format does not depend on the backend - all buffer sizes are
 * fixed in sft_constants.hpp, so two endpoints can interoperate as long
 * as they pick AEADs with compatible nonce/tag sizes. Mixing TweetNaCl
 * (XSalsa20) with monocypher (XChaCha20) does NOT interop because the
 * stream ciphers differ.
 */
class Crypto {
public:
    /**
     * Generate an ephemeral Curve25519 keypair. Both buffers must be at least
     * PUBKEY_SIZE / SECRETKEY_SIZE bytes.
     */
    static void generateKeypair(unsigned char *pk, unsigned char *sk);

    /**
     * Derive the 32-byte symmetric key for this session.
     *
     * shared = crypto_box_beforenm(peer_pk, my_sk)   (X25519 + HSalsa20)
     * key    = SHA-512(shared || passphrase)[0:32]
     *
     * Mixing the passphrase into the KDF means a passive attacker without
     * the passphrase cannot derive the same key even if they intercept both
     * public keys, and an active MITM substituting their own keys produces
     * a different key on each side - first decrypt fails.
     *
     * @param out_key  buffer of SHARED_KEY_SIZE bytes
     */
    static void deriveSharedKey(const unsigned char *my_sk,
                                const unsigned char *peer_pk,
                                const std::string &passphrase,
                                unsigned char *out_key);

    /**
     * Encrypt a single packet.
     *
     * Wire format of the returned buffer:
     *   [NONCE_SIZE nonce] [MAC_SIZE Poly1305 tag] [len bytes ciphertext]
     *
     * @param out_len  receives NONCE_SIZE + MAC_SIZE + len
     * @return owning buffer; caller wraps it in framing (length prefix etc.)
     */
    static std::unique_ptr<unsigned char[]> encryptPacket(
            const unsigned char *plaintext, size_t len,
            const unsigned char *key,
            size_t &out_len);

    /**
     * Decrypt a packet produced by encryptPacket. `in` must be the full
     * nonce || mac || ciphertext blob and `in_len` must equal its size.
     *
     * @param out_len  receives plaintext length (in_len - CRYPTO_OVERHEAD)
     * @return owning buffer on success, nullptr on MAC failure or short input
     */
    static std::unique_ptr<unsigned char[]> decryptPacket(
            const unsigned char *in, size_t in_len,
            const unsigned char *key,
            size_t &out_len);

    /**
     * Best-effort secure-zero a buffer. Used to wipe key material in
     * destructors so it does not linger on freed heap.
     */
    static void zeroize(void *ptr, size_t len);
};

#endif //SFT_CRYPTO_HPP
