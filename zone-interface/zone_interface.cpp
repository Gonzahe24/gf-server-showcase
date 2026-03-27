// =============================================================================
// CZoneInterface implementation
//
// Reverse-engineered from decompiled ZoneServer binary using Ghidra.
//
// Constructor: registers command generators for all CNC_CZ_ commands the
// client can send. Each RegisterCommand<T>() call populates the NCID -> generator
// dispatch table inherited from CInterfaceImpl<CZoneInterface>.
//
// DispatchCommand: large switch on NCID, dispatching to handler functions.
// In the original binary, each command had its own CNE_CZ_ event class;
// our reconstruction consolidates all dispatch into a single switch.
//
// OnClose: saves player state, removes from spatial node, notifies other servers.
// =============================================================================

#include "zone_interface.h"
#include <cstdio>
#include <ctime>

// Forward-declare example command structs (defined in commands_example.h)
namespace lapis {
    struct CNC_CZ_ZoneServerMoveSelf;
    struct CNC_CZ_ZoneServerReceiveTicket;
    struct CNC_CZ_ZoneServerPing;
    struct CNC_CZ_ZoneServerMeleeAttack;
    struct CNC_CZ_ZoneServerChatNormal;
}

// ===========================================================================
// Constructor: register all command generators
// ===========================================================================

CZoneInterface::CZoneInterface(lapis::CSession* session, CZoneServer* server)
    : m_server(server)
{
    (void)session;

    // --- Session commands ---
    RegisterCommand<lapis::CNC_CZ_ZoneServerReceiveTicket>("ZoneServerReceiveTicket");
    RegisterCommand<lapis::CNC_CZ_ZoneServerPing>("ZoneServerPing");

    // --- Movement commands ---
    RegisterCommand<lapis::CNC_CZ_ZoneServerMoveSelf>("ZoneServerMoveSelf");

    // --- Combat commands ---
    RegisterCommand<lapis::CNC_CZ_ZoneServerMeleeAttack>("ZoneServerMeleeAttack");

    // --- Chat commands ---
    RegisterCommand<lapis::CNC_CZ_ZoneServerChatNormal>("ZoneServerChatNormal");

    // ... (633 total CNC_CZ_ commands registered in the full implementation)
    // Categories include: movement, combat, spells, items, trade, quest,
    // guild, party, NPC dialog, bank, duel, elf/pet, auction house, mail, etc.

    fprintf(stderr, "[ZONE] CZoneInterface created with %zu registered commands\n",
            m_generators.size());
}

CZoneInterface::~CZoneInterface() {
    m_character = nullptr;
    m_server = nullptr;
}

// ===========================================================================
// OnClose: client disconnected - clean up game state
// ===========================================================================

void CZoneInterface::OnClose() {
    fprintf(stderr, "[ZONE] OnClose: accountID=%d charDBID=%d\n",
            m_accountId, m_characterDBID);

    if (m_character) {
        // From decompiled OnClose logic:
        // 1. Cancel active trade if any
        if (IsTrading()) {
            // Notify trade partner, revert items
            ResetTradeState();
        }

        // 2. Cancel active duel if any
        // CDuelFactory::Instance->CancelDuel(m_character);

        // 3. Remove from party/team
        // CTeamFactory::Instance->RemoveMember(m_character);

        // 4. Save character to database
        // Serializes position, HP, MP, EXP, inventory, equipment, quests
        // to PostgreSQL via CPGDatabase::ExecCommandOk()
        fprintf(stderr, "[ZONE] Saving character %d to database...\n", m_characterDBID);

        // 5. Remove from spatial node (stops broadcasting to/from this player)
        // CNode::Remove(m_character);

        // 6. Notify WorldServer of logout
        // CWorldZoneClient::Instance->SendPlayerLogout(m_sessionId);

        // 7. Notify MissionServer of logout
        // CMissionZoneService::Instance->SendPlayerLogout(m_sessionId);

        // 8. Return character to factory pool
        // CPlayerFactory::Instance->DestroyPlayer(m_character);
        m_character = nullptr;
    }
}

// ===========================================================================
// DispatchCommand: NCID-based command routing
// ===========================================================================

void CZoneInterface::DispatchCommand(lapis::INetCommand* netCommand) {
    if (!netCommand) return;

    uint16_t ncid = netCommand->GetNCID();

    // Movement packets (NCID 30) are very frequent -- skip logging for them
    if (ncid != 30) {
        fprintf(stderr, "[ZONE-DISPATCH] NCID=%u (0x%04X) name=%s\n",
                ncid, ncid, GetNetCommandName(ncid).c_str());
    }

    switch (ncid) {

    // --- Session ---
    case 29: {  // ZoneServerReceiveTicket
        // Validate the 22-byte auth ticket against TicketServer
        // On success: load character from DB, place in world, send zone entry packets
        fprintf(stderr, "[ZONE] Received auth ticket, validating...\n");
        break;
    }

    case 177: {  // ZoneServerPing
        // Echo back the ping with server timestamp for latency measurement
        break;
    }

    // --- Movement ---
    case 30: {  // ZoneServerMoveSelf
        // Update character position, broadcast to nearby players via CNode
        // Validates movement speed against server-side limits (anti-cheat)
        break;
    }

    // --- Combat ---
    case 37: {  // ZoneServerMeleeAttack
        // Validate target exists and is in range
        // Calculate damage using CCharFormula (decompiled damage formulas)
        // Apply damage, check for death, award EXP if kill
        // Broadcast damage packet to nearby players
        break;
    }

    // --- Chat ---
    case 110: {  // ZoneServerChatNormal
        // Broadcast chat message to all players in the same spatial node
        break;
    }

    // ... (dispatch cases for all 633 CNC_CZ_ commands)
    // Full implementation includes: spell casting, item use, trade,
    // quest accept/complete, guild operations, party management,
    // NPC dialog, bank deposit/withdraw, duel request/accept,
    // elf/pet commands, auction house, mail system, etc.

    default:
        fprintf(stderr, "[ZONE] Unhandled NCID=%u\n", ncid);
        break;
    }

    delete netCommand;
}
