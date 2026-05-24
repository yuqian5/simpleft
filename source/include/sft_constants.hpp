#ifndef SFT_SFT_CONSTANTS_HPP
#define SFT_SFT_CONSTANTS_HPP

#define MAX_PACKET_SIZE 8192
#define BACKLOG 1
#define PACKET_HEADER_SIZE 4
#define READ_RECEIPT "OK"
#define READ_RECEIPT_SIZE 2

// Sizes from TweetNaCl (curve25519 / xsalsa20-poly1305 / sha512).
// Duplicated here so the rest of the codebase doesn't have to include
// tweetnacl.h directly.
#define PUBKEY_SIZE 32
#define SECRETKEY_SIZE 32
#define SHARED_KEY_SIZE 32
#define NONCE_SIZE 24
#define MAC_SIZE 16

// Per-frame overhead on the wire: nonce + Poly1305 tag.
#define CRYPTO_OVERHEAD (NONCE_SIZE + MAC_SIZE)

#endif //SFT_SFT_CONSTANTS_HPP
