# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**simpleft** (`sft`) is a single-binary C++ command-line tool for peer-to-peer file transfer. There is no server; one side runs in receive mode (`rx`) and listens, the other side runs in transmit mode (`tx`) and connects. Traffic is end-to-end encrypted; a shared passphrase is required on both sides. The crypto backend is selectable at build time (`-DSFT_CRYPTO_BACKEND=monocypher` (default) or `tweetnacl`) - both endpoints must use the same backend because they ship with different stream ciphers and KDF hashes (see "Crypto layout" below). C++20, POSIX sockets, no system-installed third-party libraries. Builds on Linux and macOS (CI matrix: ubuntu-latest with gcc/clang, macos-latest with clang). Windows is not supported - the code uses `<sys/socket.h>`, `<arpa/inet.h>`, `<termios.h>`, and shells out to `tar`.

## Build

All CMake/build commands must be run from the `source/` directory (the `CMakeLists.txt` lives there, not at the repo root):

```bash
# Out-of-tree build (matches CI)
cd source
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Or the in-tree form used in the README
cd source
cmake CMakeLists.txt
make
make install   # installs the `sft` binary to <prefix>/bin
```

Debug builds get `-Wall -Wextra` (see `source/CMakeLists.txt`). The default build type is `Release` if `CMAKE_BUILD_TYPE` is not set.

CMake calls `FIND_PROGRAM(TAR tar)` and fatal-errors if `tar` is missing - the binary also invokes `tar` at runtime (via `fork`+`execvp`, see `Transceiver::packFile`), so `tar` must be on `PATH` on both sides. `/dev/urandom` must also exist (the monocypher backend opens it directly from `src/crypto/Crypto_monocypher.cpp`; the tweetnacl backend uses the shim in `lib/tweetnacl/randombytes.c`).

To switch backends:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSFT_CRYPTO_BACKEND=tweetnacl
```

The CMake configure step prints the selected backend (`-- sft crypto backend: ...`) - check that line before assuming a build is current. If you previously configured `build/` with a different backend, CMake caches the old value and *won't* pick up a new default in `CMakeLists.txt`; either pass `-DSFT_CRYPTO_BACKEND=...` explicitly or `rm -rf build` and reconfigure.

Adding a third backend means dropping `src/crypto/Crypto_<name>.cpp` (implementing every method in `include/Crypto.hpp`) and extending the conditional in `CMakeLists.txt` with the new sources / include dirs.

## Tests

There are no tests. CI runs `ctest` but no targets are registered in `CMakeLists.txt`, so it is a no-op. If adding tests, register them with CMake so the existing CI job picks them up.

There is no checked-in test file. Smoke-testing is done by running `sft rx` and `sft tx` against each other on loopback.

## Run

```bash
sft rx -p <port> -k                              # receive (interactive prompt)
sft rx -p <port> -k -loop                        # receive, then keep listening
sft tx -ip4 <IPv4 addr> -p <port> -k <path>      # send over IPv4
sft tx -ip6 <IPv6 addr> -p <port> -k <path>      # send over IPv6

SFT_PASSPHRASE=... sft rx -p <port>              # receive (env-driven, no -k)
SFT_PASSPHRASE=... sft tx -ip4 <addr> -p <port> <path>
```

`-loop` (rx-only) keeps the listener running after each transfer. Connection-level failures (bad handshake, mismatched passphrase, mid-transfer disconnect) log a warning and wait for the next peer; only filesystem failures (e.g. cannot create the buffer file) still abort the process.

`-h` or `--help` prints the flag reference to stdout (no ANSI colour) and exits 0. It short-circuits before any other validation, so `sft -h` works on its own.

Encryption is **mandatory**; the passphrase must come from one of two sources:

1. `-k` (standalone flag, no inline value): prompts on the controlling terminal with echo disabled. If stdin is not a TTY, one line is read plainly from stdin (so `echo pw | sft -k ...` works for piping).
2. `SFT_PASSPHRASE` env var: used when `-k` is absent. The env var is `unsetenv`'d immediately after copying so it does not leak to the `tar` child or other future children. Note that on Linux env vars are visible via `/proc/<pid>/environ` to the owning user, and on macOS via `ps -E` to the same uid - `-k` is preferred for interactive use.

`-k` takes precedence if both are set. Both sides must end up with the same passphrase or the receiver aborts on the first encrypted packet.

Defaults: port `8203`, IP `127.0.0.1` (IPv4). The path may be a file or a directory - it is `tar`-packed before sending.

## Architecture

### Process model
`main.cpp` parses argv into a `CMD_ARGS` struct (`include/cli/CmdArgs.hpp`) via `parseInput` and instantiates either an `RX` or a `TX` object. Both constructors do all the work synchronously and the program exits when the constructor returns. There is no event loop; one transfer per invocation.

### Class hierarchy
- `Transceiver` (base) - tar pack/unpack helpers and temp-file cleanup. Both `TX` and `RX` inherit from it (`protected` inheritance).
- `TX` - opens an IPv4 or IPv6 client socket, retries `connect()` up to 30× with 500 ms backoff, then runs the crypto handshake, then streams the packed file. Holds a 32-byte `sharedKey` member; the destructor calls `Crypto::zeroize` on it before closing the socket.
- `RX` - always opens an `AF_INET6` listening socket with `IPV6_V6ONLY=0` so a single socket accepts both IPv4 and IPv6 clients. `BACKLOG=1` (one pending connection). Runs the handshake before entering the receive loop. Same `sharedKey` ownership pattern as `TX`.
- `Crypto` - backend-agnostic facade declared in `include/Crypto.hpp`. Exactly one `src/crypto/Crypto_<backend>.cpp` is compiled per build (CMake option `SFT_CRYPTO_BACKEND`). Provides `generateKeypair`, `deriveSharedKey`, `encryptPacket`, `decryptPacket`, and `zeroize`. Everything outside the backend file uses the compact wire format `[24 nonce][16 MAC][N ciphertext]`; no library-specific padding leaks out.

### Crypto layout

```
source/lib/monocypher/   (default backend)
├── README.md            (provenance note + upgrade procedure)
├── monocypher.c         (upstream, unmodified - replace wholesale on upgrade)
└── monocypher.h         (upstream, unmodified)

