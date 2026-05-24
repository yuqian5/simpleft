#include "../include/Packet.hpp"

#include <cstring>

#include "../include/Crypto.hpp"
#include "../include/sft_constants.hpp"

// Compose [4-byte BE length] [crypto body] into a single owning buffer.
// `body` is consumed by this function; its size is encoded in the header.
static std::unique_ptr<unsigned char[]> frame(
        std::unique_ptr<unsigned char[]> body, size_t bodyLen,
        size_t &outTotalLen) {
    outTotalLen = PACKET_HEADER_SIZE + bodyLen;
    auto out = std::make_unique<unsigned char[]>(outTotalLen);

    out[0] = (bodyLen >> 24) & 0xFF;
    out[1] = (bodyLen >> 16) & 0xFF;
    out[2] = (bodyLen >> 8) & 0xFF;
    out[3] = bodyLen & 0xFF;
    std::memcpy(out.get() + PACKET_HEADER_SIZE, body.get(), bodyLen);

    return out;
}

std::unique_ptr<unsigned char[]> Packet::serialize(
        const unsigned char *plaintext, size_t len,
        const unsigned char *key,
        size_t &out_total_len) {
    size_t bodyLen = 0;
    auto body = Crypto::encryptPacket(plaintext, len, key, bodyLen);
    return frame(std::move(body), bodyLen, out_total_len);
}

std::unique_ptr<unsigned char[]> Packet::generateTerminationPacket(
        const unsigned char *key,
        size_t &out_total_len) {
    // Empty plaintext encrypts to [nonce(24)][MAC(16)] — 40 bytes. The
    // length header advertises that, and the receiver decrypts it like any
    // other frame; seeing an empty plaintext payload is the EOF signal.
    return serialize(nullptr, 0, key, out_total_len);
}
