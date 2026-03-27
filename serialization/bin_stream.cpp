// =============================================================================
// Binary stream implementation
//
// Reverse-engineered from decompiled x86 ELF binary using Ghidra.
// Error strings in the original binary (e.g., "CInBinStream::operator >> (char &Char)")
// were used to confirm exact method signatures and class hierarchy.
//
// Key design decisions recovered from decompilation:
//   - Strings are length-prefixed with uint16, NOT null-terminated
//   - Float reads reject NaN/Infinity (prevents game state corruption)
//   - ThrowEnable flag: true=throw std::underflow_error, false=abort()
//   - Hex dump uses 24 bytes per line (matched to original Dump() output)
// =============================================================================

#include "bin_stream.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>

namespace lapis {

// ---------------------------------------------------------------------------
// CBinStream (base)
// ---------------------------------------------------------------------------

bool CBinStream::ThrowEnable = true;

CBinStream::CBinStream()
    : m_external(false)
    , m_streamSpace(new std::vector<char>())
{}

CBinStream::CBinStream(std::vector<char>* externalBuffer)
    : m_external(true)
    , m_streamSpace(externalBuffer)
{}

CBinStream::~CBinStream() {
    if (!m_external) {
        delete m_streamSpace;
    }
}

std::vector<char>* CBinStream::ExportPacket() {
    m_external = true;  // Caller now owns the buffer
    return m_streamSpace;
}

void CBinStream::Dump() const {
    const auto& buf = *m_streamSpace;
    size_t total = buf.size();
    std::fprintf(stderr, "Remain = %zu bytes\n", total);

    size_t offset = 0;
    while (offset < total) {
        char line[128];
        char* p = line;

        // Hex portion: 24 bytes per line (matches original format)
        for (int i = 0; i < 24; ++i) {
            if (offset + i < total) {
                std::sprintf(p, "%2X", static_cast<unsigned char>(buf[offset + i]));
            } else {
                p[0] = ' '; p[1] = ' '; p[2] = '\0';
            }
            p += 2;
        }
        *p++ = ' ';

        // ASCII portion
        for (int i = 0; i < 24 && offset + i < total; ++i) {
            char c = buf[offset + i];
            *p++ = (c >= 0x20 && c < 0x7f) ? c : '_';
        }
        *p = '\0';

        std::fprintf(stderr, "%s\n", line);
        offset += 24;
    }
}

// ---------------------------------------------------------------------------
// CInBinStream (input / deserialization)
// ---------------------------------------------------------------------------

CInBinStream::CInBinStream() : CBinStream(), m_pos(0) {}

CInBinStream::CInBinStream(std::vector<char>* externalBuffer)
    : CBinStream(externalBuffer), m_pos(0) {}

CInBinStream::~CInBinStream() { m_pos = 0; }

size_t CInBinStream::remaining() const {
    size_t total = m_streamSpace->size();
    return (static_cast<size_t>(m_pos) >= total) ? 0 : total - m_pos;
}

bool CInBinStream::eof() const {
    return static_cast<size_t>(m_pos) >= m_streamSpace->size();
}

void CInBinStream::checkRemaining(size_t needed, const char* context) const {
    if (remaining() < needed) {
        fprintf(stderr, "[NET-STREAM] UNDERFLOW: %s (need %zu, have %zu, pos=%d, total=%zu)\n",
                context, needed, remaining(), m_pos, m_streamSpace->size());
        if (ThrowEnable) throw std::underflow_error(context);
        std::abort();
    }
}

CInBinStream& CInBinStream::operator>>(char& val) {
    checkRemaining(1, "CInBinStream::operator >> (char &Char)");
    val = (*m_streamSpace)[m_pos];
    m_pos += 1;
    return *this;
}

CInBinStream& CInBinStream::operator>>(unsigned char& val) {
    checkRemaining(1, "CInBinStream::operator >> (unsigned char &UnsignedChar)");
    val = static_cast<unsigned char>((*m_streamSpace)[m_pos]);
    m_pos += 1;
    return *this;
}

CInBinStream& CInBinStream::operator>>(short& val) {
    checkRemaining(2, "CInBinStream::operator >> (short &Short)");
    std::memcpy(&val, m_streamSpace->data() + m_pos, 2);
    m_pos += 2;
    return *this;
}

CInBinStream& CInBinStream::operator>>(unsigned short& val) {
    checkRemaining(2, "CInBinStream::operator >> (unsigned short &UnsignedShort)");
    std::memcpy(&val, m_streamSpace->data() + m_pos, 2);
    m_pos += 2;
    return *this;
}

CInBinStream& CInBinStream::operator>>(int& val) {
    checkRemaining(4, "CInBinStream::operator >> (int &Int)");
    std::memcpy(&val, m_streamSpace->data() + m_pos, 4);
    m_pos += 4;
    return *this;
}

CInBinStream& CInBinStream::operator>>(unsigned int& val) {
    checkRemaining(4, "CInBinStream::operator >> (unsigned int &UnsignedInt)");
    std::memcpy(&val, m_streamSpace->data() + m_pos, 4);
    m_pos += 4;
    return *this;
}

CInBinStream& CInBinStream::operator>>(long long& val) {
    checkRemaining(8, "CInBinStream::operator >> (longlong &LongLong)");
    std::memcpy(&val, m_streamSpace->data() + m_pos, 8);
    m_pos += 8;
    return *this;
}

CInBinStream& CInBinStream::operator>>(unsigned long long& val) {
    checkRemaining(8, "CInBinStream::operator >> (unsigned long long &UnsignedLongLong)");
    std::memcpy(&val, m_streamSpace->data() + m_pos, 8);
    m_pos += 8;
    return *this;
}

CInBinStream& CInBinStream::operator>>(float& val) {
    checkRemaining(4, "CInBinStream::operator >> (float &Float)");
    std::memcpy(&val, m_streamSpace->data() + m_pos, 4);
    m_pos += 4;

    // NaN/Infinity rejection: prevents game state corruption from malformed packets.
    // This check was found in the original decompiled binary.
    if (std::isinf(val) || std::isnan(val)) {
        if (ThrowEnable) {
            throw std::domain_error(
                "CInBinStream::operator >> (float &Float): INF or NAN");
        }
        std::abort();
    }
    return *this;
}

CInBinStream& CInBinStream::operator>>(std::string& val) {
    // String format: uint16 length prefix + raw bytes (no null terminator)
    unsigned short len = 0;
    *this >> len;
    checkRemaining(len, "CInBinStream::operator >> (std::string &String)");
    val.assign(m_streamSpace->data() + m_pos, len);
    m_pos += len;
    return *this;
}

void CInBinStream::GetBinary(char* buf, unsigned int size) {
    checkRemaining(size, "CInBinStream::GetBinary");
    std::memcpy(buf, m_streamSpace->data() + m_pos, size);
    m_pos += size;
}

// ---------------------------------------------------------------------------
// COutBinStream (output / serialization)
// ---------------------------------------------------------------------------

COutBinStream::COutBinStream() : CBinStream() {}

COutBinStream::COutBinStream(std::vector<char>* externalBuffer)
    : CBinStream(externalBuffer) {}

void COutBinStream::appendBytes(const char* data, size_t len) {
    m_streamSpace->insert(m_streamSpace->end(), data, data + len);
}

COutBinStream& COutBinStream::operator<<(char val) {
    m_streamSpace->push_back(val);
    return *this;
}

COutBinStream& COutBinStream::operator<<(unsigned char val) {
    m_streamSpace->push_back(static_cast<char>(val));
    return *this;
}

COutBinStream& COutBinStream::operator<<(short val) {
    appendBytes(reinterpret_cast<const char*>(&val), 2);
    return *this;
}

COutBinStream& COutBinStream::operator<<(unsigned short val) {
    appendBytes(reinterpret_cast<const char*>(&val), 2);
    return *this;
}

COutBinStream& COutBinStream::operator<<(int val) {
    appendBytes(reinterpret_cast<const char*>(&val), 4);
    return *this;
}

COutBinStream& COutBinStream::operator<<(unsigned int val) {
    appendBytes(reinterpret_cast<const char*>(&val), 4);
    return *this;
}

COutBinStream& COutBinStream::operator<<(long long val) {
    appendBytes(reinterpret_cast<const char*>(&val), 8);
    return *this;
}

COutBinStream& COutBinStream::operator<<(unsigned long long val) {
    appendBytes(reinterpret_cast<const char*>(&val), 8);
    return *this;
}

COutBinStream& COutBinStream::operator<<(float val) {
    appendBytes(reinterpret_cast<const char*>(&val), 4);
    return *this;
}

COutBinStream& COutBinStream::operator<<(const std::string& val) {
    // String format: uint16 length prefix + raw bytes (no null terminator)
    auto len = static_cast<unsigned short>(val.size());
    *this << len;
    if (len > 0) {
        appendBytes(val.data(), len);
    }
    return *this;
}

void COutBinStream::PutBinary(const char* data, unsigned int size) {
    if (size > 0) {
        appendBytes(data, size);
    }
}

} // namespace lapis
