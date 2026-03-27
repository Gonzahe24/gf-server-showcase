// =============================================================================
// Example CNC_CZ_ command structs - Client -> ZoneServer protocol
//
// Reverse-engineered from decompiled ZoneServer binary using Ghidra.
// Binary layout recovered by tracing Serialize/Deserialize functions.
//
// Naming convention (from original binary's string table):
//   "ZoneServer*" = sent FROM client TO zone server (server receives)
//   "Client*"     = sent FROM zone server TO client (client receives)
//
// There are ~633 CNC_CZ_ commands total. These are representative examples.
// =============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lapis {

class CInBinStream;
class COutBinStream;

// ---------------------------------------------------------------------------
// CNC_CZ_ZoneServerMoveSelf (NCID 30)
// Direction: Client -> ZoneServer
// The most frequent packet: client requests character movement.
//
// Decompiled Deserialize @ 0x0066c780:
//   >> DX >> DY >> TX >> TY
//   >> count; for count: >> char (PreDir vector - pathfinding waypoints)
//   >> FaceDir >> MoveAction
//
// Wire format: [DX:u16][DY:u16][TX:u16][TY:u16][count:u8][PreDir:count*u8][FaceDir:s16][MoveAction:s16]
// ---------------------------------------------------------------------------
struct CNC_CZ_ZoneServerMoveSelf {
    static constexpr uint16_t NCID = 30;

    uint16_t DX{};              // Current X position (map coordinates)
    uint16_t DY{};              // Current Y position
    uint16_t TX{};              // Target X position
    uint16_t TY{};              // Target Y position
    std::vector<char> PreDir;   // Pathfinding direction waypoints
    int16_t FaceDir{};          // Character facing direction (0-359)
    int16_t MoveAction{};       // Movement animation type

    void serialize(COutBinStream& out) const {
        out << DX << DY << TX << TY;
        out << static_cast<char>(PreDir.size());
        for (char d : PreDir) out << d;
        out << FaceDir << MoveAction;
    }

    void deserialize(CInBinStream& in) {
        in >> DX >> DY >> TX >> TY;
        char count;
        in >> count;
        PreDir.resize(static_cast<unsigned char>(count));
        for (int i = 0; i < static_cast<unsigned char>(count); ++i) {
            in >> PreDir[i];
        }
        in >> FaceDir >> MoveAction;
    }
};

// ---------------------------------------------------------------------------
// CNC_CZ_ZoneServerReceiveTicket (NCID 29)
// Direction: Client -> ZoneServer
// Auth ticket sent immediately after connecting. 22-byte opaque ticket data.
//
// Decompiled Deserialize @ 0x0066cb80: reads 22 bytes one at a time
// ---------------------------------------------------------------------------
struct CNC_CZ_ZoneServerReceiveTicket {
    static constexpr uint16_t NCID = 29;

    static constexpr int TICKET_SIZE = 22;
    uint8_t Ticket[TICKET_SIZE]{};

    void serialize(COutBinStream& out) const {
        for (int i = 0; i < TICKET_SIZE; ++i)
            out << Ticket[i];
    }

    void deserialize(CInBinStream& in) {
        for (int i = 0; i < TICKET_SIZE; ++i)
            in >> Ticket[i];
    }
};

// ---------------------------------------------------------------------------
// CNC_CZ_ZoneServerPing (NCID 177)
// Direction: Client -> ZoneServer
// Keepalive and latency measurement.
//
// Decompiled Deserialize @ 0x00674220: >> StarterID >> StartTime >> Counter
// ---------------------------------------------------------------------------
struct CNC_CZ_ZoneServerPing {
    static constexpr uint16_t NCID = 177;

    int32_t StarterID{};     // Player who initiated ping
    int32_t StartTime{};     // Timestamp when ping was sent
    int32_t Counter{};       // Incrementing counter

    void serialize(COutBinStream& out) const {
        out << StarterID << StartTime << Counter;
    }

    void deserialize(CInBinStream& in) {
        in >> StarterID >> StartTime >> Counter;
    }
};

// ---------------------------------------------------------------------------
// CNC_CZ_ZoneServerMeleeAttack (NCID 37)
// Direction: Client -> ZoneServer
// Client requests a melee attack on a target.
//
// Decompiled Deserialize @ 0x0066f420: >> TargetType >> TargetID
// ---------------------------------------------------------------------------
struct CNC_CZ_ZoneServerMeleeAttack {
    static constexpr uint16_t NCID = 37;

    int8_t  TargetType{};    // 0=monster, 1=player, 2=NPC, etc.
    int32_t TargetID{};      // Game object ID of the target

    void serialize(COutBinStream& out) const {
        out << TargetType << TargetID;
    }

    void deserialize(CInBinStream& in) {
        in >> TargetType >> TargetID;
    }
};

// ---------------------------------------------------------------------------
// CNC_CZ_ZoneServerChatNormal (NCID 110)
// Direction: Client -> ZoneServer
// Normal chat message (visible to nearby players).
//
// Decompiled Deserialize: >> Message (length-prefixed string)
// ---------------------------------------------------------------------------
struct CNC_CZ_ZoneServerChatNormal {
    static constexpr uint16_t NCID = 110;

    std::string Message;

    void serialize(COutBinStream& out) const {
        out << Message;
    }

    void deserialize(CInBinStream& in) {
        in >> Message;
    }
};

// ---------------------------------------------------------------------------
// CNC_CZ_ClientMoveSelf (NCID 31)
// Direction: ZoneServer -> Client (broadcast to nearby players)
// Server tells all nearby clients about a character's movement.
//
// Wire format includes the moving character's ID so observers know who moved.
// ---------------------------------------------------------------------------
struct CNC_CZ_ClientMoveSelf {
    static constexpr uint16_t NCID = 31;

    int32_t  CharacterID{};     // Who is moving
    uint16_t DX{};
    uint16_t DY{};
    uint16_t TX{};
    uint16_t TY{};
    std::vector<char> PreDir;
    int16_t  FaceDir{};
    int16_t  MoveAction{};

    void serialize(COutBinStream& out) const {
        out << CharacterID << DX << DY << TX << TY;
        out << static_cast<char>(PreDir.size());
        for (char d : PreDir) out << d;
        out << FaceDir << MoveAction;
    }

    void deserialize(CInBinStream& in) {
        in >> CharacterID >> DX >> DY >> TX >> TY;
        char count;
        in >> count;
        PreDir.resize(static_cast<unsigned char>(count));
        for (int i = 0; i < static_cast<unsigned char>(count); ++i) {
            in >> PreDir[i];
        }
        in >> FaceDir >> MoveAction;
    }
};

} // namespace lapis
