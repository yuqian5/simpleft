# Monocypher

Vendored copy of [Monocypher](https://monocypher.org), a small, fast,
single-file portable C crypto library by Loup Vaillant. Dual-licensed
under BSD-2-Clause and CC0-1.0 (we are not adding either licence file to
the repo because both are permissive and the licence header inside
`monocypher.h` / `monocypher.c` is sufficient attribution).

Source files were fetched verbatim from
`https://github.com/LoupVaillant/Monocypher/blob/master/src/`. To upgrade,
replace both files wholesale - do **not** edit upstream sources in place.

This is the alternative crypto backend (selected with
`-DSFT_CRYPTO_BACKEND=monocypher`, which is the default). It uses
XChaCha20-Poly1305 instead of TweetNaCl's XSalsa20-Poly1305 and is
substantially faster on portable C builds (typically 3-5x). Wire frames
have the same `[24 nonce][16 MAC][N ciphertext]` layout as the TweetNaCl
backend but the stream ciphers differ, so both endpoints must be built
with the same backend.

`randombytes` is **not** declared by monocypher itself - monocypher takes
randomness as an explicit input. The wrapper in
`src/crypto/Crypto_monocypher.cpp` opens `/dev/urandom` directly.
