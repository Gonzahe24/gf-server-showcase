# =============================================================================
# MMORPG Server - Top-level CMake Configuration
#
# Reconstructed from decompiled x86 ELF binaries using Ghidra.
# Builds 6 server executables and 3 shared libraries from reverse-engineered
# source code. The original build system was GNU Make; this CMake configuration
# was created to provide a modern, modular build.
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(gf-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(PostgreSQL REQUIRED)
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Shared libraries (used by all 6 servers)
# ---------------------------------------------------------------------------

# lapis:: framework - networking (epoll), events, DB (libpq), streams, utilities
add_subdirectory(libs/lapis)

# GameData - pipe-delimited data file loader (items, spells, monsters, maps)
add_subdirectory(libs/gamedata)

# Protocol - 970 CNetCommand structs across 10 command families (CZ, ZW, MW, etc.)
add_subdirectory(libs/protocol)

# ---------------------------------------------------------------------------
# Server executables
# ---------------------------------------------------------------------------

# LoginServer  - Account authentication, server list
add_subdirectory(servers/login)

# TicketServer - Session ticket generation/validation
add_subdirectory(servers/ticket)

# GatewayServer - Client routing, connection handoff between servers
add_subdirectory(servers/gateway)

# WorldServer  - Character management, cross-zone coordination, guild/party
add_subdirectory(servers/world)

# MissionServer - Quest state tracking, completion logic, rewards
add_subdirectory(servers/mission)

# ZoneServer   - Real-time gameplay: movement, combat, spells, NPCs, monsters
add_subdirectory(servers/zone)

# ---------------------------------------------------------------------------
# Tests (optional)
# ---------------------------------------------------------------------------
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
