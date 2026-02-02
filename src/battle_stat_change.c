#include "global.h"
#include "battle_util.h"
#include "battle_stat_change.h"
#include "battle_scripts.h"
#include "battle_ai_util.h"
#include "battle_controllers.h"
#include "item.h"
#include "constants/battle_anim.h"

static void MoveEffect_LowerStatsCallback(struct MoveEffectResult *result);
static void MoveEffect_RaiseStatsCallback(struct MoveEffectResult *result);
static void ChangeBattlerStats(union StatChanger statChanger, u32 battler);
static void SetMoveEffectTriggerResult(struct MoveEffectResult *result);
static void ChangeStatBuffsWithResult(struct MoveEffectResult *result, union StatChangeFlags flags);
static bool32 MoveEffectBlockedByMist(struct MoveEffectResult *result, const u8 *failPtr);
static bool32 MoveEffectBlockedByItemOrAbilityPreventingAnyStatDrop(struct MoveEffectResult *result, const u8 *itemFailPtr, const u8 *abilityFailPtr);
static bool32 MoveEffectBlockedByAbilityPreventingSpecificStatDrop(struct MoveEffectResult *result, const u8 *failPtr);
static bool32 MoveEffectBlockedByFlowerVeil(struct MoveEffectResult *result, const u8 *failPtr);
static bool32 MoveEffectBlockedByMirrorArmor(struct MoveEffectResult *result, const u8 *failPtr);
static bool32 CheckStatChangerForAllStats(struct MoveEffectResult *result);
static u16 ReverseStatChangeMoveEffect(u16 moveEffect);

bool32 ChangeStatBuffsStatChanger(u32 battler, union StatChanger statChanger, union StatChangeFlags flags, const u8 *failPtr)
{
    u32 stat;
    struct MoveEffectResult result = (struct MoveEffectResult) {
        .battlerAtk = gBattlerAttacker,
        .battlerDef = gBattlerTarget,
        .effectBattler = battler,
        .move = gCurrentMove,
        .moveEffect = (statChanger.isNegative ? MOVE_EFFECT_LOWER_STATS : MOVE_EFFECT_RAISE_STATS),
        .certain = (flags.certain || ((battler == gBattlerAttacker) && !flags.mirrorArmored && !flags.statDropPrevention)),
        .notProtectAffected = flags.notProtectAffected,
        .statDropPrevention = flags.statDropPrevention,
        .mirrorArmored = flags.mirrorArmored,
        .pushInstr = failPtr,
        .statChanger = statChanger,
        .multistring = (gBattlerTarget == battler), // Set multistring depending on mon raising/lowering stats
        .ability = GetBattlerAbility(battler),
        .holdEffect = GetBattlerHoldEffect(battler, TRUE),
    };

    ChangeStatBuffsWithResult(&result, flags);

    // Buffer the stat text (required for ability activation)
    if ((stat = GetStatChangerStat(result.statChanger, TRUE)) && stat != STAT_MULTIPLE)
    {
        PREPARE_STAT_BUFFER(gBattleTextBuff1, stat);
        GenerateAndBufferStatChangeString(gBattleTextBuff2, GetStatChangerStatValue(gBattleScripting.statChanger, stat));
    }

    if (result.multistring == B_MSG_STAT_WONT_INCREASE) // same as B_MSG_STAT_WONT_DECREASE
    {
        // Has to be set now - even if only checking
        gBattleCommunication[MULTISTRING_CHOOSER] = result.multistring;
        gBattleScripting.statChangerKey = result.statChangerKey;

        // If not allowing pointer, script continues normally
        if (!flags.allowPtr)
            return STAT_CHANGE_DIDNT_WORK;
        gBattleStruct->moveResultFlags[battler] |= MOVE_RESULT_MISSED;
        return STAT_CHANGE_WORKED;
    }

    if (flags.onlyChecking)
    {
        if (!flags.allowPtr)
            return result.failed;

        // Cannot set statChanger if only checking
        result.statChangerKey.value = 0;
    }

    // Run callbacks to extract result variables
    if (!result.failed && !flags.onlyChecking)
    {
        if (result.statChanger.isNegative)
            MoveEffect_LowerStatsCallback(&result);
        else
            MoveEffect_RaiseStatsCallback(&result);
    }

    // If allowPtr is not set, no jumping
    if (!flags.allowPtr)
        result.nextInstr = NULL;

    // Apply the results of the move effect
    SetMoveEffectTriggerResult(&result);

    return result.failed;
}

