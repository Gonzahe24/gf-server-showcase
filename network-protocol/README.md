# Network Protocol

TCP connection layer with RSA key exchange and RC4 stream encryption.

## Protocol Stack (bottom to top)

1. **TCP socket** - Non-blocking, managed by epoll
2. **Length header** - 2 bytes little-endian (body size, excludes header)
3. **RC4 cipher** - 5-byte session key, applied to body only
4. **NCID** - 2-byte Network Command ID (first 2 bytes of decrypted body)
5. **Payload** - Type-specific binary serialization

## Key Exchange Sequence

1. Server generates RSA-2048 keypair and sends public key `(n, e)` in raw binary
2. Client generates random 5-byte session key
3. Client RSA-encrypts the session key with `RSA_PKCS1_PADDING` and sends it
4. Both sides init `RC4_set_key()` with the shared key
5. All subsequent traffic is RC4-encrypted

Recovered from decompiled binary offsets `0x00aef310`-`0x00af0f90`.
