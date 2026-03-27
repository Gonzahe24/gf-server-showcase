// =============================================================================
// CNetCommand<T> + CInterfaceImpl<T> - Template command dispatch system
//
// Reverse-engineered from decompiled x86 ELF binary using Ghidra.
//
// This is the core pattern that drives all 970 network commands:
//
//   1. Each command is a plain struct with NCID, serialize(), deserialize()
//   2. CNetCommand<T> wraps it as an INetCommand (adds NCID header to wire format)
//   3. CNetCommandGeneratorImpl<T> creates commands from incoming streams
//   4. CInterfaceImpl<T>::RegisterCommand<CmdType>() maps NCID -> generator
//   5. On packet arrival, GenerateNetCommand() reads NCID, finds generator, deserializes
//
// This pattern was identified by tracing vtable layouts in Ghidra:
//   INetCommand vtable: ~dtor, _Serialize, Deserialize, GetNCID, ConvertToEvent
//   IInterface vtable: ~dtor, ComposeOutput, ResolveInput, GenerateNetCommand, ...
// =============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <cstdio>

namespace lapis {

class CInBinStream;
class COutBinStream;
class CEvent;

// ---------------------------------------------------------------------------
// INetCommand: abstract base for all network commands.
// Vtable recovered from decompiled binary.
// ---------------------------------------------------------------------------
class INetCommand {
public:
    virtual ~INetCommand() = default;

    // Serialize: writes NCID (uint16) + payload to output stream
    virtual void _Serialize(COutBinStream& stream) const = 0;

    // Deserialize: reads payload from input stream (NCID already consumed)
    virtual bool Deserialize(CInBinStream& stream) = 0;

    // Get the Network Command ID for this command type
    virtual uint16_t GetNCID() const = 0;

    // Convert to a framework event for dispatch (optional)
    virtual CEvent* ConvertToEvent() { return nullptr; }
};

// ---------------------------------------------------------------------------
// CNetCommand<T>: wraps a command struct T as an INetCommand.
//
// T must provide:
//   static constexpr uint16_t NCID
//   void serialize(COutBinStream&) const
//   void deserialize(CInBinStream&)
//
// From decompiled: _Serialize writes NCID first via operator<<, then calls
// T::serialize(). GetNCID returns T::NCID.
// ---------------------------------------------------------------------------
template <typename T>
class CNetCommand : public T, public INetCommand {
public:
    ~CNetCommand() override = default;

    void _Serialize(COutBinStream& stream) const override {
        stream << T::NCID;       // 2-byte NCID header
        T::serialize(stream);    // Type-specific payload
    }

    bool Deserialize(CInBinStream& stream) override {
        T::deserialize(stream);  // NCID already consumed by dispatch
        return true;
    }

    uint16_t GetNCID() const override {
        return T::NCID;
    }
};

// ---------------------------------------------------------------------------
// INetCommandGenerator: factory interface for creating commands from streams.
// ---------------------------------------------------------------------------
class INetCommandGenerator {
public:
    virtual ~INetCommandGenerator() = default;
    virtual INetCommand* operator()(CInBinStream& stream) = 0;
};

// ---------------------------------------------------------------------------
// CNetCommandGeneratorImpl<T>: allocates CNetCommand<T> and deserializes.
// From decompiled: allocates, calls Deserialize, returns pointer.
// ---------------------------------------------------------------------------
template <typename T>
class CNetCommandGeneratorImpl : public INetCommandGenerator {
public:
    INetCommand* operator()(CInBinStream& stream) override {
        auto* cmd = new CNetCommand<T>();
        if (!cmd->Deserialize(stream)) {
            delete cmd;
            return nullptr;
        }
        return cmd;
    }
};

// ---------------------------------------------------------------------------
// CInterfaceImpl<T>: template interface with NCID dispatch table.
//
// Each server interface (CZoneInterface, CWorldInterface, etc.) inherits
// this and calls RegisterCommand<CmdType>("name") in its constructor to
// populate the dispatch table.
//
// On packet arrival:
//   1. GenerateNetCommand() reads uint16 NCID from stream
//   2. Looks up NCID in m_generators map
//   3. Calls the generator to create and deserialize the command
//   4. Returns the command for event dispatch
// ---------------------------------------------------------------------------
template <typename T>
class CInterfaceImpl {
public:
    INetCommand* GenerateNetCommand(CInBinStream& stream) {
        uint16_t ncid = 0;
        stream >> ncid;

        auto it = m_generators.find(ncid);
        if (it == m_generators.end()) {
            fprintf(stderr, "[NET] GenerateNetCommand: NCID %u NOT FOUND in %zu generators\n",
                    ncid, m_generators.size());
            return nullptr;
        }

        try {
            return (*it->second)(stream);
        } catch (const std::underflow_error& e) {
            auto nameIt = m_commandNames.find(ncid);
            std::string name = (nameIt != m_commandNames.end())
                             ? nameIt->second : "UNKNOWN";
            fprintf(stderr, "[NET] DESERIALIZE ERROR: NCID=%u (%s) - %s\n",
                    ncid, name.c_str(), e.what());
            return nullptr;
        }
    }

    std::string GetNetCommandName(uint16_t ncid) const {
        auto it = m_commandNames.find(ncid);
        return (it != m_commandNames.end()) ? it->second : "UNKNOWN";
    }

protected:
    // Register a command type with its NCID and human-readable name
    template <typename CmdType>
    void RegisterCommand(const std::string& name) {
        uint16_t ncid = CmdType::NCID;
        m_generators[ncid] = std::make_unique<CNetCommandGeneratorImpl<CmdType>>();
        m_commandNames[ncid] = name;
    }

    std::unordered_map<uint16_t, std::unique_ptr<INetCommandGenerator>> m_generators;
    std::unordered_map<uint16_t, std::string> m_commandNames;
};

} // namespace lapis
