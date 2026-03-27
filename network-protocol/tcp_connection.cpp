// =============================================================================
// CMSTCPConnection implementation
//
// Reverse-engineered from decompiled x86 ELF binary using Ghidra.
// Key functions traced from decompiled offsets:
//   Open()             @ 0x00aef420
//   ReadRSAPublicKey() @ 0x00af0d20
//   SendSessionKey()   @ 0x00af0f90
//   FillInputBuffer()  @ 0x00af0af0
//   MakePackets()      @ 0x00af0c10
//   OnParsingHeader()  @ 0x00af0990
//   NextOutputPacket() @ 0x00af0770
//
// The protocol stack handles:
//   1. RSA-2048 key exchange (server sends pubkey, client sends encrypted session key)
//   2. RC4 stream cipher on all subsequent packets
//   3. Length-prefixed binary packets (2 bytes LE header + encrypted body)
// =============================================================================

#include "tcp_connection.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>

namespace lapis {

// --- CConnectionSocket: delegates epoll events to owning connection ---

bool CConnectionSocket::OnRead()       { return m_owner->OnRead(); }
bool CConnectionSocket::OnWrite()      { return m_owner->OnWrite(); }
bool CConnectionSocket::WriteComplete(){ return m_owner->WriteComplete(); }
void CConnectionSocket::NetworkError() { m_owner->NetworkError(); }

bool CMSTCPConnection::WriteComplete() {
    return CurrentOutputPacket == nullptr && OutputPackets.empty();
}

// --- Constructor / Destructor ---

CMSTCPConnection::CMSTCPConnection(IInterfaceFactory* factory,
                                   const std::string& address, int port)
    : Socket(this) {
    InterfaceFactory = factory;
    Address = address;
    Port = port;
    m_networkState = NET_UNINIT;
    State = 0;
    memset(&SendKey, 0, sizeof(SendKey));
    memset(&RecvKey, 0, sizeof(RecvKey));
    memset(SessionKey, 0, sizeof(SessionKey));
}

CMSTCPConnection::~CMSTCPConnection() {
    if (ServerPublicKey) {
        RSA_free(ServerPublicKey);
        ServerPublicKey = nullptr;
    }
    delete[] SessionKeyData;
    SessionKeyData = nullptr;
    // n_buf and e_buf are freed during ReadRSAPublicKey after key reconstruction.
    // The destructor in the original binary (0x00aef310) does not free them.
}

// --- Open: create socket, connect, begin RSA key exchange ---

bool CMSTCPConnection::Open() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    Socket.FD = fd;
    if (fd < 0) {
        Socket.FD = -1;
        perror("TCP connection: socket");
        return false;
    }

    struct hostent* host = gethostbyname(Address.c_str());
    if (!host) {
        fprintf(stderr, "Invalid hostname '%s'\n", Address.c_str());
        return false;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(Port));
    memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        fprintf(stderr, "TCP connection: %s:%d %s\n",
                inet_ntoa(addr.sin_addr), Port, strerror(errno));
        close(fd);
        Socket.FD = -1;
        return false;
    }

    // Set non-blocking for epoll integration
    int flags = fcntl(fd, F_GETFL);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("TCP connection: fcntl");
        close(fd);
        Socket.FD = -1;
        return false;
    }

    // Begin RSA key exchange: wait for server to send its public key
    m_networkState = NET_EXCHANGE_KEY_1;
    State = 0;
    delete[] n_buf; n_buf = nullptr;
    delete[] e_buf; e_buf = nullptr;

    if (ServerPublicKey) {
        RSA_free(ServerPublicKey);
        ServerPublicKey = nullptr;
    }
    ServerPublicKey = RSA_new();

    // Generate random 5-byte session key for RC4
    RAND_bytes(SessionKey, RC4_KEY_LENGTH);
    RC4_set_key(&SendKey, RC4_KEY_LENGTH, SessionKey);
    RC4_set_key(&RecvKey, RC4_KEY_LENGTH, SessionKey);

    // Reset I/O state
    CurrentInputPacket = nullptr;
    HeadIndex = 0;
    m_readState = READ_HEADER_0;
    TailIndex = 0x1FFF;  // From decompiled initialization
    CurrentOutputPacket = nullptr;

    fprintf(stderr, "[NET] Connected to %s:%d, awaiting RSA key...\n",
            Address.c_str(), Port);
    return true;
}

// --- Close: shutdown and free all resources ---

