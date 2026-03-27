# Zone Interface

The client-facing network handler for ZoneServer -- the server responsible for
all real-time gameplay (movement, combat, spells, items, NPCs, monsters).

## Flow

1. Client connects -> `CZoneInterfaceFactory::Create()` makes a new `CZoneInterface`
2. Constructor calls `RegisterCommand<T>()` for all 633 CNC_CZ_ commands
3. Each packet: `GenerateNetCommand()` reads NCID, deserializes -> `DispatchCommand()` routes via switch
4. On disconnect: `OnClose()` saves character, removes from world, notifies other servers

## Per-Session State

Each connected client has its own `CZoneInterface` tracking:
- Account ID, character DB ID, character pointer
- NPC dialog state (which NPC the player is talking to)
- Trade negotiation state (partner, locked, confirmed, gold, 8 item slots)

## Command Categories

Movement, combat, spells, items, trade, quests, guild, party, NPC dialog,
bank/warehouse, duels, elf/pet, auction house, mail, and more.

Recovered from decompiled constructor which registers `CNetCommandGeneratorImpl`
for each command type.
