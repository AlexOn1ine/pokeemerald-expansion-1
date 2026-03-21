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

// Multi stat checks
bool32 CanAnyStatChange(struct BattleCalcValues *cv, struct StatChange *st)
{
    u32 numAdditionalEffects = GetMoveAdditionalEffectCount(cv->move);
    u32 additionalEffectsCounter = 0;
    u32 canAnyStatChange = FALSE;
    enum Stat statChangeQueueCounter= STAT_ATK;

    while (additionalEffectsCounter < numAdditionalEffects)
    {
        while (statChangeQueueCounter< NUM_STATS)
        {
            const struct AdditionalEffect *additionalEffect = GetMoveAdditionalEffectById(cv->move, additionalEffectsCounter);
            st->stat = statChangeQueueCounter++;

            if (!IsStatSet(st->stat, additionalEffect))
                continue;

            if (IsStatDecreaseEffect(additionalEffect->moveEffect))
                st->stage = GetNegativeStatStage(additionalEffect->moveEffect);
            else
                st->stage = GetPositiveStatStage(additionalEffect->moveEffect);

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
                continue;

            // SetStatChange(st->battler, st->stat, st->stage);
            canAnyStatChange = TRUE;
        }

        statChangeQueueCounter= STAT_ATK;
        additionalEffectsCounter++;
    }

    return canAnyStatChange;
}

void TryStatChange(struct BattleCalcValues *cv, struct StatChange *st)
{
    u32 numAdditionalEffects = GetMoveAdditionalEffectCount(cv->move);

    while (gBattleStruct->additionalEffectsCounter < numAdditionalEffects)
    {
        while (gBattleStruct->statChangeQueueCounter< NUM_STATS)
        {
            const struct AdditionalEffect *additionalEffect = GetMoveAdditionalEffectById(gCurrentMove, gBattleStruct->additionalEffectsCounter);
            st->stat = gBattleStruct->statChangeQueueCounter++;

            if (!IsStatSet(st->stat, additionalEffect))
                continue;

            if (IsStatDecreaseEffect(additionalEffect->moveEffect))
                st->stage = GetNegativeStatStage(additionalEffect->moveEffect);
            else
                st->stage = GetPositiveStatStage(additionalEffect->moveEffect);

            AdjustStatStage(cv, st);

            if (st->stage < 0)
            {
                if (CanDecreaseStat(cv, st) == STAT_CHANGE_DIDNT_WORK)
                    return;

                if (DecreaseStat(cv, st) == STAT_CHANGE_WORKED)
                    return;
            }
            else if (IncreaseStat(cv, st) == STAT_CHANGE_WORKED)
            {
                return;
            }
        }

        gBattleStruct->additionalEffectsCounter++;
        gBattleStruct->statChangeQueueCounter= 0;
    }

    gBattleStruct->moveResultFlags[st->battler] |= MOVE_RESULT_DOESNT_AFFECT_FOE;
    gBattleStruct->additionalEffectsCounter = 0;
    st->nextBattler = TRUE;
}

enum StatChangeResult TryNonMoveStatChange(struct BattleCalcValues *cv, struct StatChange *st)
{
    while (gBattleStruct->statChangeQueueCounter < gSpecialStatuses[st->battler].statStageAmount)
    {
        u32 count = gBattleStruct->statChangeQueueCounter++;
        st->stat = gSpecialStatuses[st->battler].statStageQueue[count].stat;
        st->stage = gSpecialStatuses[st->battler].statStageQueue[count].stage;

        AdjustStatStage(cv, st);

        if (st->stage < 0)
        {
            if (CanDecreaseStat(cv, st) == STAT_CHANGE_DIDNT_WORK)
            {
                if (st->silentFailure || st->onlyChecking)
                    continue;
                return STAT_CHANGE_BLOCKED_BY_TARGET;
            }

            if (DecreaseStat(cv, st) == STAT_CHANGE_WORKED)
                return STAT_CHANGE_WORKED;
        }
        else if (IncreaseStat(cv, st) == STAT_CHANGE_WORKED)
        {
            return STAT_CHANGE_WORKED;
        }
    }

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

static enum StatChangeResult CanDecreaseStat(struct BattleCalcValues *cv, struct StatChange *st)
{
    enum StatChangeResult result = STAT_CHANGE_WORKED;

    u32 flowerVeilBattler = 0;

    // if (GetMoveEffect(cv->move) == EFFECT_CURSE || cv->battlerAtk == st->battler)
    //     st->certain = TRUE;

    enum Ability abilityAtk = cv->abilities[cv->battlerAtk];
    enum Ability ability = cv->abilities[st->battler];

