// =============================================================================
// CZoneInterface - Client-facing network interface for ZoneServer
//
// Reverse-engineered from decompiled ZoneServer binary using Ghidra.
// Original class: lapis::CZoneInterface
//
// This is the server-side handler for each connected client. When a player
// connects to the zone server:
//   1. CZoneInterfaceFactory::Create() makes a new CZoneInterface
//   2. Constructor registers CNetCommandGeneratorImpl for every CNC_CZ_ command
//   3. Incoming packets are dispatched via CreateCommandEvent() -> NCID switch
//   4. OnClose() handles disconnection (save player, remove from world)
//
// The interface also tracks per-session state: account ID, character pointer,
// NPC dialog state, and trade negotiation state.
// =============================================================================

#pragma once

#include "net_command.h"  // CInterfaceImpl<T>, CNetCommand<T>
#include <cstdint>
#include <string>

namespace lapis {

class CSession;
class CCharacter;

} // namespace lapis

class CZoneServer;

class CZoneInterface : public lapis::CInterfaceImpl<CZoneInterface> {
public:
    CZoneInterface(lapis::CSession* session, CZoneServer* server);
    ~CZoneInterface();

    // Called when client connection closes.
    // From decompiled: saves player state to DB, removes from spatial node,
    // notifies WorldServer and MissionServer of logout.
    void OnClose();

    // Dispatch deserialized CNC_CZ_ commands to handler functions.
    // Uses a large switch on NCID (original binary had one CNE_CZ_ event
    // class per command; our reconstruction consolidates into a single switch).
    void DispatchCommand(lapis::INetCommand* netCommand);

    // --- Accessors ---
    CZoneServer* GetServer() const { return m_server; }

    // Session-bound player data (set after ticket validation)
    uint32_t GetSessionID() const { return m_sessionId; }
    void SetSessionID(uint32_t id) { m_sessionId = id; }

    int32_t GetAccountID() const { return m_accountId; }
    void SetAccountID(int32_t id) { m_accountId = id; }

    int32_t GetCharacterDBID() const { return m_characterDBID; }
    void SetCharacterDBID(int32_t id) { m_characterDBID = id; }

    lapis::CCharacter* GetCharacter() const { return m_character; }
    void SetCharacter(lapis::CCharacter* ch) { m_character = ch; }

    bool IsLoggedIn() const { return m_character != nullptr; }

    // NPC dialog state (tracked per-interface in decompiled code)
    int32_t GetNPCTalkID() const { return m_npcTalkID; }
    void SetNPCTalkID(int32_t id) { m_npcTalkID = id; }

    // --- Trade system state (from decompiled STradeData) ---
    static constexpr int TRADE_SLOTS = 8;

    int32_t GetTradePartnerID() const { return m_tradePartnerID; }
    void SetTradePartnerID(int32_t id) { m_tradePartnerID = id; }
    bool IsTrading() const { return m_tradePartnerID != 0; }

    bool IsTradeLocked() const { return m_tradeLocked; }
    void SetTradeLocked(bool v) { m_tradeLocked = v; }

    bool IsTradeConfirmed() const { return m_tradeConfirmed; }
    void SetTradeConfirmed(bool v) { m_tradeConfirmed = v; }

    int32_t GetTradeGold() const { return m_tradeGold; }
    void SetTradeGold(int32_t g) { m_tradeGold = g; }

    struct TradeSlotEntry {
        int8_t containerType = -1;
        int16_t inventoryIndex = 0;
        int16_t inventoryLoc = -1;
        int16_t number = 0;
        bool occupied = false;
    };
    TradeSlotEntry& GetTradeSlot(int i) { return m_tradeSlots[i]; }

    void ResetTradeState() {
        m_tradePartnerID = 0;
        m_tradeLocked = false;
        m_tradeConfirmed = false;
        m_tradeGold = 0;
        for (int i = 0; i < TRADE_SLOTS; ++i)
            m_tradeSlots[i] = TradeSlotEntry{};
    }

private:
    CZoneServer* m_server;
    uint32_t m_sessionId = 0;
    int32_t m_accountId = 0;
    int32_t m_characterDBID = 0;
    lapis::CCharacter* m_character = nullptr;
    int32_t m_npcTalkID = 0;
    int32_t m_tradePartnerID = 0;
    bool m_tradeLocked = false;
    bool m_tradeConfirmed = false;
    int32_t m_tradeGold = 0;
    TradeSlotEntry m_tradeSlots[TRADE_SLOTS] = {};
};

// Factory: creates CZoneInterface instances for new client connections.
// From decompiled: CMSTCPConnection::Open calls InterfaceFactory->Create(session)
class CZoneInterfaceFactory {
public:
    explicit CZoneInterfaceFactory(CZoneServer* server) : m_server(server) {}

    CZoneInterface* Create(lapis::CSession* session) {
        return new CZoneInterface(session, m_server);
    }

private:
    CZoneServer* m_server;
};
