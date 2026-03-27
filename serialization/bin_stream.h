// =============================================================================
// CInBinStream / COutBinStream - Binary serialization for network protocol
//
// Reverse-engineered from decompiled x86 ELF binary using Ghidra.
// Original error strings preserved (e.g., "CInBinStream::operator >> (char &Char)")
// which confirmed the exact class and method names.
//
// Features:
//   - Type-safe operator<< / operator>> for all primitive types
//   - Bounds checking on every read (throws std::underflow_error)
//   - NaN/Infinity rejection on float reads (matches original behavior)
//   - Length-prefixed strings (uint16 length + raw bytes, no null terminator)
//   - Raw binary read/write for byte arrays
//   - External buffer mode (wraps existing packet buffer without copying)
// =============================================================================

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace lapis {

// Base binary stream: wraps a std::vector<char> as the packet buffer.
// Original uses SPacket (typedef for vector<char>) with CPacketAllocator pool;
// simplified to direct vector ownership for this showcase.
class CBinStream {
public:
    CBinStream();
    explicit CBinStream(std::vector<char>* externalBuffer);
    virtual ~CBinStream();

    void Dump() const;                          // Hex dump for debugging
    std::vector<char>* ExportPacket();          // Transfer buffer ownership

    const std::vector<char>& buffer() const { return *m_streamSpace; }
    std::vector<char>& buffer() { return *m_streamSpace; }

    static bool ThrowEnable;  // true=throw on error, false=abort()

protected:
    bool m_external;
    std::vector<char>* m_streamSpace;
};

// Input stream: deserializes binary data from network packets.
// Maintains a read position and checks bounds before every read.
class CInBinStream : public CBinStream {
public:
    CInBinStream();
    explicit CInBinStream(std::vector<char>* externalBuffer);
    ~CInBinStream() override;

    // Primitive type extraction (all little-endian, matching x86 wire format)
    CInBinStream& operator>>(char& val);
    CInBinStream& operator>>(unsigned char& val);
    CInBinStream& operator>>(short& val);
    CInBinStream& operator>>(unsigned short& val);
    CInBinStream& operator>>(int& val);
    CInBinStream& operator>>(unsigned int& val);
    CInBinStream& operator>>(long long& val);
    CInBinStream& operator>>(unsigned long long& val);
    CInBinStream& operator>>(float& val);          // Rejects NaN and Infinity
    CInBinStream& operator>>(std::string& val);    // uint16 length prefix

    // Convenience overloads
    CInBinStream& operator>>(signed char& val) {
        char c; *this >> c; val = static_cast<signed char>(c); return *this;
    }
    CInBinStream& operator>>(bool& val) {
        char c; *this >> c; val = (c != 0); return *this;
    }

    // Raw binary read
    void GetBinary(char* buf, unsigned int size);
    void GetBinary(uint8_t* buf, unsigned int size) {
        GetBinary(reinterpret_cast<char*>(buf), size);
    }

    int position() const { return m_pos; }
    size_t remaining() const;
    bool eof() const;

private:
    void checkRemaining(size_t needed, const char* context) const;
    int m_pos;
};

// Output stream: serializes data for sending over the network.
// Appends data to the end of the underlying vector.
class COutBinStream : public CBinStream {
public:
    COutBinStream();
    explicit COutBinStream(std::vector<char>* externalBuffer);
    ~COutBinStream() override = default;

    // Primitive type insertion (all little-endian)
    COutBinStream& operator<<(char val);
    COutBinStream& operator<<(unsigned char val);
    COutBinStream& operator<<(short val);
    COutBinStream& operator<<(unsigned short val);
    COutBinStream& operator<<(int val);
    COutBinStream& operator<<(unsigned int val);
    COutBinStream& operator<<(long long val);
    COutBinStream& operator<<(unsigned long long val);
    COutBinStream& operator<<(float val);
    COutBinStream& operator<<(const std::string& val);  // uint16 length prefix

    // Convenience overloads
    COutBinStream& operator<<(signed char val) { return *this << static_cast<char>(val); }
    COutBinStream& operator<<(bool val) { return *this << static_cast<char>(val ? 1 : 0); }

    // Raw binary write
    void PutBinary(const char* data, unsigned int size);
    void PutBinary(const uint8_t* data, unsigned int size) {
        PutBinary(reinterpret_cast<const char*>(data), size);
    }

private:
    void appendBytes(const char* data, size_t len);
};

} // namespace lapis
