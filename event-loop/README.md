# Event Loop (CSJFramework)

The Server-Job Framework is the main loop powering every server in the architecture.

## Design

All 6 servers (Login, Ticket, Gateway, World, Mission, Zone) share this loop:

1. `Init()` -- Create event queue, socket manager (epoll fd + RSA keypair)
2. `DeclareTcp()` / `ConnectTcp()` -- Register listeners and outgoing connections
3. `Run()` -- Enter the main loop: dispatch events, epoll for network I/O
4. `Shutdown()` -- Set flag, tear down socket manager

## Epoll Timeout Strategy

The loop dynamically adjusts epoll timeout based on pending events:

- **Immediate events** queued -> `epoll(timeout=0)` (non-blocking, process I/O then dispatch)
- **Real-time event** scheduled -> `epoll(timeout=delta)` (sleep until next timer)
- **No events** -> `epoll(timeout=NULL)` (block until network activity)

Recovered from decompiled `SJFramework.cc` (referenced in log strings).
