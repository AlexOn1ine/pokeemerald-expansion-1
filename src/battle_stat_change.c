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

static enum StatChangeResult CanDecreaseStat(struct BattleCalcValues *cv, struct StatChange *st);
static enum StatChangeResult DecreaseStat(struct BattleCalcValues *cv, struct StatChange *st);
static enum StatChangeResult IncreaseStat(struct BattleCalcValues  *cv, struct StatChange *st);

static void AdjustStatStage(struct BattleCalcValues *cv, struct StatChange *st);
static void TryPlayStatChangeAnimation(struct BattleCalcValues *cv, struct StatChange *st);

static bool32 CanAbilityPreventStatLoss(enum Ability ability);
static bool32 AbilityPreventsSpecificStatDrop(u32 ability, u32 stat);
static bool32 GetPositiveStatStage(u32 effect);
static bool32 GetNegativeStatStage(u32 effect);
static u32 GetNumPositiveStats(enum BattlerId battler);
static u32 GetNumNegativeStats(enum BattlerId battler);

// Single stat check
bool32 CanStatChange(struct BattleCalcValues *cv, struct StatChange *st)
{
    AdjustStatStage(cv, st);

    if (st->stage < 0)
    {
        if (CompareStat(st->battler, st->stat, MIN_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
            return FALSE;
    }
    else
    {
        if (CompareStat(st->battler, st->stat, MAX_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
            return FALSE;
    }

    if (st->stage < 0 && CanDecreaseStat(cv, st) == STAT_CHANGE_DIDNT_WORK)
        return FALSE;

    return TRUE;
}

static inline bool32 TargetNotAffected(struct StatChange *st)
{
    if (!st->onlyChecking)
    {
        st->script = BattleScript_ItDoesntScrTarget;
        gBattleScripting.battler = st->battler;
    }

    return TRUE;
}

// Figure out a strat when to print failure and when not
static bool32 IsBlockedBySpecificCondition(struct BattleCalcValues *cv, struct StatChange *st)
{
    switch (GetMoveEffect(cv->move))
    {
    case EFFECT_STAT_CHANGE_ON_STATUS:
        if (!(gBattleMons[st->battler].status1 & GetMoveStatusOnStatChange(cv->move)))
            return TargetNotAffected(st);
        break;
    case EFFECT_STRENGTH_SAP:
        if (CompareStat(st->battler, st->stat, MAX_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
            return TargetNotAffected(st);
        break;
    case EFFECT_STAT_CHANGE_MAGNETIC:
        if (cv->abilities[st->battler] != ABILITY_PLUS && cv->abilities[st->battler] != ABILITY_MINUS)
            return TargetNotAffected(st);
        break;
    case EFFECT_ROTOTILLER:
        if (!IsBattlerGrounded(st->battler, cv->abilities[st->battler], cv->holdEffects[st->battler])
         || !IS_BATTLER_OF_TYPE(st->battler, TYPE_GRASS))
            return TargetNotAffected(st);
        break;

    default:
        break;
    }

    return FALSE;
}

static bool32 CanMoveMissTarget(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (st->certain || !st->checkAccuracy)
        return FALSE;

    if (!DoesMoveMissTarget(cv))
        return FALSE;

    st->targetMissed = TRUE;

    if (!st->onlyChecking)
    {
        gBattleScripting.battler = st->battler;
        gBattleCommunication[MISS_TYPE] = B_MSG_MISSED;
        gBattleStruct->moveResultFlags[st->battler] |= MOVE_RESULT_MISSED;
        st->script = BattleScript_MissedTarget;
        return TRUE;
    }

    return TRUE;
}

// Multi stat checks
bool32 CanAnyStatChange(struct BattleCalcValues *cv, struct StatChange *st)
{
    u32 numAdditionalEffects = GetMoveAdditionalEffectCount(cv->move);
    u32 additionalEffectsCounter = 0;
    u32 canAnyStatChange = FALSE;
    enum Stat counter = STAT_ATK;

    if (IsBlockedBySpecificCondition(cv, st))
        return FALSE;

    while (additionalEffectsCounter < numAdditionalEffects)
    {
        while (counter < NUM_BATTLE_STATS)
        {
            const struct AdditionalEffect *additionalEffect = GetMoveAdditionalEffectById(cv->move, additionalEffectsCounter );
            st->stat = counter++;

            if (!IsStatSet(st->stat, additionalEffect))
                continue;

            if (IsStatDecreaseEffect(additionalEffect->moveEffect))
                st->stage = GetNegativeStatStage(additionalEffect->moveEffect);
            else
                st->stage = GetPositiveStatStage(additionalEffect->moveEffect);

            SetStatChange(st->battler, st->stat, st->stage);
            AdjustStatStage(cv, st);

            if (st->stage < 0)
            {
                if (CompareStat(st->battler, st->stat, MIN_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
                    continue;
            }
            else
            {
                if (CompareStat(st->battler, st->stat, MAX_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
                    continue;
            }

            if (st->stage < 0 && CanDecreaseStat(cv, st) == STAT_CHANGE_DIDNT_WORK)
            {
                continue;
            }

            canAnyStatChange = TRUE;
        }

        counter = STAT_ATK;
        additionalEffectsCounter++;
    }

    // Ignore acc check for ai
    if (canAnyStatChange && CanMoveMissTarget(cv, st))
        return FALSE;

    return canAnyStatChange;
}

enum StatChangeResult TryStatChange(struct BattleCalcValues *cv, struct StatChange *st)
{
    for (u32 counter = 0; counter < st->statStageAmount; counter++)
    {
        if (counter + 1 == st->statStageAmount) // Avoids redundant looping
            st->nextBattler = TRUE;

        if (st->statStageQueue[counter].done)
            continue;

        st->stat = st->statStageQueue[counter].stat;
        st->stage = st->statStageQueue[counter].stage;
        st->statStageQueue[counter].done = TRUE;

        AdjustStatStage(cv, st);
        if (st->stage < 0)
        {
            if (CanDecreaseStat(cv, st) == STAT_CHANGE_DIDNT_WORK)
            {
                if (st->silentFailure)
                    continue;
                return STAT_CHANGE_BLOCKED_BY_TARGET;
            }

            if (DecreaseStat(cv, st) == STAT_CHANGE_WORKED)
            {
                return STAT_CHANGE_WORKED;
            }
        }
        else if (IncreaseStat(cv, st) == STAT_CHANGE_WORKED)
        {
            return STAT_CHANGE_WORKED;
        }
    }

    st->nextBattler = TRUE;
    return STAT_CHANGE_DIDNT_WORK;
}

enum StatChangeResult TrySingleStatChange(struct BattleCalcValues *cv, struct StatChange *st)
{
    AdjustStatStage(cv, st);

    if (st->stage < 0)
    {
        if (CanDecreaseStat(cv, st) == STAT_CHANGE_DIDNT_WORK)
            return STAT_CHANGE_DIDNT_WORK;

        if (DecreaseStat(cv, st) == STAT_CHANGE_WORKED)
            return STAT_CHANGE_WORKED;
    }
    else if (IncreaseStat(cv, st) == STAT_CHANGE_WORKED)
    {
        return STAT_CHANGE_WORKED;
    }

    return STAT_CHANGE_DIDNT_WORK;
}

// Needs to check all stats
static bool32 IsSubstituteBlocked(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (!st->nonMoveStatChange || st->certain)
        return FALSE;

    if (!gBattleMons[st->battler].volatiles.substitute)
        return FALSE;

    return TRUE;
}

static bool32 IsMistProtected(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (gSideTimers[GetBattlerSide(st->battler)].mistTimer == 0)
        return FALSE;

    if (st->certain)
        return FALSE;

    if (st->battler == cv->battlerDef && cv->abilities[cv->battlerAtk] == ABILITY_INFILTRATOR)
        return FALSE;

    if (!st->onlyChecking)
    {
        gBattleScripting.battler = st->battler;
        st->script = BattleScript_MistProtected;
    }

    return TRUE;
}

static bool32 IsFlowerVeilBlocked(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (st->certain)
        return FALSE;

    u32 flowerVeilBattler = IsFlowerVeilProtected(st->battler);
    if (!flowerVeilBattler)
        return FALSE;

    if (!st->onlyChecking)
    {
        st->script = BattleScript_FlowerVeilProtectsRet;
        gBattleScripting.battler = st->battler;
        gBattlerAbility = flowerVeilBattler - 1;
        gLastUsedAbility = ABILITY_FLOWER_VEIL;
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
        if (GetConfig(B_UPDATED_INTIMIDATE) >= GEN_8)
            st->script = BattleScript_AbilityPopUp;
        break;
    case ABILITY_GUARD_DOG: // TODO
        st->stage = -1 * st->stage; // This does not work. I need to handle the stat correctly. Invert stat and mark it as not done
        st->script = BattleScript_AbilityPopUp;
        break;
    default:
        return FALSE;
    }

    gLastUsedAbility = cv->abilities[st->battler];
    gBattlerAbility = st->battler;
    gBattleScripting.battler = st->battler;
    RecordAbilityBattle(st->battler, cv->abilities[st->battler]);
    return TRUE;
}

static bool32 IsAbilityBlocked(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (st->certain)
        return FALSE;

    if (!CanAbilityPreventStatLoss(cv->abilities[st->battler])
     && !AbilityPreventsSpecificStatDrop(cv->abilities[st->battler], st->stat))
        return FALSE;

    if (!st->onlyChecking)
    {
        st->script = BattleScript_AbilityNoStatLoss;
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
     // || cv->battlerAtk == st->battler // TODO: ???
     || st->mirrorArmored)
        return FALSE;

    // Parting Shot for tomorrow
    if (GetMoveEffect(cv->move) == EFFECT_PARTING_SHOT)
        gBattleScripting.animTargetsHit = 1;

    if (!st->onlyChecking || st->allowMirrorArmor)
    {
        if (cv->battlerAtk == st->battler)
            gBattleScripting.battler = cv->battlerDef;
        else
            gBattleScripting.battler = cv->battlerAtk;
        SetStatChange2(gBattleScripting.battler, st->stat, st->stage);
        st->script = BattleScript_MirrorArmorReflect;
        gBattlerAbility = st->battler;
        RecordAbilityBattle(st->battler, cv->abilities[st->battler]);
    }

    return TRUE;
}

static enum StatChangeResult CanDecreaseStat(struct BattleCalcValues *cv, struct StatChange *st)
{
    enum StatChangeResult result = STAT_CHANGE_WORKED;

    // if (GetMoveEffect(cv->move) == EFFECT_CURSE || cv->battlerAtk == st->battler)
    //     st->certain = TRUE;

    if (IsSubstituteBlocked(cv,st)
     || IsMistProtected(cv, st)
     || IsFlowerVeilBlocked(cv, st)
     || IsClearAmuletBlocked(cv, st)
     || IsIntimidateBlocked(cv, st)
     || IsAbilityBlocked(cv, st)
     || IsMirrorArmorReflected(cv, st))
    {
        result = STAT_CHANGE_DIDNT_WORK;
    }

    if (result == STAT_CHANGE_DIDNT_WORK && !st->onlyChecking && !st->silentFailure)
    {
        if (st->battler != cv->battlerAtk) // Don't set result flags on self
            st->statChangePrevented = TRUE;
        PREPARE_STAT_BUFFER(gBattleTextBuff1, st->stat);
    }

    return result;
}

static enum StatChangeResult DecreaseStat(struct BattleCalcValues *cv, struct StatChange *st)
{
    u32 currStage = gBattleMons[st->battler].statStages[st->stat];

    PREPARE_STAT_BUFFER(gBattleTextBuff1, st->stat);

    if (currStage == (MIN_STAT_STAGE + 1))
        st->stage = -1;
    else if (currStage == 2 && st->stage < -2)
        st->stage = -2;

    if (st->stage == -2)
    {
        PREPARE_STRING_BUFFER(gBattleTextBuff2, STRINGID_STATHARSHLY);
    }
    else if (st->stage <= -3)
    {
        PREPARE_STRING_BUFFER(gBattleTextBuff2, STRINGID_SEVERELY);
    }
    else
    {
        PREPARE_STRING_BUFFER(gBattleTextBuff2, STRINGID_EMPTYSTRING3);
    }


    if (currStage == MIN_STAT_STAGE)
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_WONT_CHANGE;
        gBattleScripting.battler = st->battler;
        st->script = BattleScript_DecreaseStatChangeMessage;
        if (st->onlyChecking)
            return STAT_CHANGE_DIDNT_WORK;
        st->statChangePrevented = TRUE;
        return STAT_CHANGE_WORKED; // Handle failure
    }
    else if (!st->onlyChecking)
    {
        gBattleMons[st->battler].volatiles.tryEjectPack = TRUE;
        gProtectStructs[st->battler].lashOutAffected = TRUE;
        gBattleScripting.statChanger |= STAT_BUFF_NEGATIVE;
    }

    if (!st->onlyChecking)
    {
        gBattleScripting.battler = st->battler;
        gBattleMons[st->battler].statStages[st->stat] += st->stage;
        if (gBattleMons[st->battler].statStages[st->stat] < MIN_STAT_STAGE)
            gBattleMons[st->battler].statStages[st->stat] = MIN_STAT_STAGE;

        if (st->itemMessage)
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_CHANGED_ITEM;
        else
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_CHANGED;

        st->script = BattleScript_DecreaseStatChangeMessage;
        TryPlayStatChangeAnimation(cv, st);
    }

    return STAT_CHANGE_WORKED;
}

static enum StatChangeResult IncreaseStat(struct BattleCalcValues *cv, struct StatChange *st)
{
    u32 currStage = gBattleMons[st->battler].statStages[st->stat];

    PREPARE_STAT_BUFFER(gBattleTextBuff1, st->stat);

    if (currStage == MAX_STAT_STAGE - 1)
        st->stage = 1;
    else if (currStage == MAX_STAT_STAGE - 2  && st->stage > 2)
        st->stage = 2;

    if (st->stage == 2)
    {
        PREPARE_STRING_BUFFER(gBattleTextBuff2, STRINGID_STATSHARPLY);
    }
    else if (st->stage >= 3)
    {
        PREPARE_STRING_BUFFER(gBattleTextBuff2, STRINGID_DRASTICALLY);
    }
    else
    {
        PREPARE_STRING_BUFFER(gBattleTextBuff2, STRINGID_EMPTYSTRING3);
    }

    if (gBattleMons[st->battler].statStages[st->stat] == MAX_STAT_STAGE)
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_WONT_CHANGE;
        st->script = BattleScript_IncreaseStatChangeMessagePause;
        gBattleScripting.battler = st->battler;
        if (st->onlyChecking)
            return STAT_CHANGE_DIDNT_WORK;
        st->statChangePrevented = TRUE;
        return STAT_CHANGE_WORKED; // Handle failure
    }
    else if (!st->onlyChecking)
    {
        u32 stageIncrease = st->stage;

        if ((st->stage + gBattleMons[st->battler].statStages[st->stat]) > MAX_STAT_STAGE)
            stageIncrease  = MAX_STAT_STAGE - gBattleMons[st->battler].statStages[st->stat];


        if (stageIncrease > 0)
        {
            // Check Mirror Herb / Opportunist
            for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
            {
                if (IsBattlerAlly(battler, st->battler))
                    continue; // Only triggers on opposing side

                if (GetBattlerAbility(battler) == ABILITY_OPPORTUNIST
                 && gProtectStructs[st->battler].activateOpportunist == 0) // don't activate opportunist on other mon's opportunist raises
                {
                    gProtectStructs[battler].activateOpportunist = 2;      // set stats to copy
                }
                if (GetBattlerHoldEffect(battler) == HOLD_EFFECT_MIRROR_HERB)
                {
                    gProtectStructs[battler].eatMirrorHerb = 1;
                }

                if (gProtectStructs[battler].activateOpportunist == 2 || gProtectStructs[battler].eatMirrorHerb == 1)
                {
                    gQueuedStatBoosts[battler].stats |= (1 << (st->stat - 1));    // -1 to start at atk
                    gQueuedStatBoosts[battler].statChanges[st->stat - 1] += stageIncrease;
                }
            }
        }
    }

    if (!st->onlyChecking)
    {
        gBattleScripting.battler = st->battler;
        gBattleMons[st->battler].statStages[st->stat] += st->stage;
        gProtectStructs[st->battler].statRaised = TRUE;
        if (gBattleMons[st->battler].statStages[st->stat] > MAX_STAT_STAGE)
            gBattleMons[st->battler].statStages[st->stat] = MAX_STAT_STAGE;

        if (st->itemMessage)
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_CHANGED_ITEM;
        else
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_CHANGED;

        st->script = BattleScript_IncreaseStatChangeMessage;
        TryPlayStatChangeAnimation(cv, st);
    }

    return STAT_CHANGE_WORKED;
}

static void TryPlayStatChangeAnimation(struct BattleCalcValues *cv, struct StatChange *st)
{
    u32 statAnimId = st->stat;

    if (st->stage <= -1) // goes down
    {
        if (gBattleStruct->negativeAnimPlayed && !st->forceAnim)
            return;

        u32 isStatChangeByTwo = abs(st->stage) > 1;
        u32 numNegativeStats = 0;
        if (!st->forceAnim)
            numNegativeStats = GetNumNegativeStats(st->battler);

        statAnimId += isStatChangeByTwo ? STAT_ANIM_MINUS2 : STAT_ANIM_MINUS1;

        if (ShouldDefiantCompetitiveActivate(st->battler, cv->abilities[st->battler]))
            numNegativeStats = 0;

        if (numNegativeStats > 1) // more than one stat, so the color is gray
            statAnimId = isStatChangeByTwo ? STAT_ANIM_MULTIPLE_MINUS2 : STAT_ANIM_MULTIPLE_MINUS1;

        if (!st->forceAnim)
            gBattleStruct->negativeAnimPlayed = TRUE;
    }
    else // goes up
    {
        if (gBattleStruct->positiveAnimPlayed && !st->forceAnim)
            return;

        u32 isStatChangeByTwo = st->stage > 1;
        u32 numPositiveStats = 0;

        if (!st->forceAnim)
            numPositiveStats = GetNumPositiveStats(st->battler);

        statAnimId += isStatChangeByTwo ? STAT_ANIM_PLUS2 : STAT_ANIM_PLUS1;

        if (ShouldDefiantCompetitiveActivate(st->battler, cv->abilities[st->battler]))
            numPositiveStats = 0;

        if (numPositiveStats > 1)
            statAnimId = isStatChangeByTwo ? STAT_ANIM_MULTIPLE_PLUS2 : STAT_ANIM_MULTIPLE_PLUS1;

        if (!st->forceAnim)
            gBattleStruct->positiveAnimPlayed = TRUE;
    }

    BtlController_EmitBattleAnimation(st->battler, B_COMM_TO_CONTROLLER, B_ANIM_STATS_CHANGE, statAnimId);
    MarkBattlerForControllerExec(st->battler);
}

static void AdjustStatStage(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (GetMoveEffect(cv->move) == EFFECT_GROWTH && IsBattlerWeatherAffected(cv->holdEffects[st->battler], GetWeather(), B_WEATHER_SUN))
        st->stage = 2;

    switch (cv->abilities[st->battler])
    {
    case ABILITY_CONTRARY:
        st->stage = st->stage * -1;
        if (!st->onlyChecking)
            RecordAbilityBattle(st->battler, cv->abilities[st->battler]);
        break;
    case ABILITY_SIMPLE:
        st->stage = st->stage * 2;
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

static bool32 AbilityPreventsSpecificStatDrop(u32 ability, u32 stat)
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

// Union is not possible
bool32 IsStatSet(u32 stat, const struct AdditionalEffect *additionalEffect)
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
    }

    return FALSE;
}

bool32 IsStatDecreaseEffect(u32 effect)
{
    return effect >= STAT_CHANGE_EFFECT_MINUS_1 && effect <= STAT_CHANGE_EFFECT_MINUS_6;
}

bool32 UNUSED IsStatIncreaseEffect(u32 effect)
{
    return effect >= STAT_CHANGE_EFFECT_PLUS_1 && effect <= STAT_CHANGE_EFFECT_PLUS_6;
}

static bool32 GetPositiveStatStage(u32 effect)
{
    return effect - STAT_CHANGE_EFFECT_PLUS_1 + 1;
}

static bool32 GetNegativeStatStage(u32 effect)
{
    return -1 * (effect - STAT_CHANGE_EFFECT_MINUS_1 + 1);
}

// TODO just the internatl queue
static u32 GetNumPositiveStats(enum BattlerId battler)
{
    u32 num = 0;
    for (u32 i = STAT_ATK; i < gSpecialStatuses[battler].statStageAmount; i++)
    {
        if (gSpecialStatuses[battler].statStageQueue[i].stage > 0)
            num++;
    }
    return num;
}

static u32 GetNumNegativeStats(enum BattlerId battler)
{
    u32 num = 0;
    for (u32 i = STAT_ATK; i < gSpecialStatuses[battler].statStageAmount; i++)
    {
        if (gSpecialStatuses[battler].statStageQueue[i].stage < 0)
            num++;
    }
    return num;
}

void SetStatChange(enum BattlerId battler, enum Stat stat, s32 stage)
{
    gSpecialStatuses[battler].statStageQueue[gSpecialStatuses[battler].statStageAmount].stat = stat;
    gSpecialStatuses[battler].statStageQueue[gSpecialStatuses[battler].statStageAmount].stage = stage;
    gSpecialStatuses[battler].statStageAmount++;
}

// Multiply stats can be boosted by a response ability
void SetStatChange2(enum BattlerId battler, enum Stat stat, s32 stage)
{
    // if (stage < 0)
    // {
    //     if (CompareStat(battler, stat, MIN_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
    //         return;
    // }
    // else
    // {
    //     if (CompareStat(battler, stat, MAX_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
    //         return;
    // }
    //
    gSpecialStatuses[battler].statStageQueue2[gSpecialStatuses[battler].statStageAmount2].stat = stat;
    gSpecialStatuses[battler].statStageQueue2[gSpecialStatuses[battler].statStageAmount2].stage = stage;
    gSpecialStatuses[battler].statStageAmount2++;
}

void ClearStatChangeValues(void)
{
    for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
    {
        memset(gSpecialStatuses[battler].statStageQueue, 0, sizeof(gSpecialStatuses[battler].statStageQueue));
        gSpecialStatuses[battler].statStageAmount = 0;
    }
    gBattleStruct->negativeAnimPlayed = 0;
    gBattleStruct->positiveAnimPlayed = 0;
    gBattleStruct->statChangeBattler  = 0;
}

void ClearOtherStatChangeValues(enum BattlerId battler)
{
    memset(gSpecialStatuses[battler].statStageQueue2, 0, sizeof(gSpecialStatuses[battler].statStageQueue2));
    gSpecialStatuses[battler].statStageAmount2 = 0;
    gBattleStruct->negativeAnimPlayed = 0;
    gBattleStruct->positiveAnimPlayed = 0;
}

// In case it turns out that we need to check this before anything else
#if 0
static bool32 IsStatAtMinStage(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (CompareStat(st->battler, st->stat, MIN_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_WONT_CHANGE;
        st->script = BattleScript_DecreaseStatChangeMessage;
        return TRUE;
    }
    return FALSE;
}

static bool32 IsStatAtMaxStage(struct BattleCalcValues *cv, struct StatChange *st)
{
    if (CompareStat(st->battler, st->stat, MAX_STAT_STAGE, CMP_EQUAL, ABILITY_NONE))
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_WONT_CHANGE;
        st->script = BattleScript_IncreaseStatChangeMessage;
        return TRUE;
    }
    return FALSE;
}
#endif

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

// OLD FUNCTIONS
u32 ChangeStatBuffs(enum BattlerId battler, s8 statValue, enum Stat statId, union StatChangeFlags flags, u32 stats, const u8 *BS_ptr) { return STAT_CHANGE_WORKED; }