static void MoveEffect_LowerStatsCallback(struct MoveEffectResult *result)
{
    // Physically change the battler's stats
    ChangeBattlerStats(result->statChanger, result->effectBattler);

    // Set volatiles
    gProtectStructs[result->effectBattler].tryEjectPack = TRUE;
    gProtectStructs[result->effectBattler].lashOutAffected = TRUE;
    gSpecialStatuses[result->effectBattler].changedStatsBattlerId = result->battlerAtk;

    // use single stat animations when Defiant/Competitive activate
    TryPlayStatChangeAnimation(result->effectBattler, result->statChanger, ShouldDefiantCompetitiveActivate(result->effectBattler));
}

static void MoveEffect_RaiseStatsCallback(struct MoveEffectResult *result)
{
    // Physically change the battler's stats
    ChangeBattlerStats(result->statChanger, result->effectBattler);

    // Set volatiles
    gProtectStructs[result->effectBattler].statRaised = TRUE;

    // Check Mirror Herb / Opportunist
    for (u32 battler = 0; battler < gBattlersCount; battler++)
    {
        if (IsBattlerAlly(battler, result->effectBattler))
            continue; // Only triggers on opposing side

        if (GetBattlerAbility(battler) == ABILITY_OPPORTUNIST
            && gProtectStructs[result->effectBattler].activateOpportunist == 0) // don't activate opportunist on other mon's opportunist raises
            gProtectStructs[battler].activateOpportunist = 2; // set stats to copy

        if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_MIRROR_HERB)
            gProtectStructs[battler].eatMirrorHerb = 1;

        if (gProtectStructs[battler].activateOpportunist == 2 || gProtectStructs[battler].eatMirrorHerb == 1)
            QueueStatBoostsForMirrorHerbOpportunist(battler, result->statChanger);
    }

    TryPlayStatChangeAnimation(result->effectBattler, result->statChanger, FALSE);
}

// This does NOT do checking to make sure stat stages are within allowed bounds
// Make sure the argument passed to it has already been checked and adjusted
static void ChangeBattlerStats(union StatChanger statChanger, u32 battler)
{
    // If only raising the one stat, adjust that one
    if (statChanger.backwardsCompatibleStatId)
        gBattleMons[battler].statStages[statChanger.backwardsCompatibleStatId] += GetStatChangerStatValue(statChanger, statChanger.backwardsCompatibleStatId);
    else // Otherwise raise all of them
    {
        // Whether it's a drop or a raise
        s32 negativeModifier = (statChanger.isNegative ? -1 : 1);

        // Adjust all stats
        gBattleMons[battler].statStages[STAT_ATK] += (statChanger.attack * negativeModifier);
        gBattleMons[battler].statStages[STAT_DEF] += (statChanger.defense * negativeModifier);
        gBattleMons[battler].statStages[STAT_SPEED] += (statChanger.speed * negativeModifier);
        gBattleMons[battler].statStages[STAT_SPATK] += (statChanger.spAttack * negativeModifier);
        gBattleMons[battler].statStages[STAT_SPDEF] += (statChanger.spDefense * negativeModifier);
        gBattleMons[battler].statStages[STAT_ACC] += (statChanger.accuracy * negativeModifier);
        gBattleMons[battler].statStages[STAT_EVASION] += (statChanger.evasion * negativeModifier);
    }
}