bool CMSTCPConnection::Close() {
    if (Socket.FD >= 0) {
        close(Socket.FD);
        Socket.FD = -1;
    }

    // Free packet buffers
    delete CurrentInputPacket;
    CurrentInputPacket = nullptr;
    for (auto* pkt : InputPackets) delete pkt;
    InputPackets.clear();
    for (auto* pkt : OutputPackets) delete pkt;
    OutputPackets.clear();
    delete CurrentOutputPacket;
    CurrentOutputPacket = nullptr;

    // Free RSA resources
    if (ServerPublicKey) {
        RSA_free(ServerPublicKey);
        ServerPublicKey = nullptr;
    }
    delete[] SessionKeyData;
    SessionKeyData = nullptr;

    m_networkState = NET_UNINIT;
    return true;
}

// --- RC4 encryption/decryption (applied to packet body only) ---

void CMSTCPConnection::Encryption(size_t size, uint8_t* in_text, uint8_t* out_text) {
    RC4(&SendKey, size, in_text, out_text);
}

void CMSTCPConnection::Decryption(size_t size, uint8_t* in_text, uint8_t* out_text) {
    RC4(&RecvKey, size, in_text, out_text);
}

// --- ReadRSAPublicKey: read server's RSA-2048 public key (3-phase state machine) ---
// Phase 0: Read n_size (4 bytes) + e_size (4 bytes)
// Phase 1: Read n_buf (n_size bytes) - RSA modulus
// Phase 2: Read e_buf (e_size bytes) - RSA exponent
// Then reconstruct RSA key from (n, e) and transition to KEY_2 state.

bool CMSTCPConnection::ReadRSAPublicKey() {
    while (true) {
        if (State > 2) {
            // All RSA key data received -- reconstruct the key
            BIGNUM* bn_n = BN_bin2bn(n_buf, n_size, nullptr);
            BIGNUM* bn_e = BN_bin2bn(e_buf, e_size, nullptr);
            RSA_set0_key(ServerPublicKey, bn_n, bn_e, nullptr);

            delete[] n_buf; n_buf = nullptr;
            delete[] e_buf; e_buf = nullptr;

            // Transition to "send session key" state
            m_networkState = NET_EXCHANGE_KEY_2;
            State = 0;
            SessionKeyData = nullptr;

            fprintf(stderr, "[NET] RSA public key received, sending session key...\n");
            return true;
        }

        // Calculate available bytes in circular buffer
        int avail;
        int head = HeadIndex, tail = TailIndex;
        if (tail < head) avail = head - tail - 1;
        else avail = head + (0x1FFF - tail);

        switch (State) {
        case 0:
            // Need 8 bytes: n_size (uint32) + e_size (uint32)
            if (avail < 8) return false;
            memcpy(&n_size, InputBuffer + TailIndex + 1, 4);
            memcpy(&e_size, InputBuffer + TailIndex + 5, 4);
            TailIndex = (TailIndex + 8) % INPUT_BUFFER_SIZE;
            State++;
            break;
        case 1:
            if (static_cast<uint32_t>(avail) < n_size) return false;
            n_buf = new uint8_t[n_size];
            memcpy(n_buf, InputBuffer + TailIndex + 1, n_size);
            TailIndex = (TailIndex + n_size) % INPUT_BUFFER_SIZE;
            State++;
            break;
        case 2:
            if (static_cast<uint32_t>(avail) < e_size) return false;
            e_buf = new uint8_t[e_size];
            memcpy(e_buf, InputBuffer + TailIndex + 1, e_size);
            TailIndex = (TailIndex + e_size) % INPUT_BUFFER_SIZE;
            State++;
            break;
        }
    }
}

// --- SendSessionKey: RSA-encrypt the 5-byte session key and send it ---

