/**
 * buff_system.cpp - Simplified buff/enchant system
 *
 * Reverse-engineered from Ghidra decompilation of the original game server.
 * Shows the stacking logic, command-based stat modification, and tick system
 * exactly as recovered from the original binary's enchant handler.
 *
 * This is a SIMPLIFIED educational version for portfolio purposes.
 * Real memory addresses and proprietary data have been removed.
 */

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace lapis {

struct BuffEntry {
    uint32_t id;            // unique instance ID
    uint32_t effectId;      // spell effect type
    int32_t  level;         // buff level (for stacking comparison)
    int32_t  remainTime;    // in tenths of seconds (280 = 28 real seconds)
    uint8_t  enchantTimes;  // how many times restacked
    uint16_t stackCount;    // current stack count
    uint32_t flags;
};

/// Stat modification command — executed on apply, rolled back on removal.
/// This pattern ensures clean state even if the server crashes mid-buff.
struct StatCommand {
    uint32_t attrId;
    int32_t  delta;   // positive = buff, negative = debuff
};

class CBuffManager {
    // Dual-map indexing: find buffs by unique ID or by effect type
    std::unordered_map<uint32_t, BuffEntry> m_byId;
    std::unordered_map<uint32_t, uint32_t>  m_byEffect;  // effectId -> id

    // Active stat modifications per buff ID, for rollback
    std::unordered_map<uint32_t, std::vector<StatCommand>> m_commands;

public:
    /// Apply a buff with restack logic (recovered from decompiled enchant handler).
    /// Returns true if the buff was applied or restacked.
    bool AddBuff(BuffEntry newBuff, const std::vector<StatCommand>& commands,
                 /* character ptr for applying stat deltas */ void* character) {

        auto it = m_byEffect.find(newBuff.effectId);
        if (it != m_byEffect.end()) {
            auto& existing = m_byId[it->second];

            if (newBuff.level == existing.level) {
                // Same level: reset duration, increment restack counter
                existing.remainTime = newBuff.remainTime;
                existing.enchantTimes++;
                return true;
            }
            if (newBuff.level < existing.level) {
                // Higher-level buff already active: reject the weaker one
                return false;
            }
            // New buff is stronger: remove old, then fall through to apply new
            RemoveBuff(existing.id, character);
        }

        // Execute stat modification commands (the "command pattern")
        m_commands[newBuff.id] = commands;
        for (auto& cmd : commands) {
            ApplyStatDelta(character, cmd.attrId, cmd.delta);
        }

        m_byId[newBuff.id] = newBuff;
        m_byEffect[newBuff.effectId] = newBuff.id;
        return true;
    }

    /// Remove a buff and roll back its stat modifications.
    void RemoveBuff(uint32_t buffId, void* character) {
        auto it = m_byId.find(buffId);
        if (it == m_byId.end()) return;

        // Rollback: apply inverse of each stat command
        auto cmdIt = m_commands.find(buffId);
        if (cmdIt != m_commands.end()) {
            for (auto& cmd : cmdIt->second) {
                ApplyStatDelta(character, cmd.attrId, -cmd.delta);
            }
            m_commands.erase(cmdIt);
        }

        m_byEffect.erase(it->second.effectId);
        m_byId.erase(it);
    }

    /// Tick all active buffs. Called once per real second.
    /// Durations are in tenths of seconds, so we decrement by 10 each tick.
    void TickAll(void* character) {
        std::vector<uint32_t> expired;
        for (auto& [id, buff] : m_byId) {
            buff.remainTime -= 10;
            if (buff.remainTime <= 0) {
                expired.push_back(id);
            }
        }
        for (uint32_t id : expired) {
            RemoveBuff(id, character);
            // Notify client: send ClientEnchantRemove packet
        }
    }

private:
    /// Apply a stat delta to the character (simplified placeholder).
    void ApplyStatDelta(void* character, uint32_t attrId, int32_t delta) {
        // In the real implementation, this modifies the character's stat array
        // and triggers a NotifyBuffStatsChanged() -> AttrRefresh broadcast
        (void)character; (void)attrId; (void)delta;
    }
};

} // namespace lapis