static void SetMoveEffectTriggerResult(struct MoveEffectResult *result)
{
    // Record any ability that triggers during a stat change activation
    if (result->recordBattlerAbility)
        RecordAbilityBattle(result->effectBattler, result->ability);

    if (result->blockedByAbility && result->lastUsedAbility)
    {
        gLastUsedAbility = result->lastUsedAbility;
        gBattlerAbility = result->blockedByAbility - 1;
        RecordAbilityBattle(gBattlerAbility, gLastUsedAbility);

        if (result->lastUsedAbility == ABILITY_MIRROR_ARMOR)
            gBattleScripting.statChanger = result->statChanger;
    }

    if (result->blockedByItem)
        gLastUsedItem = result->lastUsedItem;

    if (result->statLowered)
        gSpecialStatuses[result->effectBattler].statLowered = TRUE;

    if (result->statChangerKey.allStats)
    {
        gBattleScripting.statChanger = result->statChanger;
        gBattleScripting.statChangerKey = result->statChangerKey;
    }
    gBattleCommunication[MULTISTRING_CHOOSER] = result->multistring;
    gEffectBattler = result->effectBattler;

    if (result->scriptingBattler)
        gBattleScripting.battler = result->scriptingBattler - 1;

    if (result->nextInstr)
    {
        if (result->battlescriptPush && result->pushInstr != NULL)
            BattleScriptPush(result->pushInstr);
        gBattlescriptCurrInstr = result->nextInstr;
    }
}

void TryPlayStatChangeAnimation(u32 battler, union StatChanger statChanger, bool32 singleStatOnly)
{
    // How many stats are lined up to be changed
    // For moves that raise several stats, the scripts are written in such a way that
    // they are raised one at a time. However, the animation requires taking into
    // account all stats that will be raised by that move at once.
    u32 changeableStatsCount = singleStatOnly ? 1: CountStatChangerStats(statChanger);
    if ((!gBattleScripting.statAnimPlayed || !statChanger.backwardsCompatibleStatId) && changeableStatsCount > 0) // failsafe
    {
        // This prevents the stat change animation going off multiple times per turn
        gBattleScripting.statAnimPlayed = (changeableStatsCount > 1);

        MarkBattlerForControllerExec(battler);
        BtlController_EmitBattleAnimation(
            battler,
            B_COMM_TO_CONTROLLER,
            B_ANIM_STATS_CHANGE,
            &gDisableStructs[battler],
            GetStatAnimArgFromStatChanger(statChanger, singleStatOnly ? statChanger.backwardsCompatibleStatId : 0)); // To do - get this work with Defiant
    }
    else // final stat that can be changed
    {
        gBattleScripting.statAnimPlayed = FALSE;
    }
}

static void ChangeStatBuffsWithResult(struct MoveEffectResult *result, union StatChangeFlags flags)
{
    if (result->ability == ABILITY_CONTRARY)
    {
        result->statChanger.isNegative ^= TRUE;
        result->recordBattlerAbility = TRUE;
        if (flags.updateMoveEffect)
            result->moveEffect = ReverseStatChangeMoveEffect(result->moveEffect);
    }
    else if (result->ability == ABILITY_SIMPLE)
    {
        // Double all stats but make sure we don't overflow
        result->statChanger.attack = min(MAX_STAT_STAGE, result->statChanger.attack * 2);
        result->statChanger.defense = min(MAX_STAT_STAGE, result->statChanger.defense * 2);
        result->statChanger.speed = min(MAX_STAT_STAGE, result->statChanger.speed * 2);
        result->statChanger.spAttack = min(MAX_STAT_STAGE, result->statChanger.spAttack * 2);
        result->statChanger.spDefense = min(MAX_STAT_STAGE, result->statChanger.spDefense * 2);
        result->statChanger.accuracy = min(MAX_STAT_STAGE, result->statChanger.accuracy * 2);
        result->statChanger.evasion = min(MAX_STAT_STAGE, result->statChanger.evasion * 2);
        result->recordBattlerAbility = TRUE;
    }

    // Trying to decrease a stat
    // Check all possible items and abilities that would block this
    // Add information such as blocking abilities and scripts that should be called
    // to the result struct and process it at the end
    if (result->statChanger.isNegative)
    {
       if (MoveEffectBlockedByMist(result, BattleScript_MistProtected)
        || MoveEffectBlockedByItemOrAbilityPreventingAnyStatDrop(result, BattleScript_ItemNoStatLoss, BattleScript_AbilityNoStatLoss)
        || MoveEffectBlockedByFlowerVeil(result, BattleScript_FlowerVeilProtectsRet)
        || MoveEffectBlockedByAbilityPreventingSpecificStatDrop(result, BattleScript_AbilityNoSpecificStatLoss)
        || MoveEffectBlockedByMirrorArmor(result, BattleScript_MirrorArmorReflect))
            return;
    }

    // Passed all ability & item checks - check stat changer for all stats
    // Even if only checking one stat at a time, need to check all stats
    // that we're trying to raise in order to ensure the correct stat change anim
    result->failed = CheckStatChangerForAllStats(result);
}