bool CMSTCPConnection::SendSessionKey() {
    if (State == 0) {
        WriteLen = 0;
        delete[] SessionKeyData;
        SessionKeyData = nullptr;

        int rsaSize = RSA_size(ServerPublicKey);
        SessionKeyData = new uint8_t[rsaSize];
        int encLen = RSA_public_encrypt(RC4_KEY_LENGTH, SessionKey,
                                        SessionKeyData, ServerPublicKey,
                                        RSA_PKCS1_PADDING);
        if (encLen != rsaSize) {
            unsigned long e = ERR_get_error();
            char errmsg[256];
            ERR_error_string(e, errmsg);
            fprintf(stderr, "[NET] RSA_public_encrypt error: %s\n", errmsg);
            return false;
        }
        State++;
    } else if (State != 1) {
        return true;
    }

    // Send the RSA-encrypted session key (may require multiple send() calls)
    int rsaSize = RSA_size(ServerPublicKey);
    size_t toSend = rsaSize - WriteLen;
    ssize_t sent = send(Socket.FD, SessionKeyData + WriteLen, toSend, MSG_NOSIGNAL);

    if (sent == 0) return false;
    if (sent < 0) {
        int err = errno;
        return (err == EINTR || err == EAGAIN);
    }

    if (sent < static_cast<ssize_t>(toSend)) {
        WriteLen += static_cast<int>(sent);
        return true;  // Partial send, will continue on next OnWrite
    }

    // Session key fully sent -- cleanup RSA and transition to encrypted mode
    if (ServerPublicKey) {
        RSA_free(ServerPublicKey);
        ServerPublicKey = nullptr;
    }
    delete[] SessionKeyData;
    SessionKeyData = nullptr;

    m_networkState = NET_READY;
    fprintf(stderr, "[NET] Session key sent. Encrypted channel established.\n");
    return true;
}

// --- FillInputBuffer: recv() into circular buffer ---
// Returns: 1 = got data, 0 = would-block, -1 = error/disconnect

short CMSTCPConnection::FillInputBuffer(bool firstRead) {
    int head = HeadIndex;
    int tail = TailIndex;
    if (tail == head) return 1;

    // Calculate contiguous recv size (circular buffer wrapping)
    int size;
    if (tail > head) size = tail - head;
    else size = INPUT_BUFFER_SIZE - head;

    if (size <= 0) return (size == 0) ? 1 : -1;

    ssize_t received = recv(Socket.FD, InputBuffer + head,
                            static_cast<size_t>(size), 0);
    if (received == 0) {
        if (!firstRead) return 0;
        fprintf(stderr, "[NET] recv() returned 0 -- peer disconnected\n");
        return -1;
    }
    if (received > 0) {
        HeadIndex = (head + static_cast<int>(received)) % INPUT_BUFFER_SIZE;
        return 1;
    }

    int err = errno;
    if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR) return 0;
    perror("FillBuffer");
    return -1;
}

// --- OnParsingHeader: 2-byte length header state machine ---
// Packet format: [len_lo][len_hi][RC4-encrypted body]
// Length is body size (excludes the 2-byte header itself).

bool CMSTCPConnection::OnParsingHeader() {
    switch (m_readState) {
    case READ_HEADER_0: {
        int nextTail = (TailIndex + 1) % INPUT_BUFFER_SIZE;
        if (nextTail == HeadIndex) return true;  // No data yet
        TailIndex = nextTail;
        uint8_t b0 = InputBuffer[TailIndex];
        m_readState = READ_HEADER_1;
        CurrentInputPacketLength = b0;
    }
    // fall through
    case READ_HEADER_1: {
        int nextTail = (TailIndex + 1) % INPUT_BUFFER_SIZE;
        if (nextTail == HeadIndex) return true;  // Need one more byte
        TailIndex = nextTail;
        uint8_t b1 = InputBuffer[TailIndex];
        m_readState = READ_HEADER_DONE;
        CurrentInputPacketLength =
            static_cast<uint16_t>(b1) << 8 |
            static_cast<uint8_t>(CurrentInputPacketLength & 0xFF);
    }
    // fall through
    case READ_HEADER_DONE:
        if (CurrentInputPacketLength > MAX_PACKET_SIZE) {
            fprintf(stderr, "[NET] Invalid packet size %d, dropping connection.\n",
                    CurrentInputPacketLength);
            return false;
        }
        CurrentInputPacket = new SPacket();
        CurrentInputPacket->reserve(CurrentInputPacketLength);
        m_readState = READ_BODY;
        return true;
    case READ_BODY:
        return true;
    default:
        fprintf(stderr, "[NET] Bug: undefined read state. Connection dropped.\n");
        return false;
    }
}

// --- MakePackets: parse complete packets from circular buffer ---
// Each packet: read length header -> read body -> RC4 decrypt body -> queue

