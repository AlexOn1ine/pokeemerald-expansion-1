#include "global.h"
#include "battle.h"
#include "battle_scripts.h"
#include "battle_util.h"
#include "battle_stat_change.h"
#include "battle_ai_record.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_ai_util.h"
#include "item.h"
#include "move.h"

#define END_NUM_CHANGES -1

static const u8 *const statString[] =
{
    NULL,
    COMPOUND_STRING("Attack"),
    COMPOUND_STRING("Defense"),
    COMPOUND_STRING("Speed"),
    COMPOUND_STRING("Sp. Atk"),
    COMPOUND_STRING("Sp. Def"),
    COMPOUND_STRING("Accuracy"),
    COMPOUND_STRING("Evasion"),
};

// Stat change
static bool32 IsMinStat(enum BattlerId battler, enum Stat stat);
static void DecreaseStat(struct BattleCalcValues *cv, struct StatChange *st);

static bool32 IsMaxStat(enum BattlerId battler, enum Stat stat);

static void IncreaseStat(struct BattleCalcValues *cv, struct StatChange *st);
static void CheckOpportunistMirrorHerb(struct BattleCalcValues *cv, struct StatChange *st, u32 stageIncrease);

static void BuildStatChangeString(s32 stage, s8 statsChanged[], u32 numChanges, enum StatChangeProcess process);
static void StatChanged(struct BattleCalcValues *cv, struct StatChange *st, bool32 isMaxStage);
static void TryPlayStatChangeAnimation(struct BattleCalcValues *cv, struct StatChange *st);

// Failure handling
static bool32 IsSubstituteBlocked(struct BattleCalcValues *cv, struct StatChange *st);
static bool32 IsMistProtected(struct BattleCalcValues *cv, struct StatChange *st);
static bool32 IsFlowerVeilBlocked(struct BattleCalcValues *cv, struct StatChange *st);
static bool32 IsClearAmuletBlocked(struct BattleCalcValues *cv, struct StatChange *st);
static bool32 IsIntimidateBlocked(struct BattleCalcValues *cv, struct StatChange *st);
static bool32 IsAbilityBlocked(struct BattleCalcValues *cv, struct StatChange *st);
static bool32 IsMirrorArmorReflected(struct BattleCalcValues *cv, struct StatChange *st);

// Utitily
static void AdjustStatStage(struct BattleCalcValues *cv, struct StatChange *st);
static bool32 CanAbilityPreventStatLoss(enum Ability ability);
static bool32 AbilityPreventsSpecificStatDrop(enum Ability ability, enum Stat stat);
static u32 GetNumPositiveStats(struct StatChange *st);
static u32 GetNumNegativeStats(struct StatChange *st);
static void MarkStatsAsDone(struct StatChange *st, enum Stat stat);

enum Stat const sAccurateStatOrder[NUM_BATTLE_STATS] =
{
    STAT_HP,
    STAT_ATK,
    STAT_DEF,
    STAT_SPATK,
    STAT_SPDEF,
    STAT_SPEED,
    STAT_ACC,
    STAT_EVASION,
};

