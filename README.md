# simpleft
[![CMake on linux and macos](https://github.com/yuqian5/simpleft/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=master)](https://github.com/yuqian5/simpleft/actions/workflows/cmake-multi-platform.yml)

## Menu
- [About](#description)
- [Install](#install)
    - [Homebrew](#homebrew)
    - [Compile from source](#compile-from-source)
- [Usage](#usage)
    - [Passphrase](#passphrase)
- [Security](#security)

## Description
**simpleft** is a lightweight command-line tool for peer-to-peer file sharing. No server, no configuration, no hassle. Every transfer is end-to-end encrypted with a shared passphrase - both ends must enter the same passphrase or the transfer aborts.

## Install
### Homebrew
*
        brew tap yuqian5/simpleft
        brew install simpleft

### Compile from source
*
        cd source
        cmake CMakeLists.txt
        make
        make install

## Usage

Run `sft -h` (or `sft --help`) at any time to print the full flag reference.

- To receive files:
    ```
    sft rx -p [port] -k
    ```
- To receive and keep listening for more transfers (rx-only):
    ```
    sft rx -p [port] -k -loop
    ```
- To send files over IPv4:
    ```
    sft tx -ip4 [IPv4 address] -p [port] -k [path]
    ```
- To send files over IPv6:
    ```
    sft tx -ip6 [IPv6 address] -p [port] -k [path]
    ```

Note: port and IP address are optional. Defaults: port `8203`, IP `127.0.0.1` (IPv4). The path may be a file or a directory - it is `tar`-packed before sending.

### Passphrase
Encryption is mandatory. The passphrase can come from either source:

- **`-k` flag** (interactive): prompts on the terminal with echo disabled.
    ```
    echo "my-secret" | sft rx -p 8203 -k
    ```
- **`SFT_PASSPHRASE` env var** (non-interactive): used when `-k` is absent.
    ```
    SFT_PASSPHRASE=my-secret sft rx -p 8203
    SFT_PASSPHRASE=my-secret sft tx -ip4 1.2.3.4 -p 8203 file.txt
    ```

Both sides must end up with the same passphrase; otherwise the receiver aborts on the first encrypted packet. `-k` takes precedence if both are set.

## Security
- **Ephemeral Curve25519 keypairs** are generated for every transfer and discarded after, giving each session its own keys (forward secrecy).
- **XSalsa20-Poly1305** authenticates and encrypts every frame, including the end-of-stream marker. Any tampering is detected and aborts the transfer.
- **The shared passphrase** is mixed into key derivation (SHA-512), so an active MITM substituting their own public keys cannot impersonate either side - it would derive a different key from the legitimate parties, and decryption would fail on the first packet.
- The cryptography uses a vendored copy of [TweetNaCl](https://tweetnacl.cr.yp.to/) (`source/lib/tweetnacl/`). No system-installed crypto libraries are required.
