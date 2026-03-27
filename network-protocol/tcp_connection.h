// =============================================================================
// CMSTCPConnection - Client-side TCP connection with RSA/RC4 encryption
//
// Reverse-engineered from decompiled x86 ELF binary using Ghidra.
// Original class at decompiled offset 0x00aef310 (destructor).
//
// Protocol stack (bottom to top):
//   TCP socket (non-blocking, epoll-managed)
//   -> 2-byte little-endian length header
//   -> RC4 stream cipher (5-byte session key)
//   -> 2-byte NCID (Network Command ID)
//   -> Payload (type-specific binary serialization)
//
// Key exchange sequence:
//   1. Server sends RSA-2048 public key (n_size, e_size, n_buf, e_buf)
//   2. Client generates random 5-byte session key
//   3. Client RSA-encrypts session key and sends to server
//   4. Both sides initialize RC4 with the shared session key
//   5. All subsequent packets are RC4-encrypted
// =============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <list>
#include <vector>
#include <openssl/rc4.h>
#include <openssl/rsa.h>

namespace lapis {

class IInterfaceFactory;
class INetCommand;
class CMSTCPConnection;

// Packet buffer type - mirrors original SPacket typedef
using SPacket = std::vector<char>;

// Network state machine (recovered from decompiled enum)
enum NetworkState : int {
    NET_UNINIT         = 0,
    NET_EXCHANGE_KEY_1 = 1,  // Waiting for RSA public key from server
    NET_EXCHANGE_KEY_2 = 2,  // Sending RSA-encrypted session key
    NET_READY          = 3,  // Encrypted channel established
    NET_ERROR          = 4   // Fatal error
};

// Packet parsing state machine (recovered from decompiled ReadState)
enum ReadState : short {
    READ_HEADER_0    = 0,  // Waiting for first byte of length
    READ_HEADER_1    = 1,  // Waiting for second byte of length
    READ_HEADER_DONE = 2,  // Length parsed, allocating packet buffer
    READ_BODY        = 3   // Reading packet body bytes
};

// Constants recovered from decompiled binary
static constexpr int INPUT_BUFFER_SIZE = 0x2000;  // 8192-byte circular buffer
static constexpr int MAX_PACKET_SIZE   = 0x2000;
static constexpr int RC4_KEY_LENGTH    = 5;        // 5-byte RC4 session key

// CConnectionSocket: delegates epoll events back to the owning connection.
// In the original binary, the Socket member's vtable entries point back to
// the Connection's methods; this achieves the same delegation pattern.
class CConnectionSocket {
public:
    explicit CConnectionSocket(CMSTCPConnection* owner)
        : m_owner(owner), FD(-1) {}

    bool OnRead();
    bool OnWrite();
    bool WriteComplete();
    void NetworkError();

    int FD;

private:
    CMSTCPConnection* m_owner;
};

// CMSTCPConnection: client-side TCP connection (connects out to a remote server).
// Contains its own CConnectionSocket (at offset 0x20B0 in original binary)
// separate from the base channel class.
//
// Handles RSA public key reading, session key encryption, and normal packet I/O
// through a circular input buffer and queued output packets.
class CMSTCPConnection {
public:
    CMSTCPConnection(IInterfaceFactory* factory,
                     const std::string& address, int port);
    ~CMSTCPConnection();

    // --- Connection lifecycle ---
    bool Open();    // Creates socket, connects, starts RSA key exchange
    bool Close();   // Shuts down connection, frees all resources

    // --- Socket event handlers (called by CConnectionSocket delegation) ---
    bool OnRead();
    bool OnWrite();
    void NetworkError();

    // --- RC4 encryption (applied to packet body, not length header) ---
    void Encryption(size_t size, uint8_t* in_text, uint8_t* out_text);
    void Decryption(size_t size, uint8_t* in_text, uint8_t* out_text);

    // --- Packet processing ---
    bool OnParsingHeader();         // State machine for 2-byte length header
    bool MakePackets();             // Parse + decrypt incoming packets
    short FillInputBuffer(bool firstRead);  // recv() into circular buffer
    bool NextOutputPacket();        // Prepare next outgoing batch

    // --- RSA key exchange ---
    bool ReadRSAPublicKey();        // Read server's RSA-2048 public key
    bool SendSessionKey();          // RSA-encrypt and send 5-byte session key

    // --- Outgoing command serialization ---
    void ComposeOutput(INetCommand* netCommand);

    // Internal socket with delegation back to this connection
    CConnectionSocket Socket;

    // Connection parameters
    IInterfaceFactory* InterfaceFactory = nullptr;
    std::string Address;
    uint32_t Port = 0;

    // Network state machine
    NetworkState m_networkState = NET_UNINIT;

    // RSA key exchange state
    int State = 0;  // 0=sizes, 1=n_buf, 2=e_buf
    uint8_t* n_buf = nullptr;
    uint8_t* e_buf = nullptr;
    uint32_t n_size = 0;
    uint32_t e_size = 0;
    RSA* ServerPublicKey = nullptr;
    uint8_t* SessionKeyData = nullptr;
    int WriteLen = 0;

    // Session key (5 bytes for RC4)
    uint8_t SessionKey[RC4_KEY_LENGTH] = {};
    RC4_KEY SendKey = {};
    RC4_KEY RecvKey = {};

    // I/O buffers
    uint8_t InputBuffer[INPUT_BUFFER_SIZE] = {};
    int HeadIndex = 0;
    int TailIndex = 0x1FFF;  // Circular buffer tail (from decompiled init)
    ReadState m_readState = READ_HEADER_0;
    int CurrentInputPacketLength = 0;
    SPacket* CurrentInputPacket = nullptr;
    std::list<SPacket*> InputPackets;
    SPacket* CurrentOutputPacket = nullptr;
    std::list<SPacket*> OutputPackets;
    char* WritePtr = nullptr;
    bool InWrite_Conn = false;
};

} // namespace lapis