static bool32 MoveEffectBlockedByItemOrAbilityPreventingAnyStatDrop(struct MoveEffectResult *result, const u8 *itemFailPtr, const u8 *abilityFailPtr)
{
    if ((result->holdEffect == HOLD_EFFECT_CLEAR_AMULET || CanAbilityPreventStatLoss(result->ability)) &&
        (result->statDropPrevention || result->battlerAtk != result->battlerDef || result->mirrorArmored) && !result->certain && gMovesInfo[result->move].effect != EFFECT_CURSE)
    {
        if (gSpecialStatuses[result->effectBattler].statLowered)
        {
            result->nextInstr = result->pushInstr;
        }
        else
        {
            result->scriptingBattler = result->effectBattler + 1;
            if (result->holdEffect == HOLD_EFFECT_CLEAR_AMULET)
            {
                result->lastUsedItem = gBattleMons[result->effectBattler].item;
                result->battlescriptPush = TRUE;
                result->nextInstr = itemFailPtr;
                result->blockedByItem = TRUE;
            }
            else
            {
                result->blockedByAbility = result->effectBattler + 1; // Sets gBattlerAbility
                result->battlescriptPush = TRUE;
                result->nextInstr = abilityFailPtr;
                result->lastUsedAbility = result->ability;
            }
            result->statLowered = TRUE;
        }
        result->failed = TRUE;
    }
    return result->failed;
}

static bool32 MoveEffectBlockedByMist(struct MoveEffectResult *result, const u8 *failPtr)
{
    if (gSideTimers[GetBattlerSide(result->effectBattler)].mistTimer &&
        !result->certain && gMovesInfo[result->move].effect != EFFECT_CURSE &&
        !(result->effectBattler == result->battlerDef &&
          GetBattlerAbility(result->battlerAtk) == ABILITY_INFILTRATOR))
    {
        if (gSpecialStatuses[result->effectBattler].statLowered)
        {
            result->nextInstr = result->pushInstr;
        }
        else
        {
            result->battlescriptPush = TRUE;
            result->scriptingBattler = result->effectBattler + 1;
            result->nextInstr = failPtr;
            result->statLowered = TRUE;
        }
        result->failed = TRUE;
    }
    return result->failed;
}

static bool32 MoveEffectBlockedByAbilityPreventingSpecificStatDrop(struct MoveEffectResult *result, const u8 *failPtr)
{
    // To do - update this to handle cases where we're checking multiple stats at once
    if (!result->certain && AbilityPreventsSpecificStatDrop(result->ability, result->statChanger.backwardsCompatibleStatId))
    {
        SetStatChangerStatValue(&result->statChanger, result->statChanger.backwardsCompatibleStatId, 0);
        result->battlescriptPush = TRUE;
        result->lastUsedAbility = result->ability;
        result->blockedByAbility = result->effectBattler + 1; // Sets gBattlerAbility
        result->scriptingBattler = result->effectBattler + 1; // required for BattleScript_AbilityNoSpecificStatLoss
        result->nextInstr = failPtr;
        result->failed = TRUE;
    }
    return result->failed;
}

