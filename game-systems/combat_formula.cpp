/**
 * combat_formula.cpp - Simplified damage calculation system
 *
 * Reverse-engineered from Ghidra decompilation of the original game server.
 * Every formula and constant was extracted by tracing the original x86 assembly.
 *
 * This is a SIMPLIFIED educational version for portfolio purposes.
 * Real memory addresses, proprietary constants, and server-specific
 * details have been removed or replaced with placeholders.
 */

#include <cstdint>
#include <algorithm>
#include <cmath>

namespace lapis {

struct CombatStats {
    int32_t strength;
    int32_t agility;
    int32_t intelligence;
    int32_t level;
    int32_t weaponDamageMin;
    int32_t weaponDamageMax;
    int32_t offhandDamageMin;   // dual-wield support
    int32_t offhandDamageMax;
    int32_t equipmentBonus;
    float   buffDamageMultiplier;  // from active buffs, e.g. 1.15 = +15%
    int32_t currentHP;
    int32_t maxHP;
    int32_t currentMP;
    int32_t maxMP;
    bool    hpConversionActive;    // skill that converts HP to damage
    bool    mpConversionActive;    // skill that converts MP to damage
    float   conversionRate;        // percentage of HP/MP converted
};

/// Calculate base physical damage from STR stat.
/// Formula recovered from decompiled melee attack handler.
int32_t CalcBaseDamage(const CombatStats& stats) {
    // STR contributes linearly with a level-scaled floor
    int32_t base = stats.strength * 2 + stats.level * 3;
    // Agility adds a small bonus (found in secondary branch of decompiled code)
    base += stats.agility / 4;
    return std::max(base, 1);
}

/// Calculate weapon contribution including dual-wield.
/// The offhand deals reduced damage (coefficient found in constant pool).
int32_t CalcWeaponDamage(const CombatStats& stats) {
    int32_t mainhand = (stats.weaponDamageMin + stats.weaponDamageMax) / 2;
    int32_t offhand  = 0;
    if (stats.offhandDamageMax > 0) {
        // Offhand penalty: 60% effectiveness (recovered from decompiled dual-wield path)
        offhand = ((stats.offhandDamageMin + stats.offhandDamageMax) / 2) * 60 / 100;
    }
    return mainhand + offhand;
}

/// Apply HP/MP to damage conversion skills.
/// These drain a percentage of current HP or MP and add it as flat damage.
int32_t CalcConversionBonus(const CombatStats& stats) {
    int32_t bonus = 0;
    if (stats.hpConversionActive) {
        bonus += static_cast<int32_t>(stats.currentHP * stats.conversionRate);
    }
    if (stats.mpConversionActive) {
        bonus += static_cast<int32_t>(stats.currentMP * stats.conversionRate);
    }
    return bonus;
}

/// Full damage calculation pipeline - layered exactly as found in decompiled code.
/// Each layer is a separate additive or multiplicative step.
int32_t CalculateDamage(const CombatStats& attacker) {
    // Layer 1: Base stat damage
    int32_t damage = CalcBaseDamage(attacker);

    // Layer 2: Weapon damage (additive)
    damage += CalcWeaponDamage(attacker);

    // Layer 3: Equipment flat bonus (additive)
    damage += attacker.equipmentBonus;

    // Layer 4: Buff multiplier (multiplicative)
    damage = static_cast<int32_t>(damage * attacker.buffDamageMultiplier);

    // Layer 5: HP/MP conversion bonus (additive, after multiplier)
    damage += CalcConversionBonus(attacker);

    return std::max(damage, 1);  // minimum 1 damage
}

/// EXP decline rate formula - reduces EXP gained from monsters
/// significantly below the player's level. Recovered from the
/// monster-kill reward handler in the decompiled zone server.
float CalcExpDeclineRate(int32_t playerLevel, int32_t monsterLevel) {
    int32_t diff = playerLevel - monsterLevel;
    if (diff <= 5)  return 1.0f;   // full EXP within 5 levels
    if (diff <= 10) return 0.8f;
    if (diff <= 15) return 0.5f;
    if (diff <= 20) return 0.2f;
    return 0.01f;  // nearly zero EXP for trivial monsters
}

} // namespace lapis
