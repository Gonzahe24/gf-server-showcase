# About This Repository

[English](#english) | [Español](#español)

## English

### What is this?

This is a **portfolio showcase** of a real MMORPG game server being reconstructed from decompiled binaries using Ghidra. The full project is private and under active development.

### Why a showcase?

The complete codebase (~30,000+ lines, 6 interconnected servers) cannot be made public because:
- It contains proprietary game protocol definitions (970 network commands)
- Publishing the full code would enable unauthorized private servers
- It includes database schemas, game data parsers, and server configurations

Instead, this repository presents **key subsystems and architectural patterns** that demonstrate the reverse engineering and reconstruction process.

### What's real and what's simplified?

- **The architecture is real** — the 6-server design, protocol stack, encryption, and event loop are faithful reconstructions from the original x86 ELF binaries
- **The code is from the actual project** — these are real subsystems extracted and cleaned up for presentation
- **Sensitive data is removed** — no connection strings, server IPs, or complete protocol definitions

### Current state (active development)

The server currently supports:
- Login, character creation/selection
- World exploration, map changes, teleportation
- NPC interaction, shops, dialogue
- Combat (melee, ranged, magic) with damage formulas from the decompiled code
- Buff/enchant system with timed effects
- Guild, friend, party systems
- Player-to-player trade
- Quest system (accept, track, complete, rewards)
- Death, respawn, EXP/leveling
- Monster spawning, AI state machines, drops
- Chat (normal, shout, world, team, guild, whisper)
- Bank/warehouse system

### Reverse engineering process

1. Load original x86 ELF binaries into Ghidra
2. Identify virtual tables and class hierarchies
3. Trace function calls and reconstruct data structures
4. Map decompiled pseudocode to clean C++17
5. Validate by comparing packet output against the original server using Frida captures

### Technologies used in the full project

C++17, CMake, PostgreSQL (libpq, 95 tables across 3 databases), OpenSSL (RSA-2048, RC4, MD5), Linux epoll, POSIX sockets/signals, Ghidra (headless + GUI), Frida (runtime instrumentation), Python 3 (Paramiko for SSH/SFTP deployment, packet analysis)

---

## Español

### ¿Qué es esto?

Este es un **portfolio showcase** de un servidor de MMORPG real que está siendo reconstruido a partir de binarios decompilados usando Ghidra. El proyecto completo es privado y está en desarrollo activo.

### ¿Por qué un showcase?

El código completo (~30,000+ líneas, 6 servidores interconectados) no puede ser público porque:
- Contiene definiciones propietarias del protocolo del juego (970 comandos de red)
- Publicar el código completo permitiría la creación de servidores privados no autorizados
- Incluye esquemas de base de datos, parsers de datos del juego, y configuraciones del servidor

En su lugar, este repositorio presenta **subsistemas clave y patrones arquitectónicos** que demuestran el proceso de ingeniería inversa y reconstrucción.

### ¿Qué es real y qué está simplificado?

- **La arquitectura es real** — el diseño de 6 servidores, el stack de protocolo, la encriptación, y el event loop son reconstrucciones fieles de los binarios ELF x86 originales
- **El código es del proyecto real** — son subsistemas reales extraídos y limpiados para presentación
- **Los datos sensibles fueron eliminados** — sin cadenas de conexión, IPs de servidor, ni definiciones completas del protocolo

### Estado actual (desarrollo activo)

El servidor actualmente soporta:
- Login, creación/selección de personaje
- Exploración del mundo, cambio de mapas, teletransporte
- Interacción con NPCs, tiendas, diálogos
- Combate (melee, rango, magia) con fórmulas de daño del código decompilado
- Sistema de buffs/encantamientos con efectos temporizados
- Sistemas de guild, amigos, party
- Comercio entre jugadores
- Sistema de quests (aceptar, rastrear, completar, recompensas)
- Muerte, respawn, EXP/nivelación
- Spawn de monstruos, máquinas de estado de IA, drops
- Chat (normal, gritar, mundo, equipo, guild, susurro)
- Sistema de banco/almacén

### Proceso de ingeniería inversa

1. Cargar binarios ELF x86 originales en Ghidra
2. Identificar tablas virtuales y jerarquías de clases
3. Rastrear llamadas a funciones y reconstruir estructuras de datos
4. Mapear pseudocódigo decompilado a C++17 limpio
5. Validar comparando la salida de paquetes contra el servidor original usando capturas de Frida

### Tecnologías utilizadas en el proyecto completo

C++17, CMake, PostgreSQL (libpq, 95 tablas en 3 bases de datos), OpenSSL (RSA-2048, RC4, MD5), Linux epoll, sockets/señales POSIX, Ghidra (headless + GUI), Frida (instrumentación runtime), Python 3 (Paramiko para deploy SSH/SFTP, análisis de paquetes)