static bool32 MoveEffectBlockedByFlowerVeil(struct MoveEffectResult *result, const u8 *failPtr)
{
    u32 index;
    if ((index = IsFlowerVeilProtected(result->effectBattler)) && !result->certain)
    {
        if (gSpecialStatuses[result->effectBattler].statLowered)
        {
            result->nextInstr = result->pushInstr;
        }
        else
        {
            result->battlescriptPush = TRUE;
            result->scriptingBattler = result->effectBattler + 1; // required for BattleScript_FlowerVeilProtectsRet
            result->nextInstr = failPtr;
            result->lastUsedAbility = ABILITY_FLOWER_VEIL;
            result->blockedByAbility = index; // Sets gBattlerAbility
            result->statLowered = TRUE;
        }
        result->failed = TRUE;
    }
    return result->failed;
}

static bool32 MoveEffectBlockedByMirrorArmor(struct MoveEffectResult *result, const u8 *failPtr)
{
    if (result->ability == ABILITY_MIRROR_ARMOR && !result->mirrorArmored && result->battlerAtk != result->battlerDef && result->effectBattler == result->battlerDef)
    {
        result->battlescriptPush = TRUE;
        result->scriptingBattler = result->effectBattler + 1;
        result->blockedByAbility = result->effectBattler + 1; // Sets gBattlerAbility
        result->lastUsedAbility = ABILITY_MIRROR_ARMOR;
        result->nextInstr = failPtr;
        result->failed = TRUE;
    }
    return result->failed;
}

static bool32 CheckStatChangerForAllStats(struct MoveEffectResult *result)
{
    s32 stage;
    bool32 atLeastOneStatChangeSuccess = FALSE;
    for (u32 statId = STAT_ATK; statId < NUM_BATTLE_STATS; statId++)
    {
        stage = GetStatChangerStatValue(result->statChanger, statId);
        if (stage != 0)
        {
            // Set the bit to print a string (whether or not it fails)
            result->statChangerKey.value |= TO_BIT(statId);

            if ((stage < 0 && !result->certain && AbilityPreventsSpecificStatDrop(result->ability, statId))
                || (stage = min(abs(stage), MaxRaiseOrLowerStatAmount(result->effectBattler, statId, result->statChanger.isNegative))) == 0)
            {
                // Update stat changer to zero for this stat - we cannot change it
                SetStatChangerStatValue(&result->statChanger, statId, 0);

                // If only changing the one stat and we've hit the limit - set multistring
                if (result->statChanger.backwardsCompatibleStatId == statId && stage == 0)
                    result->multistring = result->statChanger.isNegative ? B_MSG_STAT_WONT_DECREASE : B_MSG_STAT_WONT_INCREASE;
            }
            else
            {
                // Update with a more appropriate stage if we would exceed max/min limit
                SetStatChangerStatValue(&result->statChanger, statId, stage);
                atLeastOneStatChangeSuccess = TRUE;
            }
        }
    }

    // Skips "can't go any higher!" messages if changing multiple stats
    result->statChangerKey.skipFailStrings = atLeastOneStatChangeSuccess && !(result->multistring == B_MSG_STAT_WONT_DECREASE || result->multistring == B_MSG_STAT_WONT_INCREASE);
    return !atLeastOneStatChangeSuccess;
}

u32 MaxRaiseOrLowerStatAmount(u32 battler, u32 stat, bool32 lowering)
{
    if (lowering)
        return gBattleMons[battler].statStages[stat] - MIN_STAT_STAGE;
    else
        return MAX_STAT_STAGE - gBattleMons[battler].statStages[stat];
}

