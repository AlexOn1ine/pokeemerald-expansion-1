#ifndef GUARD_BATTLE_MOVE_STAT_CHANGE_H
#define GUARD_BATTLE_MOVE_STAT_CHANGE_H

#include "constants/battle_stat_change.h"

// Silent failure for Secondary Effects, unless mirror armor
// Also no scripts at all for checking
// Self already covered by certain
// Allow Mirror Armor for Secondary Stat Drop effects
// Mirror Armored is set to prevent looping
// Allow Mirror Armor for secondary drop effects
// Use flag to check if self. If not self you can just
// I need allowMirrorArmor because I don't want to set certain when checking
// Basically always allow mirror armor if not certain

// Weakness Policy, Rage Building, Sticky Web What happens if weakness Policy triggers on a contrary mon and mist is on the field - not a silent failure
// Non silent failure looks like to be the default for everything
// Write a test that check for self and mist
// Set battlerAtk to gBattlerAttacker and compare, also certain = FALSE when comparing eg syrup bomb

// TESTS
// 1. test mirror armor aginst syrup bomb
// 2. test syrup bomb blocked by mist

struct StatChange
{
    const u8 *script;

    enum BattlerId battler;
    enum Stat stat;
    s8 stage;
    u8 stats;

    // Flags
    u32 silentFailure:1;
    u32 passiveStatChange:1;
    u32 mirrorArmored:1;
    u32 allowMirrorArmor:1;
    u32 onlyChecking:1;
    u32 certain:1;
    u32 setFailureFlags:1;
    u32 numFailedTargets:2;
    u32 nonMoveStatChange:1;
    u32 nextBattler:1;

    u32 forceAnim:1;
    u32 statChangePrevented:1;
    // Some padding
};

u32 ChangeStatBuffs(enum BattlerId battler, s8 statValue, enum Stat statId, union StatChangeFlags flags, u32 stats, const u8 *BS_ptr);
bool32 CompareStat(enum BattlerId battler, enum Stat statId, u32 cmpTo, u32 cmpKind, enum Ability ability);
bool32 CanStatChange(struct BattleCalcValues *cv, struct StatChange *st);
bool32 CanAnyStatChange(struct BattleCalcValues *cv, struct StatChange *st);
void TryStatChange(struct BattleCalcValues *cv, struct StatChange *st);
enum StatChangeResult TryNonMoveStatChange(struct BattleCalcValues *cv, struct StatChange *st);
void SetStatChange(enum BattlerId battler, enum Stat stat, s32 stage);
enum StatChangeResult TrySingleStatChange(struct BattleCalcValues *cv, struct StatChange *st);

bool32 IsStatSet(u32 stat, const struct AdditionalEffect *additionalEffect);
bool32 IsStatDecreaseEffect(u32 effect);
bool32 IsStatIncreaseEffect(u32 effect);

#endif // GUARD_BATTLE_MOVE_STAT_CHANGE_H
