# Binary Serialization

Type-safe binary stream classes for network packet serialization and deserialization.

## Design

- **COutBinStream**: `operator<<` appends primitives to a `vector<char>` buffer
- **CInBinStream**: `operator>>` reads primitives with bounds checking on every read
- **CBinStream**: Base class managing buffer ownership (internal or external)

## Wire Format

- All integers: little-endian (native x86 byte order)
- Strings: `uint16` length prefix + raw bytes (no null terminator)
- Floats: 4 bytes IEEE 754, with NaN/Infinity rejection on deserialization
- Booleans: single byte (0 or 1)

## Safety

Every read operation checks `remaining() >= needed` before accessing the buffer.
On underflow: throws `std::underflow_error` (or calls `abort()` based on `ThrowEnable` flag).

Original error strings from the binary (e.g., `"CInBinStream::operator >> (char &Char)"`)
confirmed the exact class names, method signatures, and error handling behavior.