static union StatChanger PrepareStatChangerAny(union StatFlags stats, s32 stage, u32 backwardsCompatibleStat)
{
    union StatChanger statChanger = (union StatChanger) {
        .isNegative = (stage < 0),
        .backwardsCompatibleStatId = backwardsCompatibleStat,
        .attack = stats.attack, // All 1 or 0 depending on if the stat is selected in 'stats'
        .defense = stats.defense,
        .speed = stats.speed,
        .spAttack = stats.spAttack,
        .spDefense = stats.spDefense,
        .accuracy = stats.accuracy,
        .evasion = stats.evasion,
    };

    // Apply stat stage and return
    statChanger.allStats *= min(MAX_STAT_STAGE, abs(stage));

    return statChanger;
}

// setBackwardsCompatibleStatId need ONLY be set with older scripts using `setstatchanger` or `statbuffchange`
union StatChanger CalcStatChangerValue(u32 statId, s32 stage, bool32 setBackwardsCompatibleStatId)
{
    return PrepareStatChangerAny(TO_BIT(statId), stage, setBackwardsCompatibleStatId ? statId : 0);
}

void SetStatChanger(u32 statId, s32 stage)
{
    gBattleScripting.statChanger = CalcStatChangerValue(statId, stage, TRUE); // True for backwards compatibility with Mirror Herb/Opportunist
}

// Used to create stat changer combined with the `stats` arg of setstatbuffchange
// This arg is used in current (by the time you read this, older?) scripts as input into the resulting stat change animation
// while we actually only raise one stat a time - which must already have been set to gBattleScripting.statChanger
// The solution is to set `backwardsCompatibleStatId` so that we know which stat we're "actually" raising, but set the other bits just for the animation
union StatChanger StatChangerWithStatBitsForAnim(union StatChanger statChanger, union StatFlags stats)
{
    if (statChanger.backwardsCompatibleStatId && stats.allStats != 0) // If backwardsCompatibleStatId is not set, we must not set the other bits
    {
        statChanger.attack = statChanger.attack ? statChanger.attack : stats.attack;
        statChanger.defense = statChanger.defense ? statChanger.defense : stats.defense;
        statChanger.speed = statChanger.speed ? statChanger.speed : stats.speed;
        statChanger.spAttack = statChanger.spAttack ? statChanger.spAttack : stats.spAttack;
        statChanger.spDefense = statChanger.spDefense ? statChanger.spDefense : stats.spDefense;
        statChanger.accuracy = statChanger.accuracy ? statChanger.accuracy : stats.accuracy;
        statChanger.evasion = statChanger.evasion ? statChanger.evasion : stats.evasion;
    }
    return statChanger;
}

u32 GetStatChangerStage(union StatChanger statChanger, u32 statId)
{
    switch (statId)
    {
    case STAT_ATK:
        return statChanger.attack;
    case STAT_DEF:
        return statChanger.defense;
    case STAT_SPEED:
        return statChanger.speed;
    case STAT_SPATK:
        return statChanger.spAttack;
    case STAT_SPDEF:
        return statChanger.spDefense;
    case STAT_ACC:
        return statChanger.accuracy;
    case STAT_EVASION:
        return statChanger.evasion;
    default:
        return 0;
    }
}

s32 GetStatChangerStatValue(union StatChanger statChanger, u32 statId)
{
    return GetStatChangerStage(statChanger, statId) * (statChanger.isNegative ? -1 : 1);
}

void SetStatChangerStatValue(union StatChanger *statChanger, u32 statId, u32 value)
{
    switch (statId)
    {
    case STAT_ATK:
        statChanger->attack = value;
        return;
    case STAT_DEF:
        statChanger->defense = value;
        return;
    case STAT_SPEED:
        statChanger->speed = value;
        return;
    case STAT_SPATK:
        statChanger->spAttack = value;
        return;
    case STAT_SPDEF:
        statChanger->spDefense = value;
        return;
    case STAT_ACC:
        statChanger->accuracy = value;
        return;
    case STAT_EVASION:
        statChanger->evasion = value;
        return;
    default:
        return;
    }
}

