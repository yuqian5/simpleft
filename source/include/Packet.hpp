#ifndef FT_PACKET_HPP
#define FT_PACKET_HPP

#include <cstddef>
#include <memory>

/**
 * Frames a plaintext payload into the on-wire format used by sft:
 *
 *   [4-byte BE length] [24-byte nonce] [16-byte Poly1305 MAC] [N bytes ciphertext]
 *
 * Length covers everything after itself (nonce + MAC + ciphertext = N + 40).
 *
 * Every frame is encrypted, including the end-of-stream marker. EOF is
 * signalled by a frame with an empty plaintext payload (N == 0); the
 * receiver decrypts it like any other frame and treats `plaintext length
 * == 0` as the end of the transfer.
 */
class Packet {
public:
    /**
     * Encrypt `plaintext` (len bytes) under `key` and wrap it in the wire
     * format above.
     * @param out_total_len  receives 4 + NONCE_SIZE + MAC_SIZE + len
     */
    static std::unique_ptr<unsigned char[]> serialize(
            const unsigned char *plaintext, size_t len,
            const unsigned char *key,
            size_t &out_total_len);

    /**
     * Build the end-of-stream marker: an encrypted frame with an empty
     * plaintext. Always 4 + NONCE_SIZE + MAC_SIZE bytes on the wire.
     */
    static std::unique_ptr<unsigned char[]> generateTerminationPacket(
            const unsigned char *key,
            size_t &out_total_len);
};

#endif //FT_PACKET_HPP
