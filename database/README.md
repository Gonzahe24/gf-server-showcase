# Database Layer

PostgreSQL connection wrapper with SQL injection detection.

## Design

- Direct `libpq` usage (no ORM), matching the original binary's approach
- `ExecTuplesOk()` for SELECT, `ExecCommandOk()` for INSERT/UPDATE/DELETE
- `TestInjection()` checks for semicolons outside quoted strings before execution
- `FixMultibyte()` escapes player-supplied strings (quotes, backslashes)

## Schema

3 PostgreSQL databases, 95 total tables:
- **gf_gs**: Game state (characters, items, equipment, guilds, friends, bank)
- **gf_ls**: Login/account data (accounts, bans, server list)
- **gf_ms**: Mission/quest state (quest progress, completion records)

## Notable Findings

The original developers did NOT use parameterized queries. All SQL is constructed
via string interpolation with `FixMultibyte()` escaping. The `TestInjection()` guard
was their defense against SQL injection -- a simple semicolon detector that tracks
quote state to avoid false positives.

Recovered from decompiled binary's `.rodata` string `"Open Database Failed:"`.
