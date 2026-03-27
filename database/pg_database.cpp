// =============================================================================
// CPGDatabase implementation
//
// Reverse-engineered from decompiled x86 ELF binary using Ghidra.
// Original error string: "Open Database Failed:" (found in binary's .rodata)
//
// The SQL injection guard (TestInjection) is a notable reverse-engineering find:
// the original developers implemented a simple but effective check that detects
// attempts to append additional SQL statements via semicolons. It tracks
// quote state to avoid false positives on semicolons inside string literals.
//
// FixMultibyte was used for escaping player-supplied strings (names, chat, etc.)
// before SQL interpolation. The original code did not use parameterized queries.
// =============================================================================

#include "pg_database.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>

namespace lapis {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

CPGDatabase::CPGDatabase(const std::string& hostaddr,
                         const std::string& user,
                         const std::string& password,
                         const std::string& database)
    : m_dbConnection(nullptr)
    , m_lastResult(nullptr)
{
    // Build conninfo exactly as the original binary does
    std::string conninfo = "hostaddr=" + hostaddr
                         + " user="    + user
                         + " password='" + password + "'"
                         + " dbname="  + database;

    m_dbConnection = PQconnectdb(conninfo.c_str());
    if (!m_dbConnection || PQstatus(m_dbConnection) != CONNECTION_OK) {
        // Original binary prints this exact string and calls exit(1)
        std::fprintf(stderr, "Open Database Failed:%s\n", conninfo.c_str());
        std::exit(1);
    }
}

CPGDatabase::~CPGDatabase() {
    if (m_dbConnection) {
        PQfinish(m_dbConnection);
        m_dbConnection = nullptr;
    }
    m_errorMsg.clear();
    delete m_lastResult;
    m_lastResult = nullptr;
}

// ---------------------------------------------------------------------------
// Connection status
// ---------------------------------------------------------------------------

bool CPGDatabase::ConnectionOk() const {
    return PQstatus(m_dbConnection) == CONNECTION_OK;
}

bool CPGDatabase::ConnectionBad() const {
    return PQstatus(m_dbConnection) != CONNECTION_OK;
}

// ---------------------------------------------------------------------------
// SQL injection guard
//
// Recovered from decompiled code. Tracks quote state and detects semicolons
// outside of string literals. If a non-space character follows a semicolon
// (indicating a second SQL statement), returns true (injection detected).
//
// Example:
//   "SELECT * FROM players WHERE name='test'"  -> false (safe)
//   "SELECT * FROM players; DROP TABLE players" -> true  (injection!)
//   "SELECT * FROM players WHERE msg='a;b'"     -> false (semicolon in quotes)
// ---------------------------------------------------------------------------

bool CPGDatabase::TestInjection(const char* str) const {
    if (!str) return false;

    bool inQuote = false;
    bool afterSemicolon = false;

    for (const char* p = str; *p != '\0'; ++p) {
        if (afterSemicolon) {
            if (*p != ' ') {
                return true;  // Non-space after semicolon = injection
            }
            continue;
        }

        if (*p == '\\') {
            ++p;  // Skip escaped character
            if (*p == '\0') break;
        } else if (*p == '\'') {
            inQuote = !inQuote;
        } else if (*p == ';' && !inQuote) {
            afterSemicolon = true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Query execution
// ---------------------------------------------------------------------------

bool CPGDatabase::ExecTuplesOk(const char* query) {
    // Injection check before execution
    if (TestInjection(query)) {
        return false;
    }

    PGresult* pgRes = PQexec(m_dbConnection, query);
    if (!pgRes) {
        const char* err = PQerrorMessage(m_dbConnection);
        if (err) m_errorMsg = err;
        return false;
    }

    // Replace previous result
    delete m_lastResult;
    m_lastResult = new CPGDBResult(pgRes);

    ExecStatusType status = PQresultStatus(pgRes);
    if (status == PGRES_FATAL_ERROR) {
        const char* err = PQresultErrorMessage(pgRes);
        if (err) m_errorMsg = err;
        return false;
    }

    return true;
}

bool CPGDatabase::ExecCommandOk(const char* cmd, bool check) {
    // Injection check only when check flag is true
    if (check && TestInjection(cmd)) {
        return false;
    }

    PGresult* pgRes = PQexec(m_dbConnection, cmd);
    if (!pgRes) {
        const char* err = PQerrorMessage(m_dbConnection);
        if (err) m_errorMsg = err;
        return false;
    }

    delete m_lastResult;
    m_lastResult = new CPGDBResult(pgRes);

    ExecStatusType status = PQresultStatus(pgRes);
    if (status == PGRES_FATAL_ERROR) {
        const char* err = PQresultErrorMessage(pgRes);
        if (err) m_errorMsg = err;
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Result / Error access
// ---------------------------------------------------------------------------

CPGDBResult* CPGDatabase::GetLastResult() const {
    return m_lastResult;
}

const std::string& CPGDatabase::GetErrorMsg() const {
    return m_errorMsg;
}

// ---------------------------------------------------------------------------
// FixMultibyte: escape strings for SQL interpolation
//
// Doubles single quotes and backslashes. Used for all player-supplied strings
// (character names, chat messages, guild names, etc.) before SQL construction.
// The original code does NOT use parameterized queries.
// ---------------------------------------------------------------------------

std::string CPGDatabase::FixMultibyte(const std::string& orgString, size_t maxSize) const {
    std::string result;
    result.reserve(orgString.size() * 2);

    for (size_t i = 0; i < orgString.size(); ++i) {
        char c = orgString[i];
        if (c == '\'') {
            result += "''";       // Escape single quote
        } else if (c == '\\') {
            result += "\\\\";     // Escape backslash
        } else {
            result += c;
        }

        if (i >= maxSize && (i + 1) < orgString.size()) {
            result += "...";
            break;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Timestamp conversions
// Format: 'YYYY-MM-DD HH:MM:SS' (PostgreSQL timestamp literal)
// ---------------------------------------------------------------------------

std::string CPGDatabase::Long2TimeStamp(int time) {
    time_t t = static_cast<time_t>(time);
    struct tm* tm = localtime(&t);
    if (!tm) return "";

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                  tm->tm_hour, tm->tm_min, tm->tm_sec);
    return std::string(buf);
}

int CPGDatabase::TimeStamp2Long(const std::string& buf) {
    struct tm tm = {};
    std::istringstream iss(buf);

    char sep;
    iss >> tm.tm_year >> sep >> tm.tm_mon >> sep >> tm.tm_mday
        >> tm.tm_hour >> sep >> tm.tm_min >> sep >> tm.tm_sec;
    tm.tm_year -= 1900;
    tm.tm_mon  -= 1;

    return static_cast<int>(mktime(&tm));
}

} // namespace lapis