bool CMSTCPConnection::MakePackets() {
    while (true) {
        if (!OnParsingHeader()) return false;
        if (m_readState != READ_BODY) break;

        // Read body bytes from circular buffer into packet
        // (simplified from original GetBytes which handles wrap-around)
        int remaining = CurrentInputPacketLength -
                        static_cast<int>(CurrentInputPacket->size());
        while (remaining > 0) {
            int nextTail = (TailIndex + 1) % INPUT_BUFFER_SIZE;
            if (nextTail == HeadIndex) break;
            TailIndex = nextTail;
            CurrentInputPacket->push_back(
                static_cast<char>(InputBuffer[TailIndex]));
            remaining--;
        }

        if (remaining != 0) break;  // Incomplete packet, wait for more data

        // Decrypt the complete packet body using RC4
        SPacket* pkt = CurrentInputPacket;
        if (!pkt->empty()) {
            Decryption(pkt->size(),
                       reinterpret_cast<uint8_t*>(pkt->data()),
                       reinterpret_cast<uint8_t*>(pkt->data()));
        }

        InputPackets.push_back(pkt);
        CurrentInputPacket = nullptr;
        m_readState = READ_HEADER_0;
    }

    // Dispatch completed packets to the session's interface for NCID resolution
    // (Session->ResolveInput() reads NCID, looks up generator, deserializes)
    return true;
}

// --- NextOutputPacket: merge queued output packets into a send batch ---
// Writes 2-byte length header, RC4-encrypts body, merges into single buffer.

bool CMSTCPConnection::NextOutputPacket() {
    if (OutputPackets.empty()) return false;

    int totalSize = 0;
    SPacket* merged = new SPacket();

    while (!OutputPackets.empty()) {
        SPacket* pkt = OutputPackets.front();
        OutputPackets.pop_front();

        if (!pkt || pkt->empty()) {
            delete pkt;
            continue;
        }

        // Write 2-byte little-endian length header (body size)
        uint16_t bodyLen = static_cast<uint16_t>(pkt->size() - 2);
        pkt->data()[0] = static_cast<char>(bodyLen & 0xFF);
        pkt->data()[1] = static_cast<char>((bodyLen >> 8) & 0xFF);

        // RC4-encrypt body (skip 2-byte length header)
        if (pkt->size() > 2) {
            Encryption(pkt->size() - 2,
                       reinterpret_cast<uint8_t*>(pkt->data() + 2),
                       reinterpret_cast<uint8_t*>(pkt->data() + 2));
        }

        merged->insert(merged->end(), pkt->begin(), pkt->end());
        totalSize += static_cast<int>(pkt->size());
        delete pkt;

        // Batch limit: ~4KB per send (from decompiled constant 0x0FC0)
        if (totalSize >= 0x0FC0) break;
    }

    if (totalSize == 0) {
        delete merged;
        return false;
    }

    CurrentOutputPacket = merged;
    WritePtr = merged->data();
    return true;
}

// --- OnRead: top-level read handler ---

bool CMSTCPConnection::OnRead() {
    short fillState = FillInputBuffer(true);
    while (true) {
        if (fillState != 1) return (fillState != -1);

        // During key exchange, read RSA public key
        if (m_networkState == NET_EXCHANGE_KEY_1) {
            ReadRSAPublicKey();
        }

        // Once encrypted, parse packets
        if (m_networkState == NET_READY) {
            if (!MakePackets()) return false;
        }

        fillState = FillInputBuffer(false);
    }
}

// --- OnWrite: top-level write handler ---

bool CMSTCPConnection::OnWrite() {
    if (m_networkState == NET_READY) {
        InWrite_Conn = false;
        SPacket* outPkt = CurrentOutputPacket;

        if (!outPkt) {
            NextOutputPacket();
            outPkt = CurrentOutputPacket;
            if (!outPkt) return true;
        }

        char* buf = WritePtr;
        size_t len = outPkt->data() + outPkt->size() - buf;
        if (static_cast<long>(len) <= 0) return true;

        ssize_t sent = send(Socket.FD, buf, len, MSG_NOSIGNAL);
        if (sent == 0) return false;
        if (sent < 0) {
            int err = errno;
            return (err == EINTR || err == EAGAIN);
        }

        if (sent < static_cast<ssize_t>(len)) {
            WritePtr += sent;  // Partial send, continue next time
            return true;
        }

        // Full send -- free and prepare next batch
        delete CurrentOutputPacket;
        CurrentOutputPacket = nullptr;
        WritePtr = nullptr;
        return true;
    }

    if (m_networkState == NET_EXCHANGE_KEY_2) {
        return SendSessionKey();
    }

    return true;
}

// --- NetworkError: connection lost ---

void CMSTCPConnection::NetworkError() {
    fprintf(stderr, "[NET] Connection lost on fd=%d\n", Socket.FD);
    m_networkState = NET_UNINIT;
}

void CMSTCPConnection::ComposeOutput(INetCommand* /*netCommand*/) {
    // Serializes command via _Serialize() into output queue
    // (delegates to Session->Interface->ComposeOutput in full implementation)
}

} // namespace lapis
