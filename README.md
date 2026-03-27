# MMORPG Server Reconstruction from Decompiled Binaries

[English](#english) | [Español](#español)

---

## English

### Overview

Functional MMORPG game server reconstructed entirely through reverse engineering of original x86 ELF binaries using Ghidra. The project consists of **6 interconnected servers** handling the complete gameplay loop, a shared networking/event framework (`lapis::`), and a binary protocol with **970 network commands** across 10 protocol families.

The original server was compiled with GCC 4.4.5 targeting x86 Linux (Debian-era, eglibc-2.11.2). Every struct layout, vtable, serialization format, and state machine was recovered from the decompiled assembly and reconstructed into clean, modern C++17.

### Current State (Active Development)

- Login, character creation and selection
- World exploration, map changes, teleportation
- NPC interaction, shops, dialogue trees
- Combat system (melee, ranged, magic) with damage fórmulas from decompiled code
- Buff/enchant system with timed effects and stat recalculation
- Guild, friend, and party systems
- Player-to-player trade with multi-slot items
- Quest system (accept, track, complete, rewards)
- Death, respawn, EXP and leveling
- Monster spawning, AI state machines (idle, roam, hunt, combat, flee, return), loot drops
- Chat (normal, shout, world, team, guild, whisper)
- Bank/warehouse system
- Duel system

### Architecture

```
Client
  |
  v
LoginServer --- TicketServer --- GatewayServer
                                      |
                              WorldServer --- MissionServer
                                      |
                              ZoneServer (gameplay)
```

- **LoginServer**: Account authentication, server list
- **TicketServer**: Session ticket generation and validation
- **GatewayServer**: Client routing, connection handoff
- **WorldServer**: Character management, cross-zone coordination, guild/party
- **MissionServer**: Quest state tracking, quest completion logic
- **ZoneServer**: Real-time gameplay (movement, combat, spells, NPCs, monsters, items)

All servers share the `lapis::` framework library:
- **Networking**: epoll-based async I/O, RSA-2048 key exchange, RC4 stream cipher
- **Events**: Priority queue with immediate and real-time (timed) events
- **Serialization**: Binary streams with operator overloads, bounds checking, NaN rejection
- **Database**: PostgreSQL wrapper with SQL injection detection
- **Protocol**: 970 `CNetCommand<T>` structs with automatic NCID-based dispatch

### Technologies

| Category | Technology |
|---|---|
| Language | C++17 |
| Build System | CMake 3.16+ |
| Database | PostgreSQL (libpq), 95 tables across 3 databases |
| Encryption | OpenSSL (RSA-2048, RC4, MD5) |
| I/O Model | Linux epoll, non-blocking POSIX sockets |
| Serialization | Custom binary protocol (little-endian, length-prefixed) |
| RE Tool | Ghidra (DWARF symbols partially available) |

### Code Showcase

Each subfolder contains a focused slice of the codebase with explanatory comments:

| Folder | Description |
|---|---|
| [`build-system/`](build-system/) | Top-level CMake configuration for 6 servers + 3 libraries |
| [`network-protocol/`](network-protocol/) | TCP connection with RSA handshake, RC4 encryption, ring buffer I/O |
| [`event-loop/`](event-loop/) | CSJFramework: the epoll-driven main loop powering every server |
| [`serialization/`](serialization/) | Binary stream classes with type-safe operator overloads |
| [`command-system/`](command-system/) | Template-based NCID command dispatch (CNetCommand + CInterfaceImpl) |
| [`zone-interface/`](zone-interface/) | ZoneServer client handler: command registration, dispatch, session state |
| [`database/`](database/) | PostgreSQL wrapper with injection guard and timestamp utilities |

### Reverse Engineering Process

1. **Binary analysis**: Load x86 ELF into Ghidra, identify vtables and class hierarchies
2. **Protocol recovery**: Trace `Serialize`/`Deserialize` functions to recover wire format for each NCID
3. **State machine reconstruction**: Map `switch` statements and function pointer tables to event-driven architecture
4. **Packet capture validation**: Frida hooks on the live client to capture post-decryption packets, compare against reconstruction
5. **Iterative testing**: Debug packet-by-packet using binary search (`debug_pkt_level`) to isolate crashes

---

## Español

### Resumen

Servidor MMORPG funcional reconstruido completamente mediante ingeniería inversa de binarios ELF x86 originales usando Ghidra. El proyecto consta de **6 servidores interconectados** que manejan el ciclo completo de juego, un framework compartido de red/eventos (`lapis::`), y un protocolo binario con **970 comandos de red** en 10 familias de protocolo.

El servidor original fue compilado con GCC 4.4.5 para Linux x86 (era Debian, eglibc-2.11.2). Cada layout de struct, vtable, formato de serialización y máquina de estados fue recuperado del ensamblador descompilado y reconstruido en C++17 moderno y limpio.

### Estado Actual (Desarrollo Activo)

- Login, creación y selección de personajes
- Exploración del mundo, cambio de mapas, teletransporte
- Interacción con NPCs, tiendas, árboles de diálogo
- Sistema de combate (melee, rango, magia) con fórmulas de daño del código descompilado
- Sistema de buffs/encantamientos con efectos temporizados y recálculo de stats
- Sistemas de gremio, amigos y grupo
- Intercambio entre jugadores con items multi-slot
- Sistema de misiones (aceptar, rastrear, completar, recompensas)
- Muerte, respawn, EXP y leveleo
- Spawn de monstruos, máquinas de estado de IA (idle, roam, hunt, combat, flee, return), drops
- Chat (normal, gritar, mundo, equipo, gremio, susurro)
- Sistema de banco/almacén
- Sistema de duelos

### Arquitectura

```
Cliente
  |
  v
LoginServer --- TicketServer --- GatewayServer
                                      |
                              WorldServer --- MissionServer
                                      |
                              ZoneServer (gameplay)
```

- **LoginServer**: Autenticación de cuentas, lista de servidores
- **TicketServer**: Generación y validación de tickets de sesión
- **GatewayServer**: Enrutamiento de clientes, traspaso de conexión
- **WorldServer**: Gestión de personajes, coordinación entre zonas, gremio/grupo
- **MissionServer**: Rastreo de estado de misiones, lógica de completado
- **ZoneServer**: Gameplay en tiempo real (movimiento, combate, hechizos, NPCs, monstruos, items)

Todos los servidores comparten la librería framework `lapis::`:
- **Red**: I/O asíncrono basado en epoll, intercambio de claves RSA-2048, cifrado de flujo RC4
- **Eventos**: Cola de prioridad con eventos inmediatos y en tiempo real (temporizados)
- **Serialización**: Streams binarios con sobrecarga de operadores, verificación de límites, rechazo de NaN
- **Base de datos**: Wrapper PostgreSQL con detección de inyección SQL
- **Protocolo**: 970 structs `CNetCommand<T>` con despacho automático basado en NCID

### Tecnologías

| Categoría | Tecnología |
|---|---|
| Lenguaje | C++17 |
| Build System | CMake 3.16+ |
| Base de Datos | PostgreSQL (libpq), 95 tablas en 3 bases de datos |
| Encriptación | OpenSSL (RSA-2048, RC4, MD5) |
| Modelo I/O | Linux epoll, sockets POSIX no bloqueantes |
| Serialización | Protocolo binario custom (little-endian, prefijo de longitud) |
| Herramienta RE | Ghidra (símbolos DWARF parcialmente disponibles) |

### Proceso de Ingeniería Inversa

1. **Análisis binario**: Cargar ELF x86 en Ghidra, identificar vtables y jerarquías de clases
2. **Recuperación de protocolo**: Trazar funciones `Serialize`/`Deserialize` para recuperar formato wire de cada NCID
3. **Reconstrucción de máquinas de estado**: Mapear sentencias `switch` y tablas de punteros a funciones a arquitectura event-driven
4. **Validación por captura de paquetes**: Hooks Frida en el cliente en vivo para capturar paquetes post-descifrado, comparar contra reconstrucción
5. **Testing iterativo**: Debug paquete por paquete usando búsqueda binaria (`debug_pkt_level`) para aislar crashes
