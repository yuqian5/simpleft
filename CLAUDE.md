# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**simpleft** (`sft`) is a single-binary C++ command-line tool for peer-to-peer file transfer. There is no server; one side runs in receive mode (`rx`) and listens, the other side runs in transmit mode (`tx`) and connects. Traffic is end-to-end encrypted with a vendored copy of TweetNaCl (Curve25519 + XSalsa20-Poly1305 + SHA-512); a shared passphrase is required on both sides. C++20, POSIX sockets, no system-installed third-party libraries. Builds on Linux and macOS (CI matrix: ubuntu-latest with gcc/clang, macos-latest with clang). Windows is not supported - the code uses `<sys/socket.h>`, `<arpa/inet.h>`, `<termios.h>`, and shells out to `tar`.

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

CMake calls `FIND_PROGRAM(TAR tar)` and fatal-errors if `tar` is missing - the binary also invokes `tar` at runtime (via `fork`+`execvp`, see `Transceiver::packFile`), so `tar` must be on `PATH` on both sides. `/dev/urandom` must also exist (used by `randombytes` in `lib/tweetnacl/randombytes.c`).

## Tests

There are no tests. CI runs `ctest` but no targets are registered in `CMakeLists.txt`, so it is a no-op. If adding tests, register them with CMake so the existing CI job picks them up.

A standalone crypto round-trip exercise lives in conversation history (compile `Crypto.cpp` against the cmake-built tweetnacl objects) - there is no checked-in test file.

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
- `Crypto` - thin facade over TweetNaCl. The only file that touches the NaCl zero-padding contract (see `src/Crypto.cpp` header comment). Provides `generateKeypair`, `deriveSharedKey`, `encryptPacket`, `decryptPacket`, and `zeroize`. Everything else in the codebase uses our compact wire format (no NaCl padding leaks out).

### Crypto layout

```
source/lib/tweetnacl/
├── README.md         (provenance note)
├── tweetnacl.c       (upstream, unmodified - replace wholesale on upgrade)
├── tweetnacl.h       (upstream, unmodified)
└── randombytes.c     (local /dev/urandom shim; tweetnacl declares but does not define randombytes)
```

`tweetnacl.h` has no `extern "C"` guards; `Crypto.cpp` is the only C++ file that includes it and wraps the include in `extern "C" { ... }`. If another C++ file ever needs tweetnacl directly, add the same wrap there - do not edit upstream headers.

Primitives in use:
- Curve25519 for ephemeral key agreement (`crypto_box_keypair`, `crypto_box_beforenm`)
- SHA-512 to mix the passphrase into the derived key (`crypto_hash`)
- XSalsa20-Poly1305 for per-packet authenticated encryption (`crypto_secretbox` / `crypto_secretbox_open`)

### Wire protocol (see `Packet.cpp`, `Crypto.cpp`, `sft_constants.hpp`)

**Handshake (immediately after TCP accept/connect, before any data):**
1. RX generates an ephemeral X25519 keypair and sends its 32-byte public key in cleartext.
2. TX receives RX's pubkey, generates its own ephemeral keypair, and sends its pubkey in cleartext.
3. Both sides compute `shared = crypto_box_beforenm(peer_pk, my_sk)` and then `key = SHA-512(shared || passphrase)[0:32]`. The passphrase makes the handshake authenticated - a MITM substituting fake pubkeys derives a different key than either real endpoint, so the first encrypted packet fails MAC verification.
4. The ephemeral secret keys are wiped (`Crypto::zeroize`) as soon as the shared key is derived. The shared key is wiped in the `TX`/`RX` destructor.

**Per-frame format on the data plane:**

```
[4-byte BE length] [24-byte nonce] [16-byte Poly1305 MAC] [N bytes ciphertext]
```

`length` covers everything after itself (`N + CRYPTO_OVERHEAD = N + 40`). `MAX_PACKET_SIZE = 8192` plaintext bytes per chunk. After every data packet the receiver replies with a 2-byte plaintext `"OK"` read receipt (`READ_RECEIPT`); the sender blocks on this before sending the next, so the protocol is strictly lock-step. The receipt is **not** authenticated - it is a pacing signal, not security-critical (an attacker forging OKs only races the legitimate one).

**End-of-transfer** is an encrypted frame with an empty plaintext payload (length header = 40, ciphertext = 0 bytes). The receiver decrypts every frame; an empty plaintext signals EOF. The plaintext `0xFFFFFFFF` sentinel from the old protocol no longer exists. Because EOF is authenticated, an attacker cannot inject a fake one to truncate the transfer.

### Transfer flow
1. Both sides perform the handshake above (`TX::handshake` / `RX::handshake`).
2. TX runs `tar -cf .ft_temp_pack.tar.gz <path>` in the **current working directory** (`Transceiver::packFile`, via `fork`+`execvp` - no shell, so user paths cannot inject commands).
3. TX streams the resulting tarball in `MAX_PACKET_SIZE` chunks. Each chunk is wrapped by `Packet::serialize`, which encrypts with the shared key and prepends the length header. After the last chunk, TX sends an encrypted empty-payload frame via `Packet::generateTerminationPacket`.
4. RX reads each frame, calls `Crypto::decryptPacket`, treats decrypt failure as fatal (wrong passphrase or tampered data), writes the plaintext to `.ft_temp_pack_buffer.tar.gz`, and breaks on the first empty plaintext.
5. RX runs `tar -xf` on the buffer (`Transceiver::unpackFile`).
6. Both sides delete their temp tarball via `popen("rm ...")` (still shells out, but the filename is fixed and not user-controlled).

Consequence: `sft` must be run from a writable directory, and the temp filenames are global within that directory - running two transfers concurrently in the same cwd will collide.

### I/O helpers
`NetworkUtility::recvAll` / `sendAll` loop on `recv`/`send` until the requested byte count is transferred or the peer hangs up (`count == 0` → returns `false`, which callers treat as fatal). Use these rather than raw `recv`/`send` when adding new wire messages - the read-receipt path in `TX::transmit` uses a bare `recv` and assumes a single 2-byte read, which is the one place that is not loop-safe.

### Argument parsing
Lives under `source/{include,src}/cli/`. `ArgParser.cpp::parseInput` walks argv directly (no string concatenation, no `find()`-based substring matching) and recognises whole tokens. `rx` and `tx` are positionals; `-ip4`/`-ip6`/`-p` each consume the next argv token (POSIX-getopt-style - the value is not validated against the flag namespace, so `sft rx -p -k` is a user error that surfaces downstream); `-k` is a boolean and triggers `promptPassphrase()` after argv has fully validated. The file path is a single positional accepted in any position. Per-flag helpers (`checkIP`, `checkPort`, `promptPassphrase`) each live in their own `*Flag.{hpp,cpp}` pair. IP validation uses a single combined IPv4/IPv6 regex in `checkIP` - replace the whole regex if you need to change behaviour; do not edit it in place. Port range is `[1024, 65535]`; out-of-range falls back to `8203` with a warning.

### Logging
`Logging` is a static class with a mutex and ANSI colour codes. `logProgress` is rate-limited via `lastLogTime` to avoid spamming the terminal during large transfers - pass `ignoreTimeLimit=true` to force a write (used for the final 100% update).
