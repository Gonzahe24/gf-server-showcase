# Command System

Template-based NCID dispatch pattern used for all 970 network commands.

## Pattern

1. Define a command struct with `static constexpr uint16_t NCID`, `serialize()`, `deserialize()`
2. `CNetCommand<T>` wraps it as `INetCommand`, prepending the NCID on the wire
3. `CInterfaceImpl<T>::RegisterCommand<CmdType>("name")` maps NCID to a generator
4. On packet arrival, `GenerateNetCommand()` reads the NCID, finds the generator, deserializes

## Example: Movement (NCID 30)

```cpp
struct CNC_CZ_ZoneServerMoveSelf {
    static constexpr uint16_t NCID = 30;
    uint16_t DX, DY, TX, TY;
    std::vector<char> PreDir;
    int16_t FaceDir, MoveAction;
    void deserialize(CInBinStream& in);
};
```

## Protocol Families

10 families, 970 total commands: CZ (client-zone), ZW (zone-world), MW (mission-world),
CW (client-world), CL (client-login), G (gateway), T (ticket), DBA (database),
ZMis (zone-mission), billing.

Wire format for each command was recovered from decompiled `Serialize`/`Deserialize` at
known offsets (e.g., `0x0066c780` for MoveSelf).