u32 CountStatChangerStats(union StatChanger statChanger)
{
    return !!statChanger.attack
         + !!statChanger.defense
         + !!statChanger.speed
         + !!statChanger.spAttack
         + !!statChanger.spDefense
         + !!statChanger.accuracy
         + !!statChanger.evasion;
}

static inline bool32 AnyStatChangerStatIsSharpOrHarsh(union StatChanger statChanger)
{
    return statChanger.attack > 1
        || statChanger.defense > 1
        || statChanger.speed > 1
        || statChanger.spAttack > 1
        || statChanger.spDefense > 1
        || statChanger.accuracy > 1
        || statChanger.evasion > 1;
}

u8 GetStatChangerStat(union StatChanger statChanger, bool32 backwardsCompatible)
{
    if (backwardsCompatible && statChanger.backwardsCompatibleStatId)
        return statChanger.backwardsCompatibleStatId;
    else if (CountStatChangerStats(statChanger) > 1)
        return STAT_MULTIPLE;
    else if (statChanger.attack)
        return STAT_ATK;
    else if (statChanger.defense)
        return STAT_DEF;
    else if (statChanger.speed)
        return STAT_SPEED;
    else if (statChanger.spAttack)
        return STAT_SPATK;
    else if (statChanger.spDefense)
        return STAT_SPDEF;
    else if (statChanger.accuracy)
        return STAT_ACC;
    else if (statChanger.evasion)
        return STAT_EVASION;
    else
        return 0; // Fail state
}

struct StatChangeStrings {
    u16 base;
    u16 by2Prefix;
    u16 by3OrMorePrefix;
};

static const struct StatChangeStrings sStatRaiseStrings =
{
    .base = STRINGID_STATROSE,
    .by2Prefix = STRINGID_STATSHARPLY,
    .by3OrMorePrefix = STRINGID_DRASTICALLY,
};

static const struct StatChangeStrings sStatLowerStrings =
{
    .base = STRINGID_STATFELL,
    .by2Prefix = STRINGID_STATHARSHLY,
    .by3OrMorePrefix = STRINGID_SEVERELY,
};

void GenerateAndBufferStatChangeString(u8 *textBuffer, s32 statValue)
{
    const struct StatChangeStrings *strings = statValue > 0 ? &sStatRaiseStrings : &sStatLowerStrings;

    u32 index = 0;

    // Start the string
    textBuffer[index++] = B_BUFF_PLACEHOLDER_BEGIN;

    if (abs(statValue) >= 3)
        CopyStringToArray(textBuffer, &index, strings->by3OrMorePrefix);
    else if (abs(statValue) >= 2)
        CopyStringToArray(textBuffer, &index, strings->by2Prefix);

    CopyStringToArray(textBuffer, &index, strings->base);
}

u8 GetStatAnimArgFromStatChanger(union StatChanger statChanger, u32 singleStatOnly)
{
    return ((union StatAnimArg) {
        .isNegative = statChanger.isNegative,
        .harshly = AnyStatChangerStatIsSharpOrHarsh(statChanger),
        .stat = singleStatOnly ? singleStatOnly : GetStatChangerStat(statChanger, FALSE),
    }).value;
}

u8 GetStatAnimArg(u32 stat, s32 amount)
{
    return ((union StatAnimArg) {
        .isNegative = (amount < 0),
        .harshly = (abs(amount) > 1),
        .stat = min(stat, STAT_MULTIPLE),
    }).value;
}