    if (GetMoveEffect(cv->move) == EFFECT_STAT_CHANGE_ON_STATUS && gBattleMons[st->battler].status1 & GetMoveStatusOnStatChange(cv->move))
    {
        // TODO
        result = STAT_CHANGE_DIDNT_WORK;
    }
    if (st->nonMoveStatChange && !st->certain && gBattleMons[st->battler].volatiles.substitute)
    {
        // TODO
        result = STAT_CHANGE_DIDNT_WORK;
    }
    else if (gSideTimers[GetBattlerSide(st->battler)].mistTimer
     && !st->certain
     && abilityAtk != ABILITY_INFILTRATOR)
    {
        if (!st->onlyChecking)
            st->script = BattleScript_MistProtected;

        result = STAT_CHANGE_DIDNT_WORK;
    }
    else if (!st->certain && CanAbilityPreventStatLoss(ability))
    {
        if (!st->onlyChecking)
        {
            st->script = BattleScript_AbilityNoStatLoss;
            gLastUsedAbility = ability;
            RecordAbilityBattle(st->battler, ability);
        }

        result = STAT_CHANGE_DIDNT_WORK;
    }
    else if (!st->certain && cv->holdEffects[st->battler] == HOLD_EFFECT_CLEAR_AMULET)
    {
        if (!st->onlyChecking)
        {
            st->script = BattleScript_ItemNoStatLoss;
            gLastUsedItem = gBattleMons[st->battler].item;
            RecordItemEffectBattle(st->battler, HOLD_EFFECT_CLEAR_AMULET);
        }

        result = STAT_CHANGE_DIDNT_WORK;
    }
    else if (!st->certain && (flowerVeilBattler = IsFlowerVeilProtected(st->battler)))
    {
        if (!st->onlyChecking)
        {
            st->script = BattleScript_FlowerVeilProtectsRet;
            gBattlerAbility = flowerVeilBattler - 1;
            gLastUsedAbility = ABILITY_FLOWER_VEIL;
            RecordAbilityBattle(gBattlerAbility, ABILITY_FLOWER_VEIL);
        }

        result = STAT_CHANGE_DIDNT_WORK;
    }
    else if (!st->certain && AbilityPreventsSpecificStatDrop(ability, st->stat))
    {
        if (!st->onlyChecking)
        {
            st->script = BattleScript_AbilityNoSpecificStatLoss;
            gBattlerAbility = st->battler;
            gLastUsedAbility = ability;
            RecordAbilityBattle(st->battler, gLastUsedAbility);
        }

        result = STAT_CHANGE_DIDNT_WORK;
    }
    else if (ability == ABILITY_MIRROR_ARMOR && cv->battlerAtk != st->battler && !st->mirrorArmored)
    {
        if (GetMoveEffect(cv->move) == EFFECT_PARTING_SHOT)
            gBattleScripting.animTargetsHit = 1;

        if (!st->onlyChecking || st->allowMirrorArmor)
        {
            SetStatChange(cv->battlerAtk, st->stat, st->stage);
            st->script = BattleScript_MirrorArmorReflect;
            st->silentFailure = FALSE;
            gBattlerAbility = st->battler;
            RecordAbilityBattle(st->battler, ability);
        }

        result = STAT_CHANGE_DIDNT_WORK;
    }

    if (result == STAT_CHANGE_DIDNT_WORK && !st->onlyChecking && !st->silentFailure)
    {
        st->statChangePrevented = TRUE;
        PREPARE_STAT_BUFFER(gBattleTextBuff1, st->stat);
        gBattleScripting.battler = st->battler;
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

    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_CHANGED;

    if (currStage == MIN_STAT_STAGE)
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_WONT_CHANGE;
        gBattleScripting.statChanger |= STAT_BUFF_NEGATIVE;
        if (st->silentFailure)
            return STAT_CHANGE_WORKED;
        return STAT_CHANGE_DIDNT_WORK;
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

    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_CHANGED;

    if (gBattleMons[st->battler].statStages[st->stat] == MAX_STAT_STAGE)
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_WONT_CHANGE;
        gBattleScripting.statChanger &= ~STAT_BUFF_NEGATIVE;
        if (st->silentFailure)
            return STAT_CHANGE_WORKED;
        return STAT_CHANGE_DIDNT_WORK;
    }
    else if (!st->onlyChecking)
    {
        u32 statIncrease = st->stat;

        if ((st->stage + gBattleMons[st->battler].statStages[st->stat]) > MAX_STAT_STAGE)
            statIncrease = MAX_STAT_STAGE - gBattleMons[st->battler].statStages[st->stat];

        gProtectStructs[st->battler].statRaised = TRUE;
        gBattleScripting.statChanger &= ~STAT_BUFF_NEGATIVE;

        if (statIncrease)
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
                    gQueuedStatBoosts[battler].statChanges[st->stat - 1] += statIncrease;
                }
            }
        }
    }

    if (!st->onlyChecking)
    {
        gBattleScripting.battler = st->battler;
        gBattleMons[st->battler].statStages[st->stat] += st->stage;
        if (gBattleMons[st->battler].statStages[st->stat] > MAX_STAT_STAGE)
            gBattleMons[st->battler].statStages[st->stat] = MAX_STAT_STAGE;
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
        break;
    }
    return FALSE;
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