source/lib/tweetnacl/    (alternative backend)
├── README.md
├── tweetnacl.c
├── tweetnacl.h
└── randombytes.c        (/dev/urandom shim; tweetnacl declares but does not define randombytes)

source/src/crypto/
├── Crypto_monocypher.cpp   (X25519 + BLAKE2b-256 + XChaCha20-Poly1305 + crypto_wipe)
└── Crypto_tweetnacl.cpp    (X25519 + SHA-512 + XSalsa20-Poly1305 + handwritten zeroize)
```

Neither `monocypher.h` nor `tweetnacl.h` has `extern "C"` guards; the corresponding `Crypto_*.cpp` is the only C++ file that includes them and each wraps the include in `extern "C" { ... }`. Do not edit upstream headers - replace whole files on upgrade.

`Crypto_monocypher.cpp` has its own `/dev/urandom` reader (anonymous-namespace helper). The tweetnacl path uses the shared `randombytes` symbol from `lib/tweetnacl/randombytes.c`. Both abort on entropy failure - this is not a recoverable condition for a tool whose job is to encrypt traffic.

Primitive mapping per backend (the two are NOT wire-compatible):

| Step | monocypher | tweetnacl |
|------|------------|-----------|
| Ephemeral key agreement | X25519 (`crypto_x25519_public_key` + `crypto_x25519`) | X25519 via NaCl (`crypto_box_keypair`, `crypto_box_beforenm`) |
| KDF hash mixing passphrase | BLAKE2b-256 (`crypto_blake2b`) | SHA-512[0:32] (`crypto_hash`) |
| Per-packet AEAD | XChaCha20-Poly1305 (`crypto_aead_lock` / `crypto_aead_unlock`) | XSalsa20-Poly1305 (`crypto_secretbox` / `crypto_secretbox_open`) |

### Wire protocol (see `Packet.cpp`, `Crypto.cpp`, `sft_constants.hpp`)

**Handshake (immediately after TCP accept/connect, before any data):**
1. RX generates an ephemeral X25519 keypair and sends its 32-byte public key in cleartext.
2. TX receives RX's pubkey, generates its own ephemeral keypair, and sends its pubkey in cleartext.
3. Both sides compute `shared = X25519(peer_pk, my_sk)` and then `key = H(shared || passphrase)[0:32]` where `H` is SHA-512 (tweetnacl backend) or BLAKE2b-256 (monocypher backend, no truncation needed). The passphrase makes the handshake authenticated - a MITM substituting fake pubkeys derives a different key than either real endpoint, so the first encrypted packet fails MAC verification.
4. The ephemeral secret keys are wiped (`Crypto::zeroize`) as soon as the shared key is derived. The shared key is wiped in the `TX`/`RX` destructor.

**Per-frame format on the data plane:**

```
[4-byte BE length] [24-byte nonce] [16-byte Poly1305 MAC] [N bytes ciphertext]
```

`length` covers everything after itself (`N + CRYPTO_OVERHEAD = N + 40`). `MAX_PACKET_SIZE = 1048576` (1 MiB) plaintext bytes per chunk. There is **no application-level ACK** - TCP's own flow control plus an enlarged `SO_SNDBUF` / `SO_RCVBUF` (8 MiB on each side) provide pacing, so TX streams frames back-to-back. `TCP_NODELAY` is set on both endpoints to keep small handshake/EOF frames from getting held by Nagle.

**Frame ordering after the handshake:**
1. **Size preamble** - one encrypted frame whose 8-byte plaintext is the big-endian uint64 file size. Lets RX render a matching progress bar without changing the framing format. RX rejects the connection if the first frame's plaintext is not exactly 8 bytes.
2. **Data frames** - file body in `MAX_PACKET_SIZE` chunks.
3. **End-of-stream** - an encrypted frame with an empty plaintext payload (length header = 40, ciphertext = 0 bytes). The receiver decrypts every frame; an empty plaintext signals EOF. Because EOF is authenticated, an attacker cannot inject a fake one to truncate the transfer.

### Transfer flow
1. Both sides perform the handshake above (`TX::handshake` / `RX::handshake`).
2. TX runs `tar -cf .ft_temp_pack.tar.gz <path>` in the **current working directory** (`Transceiver::packFile`, via `fork`+`execvp` - no shell, so user paths cannot inject commands).
3. TX `stat()`s the tarball, then sends the 8-byte file size as the first encrypted frame after the handshake (the "size preamble"). RX reads it via a shared `readOneFrame` helper and rejects the connection if the decrypted plaintext is not exactly 8 bytes.
4. TX streams the tarball in `MAX_PACKET_SIZE` (1 MiB) chunks. Each chunk is wrapped by `Packet::serialize`, which encrypts with the shared key and prepends the length header. After the last chunk, TX sends an encrypted empty-payload frame via `Packet::generateTerminationPacket`.
5. RX reads each frame, calls `Crypto::decryptPacket`, treats decrypt failure as a per-connection failure (wrong passphrase or tampered data - logged then connection torn down; only fatal in non-loop mode), writes the plaintext to `.ft_temp_pack_buffer.tar.gz`, and breaks on the first empty plaintext.
6. RX runs `tar -xf` on the buffer (`Transceiver::unpackFile`).
7. Both sides delete their temp tarball via `popen("rm ...")` (still shells out, but the filename is fixed and not user-controlled).

Consequence: `sft` must be run from a writable directory, and the temp filenames are global within that directory - running two transfers concurrently in the same cwd will collide.

### I/O helpers
`NetworkUtility::recvAll` / `sendAll` loop on `recv`/`send` until the requested byte count is transferred or the peer hangs up (`count == 0` → returns `false`, which callers treat as fatal). Use these rather than raw `recv`/`send` when adding new wire messages.

### Argument parsing
Lives under `source/{include,src}/cli/`. `ArgParser.cpp::parseInput` walks argv directly (no string concatenation, no `find()`-based substring matching) and recognises whole tokens. `rx` and `tx` are positionals; `-ip4`/`-ip6`/`-p` each consume the next argv token (POSIX-getopt-style - the value is not validated against the flag namespace, so `sft rx -p -k` is a user error that surfaces downstream); `-k` is a boolean and triggers `promptPassphrase()` after argv has fully validated. The file path is a single positional accepted in any position. Per-flag helpers (`checkIP`, `checkPort`, `promptPassphrase`) each live in their own `*Flag.{hpp,cpp}` pair. IP validation uses a single combined IPv4/IPv6 regex in `checkIP` - replace the whole regex if you need to change behaviour; do not edit it in place. Port range is `[1024, 65535]`; out-of-range falls back to `8203` with a warning.

### Logging
`Logging` is a static class with a mutex around stdout/stderr writes. Each status line is prefixed with a Unicode symbol coloured by severity:

| Level | Symbol | Colour | Stream |
|-------|--------|--------|--------|
| `logInfo` | `→` | cyan | stdout |
| `logSuccess` | `✓` | green | stdout |
| `logWarning` | `⚠` | yellow | stderr |
| `logError` | `✗` | bold red | stderr |

ANSI escape codes are emitted only when the destination stream is a TTY (cached `isatty()` check at first use). Piping to a file or `tee` produces clean, ANSI-free output that stays grep-able. There are **no timestamps** in the output — the tool is short-lived and interactive; if a caller wants timestamps, they can pipe through `ts(1)`.

Progress uses a three-phase API: `logProgressStart(total)`, `logProgressUpdate(bytesDone)` (throttled to ~5 frames/sec via `steady_clock`), and `logProgressFinish()`. The bar uses U+2588 (`█`) and U+2591 (`░`) block characters and a single `\r` + `\033[K` carriage-return-and-erase redraw, so it updates in place without scroll-spamming. The line shows percentage, bytes done / total, average speed since `logProgressStart`, and ETA (formatted via `humanBytes` / `humanDuration`). When stdout is not a TTY, intermediate updates are dropped — only the final state is written, so log files stay readable. Internal `ProgressState` lives in the anonymous namespace of `Logging.cpp`, not on the class, so the public surface stays clean.