void PrepareStatsForChange(struct BattleCalcValues *cv, struct StatChange *st)
{
    struct StatStages *battlerStats = gSpecialStatuses[st->battler].statStageQueue;
    u32 numAdditionalEffects = GetMoveAdditionalEffectCount(cv->move);

    for (u32 i = 0; i < numAdditionalEffects; i++)
    {
        const struct AdditionalEffect *additionalEffect = GetMoveAdditionalEffectById(cv->move, i);

        for (enum Stat j = STAT_ATK; j < NUM_BATTLE_STATS; j++)
        {
            st->stat = sAccurateStatOrder[j];
            st->stage = GetStatStage(st->stat, additionalEffect);

            if (st->stage == 0)
                continue;

            if (additionalEffect->moveEffect == STAT_CHANGE_EFFECT_MINUS)
                st->stage = -1 * st->stage;

            AdjustStatStage(cv, st);
            u32 amount = gSpecialStatuses[st->battler].statStageAmount;
            SetStatChange(st->battler, st->stat, st->stage);

            // Workaround for contrary
            if (cv->moveEffect == EFFECT_BELLY_DRUM && !CompareStat(st->battler, st->stat, MAX_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
            {
                battlerStats[amount].state = STAT_CHANGE_AT_MAX;
            }
            else if (st->stage < 0)
            {
                if (CompareStat(st->battler, st->stat, MIN_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
                    battlerStats[amount].state = STAT_CHANGE_AT_MIN;
                else if (CanDecreaseStat(cv, st) == STAT_CHANGE_DIDNT_WORK)
                    battlerStats[amount].state = STAT_CHANGE_PREVENTED;
                else
                    battlerStats[amount].state = STAT_CHANGE_DO_CHANGE;
            }
            else
            {
                if (CompareStat(st->battler, st->stat, MAX_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
                    battlerStats[st->stat].state = STAT_CHANGE_AT_MAX;
                else
                    battlerStats[st->stat].state = STAT_CHANGE_DO_CHANGE;
            }
        }
    }
}

static bool32 AreNegativeStatsInProcess(enum StatChangeProcess process)
{
    return process == PROCESS_STAT_DECREASING || process == PROCESS_STAT_AT_MIN;
}

static bool32 ArePositiveStatsInProcess(enum StatChangeProcess process)
{
    return process == PROCESS_STAT_INCREASING || process == PROCESS_STAT_AT_MAX;
}

// if (cv->move == MOVE_NONE)
//     AdjustStatStage(cv, st);
enum StatChangeResult TryStatChange(struct BattleCalcValues *cv, struct StatChange *st)
{
    u32 numChanges = 0;
    s8 statsChanged[NUM_BATTLE_STATS + 1];
    s32 minMaxStage = 0;

    enum StatChangeProcess process = PROCESS_STAT_DEFAULT;

    for (u32 i = 0; i < NUM_BATTLE_STATS + 1; i++)
        statsChanged[i] = END_NUM_CHANGES;

    for (u32 i = 0; i < st->statStageAmount; i++)
    {
        if (st->statStageQueue[i].state == STAT_CHANGE_DONE)
        {
            continue;
        }

        st->stat = st->statStageQueue[i].stat;
        st->stage = st->statStageQueue[i].stage;

        if (!ArePositiveStatsInProcess(process) && !AreNegativeStatsInProcess(process))
        {
            if (st->stage < 0 && CanDecreaseStat(cv, st) == STAT_CHANGE_DIDNT_WORK)
                return STAT_CHANGE_BLOCKED_BY_TARGET;
        }

        if (st->stage < 0 && !ArePositiveStatsInProcess(process))
        {
            if (process != PROCESS_STAT_DECREASING && IsMinStat(st->battler, st->stat))
            {
                process = PROCESS_STAT_AT_MIN;
            }
            else if (process != PROCESS_STAT_AT_MIN)
            {
                DecreaseStat(cv, st);
                process = PROCESS_STAT_DECREASING;
                minMaxStage = min(minMaxStage, st->stage);
            }

            st->statStageQueue[i].state = STAT_CHANGE_DONE;
            statsChanged[numChanges++] = st->stat;
        }
        else if (!AreNegativeStatsInProcess(process))
        {
            if (process != PROCESS_STAT_INCREASING && IsMaxStat(st->battler, st->stat))
            {
                process = PROCESS_STAT_AT_MAX;
            }
            else if (process != PROCESS_STAT_AT_MAX)
            {
                IncreaseStat(cv, st);
                process = PROCESS_STAT_INCREASING;
                minMaxStage = max(minMaxStage, st->stage);
            }
            st->statStageQueue[i].state = STAT_CHANGE_DONE;
            statsChanged[numChanges++] = st->stat;
        }
    }

    if (process == PROCESS_STAT_DEFAULT)
        return STAT_CHANGE_DIDNT_WORK;

    if (st->doneSideBattlers)
        return STAT_CHANGE_WORKED;

    BuildStatChangeString(minMaxStage, statsChanged, numChanges, process);

    if (st->doSideBattlers)
    {
        st->script = BattleScript_IncreaseStatChangeMessageTwo;
        gBattlerTarget = st->battler; // TODO: Remove
    }
    else
    {
        st->script = BattleScript_IncreaseStatChangeMessage;
    }

    if (process == PROCESS_STAT_DECREASING || process == PROCESS_STAT_INCREASING)
    {
        st->stage = minMaxStage;
        TryPlayStatChangeAnimation(cv, st);
    }

    return STAT_CHANGE_WORKED;
}

#include "string_util.h"
static void BuildStatChangeString(s32 stage, s8 statsChanged[], u32 numChanges, enum StatChangeProcess process)
{
    StringCopy(gBattleTextBuff3, statString[statsChanged[0]]);

    if (numChanges > 1)
    {
        for (u32 i = 1; i < NUM_BATTLE_STATS; i++)
        {
            if (statsChanged[i] == END_NUM_CHANGES)
                break;

            StringAppend(gBattleTextBuff3, COMPOUND_STRING(", "));
            if (statsChanged[i + 1] == END_NUM_CHANGES) {
                StringAppend(gBattleTextBuff3, COMPOUND_STRING("and "));
            }
            StringAppend(gBattleTextBuff3, statString[statsChanged[i]]);
        }
    }

    if (process == PROCESS_STAT_AT_MAX)
    {
        StringAppend(gBattleTextBuff3, COMPOUND_STRING(" won't go any higher!"));
    }
    else if (process == PROCESS_STAT_AT_MIN)
    {
        StringAppend(gBattleTextBuff3, COMPOUND_STRING(" won't go any lower!"));
    }
    else
    {
        if (stage == 2) {
            StringAppend(gBattleTextBuff3, COMPOUND_STRING(" rose sharply!"));
        } else if (stage >= 3) {
            StringAppend(gBattleTextBuff3, COMPOUND_STRING(" rose drastically!"));
        } else if (stage > 0) {
            StringAppend(gBattleTextBuff3, COMPOUND_STRING(" rose!"));
        } else if (stage == -2) {
            StringAppend(gBattleTextBuff3, COMPOUND_STRING(" fell harshly!"));
        } else if (stage <= -3) {
            StringAppend(gBattleTextBuff3, COMPOUND_STRING(" fell severely!"));
        } else {
            StringAppend(gBattleTextBuff3, COMPOUND_STRING(" fell!"));
        }
    }
}

static bool32 AreNegativeStatChangesDone(struct StatChange *st)
{
    for (u32 i = 0; i < st->statStageAmount; i++)
    {
        if (st->statStageQueue[i].stage < 0 && st->statStageQueue[i].state != STAT_CHANGE_DONE)
            return FALSE;
    }
    return TRUE;
}

enum StatChangeResult CanDecreaseStat(struct BattleCalcValues *cv, struct StatChange *st)
{
    // if (AreNegativeStatChangesDone(st))
    //     return STAT_CHANGE_WORKED;

    if ((st->silentFailure || st->onlyChecking) && IsSubstituteBlocked(cv, st))
        return STAT_CHANGE_DIDNT_WORK;

    if (IsMistProtected(cv, st)
     || IsIntimidateBlocked(cv, st)
     || IsFlowerVeilBlocked(cv, st)
     || IsClearAmuletBlocked(cv, st)
     || IsAbilityBlocked(cv, st)
     || IsMirrorArmorReflected(cv, st))
        return STAT_CHANGE_DIDNT_WORK;
    return STAT_CHANGE_WORKED;
}

static bool32 IsMinStat(enum BattlerId battler, enum Stat stat)
{
    if (gBattleMons[battler].statStages[stat] == MIN_STAT_STAGE)
        return TRUE;
    return FALSE;
}

static void DecreaseStat(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (st->onlyChecking)
        return;

    u32 currStage = gBattleMons[st->battler].statStages[st->stat];

    if (currStage == (MIN_STAT_STAGE + 1))
        st->stage = -1;
    else if (currStage == 2 && st->stage < -2)
        st->stage = -2;

    gBattleMons[st->battler].volatiles.tryEjectPack = TRUE;
    gProtectStructs[st->battler].lashOutAffected = TRUE;

    if (!st->stickyWeb)
    {
        if (st->certain || (cv->battlerAtk != st->battler && IsBattlerAlly(cv->battlerAtk, st->battler)))
            gBattleStruct->ignoreDefiant = TRUE;
    }

    StatChanged(cv, st, FALSE);
    st->script = BattleScript_DecreaseStatChangeMessage;
}

static bool32 IsMaxStat(enum BattlerId battler, enum Stat stat)
{
    if (gBattleMons[battler].statStages[stat] == MAX_STAT_STAGE)
        return TRUE;
    return FALSE;
}

static void IncreaseStat(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (st->onlyChecking)
        return;

    u32 currStage = gBattleMons[st->battler].statStages[st->stat];
    bool32 isMaxStage = st->stage >= 12;

    if (currStage == MAX_STAT_STAGE - 1)
        st->stage = 1;
    else if (currStage == MAX_STAT_STAGE - 2 && st->stage > 2)
        st->stage = 2;

    u32 stageIncrease = st->stage;

    if ((st->stage + gBattleMons[st->battler].statStages[st->stat]) > MAX_STAT_STAGE)
        stageIncrease = MAX_STAT_STAGE - gBattleMons[st->battler].statStages[st->stat];

    CheckOpportunistMirrorHerb(cv, st, stageIncrease);

    gProtectStructs[st->battler].statRaised = TRUE;
    StatChanged(cv, st, isMaxStage);
}

static void CheckOpportunistMirrorHerb(struct BattleCalcValues *cv, struct StatChange *st, u32 stageIncrease)
{
    if (stageIncrease == 0)
        return;

    for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
    {
        if (IsBattlerAlly(battler, st->battler))
            continue; // Only triggers on opposing side

        if (CompareStat(battler, st->stat, MAX_STAT_STAGE, CMP_EQUAL, cv->abilities[battler]))
            continue;

        if (cv->abilities[battler] == ABILITY_OPPORTUNIST && !st->opportunistActivation)
            gProtectStructs[battler].activateOpportunist = TRUE;

        if (cv->holdEffects[battler] == HOLD_EFFECT_MIRROR_HERB && !st->mirrorHerbActivation)
            gProtectStructs[battler].eatMirrorHerb = TRUE;

        if (gProtectStructs[battler].activateOpportunist || gProtectStructs[battler].eatMirrorHerb)
        {
            gQueuedStatBoosts[battler].stats |= (1 << (st->stat - 1)); // -1 to start at atk
            gQueuedStatBoosts[battler].statChanges[st->stat - 1] += stageIncrease;
        }
    }
}

static void StatChanged(struct BattleCalcValues *cv, struct StatChange *st, bool32 isMaxStage)
{
    gBattleStruct->moveResultFlags[st->battler] |= MOVE_RESULT_STAT_CHANGED;
    gBattleScripting.battler = st->battler;
    gBattleMons[st->battler].statStages[st->stat] += st->stage;

    if (st->stage > 0)
    {
        if (gBattleMons[st->battler].statStages[st->stat] > MAX_STAT_STAGE)
            gBattleMons[st->battler].statStages[st->stat] = MAX_STAT_STAGE;
    }
    else
    {
        if (gBattleMons[st->battler].statStages[st->stat] < MIN_STAT_STAGE)
            gBattleMons[st->battler].statStages[st->stat] = MIN_STAT_STAGE;
    }

    if (cv->moveEffect == EFFECT_STOCKPILE && st->stage > 0)
    {
        switch (st->stat)
        {
        case STAT_DEF:
            gBattleMons[st->battler].volatiles.stockpileDef++;
            break;
        case STAT_SPDEF:
            gBattleMons[st->battler].volatiles.stockpileSpDef++;
            break;
        default:
            break;
        }
    }

    if (cv->moveEffect == EFFECT_BELLY_DRUM)
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_CHANGED_BELLY_DRUM;
    }
    else if (isMaxStage)
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_MAXED;
    }
    else if (st->itemMessage)
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_CHANGED_ITEM;
    }
    else
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_CHANGED;
    }
}

static void TryPlayStatChangeAnimation(struct BattleCalcValues *cv, struct StatChange *st)
{
    u32 statAnimId = st->stat;

    if (st->stage <= -1) // goes down
    {
        enum BattleSide side = GetBattlerSide(st->battler);
        // if (gBattleStruct->negativeAnimPlayed[side])
        //     return;

        u32 numNegativeStats = 0;
        bool32 isStatChangeByTwo = abs(st->stage) > 1;

        gBattleStruct->negativeAnimPlayed[side] = TRUE;
        numNegativeStats = GetNumNegativeStats(st);

        statAnimId += isStatChangeByTwo ? STAT_ANIM_MINUS2 : STAT_ANIM_MINUS1;

        if (ShouldDefiantCompetitiveActivate(st->battler, cv->abilities[st->battler]))
            numNegativeStats = 0;

        if (numNegativeStats > 1) // more than one stat, so the color is gray
            statAnimId = isStatChangeByTwo ? STAT_ANIM_MULTIPLE_MINUS2 : STAT_ANIM_MULTIPLE_MINUS1;
    }
    else // goes up
    {
        enum BattleSide side = GetBattlerSide(st->battler);
        // if (gBattleStruct->positiveAnimPlayed[side])
        //     return;

        u32 numPositiveStats = 0;
        bool32 isStatChangeByTwo = st->stage > 1;

        gBattleStruct->positiveAnimPlayed[side] = TRUE;
        numPositiveStats = GetNumPositiveStats(st);

        statAnimId += isStatChangeByTwo ? STAT_ANIM_PLUS2 : STAT_ANIM_PLUS1;

        if (numPositiveStats > 1)
            statAnimId = isStatChangeByTwo ? STAT_ANIM_MULTIPLE_PLUS2 : STAT_ANIM_MULTIPLE_PLUS1;
    }

    BtlController_EmitBattleAnimation(st->battler, B_COMM_TO_CONTROLLER, B_ANIM_STATS_CHANGE, statAnimId);
    MarkBattlerForControllerExec(st->battler);

    enum BattlerId partner = BATTLE_PARTNER(st->battler);

    if (st->doSideBattlers)
    {
        gBattleStruct->multiStatChangeAnim = TRUE;
        BtlController_EmitBattleAnimation(partner,
                B_COMM_TO_CONTROLLER, B_ANIM_STATS_CHANGE, statAnimId);
        MarkBattlerForControllerExec(partner);
    }
}

void ResetAnimPlayedFlags(void)
{
    for (enum BattleSide side = 0; side < NUM_BATTLE_SIDES; side++)
    {
        gBattleStruct->negativeAnimPlayed[side] = FALSE;
        gBattleStruct->positiveAnimPlayed[side] = FALSE;
    }
}

static bool32 IsSubstituteBlocked(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (st->ignoreCertainFailure)
        return FALSE;

    if (st->certain || GetBattlerMoveTargetType(cv->battlerAtk, cv->move) == TARGET_ALLY)
        return FALSE;

    if (!IsSubstituteProtected(cv->battlerAtk, st->battler, cv->abilities[cv->battlerAtk], cv->move))
        return FALSE;

    if (!st->onlyChecking)
    {
        gBattleScripting.battler = st->battler;
        st->script = BattleScript_ButItFailedRet;
    }

    return TRUE;
}

static bool32 IsMistProtected(struct BattleCalcValues *cv, struct StatChange *st) {
    if (gSideTimers[GetBattlerSide(st->battler)].mistTimer == 0)
        return FALSE;

    if (st->certain)
        return FALSE;

    if (!IsBattlerAlly(st->battler, cv->battlerAtk) && cv->abilities[cv->battlerAtk] == ABILITY_INFILTRATOR)
        return FALSE;

    if (!st->onlyChecking)
    {
        MarkStatsAsDone(st, NUM_BATTLE_STATS);
        gBattleScripting.battler = st->battler;
        st->script = BattleScript_MistProtected;
    }

    return TRUE;
}

static enum BattlerId StatChange_IsFlowerVeilProtected(struct BattleCalcValues *cv, enum BattlerId battler)
{
    if (!IS_BATTLER_OF_TYPE(battler, TYPE_GRASS))
        return MAX_BATTLERS_COUNT;

    for (enum BattlerId abilityBattler = B_BATTLER_0; abilityBattler < gBattlersCount; abilityBattler ++)
    {
        if (!IsBattlerAlly(battler, abilityBattler))
            continue;
        if (cv->abilities[abilityBattler] == ABILITY_FLOWER_VEIL)
            return abilityBattler;
    }

    return MAX_BATTLERS_COUNT;
}

static bool32 IsFlowerVeilBlocked(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (st->certain)
        return FALSE;

    enum BattlerId flowerVeilBattler = StatChange_IsFlowerVeilProtected(cv, st->battler);

    if (flowerVeilBattler == MAX_BATTLERS_COUNT)
        return FALSE;

    if (!st->onlyChecking)
    {
        st->script = BattleScript_FlowerVeilProtectsRet;
        gBattleScripting.battler = st->battler;
        gBattlerAbility = flowerVeilBattler;
        gLastUsedAbility = ABILITY_FLOWER_VEIL;
        MarkStatsAsDone(st, NUM_BATTLE_STATS);
        RecordAbilityBattle(gBattlerAbility, ABILITY_FLOWER_VEIL);
    }

    return TRUE;
}

static bool32 IsClearAmuletBlocked(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (st->certain)
        return FALSE;

    if (cv->holdEffects[st->battler] != HOLD_EFFECT_CLEAR_AMULET)
        return FALSE;

    if (!st->onlyChecking)
    {
        st->script = BattleScript_ItemNoStatLoss;
        gBattleScripting.battler = st->battler;
        gLastUsedItem = gBattleMons[st->battler].item;
        MarkStatsAsDone(st, NUM_BATTLE_STATS);
        RecordItemEffectBattle(st->battler, HOLD_EFFECT_CLEAR_AMULET);
    }

    return TRUE;
}

static bool32 IsIntimidateBlocked(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (!st->intimidate)
        return FALSE;

    switch (cv->abilities[st->battler])
    {
    case ABILITY_INNER_FOCUS:
    case ABILITY_SCRAPPY:
    case ABILITY_OWN_TEMPO:
    case ABILITY_OBLIVIOUS:
        if (GetConfig(B_UPDATED_INTIMIDATE) < GEN_8)
            return FALSE;
        PREPARE_STAT_BUFFER(gBattleTextBuff1, st->stat);
        st->script = BattleScript_AbilityNoSpecificStatLoss;
        break;
    case ABILITY_GUARD_DOG:
    {
        enum BattlerId flowerVeilBattler = StatChange_IsFlowerVeilProtected(cv, st->battler);

        if (flowerVeilBattler != MAX_BATTLERS_COUNT
         && GetBattlerRawSpeedOrder(flowerVeilBattler) < GetBattlerRawSpeedOrder(st->battler))
            return FALSE;

        if (!CompareStat(st->battler, STAT_ATK, MIN_STAT_STAGE, CMP_GREATER_THAN, cv->abilities[st->battler]))
            return FALSE;

        SetStatChange2(st->battler, st->stat, -1 * st->stage);
        st->script = BattleScript_DefiantActivates;
        gEffectBattler = st->battler;
        break;
    }
    default:
        return FALSE;
    }

    gLastUsedAbility = cv->abilities[st->battler];
    gBattlerAbility = st->battler;
    gBattleScripting.battler = st->battler;
    MarkStatsAsDone(st, st->stat);
    RecordAbilityBattle(st->battler, cv->abilities[st->battler]);
    return TRUE;
}

static bool32 IsAbilityBlocked(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (st->certain)
        return FALSE;

    if (CanAbilityPreventStatLoss(cv->abilities[st->battler]))
    {
        if (!st->onlyChecking)
        {
            MarkStatsAsDone(st, NUM_BATTLE_STATS);
            st->script = BattleScript_AbilityNoStatLoss;
        }
    }
    else if (AbilityPreventsSpecificStatDrop(cv->abilities[st->battler], st->stat))
    {
        if (!st->onlyChecking)
        {
            MarkStatsAsDone(st, st->stat);
            PREPARE_STAT_BUFFER(gBattleTextBuff1, st->stat);
            st->script = BattleScript_AbilityNoSpecificStatLoss;
        }
    }
    else
    {
        return FALSE;
    }

    if (!st->onlyChecking)
    {
        gBattleScripting.battler = st->battler;
        gBattlerAbility = st->battler;
        gLastUsedAbility = cv->abilities[st->battler];
        RecordAbilityBattle(st->battler, gLastUsedAbility);
    }

    return TRUE;
}

static bool32 IsMirrorArmorReflected(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (cv->abilities[st->battler] != ABILITY_MIRROR_ARMOR
     || st->ignoreMirrorArmored
     || st->certain)
        return FALSE;

    if (st->onlyChecking && !st->ignoreCertainFailure)
        return TRUE;

    if (st->checkMirrorArmor || !st->ignoreCertainFailure)
    {
        st->silentFailure = FALSE; // Mirror Armor still deflects damaging move stat drops
        st->script = BattleScript_MirrorArmorReflect;
        gBattlerAbility = st->battler;
        RecordAbilityBattle(st->battler, cv->abilities[st->battler]);

        if (st->stickyWeb)
        {
            if (GetConfig(B_MIRROR_ARMOR_STICKY_WEB) >= GEN_9)
            {
                st->script = BattleScript_AbilityPopUp;
                return TRUE;
            }
            else if (gSideTimers[GetBattlerSide(st->battler)].stickyWebBattlerId != 0xFF)
            {
                gBattleScripting.battler = gSideTimers[GetBattlerSide(st->battler)].stickyWebBattlerId;
            }
        }
        else
        {
            gBattleScripting.battler = cv->battlerAtk;

            if (IsBattlerAlly(cv->battlerAtk, st->battler))
                gBattleStruct->ignoreDefiant = TRUE;

            gBattleStruct->allowPartingShot = TRUE;
        }

        for (u32 i = 0; i < st->statStageAmount; i++)
        {
            enum Stat stat = st->statStageQueue[i].stat;
            s32 stage = st->statStageQueue[i].stage;
            if (stage < 0)
            {
                st->statStageQueue[i].state = STAT_CHANGE_DONE;
                SetStatChange2(gBattleScripting.battler, stat, stage);
            }
        }

        return TRUE;
    }

    return FALSE;
}

// There is a similar function AI_GetAdjustedStatStage that needs to be updated if things are changed here
static void AdjustStatStage(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (cv->moveEffect == EFFECT_GROWTH && GetAttackerWeather(cv->holdEffects[st->battler], cv->abilities[st->battler], GetWeather()) & B_WEATHER_SUN)
        st->stage = 2;

    if (st->stage == STAT_CHANGE_FORCE_MAX)
        st->stage = 12;

    switch (cv->abilities[st->battler])
    {
    case ABILITY_CONTRARY:
        st->stage = -1 * st->stage;
        if (!st->onlyChecking)
            RecordAbilityBattle(st->battler, cv->abilities[st->battler]);
        break;
    case ABILITY_SIMPLE:
        st->stage = 2 * st->stage;
        if (!st->onlyChecking)
            RecordAbilityBattle(st->battler, cv->abilities[st->battler]);
        break;
    default:
        break;
    }
}

static bool32 CanAbilityPreventStatLoss(enum Ability ability)
{
    switch (ability)
    {
    case ABILITY_CLEAR_BODY:
    case ABILITY_FULL_METAL_BODY:
    case ABILITY_WHITE_SMOKE:
        return TRUE;
    default:
        return FALSE;
    }
}

static bool32 AbilityPreventsSpecificStatDrop(enum Ability ability, enum Stat stat)
{
    switch (ability)
    {
    case ABILITY_ILLUMINATE:
        if (B_ILLUMINATE_EFFECT < GEN_9)
            return FALSE;
    case ABILITY_KEEN_EYE:
    case ABILITY_MINDS_EYE:
        return stat == STAT_ACC;
    case ABILITY_HYPER_CUTTER:
        return stat == STAT_ATK;
    case ABILITY_BIG_PECKS:
        return stat == STAT_DEF;
    default:
        return FALSE;
    }
}

bool32 ShouldDefiantCompetitiveActivate(enum BattlerId battler, enum Ability ability)
{
    enum BattleSide side = GetBattlerSide(battler);

    if (gBattleStruct->ignoreDefiant)
        return FALSE;

    switch (ability)
    {
    case ABILITY_DEFIANT:
        if (CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_EQUAL, ability))
            return FALSE;
        break;
    case ABILITY_COMPETITIVE:
        if (CompareStat(battler, STAT_SPATK, MAX_STAT_STAGE, CMP_EQUAL, ability))
            return FALSE;
        break;
    default:
        return FALSE;
    }

    if (GetConfig(B_DEFIANT_STICKY_WEB) >= GEN_9 || !gBattleScripting.stickyWebStatDrop)
        return TRUE;

    // only activate Defiant/Competitive if Web was setup by foe
    return gSideTimers[side].stickyWebBattlerSide != side;
}

u32 GetStatStage(enum Stat stat, const struct AdditionalEffect *additionalEffect)
{
    switch (stat)
    {
    case STAT_ATK:     return additionalEffect->attack;
    case STAT_DEF:     return additionalEffect->defense;
    case STAT_SPEED:   return additionalEffect->speed;
    case STAT_SPATK:   return additionalEffect->spAtk;
    case STAT_SPDEF:   return additionalEffect->spDef;
    case STAT_ACC:     return additionalEffect->accuracy;
    case STAT_EVASION: return additionalEffect->evasion;
    default:           return 0;
    }
}

static u32 GetNumPositiveStats(struct StatChange *st)
{
    u32 num = 0;
    for (u32 i = 0; i < st->statStageAmount; i++)
    {
        if (st->statStageQueue[i].stage > 0)
            num++;
    }
    return num;
}

static u32 GetNumNegativeStats(struct StatChange *st)
{
    u32 num = 0;
    for (u32 i = 0; i < st->statStageAmount; i++)
    {
        if (st->statStageQueue[i].stage < 0)
            num++;
    }
    return num;
}

void SetStatChange(enum BattlerId battler, enum Stat stat, s32 stage)
{
    u32 amount = gSpecialStatuses[battler].statStageAmount;
    gSpecialStatuses[battler].statStageQueue[amount].stat = stat;
    gSpecialStatuses[battler].statStageQueue[amount].stage = stage;
    gSpecialStatuses[battler].statStageAmount++;
}

// Used for stat change responses like Defiant and Mirror Armor
void SetStatChange2(enum BattlerId battler, enum Stat stat, s32 stage)
{
    gSpecialStatuses[battler].statStageQueue2[gSpecialStatuses[battler].statStageAmount2].stat = stat;
    gSpecialStatuses[battler].statStageQueue2[gSpecialStatuses[battler].statStageAmount2].stage = stage;
    gSpecialStatuses[battler].statStageAmount2++;
}

void CopyOverStatStageQueue(struct StatChange *st)
{
    st->statStageQueue = gSpecialStatuses[st->battler].statStageQueue;
    st->statStageAmount = gSpecialStatuses[st->battler].statStageAmount;
}

void ClearStatChangeValues(void)
{
    for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
    {
        memset(gSpecialStatuses[battler].statStageQueue, 0, sizeof(gSpecialStatuses[battler].statStageQueue));
        gSpecialStatuses[battler].statStageAmount = 0;
    }

    ResetAnimPlayedFlags();
    gBattleStruct->statChangeBattler  = 0;
}

void ClearBattlerStatChangeValues(enum BattlerId battler)
{
    memset(gSpecialStatuses[battler].statStageQueue, 0, sizeof(gSpecialStatuses[battler].statStageQueue));
    gSpecialStatuses[battler].statStageAmount = 0;
}

void ClearOtherStatChangeValues(enum BattlerId battler)
{
    memset(gSpecialStatuses[battler].statStageQueue2, 0, sizeof(gSpecialStatuses[battler].statStageQueue2));
    gSpecialStatuses[battler].statStageAmount2 = 0;
}

void ClearBothStatChangeQueues(void)
{
    for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
    {
        memset(gSpecialStatuses[battler].statStageQueue2, 0, sizeof(gSpecialStatuses[battler].statStageQueue2));
        gSpecialStatuses[battler].statStageAmount2 = 0;
        memset(gSpecialStatuses[battler].statStageQueue, 0, sizeof(gSpecialStatuses[battler].statStageQueue));
        gSpecialStatuses[battler].statStageAmount = 0;
    }
    ResetAnimPlayedFlags();
    gBattleStruct->statChangeBattler  = 0;
}

bool32 WillAnyStatChange(enum BattlerId battler)
{
    u32 amount = gSpecialStatuses[battler].statStageAmount;
    struct StatStages *battlerStats = gSpecialStatuses[battler].statStageQueue;

    for (u32 i = 0; i < amount; i++)
    {
        if (battlerStats[i].state == STAT_CHANGE_DO_CHANGE)
            return TRUE;
    }
    return FALSE;;
}

bool32 AreAllStatsDone(enum BattlerId battler)
{
    u32 amount = gSpecialStatuses[battler].statStageAmount;
    struct StatStages *battlerStats = gSpecialStatuses[battler].statStageQueue;

    for (u32 i = 0; i < amount; i++)
    {
        if (battlerStats[i].state != STAT_CHANGE_DONE)
            return FALSE;
    }
    return TRUE;
}

bool32 AreAllStatsDone2(enum BattlerId battler)
{
    u32 amount = gSpecialStatuses[battler].statStageAmount2;
    struct StatStages *battlerStats = gSpecialStatuses[battler].statStageQueue2;

    for (u32 i = 0; i < amount; i++)
    {
        if (battlerStats[i].state != STAT_CHANGE_DONE)
            return FALSE;
    }
    return TRUE;
}

static bool32 UNUSED AreAnyStatChangesPrevented(enum BattlerId battler, enum BattlerId partner)
{
    struct StatStages *battlerStats = gSpecialStatuses[battler].statStageQueue;
    struct StatStages *partnerStats = gSpecialStatuses[partner].statStageQueue;

    for (u32 i = 1; i < NUM_BATTLE_STATS; i++)
    {
        if (battlerStats[i].state == STAT_CHANGE_PREVENTED || partnerStats[i].state == STAT_CHANGE_PREVENTED)
            return TRUE;
    }
    return FALSE;
}

bool32 AreAllStatChangesPrevented(enum BattlerId battler)
{
    u32 count = 0;
    u32 amount = gSpecialStatuses[battler].statStageAmount;
    struct StatStages *battlerStats = gSpecialStatuses[battler].statStageQueue;

    for (u32 i = 0; i < amount; i++)
    {
        if (battlerStats[i].state == STAT_CHANGE_NONE)
            continue;
        if (battlerStats[i].state == STAT_CHANGE_PREVENTED)
            continue;
        return FALSE;
    }
    return TRUE;
}

struct QueuedStatChange GetQueuedStatChangeStates(enum BattlerId battler)
{
    struct StatStages *battlerStats = gSpecialStatuses[battler].statStageQueue;

    struct QueuedStatChange change = {0};
    change.amount = gSpecialStatuses[battler].statStageAmount;

    for (u32 i = 0; i < change.amount; i++)
    {
        switch (battlerStats[i].state)
        {
        case STAT_CHANGE_NONE:
            change.numNone++;
            break;
        case STAT_CHANGE_DONE:
            change.numDone++;
            break;
        case STAT_CHANGE_PREVENTED:
            change.numPrevented++;
            break;
        case STAT_CHANGE_AT_MAX:
            change.numMax++;
            break;
        case STAT_CHANGE_AT_MIN:
            change.numMin++;
            break;
        case STAT_CHANGE_DO_CHANGE:
            change.doChange = TRUE;
            break;
        }
    }

    return change;
}

bool32 AreSameStatsAtMinMax(enum BattlerId battler, enum BattlerId partner)
{
    struct StatStages *battlerStats = gSpecialStatuses[battler].statStageQueue;
    struct StatStages *partnerStats = gSpecialStatuses[partner].statStageQueue;

    for (u32 i = 1; i < NUM_BATTLE_STATS; i++)
    {
        if (battlerStats[i].state == STAT_CHANGE_NONE && partnerStats[i].state == STAT_CHANGE_NONE)
            continue;
        if (battlerStats[i].state == STAT_CHANGE_AT_MAX && partnerStats[i].state == STAT_CHANGE_AT_MAX)
            continue;
        if (battlerStats[i].state == STAT_CHANGE_AT_MIN && partnerStats[i].state == STAT_CHANGE_AT_MIN)
            continue;
        return FALSE;
    }
    return TRUE;
}

bool32 AreStatChangesTheSame(enum BattlerId battler, enum BattlerId partner)
{
    if (!IsDoubleBattle())
        return FALSE;

    struct StatStages *battlerStats = gSpecialStatuses[battler].statStageQueue;
    struct StatStages *partnerStats = gSpecialStatuses[partner].statStageQueue;

    for (u32 i = 1; i < NUM_BATTLE_STATS; i++)
    {
        if (battlerStats[i].state == STAT_CHANGE_PREVENTED || partnerStats[i].state == STAT_CHANGE_PREVENTED)
            return FALSE;

        if (battlerStats[i].stat == partnerStats[i].stat && battlerStats[i].stage == partnerStats[i].stage)
            continue;

        return FALSE;
    }
    return TRUE;
}

bool32 IsStatChangeQueued(enum BattlerId battler)
{
    u32 amount = gSpecialStatuses[battler].statStageAmount;
    struct StatStages *battlerStats = gSpecialStatuses[battler].statStageQueue;

    for (u32 i = 0; i < amount; i++)
    {
        if (battlerStats[i].state == STAT_CHANGE_NONE)
            continue;
        return TRUE;
    }

    return FALSE;
}

// 1. Check if any stat changes are prevented (includes min / max)
//    a. Other stats can be changed -> do after anim
//    b. No stats can be changed, do before anim
// 2. If none are prevented check if both (all) are at min / max
// 3. If some stats can be changed, skip

bool32 CompareStat(enum BattlerId battler, enum Stat statId, u32 cmpTo, u32 cmpKind, enum Ability ability)
{
    bool32 ret = FALSE;
    u32 statValue = gBattleMons[battler].statStages[statId];

    // Because this command is used as a way of checking if a stat can be lowered/raised,
    // we need to do some modification at run-time.
    if (ability == ABILITY_CONTRARY)
    {
        if (cmpKind == CMP_GREATER_THAN)
            cmpKind = CMP_LESS_THAN;
        else if (cmpKind == CMP_LESS_THAN)
            cmpKind = CMP_GREATER_THAN;

        if (cmpTo == MIN_STAT_STAGE)
            cmpTo = MAX_STAT_STAGE;
        else if (cmpTo == MAX_STAT_STAGE)
            cmpTo = MIN_STAT_STAGE;
    }

    switch (cmpKind)
    {
    case CMP_EQUAL:
        if (statValue == cmpTo)
            ret = TRUE;
        break;
    case CMP_NOT_EQUAL:
        if (statValue != cmpTo)
            ret = TRUE;
        break;
    case CMP_GREATER_THAN:
        if (statValue > cmpTo)
            ret = TRUE;
        break;
    case CMP_LESS_THAN:
        if (statValue < cmpTo)
            ret = TRUE;
        break;
    case CMP_COMMON_BITS:
        if (statValue & cmpTo)
            ret = TRUE;
        break;
    case CMP_NO_COMMON_BITS:
        if (!(statValue & cmpTo))
            ret = TRUE;
        break;
    }

    return ret;
}

static void MarkStatsAsDone(struct StatChange *st, enum Stat stat)
{
    for (u32 i = 0; i < st->statStageAmount; i++)
    {
        if (st->statStageQueue[i].stat != stat && stat != NUM_BATTLE_STATS)
            continue;

        if (st->statStageQueue[i].stage < 0)
            st->statStageQueue[i].state = STAT_CHANGE_DONE;
    }
}

// The speed boost case should be removed from here but would currently create a regression. Usage now only limited to ai
bool32 CanStatChange(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (st->stage < 0)
    {
        // Special Case for speed boost since shouldn't try to lower opposing stats on speed boost
        // Also for user it might make sense to lower the stat. Regardless this whole check is better suited for CheckViability since the move wouldn't fail in this case
        if (cv->battlerAtk != st->battler && st->stat == STAT_SPEED && st->stage < 0 && cv->abilities[st->battler] == ABILITY_SPEED_BOOST)
            return FALSE;

        if (CompareStat(st->battler, st->stat, MIN_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
            return FALSE;

        if (st->stage < 0 && CanDecreaseStat(cv, st) == STAT_CHANGE_DIDNT_WORK)
            return FALSE;
    }
    else
    {
        if (CompareStat(st->battler, st->stat, MAX_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
            return FALSE;
    }

    return TRUE;
}

bool32 IsStatChangeStatusMove(enum Move move, bool32 (*isStatChange)(const struct AdditionalEffect *effect))
{
    u32 numAdditionalEffects = GetMoveAdditionalEffectCount(move);
    for (u32 i = 0; i < numAdditionalEffects; i++)
    {
        const struct AdditionalEffect *effect = GetMoveAdditionalEffectById(move, i);
        if (isStatChange(effect))
            return TRUE;
    }
    return FALSE;
}

bool32 IsAtkStatUpMove(const struct AdditionalEffect *effect)
{
    if (effect->moveEffect != STAT_CHANGE_EFFECT_PLUS)
        return FALSE;

    return effect->attack;
}

bool32 IsAtkSpAtkStatUpMove(const struct AdditionalEffect *effect)
{
    if (effect->moveEffect != STAT_CHANGE_EFFECT_PLUS)
        return FALSE;

    return effect->attack || effect->spAtk;
}

bool32 IsDefSpDefStatUpMove(const struct AdditionalEffect *effect)
{
    if (effect->moveEffect != STAT_CHANGE_EFFECT_PLUS)
        return FALSE;

    return effect->defense || effect->spDef;
}

bool32 IsAccDownEvasionUpStatChangeMove(const struct AdditionalEffect *effect)
{
    switch (effect->moveEffect)
    {
    case STAT_CHANGE_EFFECT_PLUS:
        return effect->evasion;
    case STAT_CHANGE_EFFECT_MINUS:
        return effect->accuracy;
    default:
        break;
    }

    return FALSE;
}
