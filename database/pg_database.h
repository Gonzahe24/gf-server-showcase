// =============================================================================
// CPGDatabase - PostgreSQL database connection wrapper
//
// Reverse-engineered from decompiled x86 ELF binary using Ghidra.
// Original DWARF layout recovered:
//   _vptr_CPGDatabase : vtable pointer
//   DBConnection      : PGconn*
//   LastResult        : AutoPtr<CPGDBResult>
//   ErrorMsg          : std::string
//
// Key behaviors discovered through decompilation:
//   - Constructor builds conninfo string and calls PQconnectdb(), exits on failure
//   - ExecTuplesOk/ExecCommandOk both run TestInjection first
//   - TestInjection detects semicolons outside quotes followed by non-space chars
//   - FixMultibyte escapes single quotes and backslashes for SQL safety
//   - Timestamp conversion matches PostgreSQL 'YYYY-MM-DD HH:MM:SS' format
//
// The server uses 3 PostgreSQL databases with 95 total tables:
//   gf_gs  - Game state (characters, items, guilds, etc.)
//   gf_ls  - Login/account data
//   gf_ms  - Mission/quest state
// =============================================================================

#pragma once

#include <libpq-fe.h>
#include <string>
#include <memory>

namespace lapis {

class CPGDBResult;

class CPGDatabase {
public:
    // Constructor: connects to PostgreSQL using conninfo string format:
    //   "hostaddr=<ip> user=<user> password='<pw>' dbname=<db>"
    // Exits the process on connection failure (matches original behavior).
    CPGDatabase(const std::string& hostaddr,
                const std::string& user,
                const std::string& password,
                const std::string& database);
    virtual ~CPGDatabase();

    // Non-copyable (connection handle is not shareable)
    CPGDatabase(const CPGDatabase&) = delete;
    CPGDatabase& operator=(const CPGDatabase&) = delete;

    // --- Connection status ---
    bool ConnectionOk() const;
    bool ConnectionBad() const;

    // --- Query execution ---
    // ExecTuplesOk: for SELECT queries. Returns true if PGRES_TUPLES_OK.
    bool ExecTuplesOk(const char* query);

    // ExecCommandOk: for INSERT/UPDATE/DELETE. Returns true if not PGRES_FATAL_ERROR.
    // If check==true, runs SQL injection guard first.
    bool ExecCommandOk(const char* cmd, bool check = true);

    // --- Result access ---
    CPGDBResult* GetLastResult() const;

    // --- Error info ---
    const std::string& GetErrorMsg() const;

    // --- SQL injection guard ---
    // Detects semicolons outside quoted strings followed by non-whitespace.
    // Returns true if injection is suspected.
    bool TestInjection(const char* str) const;

    // --- String utilities ---
    // Escapes single quotes and backslashes for safe SQL string interpolation.
    std::string FixMultibyte(const std::string& orgString, size_t maxSize) const;

    // --- Timestamp conversions ---
    // Converts Unix epoch to PostgreSQL timestamp literal 'YYYY-MM-DD HH:MM:SS'
    static std::string Long2TimeStamp(int time);

    // Parses PostgreSQL timestamp string back to Unix epoch
    static int TimeStamp2Long(const std::string& buf);

protected:
    PGconn*       m_dbConnection;
    CPGDBResult*  m_lastResult;
    std::string   m_errorMsg;
};

} // namespace lapis
