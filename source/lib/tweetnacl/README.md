# TweetNaCl (vendored)

`tweetnacl.c` and `tweetnacl.h` are the canonical TweetNaCl reference
implementation by Bernal, Bernstein, Janssen, Lange, Schwabe, and Yang,
released into the public domain.

Source: https://tweetnacl.cr.yp.to/20140427/

These files are **not modified**. Do not edit them — if a newer upstream
release is published, replace both files wholesale.

`randombytes.c` is a local file (not part of upstream TweetNaCl). TweetNaCl
declares `randombytes` but does not define it; the project must supply one.
This implementation reads from `/dev/urandom`.
