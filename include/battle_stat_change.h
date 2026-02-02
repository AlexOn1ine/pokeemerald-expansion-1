#ifndef GUARD_BATTLE_STAT_CHANGE
#define GUARD_BATTLE_STAT_CHANGE

#include "battle.h"
#include "constants/battle.h"
#include "constants/battle_stat_change.h"
#include "constants/hold_effects.h"
#include "constants/abilities.h"
#include "constants/battle_script_commands.h"

struct MoveEffectResult
{
    // Should be populated at initialisation
    const u8 *pushInstr; // Instruction that will be pushed if battlescriptPush is set
    const u16 move; // gCurrentMove
    const u16 heldItem; // gEffectBattler held item
    u16 ability; // gEffectBattler ability
    enum MoveEffects moveEffect:8; // Current move effect
    enum ItemHoldEffect holdEffect:8; // gEffectBattler's item hold effect
    union StatChanger statChanger;
    u32 battlerAtk:3; // gBattlerAttacker
    u32 battlerDef:3; // gBattlerTarget
    u32 effectBattler:3; // The "effect" battler is the target to which the move effect is applied (not necessarily gBattlerTarget)
    u32 certain:1;
    // padding

    // statChanger flags that must also be set on init
    u32 notProtectAffected:1; // Bypasses Protect (e.g. Intimidate Attack drop)
    u32 statDropPrevention:1;
    u32 mirrorArmored:1;

    // Flags and values - all calculated along the way
    u32 scriptingBattler:3; // gBattleScripting.battler
    u16 lastUsedAbility; // Sets gLastUsedAbility
    u16 lastUsedItem; // Sets gLastUsedItem (e.g. for Knock Off)
    union StatChangerKey statChangerKey; // Keeps the list of stats that need to have a string displayed
    u8 multistring:4; // Sets gBattleCommunication[MULTISTRING_CHOOSER]
    u8 blockedByAbility:3; // Has to correspond to battler (to account for ally abilities like Flower Veil)
    u8 blockedByItem:1; // If blocked by an item (e.g. Clear Amulet, Covert Cloak)
    u16 battlescriptPush:1; // Whether or not to push pushInstr before going to nextInstr
    u16 recordBattlerAbility:1; // Records the target's ability (not the same as lastUsedAbility, which could be an ally's ability e.g. Flower Veil)
    u16 statLowered:1; // Set to prevent Mist activating multiple times in a single turn
    u16 padding2:12;

    // "Result": whether or not the move effect fails and/or where to go next
    u16 failed:1;
    const u8 *nextInstr; // Where to set gBattlescriptCurrInstr when calculated
};

union TRANSPARENT StatChangeFlags
{
    int raw;
    u32 raw_u32;
    u16 raw_u16;
    u8 raw_u8;
    struct {
        bool32 allowPtr:1; // STAT_CHANGE_ALLOW_PTR
        bool32 mirrorArmored:1; // STAT_CHANGE_MIRROR_ARMOR
        bool32 onlyChecking:1; // STAT_CHANGE_ONLY_CHECKING
        bool32 notProtectAffected:1; // STAT_CHANGE_NOT_PROTECT_AFFECTED
        bool32 updateMoveEffect:1; // STAT_CHANGE_UPDATE_MOVE_EFFECT
        bool32 statDropPrevention:1; // STAT_CHANGE_CHECK_PREVENTION
        bool32 certain:1; // STAT_CHANGE_CERTAIN
        bool32 padding:25;
    };
};

union PACKED StatAnimArg
{
    u8 value;
    u16 value_u16; // So that we can cast from a u16
    struct PACKED {
        u8 isNegative:1;
        u8 harshly:1;
        u8 stat:6;
    };
};

union TRANSPARENT StatFlags
{
    int raw;
    u8 allStats;
    u32 value;
    struct {
        u32 unused:1; // HP
        u32 attack:1;
        u32 defense:1;
        u32 speed:1;
        u32 spAttack:1;
        u32 spDefense:1;
        u32 accuracy:1;
        u32 evasion:1;
        u32 padding:24;
    };
};

void TryPlayStatChangeAnimation(u32 battler, union StatChanger statChanger, bool32 singleStatOnly);
bool32 ChangeStatBuffsStatChanger(u32 battler, union StatChanger statChanger, union StatChangeFlags flags, const u8 *failPtr);
u32 MaxRaiseOrLowerStatAmount(u32 battler, u32 stat, bool32 lowering);
union StatChanger CalcStatChangerValue(u32 statId, s32 stage, bool32 setBackwardsCompatibleStatId);
void SetStatChanger(u32 statId, s32 stage);
union StatChanger StatChangerWithStatBitsForAnim(union StatChanger statChanger, union StatFlags stats);
u32 GetStatChangerStage(union StatChanger statChanger, u32 statId);
s32 GetStatChangerStatValue(union StatChanger statChanger, u32 statId);
void SetStatChangerStatValue(union StatChanger *statChanger, u32 statId, u32 value);
u32 CountStatChangerStats(union StatChanger statChanger);
u8 GetStatChangerStat(union StatChanger statChanger, bool32 backwardsCompatible);
void GenerateAndBufferStatChangeString(u8 *textBuffer, s32 statValue);
u8 GetStatAnimArgFromStatChanger(union StatChanger statChanger, u32 singleStatOnly);
u8 GetStatAnimArg(u32 stat, s32 amount);
bool32 CanAbilityPreventStatLoss(u32 abilityDef);

#endif // GUARD_BATTLE_STAT_CHANGE