static u16 ReverseStatChangeMoveEffect(u16 moveEffect)
{
    switch (moveEffect)
    {
    // +1
    case MOVE_EFFECT_ATK_PLUS_1:
        return MOVE_EFFECT_ATK_MINUS_1;
    case MOVE_EFFECT_DEF_PLUS_1:
        return MOVE_EFFECT_DEF_MINUS_1;
    case MOVE_EFFECT_SPD_PLUS_1:
        return MOVE_EFFECT_SPD_MINUS_1;
    case MOVE_EFFECT_SP_ATK_PLUS_1:
        return MOVE_EFFECT_SP_ATK_MINUS_1;
    case MOVE_EFFECT_SP_DEF_PLUS_1:
        return MOVE_EFFECT_SP_DEF_MINUS_1;
    case MOVE_EFFECT_ACC_PLUS_1:
        return MOVE_EFFECT_ACC_MINUS_1;
    case MOVE_EFFECT_EVS_PLUS_1:
        return MOVE_EFFECT_EVS_MINUS_1;
    // -1
    case MOVE_EFFECT_ATK_MINUS_1:
        return MOVE_EFFECT_ATK_PLUS_1;
    case MOVE_EFFECT_DEF_MINUS_1:
        return MOVE_EFFECT_DEF_PLUS_1;
    case MOVE_EFFECT_SPD_MINUS_1:
        return MOVE_EFFECT_SPD_PLUS_1;
    case MOVE_EFFECT_SP_ATK_MINUS_1:
        return MOVE_EFFECT_SP_ATK_PLUS_1;
    case MOVE_EFFECT_SP_DEF_MINUS_1:
        return MOVE_EFFECT_SP_DEF_PLUS_1;
    case MOVE_EFFECT_ACC_MINUS_1:
        return MOVE_EFFECT_ACC_PLUS_1;
    case MOVE_EFFECT_EVS_MINUS_1:
    // +2
    case MOVE_EFFECT_ATK_PLUS_2:
        return MOVE_EFFECT_ATK_MINUS_2;
    case MOVE_EFFECT_DEF_PLUS_2:
        return MOVE_EFFECT_DEF_MINUS_2;
    case MOVE_EFFECT_SPD_PLUS_2:
        return MOVE_EFFECT_SPD_MINUS_2;
    case MOVE_EFFECT_SP_ATK_PLUS_2:
        return MOVE_EFFECT_SP_ATK_MINUS_2;
    case MOVE_EFFECT_SP_DEF_PLUS_2:
        return MOVE_EFFECT_SP_DEF_MINUS_2;
    case MOVE_EFFECT_ACC_PLUS_2:
        return MOVE_EFFECT_ACC_MINUS_2;
    case MOVE_EFFECT_EVS_PLUS_2:
        return MOVE_EFFECT_EVS_MINUS_2;
    // -2
    case MOVE_EFFECT_ATK_MINUS_2:
        return MOVE_EFFECT_ATK_PLUS_2;
    case MOVE_EFFECT_DEF_MINUS_2:
        return MOVE_EFFECT_DEF_PLUS_2;
    case MOVE_EFFECT_SPD_MINUS_2:
        return MOVE_EFFECT_SPD_PLUS_2;
    case MOVE_EFFECT_SP_ATK_MINUS_2:
        return MOVE_EFFECT_SP_ATK_PLUS_2;
    case MOVE_EFFECT_SP_DEF_MINUS_2:
        return MOVE_EFFECT_SP_DEF_PLUS_2;
    case MOVE_EFFECT_ACC_MINUS_2:
        return MOVE_EFFECT_ACC_PLUS_2;
    case MOVE_EFFECT_EVS_MINUS_2:
        return MOVE_EFFECT_EVS_PLUS_2;
    case MOVE_EFFECT_RAISE_STATS:
        return MOVE_EFFECT_LOWER_STATS;
    case MOVE_EFFECT_LOWER_STATS:
        return MOVE_EFFECT_RAISE_STATS;
    default:
        return 0;
    }
}

bool32 CanAbilityPreventStatLoss(u32 abilityDef)
{
    switch (abilityDef)
    {
    case ABILITY_CLEAR_BODY:
    case ABILITY_FULL_METAL_BODY:
    case ABILITY_WHITE_SMOKE:
        return TRUE;
    }
    return FALSE;
}


