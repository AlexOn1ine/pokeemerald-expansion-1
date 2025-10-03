#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_ai_util.h" // maybe move some stuff over to battle.h
#include "battle_controllers.h"
#include "battle_util.h"
#include "battle_hold_effects.h"
#include "battle_scripts.h"
#include "string_util.h"
#include "data/hold_effects.h"

bool32 IsOnSwitchInActivation(enum HoldEffect holdEffect)          { return gHoldEffectsInfo[holdEffect].onSwitchIn; }
bool32 IsOnSwitchInFirstTurnActivation(enum HoldEffect holdEffect) { return gHoldEffectsInfo[holdEffect].onSwitchInFirstTurn; }
bool32 IsMirrorHerbActivation(enum HoldEffect holdEffect)          { return gHoldEffectsInfo[holdEffect].mirrorHerb; }
bool32 IsMirrorHerbFirstTurnActivation(enum HoldEffect holdEffect) { return gHoldEffectsInfo[holdEffect].mirrorHerbFirstTurn; }
bool32 IsWhiteHerbActivation(enum HoldEffect holdEffect)           { return gHoldEffectsInfo[holdEffect].whiteHerb; }
bool32 IsWhiteHerbFirstTurnActivation(enum HoldEffect holdEffect)  { return gHoldEffectsInfo[holdEffect].whiteHerbFirstTurn; }
bool32 IsWhiteHerbEndTurnActivation(enum HoldEffect holdEffect)    { return gHoldEffectsInfo[holdEffect].whiteHerbEndTurn; }
bool32 IsHealStatusActivation(enum HoldEffect holdEffect)          { return gHoldEffectsInfo[holdEffect].healStatus; }
bool32 IsBerryRestoreHpActivation(enum HoldEffect holdEffect)      { return gHoldEffectsInfo[holdEffect].berryRestoreHp; }
bool32 IsMagicRoomEndsActivation(enum HoldEffect holdEffect)       { return gHoldEffectsInfo[holdEffect].magicRoomEnds; }
bool32 IsBugBiteEatsActivation(enum HoldEffect holdEffect)         { return gHoldEffectsInfo[holdEffect].bugBiteEats; }
bool32 IsKeeMarangaBerryActivation(enum HoldEffect holdEffect)     { return gHoldEffectsInfo[holdEffect].keeMarangaBerry; }
bool32 IsMentalHerbActivation(enum HoldEffect holdEffect)          { return gHoldEffectsInfo[holdEffect].MentalHerb; }
bool32 IsOnTargetHitActivation(enum HoldEffect holdEffect)         { return gHoldEffectsInfo[holdEffect].onTargetAfterHit; }
bool32 IsOnAttackerAfterHitActivation(enum HoldEffect holdEffect)  { return gHoldEffectsInfo[holdEffect].onAttackerAfterHit; }
bool32 IsAfterMoveActivation(enum HoldEffect holdEffect)           { return gHoldEffectsInfo[holdEffect].afterMove; }
bool32 IsKingsRockActivation(enum HoldEffect holdEffect)           { return gHoldEffectsInfo[holdEffect].kingsRock; }
bool32 IsLifeOrbShellBellActivation(enum HoldEffect holdEffect)    { return gHoldEffectsInfo[holdEffect].lifeOrbShellBell; }
bool32 IsTryHealingActivation(enum HoldEffect holdEffect)          { return gHoldEffectsInfo[holdEffect].tryHealing; }
bool32 IsConsumeBerryActivation(enum HoldEffect holdEffect)        { return gHoldEffectsInfo[holdEffect].consumeBerry; }
bool32 IsLeftoversActivation(enum HoldEffect holdEffect)           { return gHoldEffectsInfo[holdEffect].leftovers; }
bool32 IsOrbsActivation(enum HoldEffect holdEffect)                { return gHoldEffectsInfo[holdEffect].orbs; }
bool32 IsNormalActivation(enum HoldEffect holdEffect)              { return gHoldEffectsInfo[holdEffect].normal; }
bool32 IsOnEffectActivation(enum HoldEffect holdEffect)            { return gHoldEffectsInfo[holdEffect].onEffect; }
bool32 IsActivationForceTriggered(enum HoldEffect holdEffect)      { return TRUE; }

static enum ItemEffect TryDoublePrize(u32 battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsOnPlayerSide(battler) && !gBattleStruct->moneyMultiplierItem)
    {
        gBattleStruct->moneyMultiplier *= 2;
        gBattleStruct->moneyMultiplierItem = TRUE;
    }

    return effect;
}

enum ItemEffect TryBoosterEnergy(u32 battler, enum Ability ability, ActivationTiming timing)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gDisableStructs[battler].boosterEnergyActivated || gBattleMons[battler].volatiles.transformed)
        return ITEM_NO_EFFECT;

    if (((ability == ABILITY_PROTOSYNTHESIS) && !((gBattleWeather & B_WEATHER_SUN) && HasWeatherEffect()))
     || ((ability == ABILITY_QUARK_DRIVE) && !(gFieldStatuses & STATUS_FIELD_ELECTRIC_TERRAIN)))
    {
        PREPARE_STAT_BUFFER(gBattleTextBuff1, GetHighestStatId(battler));
        gBattlerAbility = gBattleScripting.battler = battler;
        gDisableStructs[battler].boosterEnergyActivated = TRUE;
        gLastUsedItem = ITEM_BOOSTER_ENERGY;
        RecordAbilityBattle(battler, ability);
        if (timing == IsOnSwitchInFirstTurnActivation)
            BattleScriptExecute(BattleScript_BoosterEnergyEnd2);
        else
            BattleScriptCall(BattleScript_BoosterEnergyRet);
        effect = ITEM_EFFECT_OTHER;
    }

    return effect;
}

static enum ItemEffect TryRoomService(u32 battler, ActivationTiming timing)
{
    if (gFieldStatuses & STATUS_FIELD_TRICK_ROOM && CompareStat(battler, STAT_SPEED, MIN_STAT_STAGE, CMP_GREATER_THAN))
    {
        gEffectBattler = gBattleScripting.battler = battler;
        SET_STATCHANGER(STAT_SPEED, 1, TRUE);
        gBattleScripting.animArg1 = STAT_ANIM_PLUS1 + STAT_SPEED;
        gBattleScripting.animArg2 = 0;
        gLastUsedItem = gBattleMons[battler].item;

        if (timing == IsOnSwitchInFirstTurnActivation)
            BattleScriptExecute(BattleScript_ConsumableStatRaiseEnd2);
        else
            BattleScriptCall(BattleScript_ConsumableStatRaiseRet);

        return ITEM_STATS_CHANGE;
    }

    return ITEM_NO_EFFECT;
}

// TODO: TimingsW
static enum ItemEffect TryTerrainSeeds(u32 battler, u32 item, ActivationTiming timing)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    switch (GetBattlerHoldEffectParam(battler))
    {
    case HOLD_EFFECT_PARAM_ELECTRIC_TERRAIN:
        effect = TryHandleSeed(battler, STATUS_FIELD_ELECTRIC_TERRAIN, STAT_DEF, item, ITEMEFFECT_ON_SWITCH_IN_FIRST_TURN);
        break;
    case HOLD_EFFECT_PARAM_GRASSY_TERRAIN:
        effect = TryHandleSeed(battler, STATUS_FIELD_GRASSY_TERRAIN, STAT_DEF, item, ITEMEFFECT_ON_SWITCH_IN_FIRST_TURN);
        break;
    case HOLD_EFFECT_PARAM_MISTY_TERRAIN:
        effect = TryHandleSeed(battler, STATUS_FIELD_MISTY_TERRAIN, STAT_SPDEF, item, ITEMEFFECT_ON_SWITCH_IN_FIRST_TURN);
        break;
    case HOLD_EFFECT_PARAM_PSYCHIC_TERRAIN:
        effect = TryHandleSeed(battler, STATUS_FIELD_PSYCHIC_TERRAIN, STAT_SPDEF, item, ITEMEFFECT_ON_SWITCH_IN_FIRST_TURN);
        break;
    }

    return effect;
}

static bool32 CanBeInfinitelyConfused(u32 battler)
{
    enum Ability ability = GetBattlerAbility(battler);
    if  (ability == ABILITY_OWN_TEMPO
      || IsBattlerTerrainAffected(battler, ability, GetBattlerHoldEffect(battler), STATUS_FIELD_MISTY_TERRAIN)
      || gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_SAFEGUARD)
        return FALSE;
    return TRUE;
}

// TODO: Fix the berserk gene script
static enum ItemEffect TryBerserkGene(u32 battler, ActivationTiming timing)
{
    if (CanBeInfinitelyConfused(battler))
        gBattleMons[battler].volatiles.infiniteConfusion = TRUE;

    gBattlerAttacker = gEffectBattler = battler; // use scripting battler
    SET_STATCHANGER(STAT_ATK, 2, FALSE);
    gBattleScripting.animArg1 = STAT_ANIM_PLUS1 + STAT_ATK;
    gBattleScripting.animArg2 = 0;
        if (timing == IsOnSwitchInFirstTurnActivation)
        BattleScriptExecute(BattleScript_BerserkGeneRetEnd2);
    else
        BattleScriptCall(BattleScript_BerserkGeneRet);

    return ITEM_STATS_CHANGE;
}

static enum ItemEffect RestoreWhiteHerbStats(u32 battler, ActivationTiming timing)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    for (u32 i = 0; i < NUM_BATTLE_STATS; i++)
    {
        if (gBattleMons[battler].statStages[i] < DEFAULT_STAT_STAGE)
        {
            gBattleMons[battler].statStages[i] = DEFAULT_STAT_STAGE;
            effect = ITEM_STATS_CHANGE;
        }
    }
    if (effect != ITEM_NO_EFFECT)
    {
        if (timing == IsWhiteHerbActivation)
            BattleScriptCall(BattleScript_WhiteHerbRet);
        else
            BattleScriptExecute(BattleScript_WhiteHerbEnd2);
    }

    return effect;
}

static enum ItemEffect TryConsumeMirrorHerb(u32 battler, ActivationTiming timing)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gProtectStructs[battler].eatMirrorHerb)
    {
        gProtectStructs[battler].eatMirrorHerb = 0;
        ChooseStatBoostAnimation(battler);
        if (timing == IsMirrorHerbFirstTurnActivation)
            BattleScriptExecute(BattleScript_MirrorHerbCopyStatChangeEnd2);
        else
            BattleScriptCall(BattleScript_MirrorHerbCopyStatChange);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryKingsRock(u32 battlerAtk, u32 battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    enum Ability ability = GetBattlerAbility(battlerAtk);
    u32 holdEffectParam = GetBattlerHoldEffectParam(battlerAtk);

    if (B_SERENE_GRACE_BOOST >= GEN_5 && ability == ABILITY_SERENE_GRACE)
        holdEffectParam *= 2;
    if (gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_RAINBOW && gCurrentMove != MOVE_SECRET_POWER)
        holdEffectParam *= 2;
    if (IsBattlerTurnDamaged(battlerDef)
        && !MoveIgnoresKingsRock(gCurrentMove)
        && IsBattlerAlive(battlerDef)
        && RandomPercentage(RNG_HOLD_EFFECT_FLINCH, holdEffectParam)
        && ability != ABILITY_STENCH)
    {
        gBattleScripting.moveEffect = MOVE_EFFECT_FLINCH;
        BattleScriptPushCursor();
        SetMoveEffect(battlerAtk, battlerDef, FALSE, FALSE);
        BattleScriptPop();
        effect = ITEM_EFFECT_OTHER;
    }

    return effect;
}

static enum ItemEffect TryAirBallon(u32 battler, ActivationTiming timing)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (timing == IsOnTargetHitActivation)
    {
        if (IsBattlerTurnDamaged(battler))
        {
            BattleScriptCall(BattleScript_AirBaloonMsgPop);
            effect = ITEM_EFFECT_OTHER;
        }
    }
    else
    {
        BattleScriptPushCursorAndCallback(BattleScript_AirBaloonMsgIn);
        RecordItemEffectBattle(battler, HOLD_EFFECT_AIR_BALLOON);
        effect = ITEM_EFFECT_OTHER;
    }

    return effect;
}

static enum ItemEffect TryRockyHelmet(u32 battlerDef, u32 battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerTurnDamaged(battlerDef)
        && !CanBattlerAvoidContactEffects(battlerAtk, battlerDef, GetBattlerAbility(battlerAtk), GetBattlerHoldEffect(battlerAtk), gCurrentMove)
        && IsBattlerAlive(battlerAtk)
        && !IsAbilityAndRecord(battlerAtk, GetBattlerAbility(battlerAtk), ABILITY_MAGIC_GUARD))
    {
        gBattleStruct->moveDamage[battlerAtk] = GetNonDynamaxMaxHP(battlerAtk) / 6;
        if (gBattleStruct->moveDamage[battlerAtk] == 0)
            gBattleStruct->moveDamage[battlerAtk] = 1;
        PREPARE_ITEM_BUFFER(gBattleTextBuff1, gBattleMons[battlerDef].item); // TODO: Handle everything through last used item
        BattleScriptCall(BattleScript_RockyHelmetActivates);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryWeaknessPolicy(u32 battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerAlive(battlerDef)
        && IsBattlerTurnDamaged(battlerDef)
        && gBattleStruct->moveResultFlags[battlerDef] & MOVE_RESULT_SUPER_EFFECTIVE)
    {
        BattleScriptCall(BattleScript_WeaknessPolicy);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TrySnowball(u32 battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerAlive(battlerDef)
        && IsBattlerTurnDamaged(battlerDef)
        && GetBattleMoveType(gCurrentMove) == TYPE_ICE)
    {
        BattleScriptCall(BattleScript_TargetItemStatRaise);
        SET_STATCHANGER(STAT_ATK, 1, FALSE);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryLuminousMoss(u32 battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerAlive(battlerDef)
        && IsBattlerTurnDamaged(battlerDef)
        && GetBattleMoveType(gCurrentMove) == TYPE_WATER)
    {
        BattleScriptCall(BattleScript_TargetItemStatRaise);
        SET_STATCHANGER(STAT_SPDEF, 1, FALSE);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryCellBattery(u32 battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerAlive(battlerDef)
        && IsBattlerTurnDamaged(battlerDef)
        && GetBattleMoveType(gCurrentMove) == TYPE_ELECTRIC)
    {
        BattleScriptCall(BattleScript_TargetItemStatRaise);
        SET_STATCHANGER(STAT_ATK, 1, FALSE);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryAbsorbBulb(u32 battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerAlive(battlerDef)
        && IsBattlerTurnDamaged(battlerDef)
        && GetBattleMoveType(gCurrentMove) == TYPE_WATER)
    {
        effect = ITEM_STATS_CHANGE;
        BattleScriptCall(BattleScript_TargetItemStatRaise);
        SET_STATCHANGER(STAT_SPATK, 1, FALSE);
    }

    return effect;
}

static enum ItemEffect TryJabocaBerry(u32 battlerDef, u32 battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerAlive(battlerAtk)
     && IsBattlerTurnDamaged(battlerDef)
     && !DoesSubstituteBlockMove(battlerAtk, battlerDef, gCurrentMove)
     && IsBattleMovePhysical(gCurrentMove)
     && !IsAbilityAndRecord(battlerAtk, GetBattlerAbility(battlerAtk), ABILITY_MAGIC_GUARD))
    {
        gBattleStruct->moveDamage[battlerAtk] = GetNonDynamaxMaxHP(battlerAtk) / 8;
        if (gBattleStruct->moveDamage[battlerAtk] == 0)
            gBattleStruct->moveDamage[battlerAtk] = 1;
        if (GetBattlerAbility(battlerDef) == ABILITY_RIPEN)
            gBattleStruct->moveDamage[gBattlerAttacker] *= 2;

        BattleScriptCall(BattleScript_JabocaRowapBerryActivates);
        PREPARE_ITEM_BUFFER(gBattleTextBuff1, gBattleMons[battlerDef].item);  // TODO: handle through last used item
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryRowapBerry(u32 battlerAtk, u32 battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerAlive(battlerAtk)
     && IsBattlerTurnDamaged(battlerDef)
     && !DoesSubstituteBlockMove(battlerAtk, battlerDef, gCurrentMove)
     && IsBattleMoveSpecial(gCurrentMove)
     && !IsAbilityAndRecord(battlerAtk, GetBattlerAbility(battlerAtk), ABILITY_MAGIC_GUARD))
    {
        gBattleStruct->moveDamage[battlerAtk] = GetNonDynamaxMaxHP(battlerAtk) / 8;
        if (gBattleStruct->moveDamage[battlerAtk] == 0)
            gBattleStruct->moveDamage[battlerAtk] = 1;
        if (GetBattlerAbility(battlerDef) == ABILITY_RIPEN)
            gBattleStruct->moveDamage[battlerAtk] *= 2;

        BattleScriptCall(BattleScript_JabocaRowapBerryActivates);
        PREPARE_ITEM_BUFFER(gBattleTextBuff1, gBattleMons[battlerDef].item); // TODO: handle through last used item
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryBlunderPolicy(u32 battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleStruct->blunderPolicy
     && IsBattlerAlive(battlerAtk)
     && CompareStat(battlerAtk, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
    {
        gBattleStruct->blunderPolicy = FALSE;
        SET_STATCHANGER(STAT_SPEED, 2, FALSE);
        BattleScriptCall(BattleScript_AttackerItemStatRaise);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryMentalHerb(u32 battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    // Check infatuation
    if (gBattleMons[battler].volatiles.infatuation)
    {
        gBattleMons[battler].volatiles.infatuation = 0;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_INFATUATION;  // STRINGID_TARGETGOTOVERINFATUATION
        StringCopy(gBattleTextBuff1, gStatusConditionString_LoveJpn);
        effect = ITEM_EFFECT_OTHER;
    }
    if (B_MENTAL_HERB >= GEN_5)
    {
        // Check taunt
        if (gDisableStructs[battler].tauntTimer != 0)
        {
            gDisableStructs[battler].tauntTimer = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_TAUNT;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_TAUNT);
            effect = ITEM_EFFECT_OTHER;
        }
        // Check encore
        if (gDisableStructs[battler].encoreTimer != 0)
        {
            gDisableStructs[battler].encoredMove = 0;
            gDisableStructs[battler].encoreTimer = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_ENCORE;   // STRINGID_PKMNENCOREENDED
            effect = ITEM_EFFECT_OTHER;
        }
        // Check torment
        if (gBattleMons[battler].volatiles.torment == TRUE)
        {
            gBattleMons[battler].volatiles.torment = FALSE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_TORMENT;
            effect = ITEM_EFFECT_OTHER;
        }
        // Check heal block
        if (gBattleMons[battler].volatiles.healBlock)
        {
            gBattleMons[battler].volatiles.healBlock = FALSE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_HEALBLOCK;
            effect = ITEM_EFFECT_OTHER;
        }
        // Check disable
        if (gDisableStructs[battler].disableTimer != 0)
        {
            gDisableStructs[battler].disableTimer = 0;
            gDisableStructs[battler].disabledMove = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_DISABLE;
            effect = ITEM_EFFECT_OTHER;
        }
    }

    if (effect)
    {
        gBattleScripting.savedBattler = gBattlerAttacker;
        gBattlerAttacker = battler; // Needs to be fixed
        BattleScriptCall(BattleScript_MentalHerbCureRet);
    }

    return effect;
}

// TODO: Write test for multi hit
static enum ItemEffect TryThroatSray(u32 battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsSoundMove(gCurrentMove)
     && gMultiHitCounter == 0
     && IsBattlerAlive(battlerAtk)
     && IsAnyTargetTurnDamaged(battlerAtk)
     && CompareStat(battlerAtk, STAT_SPATK, MAX_STAT_STAGE, CMP_LESS_THAN)
     && !NoAliveMonsForEitherParty())   // Don't activate if battle will end
    {
        SET_STATCHANGER(STAT_SPATK, 1, FALSE);
        BattleScriptCall(BattleScript_AttackerItemStatRaise);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect DamagedStatBoostBerryEffect(u32 battlerDef, u32 battlerAtk, u32 statId, enum DamageCategory category)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (!IsBattlerAlive(battlerDef) || !CompareStat(battlerDef, statId, MAX_STAT_STAGE, CMP_LESS_THAN))
        return effect;

    if (gBattleScripting.overrideBerryRequirements
        || (!DoesSubstituteBlockMove(battlerAtk, battlerDef, gCurrentMove)
            && GetBattleMoveCategory(gCurrentMove) == category
            && IsBattlerTurnDamaged(battlerDef)))
    {
        gEffectBattler = battlerDef; // TODO: remove effect battler
        if (GetBattlerAbility(battlerDef) == ABILITY_RIPEN)
            SET_STATCHANGER(statId, 2, FALSE);
        else
            SET_STATCHANGER(statId, 1, FALSE);

        gBattleScripting.animArg1 = STAT_ANIM_PLUS1 + statId;
        gBattleScripting.animArg2 = 0;
        BattleScriptCall(BattleScript_ConsumableStatRaiseRet);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryShellBell(u32 battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleScripting.savedDmg > 0
        && !(gHitMarker & HITMARKER_UNABLE_TO_USE_MOVE)
        && !(gBattleStruct->moveResultFlags[gBattlerTarget] & MOVE_RESULT_NO_EFFECT) // TODO:
        && gBattlerAttacker != gBattlerTarget // TODO
        && !IsBattlerAtMaxHp(battlerAtk)
        && IsBattlerAlive(battlerAtk)
        && GetMoveEffect(gCurrentMove) != EFFECT_FUTURE_SIGHT
        && GetMoveEffect(gCurrentMove) != EFFECT_PAIN_SPLIT
        && (B_HEAL_BLOCKING < GEN_5 || !gBattleMons[battlerAtk].volatiles.healBlock))
    {
        gBattleStruct->moveDamage[battlerAtk] = (gBattleScripting.savedDmg / GetBattlerHoldEffectParam(battlerAtk)) * -1;
        if (gBattleStruct->moveDamage[battlerAtk] == 0)
            gBattleStruct->moveDamage[battlerAtk] = -1;
        BattleScriptCall(BattleScript_ItemHealHP_Ret);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryLifeOrb(u32 battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerAlive(battlerAtk)
        && !(gHitMarker & HITMARKER_UNABLE_TO_USE_MOVE)
        && (IsBattlerTurnDamaged(gBattlerTarget) || gBattleScripting.savedDmg > 0) // TODO
        && !IsAbilityAndRecord(battlerAtk, GetBattlerAbility(battlerAtk), ABILITY_MAGIC_GUARD)
        && !IsFutureSightAttackerInParty(battlerAtk, gBattlerTarget, gCurrentMove))
    {
        gBattleStruct->moveDamage[battlerAtk] = GetNonDynamaxMaxHP(battlerAtk) / 10;
        if (gBattleStruct->moveDamage[battlerAtk] == 0)
            gBattleStruct->moveDamage[battlerAtk] = 1;
        BattleScriptCall(BattleScript_ItemHurtRet);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryStickyBarbOnTargetHit(u32 battlerDef, u32 battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerTurnDamaged(battlerDef)
       && !CanBattlerAvoidContactEffects(battlerAtk, battlerDef, GetBattlerAbility(battlerAtk), GetBattlerHoldEffect(battlerAtk), gCurrentMove)
       && !DoesSubstituteBlockMove(battlerAtk, battlerDef, gCurrentMove)
       && IsBattlerAlive(battlerAtk)
       && CanStealItem(battlerAtk, battlerDef, gBattleMons[battlerDef].item)
       && gBattleMons[battlerAtk].item == ITEM_NONE)
    {
        // No sticky hold checks.
        gEffectBattler = battlerDef; // TODO: change to scripting battler  gEffectBattler = target
        StealTargetItem(battlerAtk, battlerDef);  // Attacker takes target's barb
        BattleScriptCall(BattleScript_StickyBarbTransfer);
        effect = ITEM_EFFECT_OTHER;
    }

    return effect;
}

static enum ItemEffect TryStickyBarbOnEndTurn(u32 battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (!IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
    {
        gBattleStruct->moveDamage[battler] = GetNonDynamaxMaxHP(battler) / 8;
        if (gBattleStruct->moveDamage[battler] == 0)
            gBattleStruct->moveDamage[battler] = 1;
        PREPARE_ITEM_BUFFER(gBattleTextBuff1, gBattleMons[battler].item);
        BattleScriptExecute(BattleScript_ItemHurtEnd2);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryToxicOrb(u32 battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    enum Ability ability = GetBattlerAbility(battler);

    if (CanBePoisoned(battler, battler, ability, ability)) // Can corrosion trigger toxic orb on itself?
    {
        gBattleMons[battler].status1 = STATUS1_TOXIC_POISON;
        BattleScriptExecute(BattleScript_ToxicOrb);
        effect = ITEM_STATUS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryFlameOrb(u32 battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    enum Ability ability = GetBattlerAbility(battler);

    if (CanBeBurned(battler, battler, ability))
    {
        gBattleMons[battler].status1 = STATUS1_BURN;
        BattleScriptExecute(BattleScript_FlameOrb);
        effect = ITEM_STATUS_CHANGE;
    }

    return effect;
}


static enum ItemEffect TryLeftovers(u32 battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleMons[battler].hp < gBattleMons[battler].maxHP
      && (B_HEAL_BLOCKING < GEN_5 || !gBattleMons[battler].volatiles.healBlock))
    {
        gBattleStruct->moveDamage[battler] = GetNonDynamaxMaxHP(battler) / 16;
        if (gBattleStruct->moveDamage[battler] == 0)
            gBattleStruct->moveDamage[battler] = 1;
        gBattleStruct->moveDamage[battler] *= -1;
        BattleScriptExecute(BattleScript_ItemHealHP_End2);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryBlackSludge(u32 battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (!IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
    {
        gBattleStruct->moveDamage[battler] = GetNonDynamaxMaxHP(battler) / 8;
        if (gBattleStruct->moveDamage[battler] == 0)
            gBattleStruct->moveDamage[battler] = 1;
        PREPARE_ITEM_BUFFER(gBattleTextBuff1, gBattleMons[battler].item);
        BattleScriptExecute(BattleScript_ItemHurtEnd2);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

/**
 *  args:
 *  - primaryBattler: battler that holds the item
 *  - secondaryBattler: battler that can be affected by the item (not always used)
 *  - holdEffect: the hold effect of the item
 *  - timing: state from where the item is used, set in src/data/hold_effects.h
 */
enum ItemEffect ItemBattleEffects(u32 primaryBattler, u32 secondaryBattler, enum HoldEffect holdEffect, ActivationTiming timing)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    u32 item = gBattleMons[primaryBattler].item;

    if (!timing(holdEffect))
        return effect;

    if (IsUnnerveBlocked(primaryBattler, item))
        return effect;

    if (!IsBattlerAlive(primaryBattler)
     && holdEffect != HOLD_EFFECT_ROWAP_BERRY // TODO: hacky workaround for them right now
     && holdEffect != HOLD_EFFECT_ROCKY_HELMET)
        return effect;

    switch (holdEffect)
    {
    case HOLD_EFFECT_DOUBLE_PRIZE:
        effect = TryDoublePrize(primaryBattler);
        break;
    case HOLD_EFFECT_ROOM_SERVICE:
        effect = TryRoomService(primaryBattler, timing);
        break;
    case HOLD_EFFECT_TERRAIN_SEED:
        effect = TryTerrainSeeds(primaryBattler, item, timing);
        break;
    case HOLD_EFFECT_BERSERK_GENE:
        effect = TryBerserkGene(primaryBattler, timing);
        break;
    case HOLD_EFFECT_BOOSTER_ENERGY:
        effect = TryBoosterEnergy(primaryBattler, GetBattlerAbility(primaryBattler), timing);
        break;
    case HOLD_EFFECT_WHITE_HERB:
        effect = RestoreWhiteHerbStats(primaryBattler, timing);
        break;
    case HOLD_EFFECT_MIRROR_HERB:
        effect = TryConsumeMirrorHerb(primaryBattler, timing);
        break;
    case HOLD_EFFECT_FLINCH:
        effect = TryKingsRock(primaryBattler, secondaryBattler);
        break;
    case HOLD_EFFECT_AIR_BALLOON:
        effect = TryAirBallon(primaryBattler, timing);
        break;
    case HOLD_EFFECT_ROCKY_HELMET:
        effect = TryRockyHelmet(primaryBattler, secondaryBattler);
        break;
    case HOLD_EFFECT_WEAKNESS_POLICY:
        effect = TryWeaknessPolicy(primaryBattler);
        break;
    case HOLD_EFFECT_SNOWBALL:
        effect = TrySnowball(primaryBattler);
        break;
    case HOLD_EFFECT_LUMINOUS_MOSS:
        effect = TryLuminousMoss(primaryBattler);
        break;
    case HOLD_EFFECT_CELL_BATTERY:
        effect = TryCellBattery(primaryBattler);
        break;
    case HOLD_EFFECT_ABSORB_BULB:
        effect = TryAbsorbBulb(primaryBattler);
        break;
    case HOLD_EFFECT_JABOCA_BERRY:
        effect = TryJabocaBerry(primaryBattler, secondaryBattler);
        break;
    case HOLD_EFFECT_ROWAP_BERRY:
        effect = TryRowapBerry(primaryBattler, secondaryBattler);
        break;
    case HOLD_EFFECT_BLUNDER_POLICY:
        effect = TryBlunderPolicy(primaryBattler);
        break;
    case HOLD_EFFECT_MENTAL_HERB:
        effect = TryMentalHerb(primaryBattler);
        break;
    case HOLD_EFFECT_THROAT_SPRAY:
        effect = TryThroatSray(primaryBattler);
        break;
    case HOLD_EFFECT_KEE_BERRY:  // consume and boost defense if used physical move
        effect = DamagedStatBoostBerryEffect(primaryBattler, secondaryBattler, STAT_DEF, DAMAGE_CATEGORY_PHYSICAL);
        break;
    case HOLD_EFFECT_MARANGA_BERRY:  // consume and boost sp. defense if used special move
        effect = DamagedStatBoostBerryEffect(primaryBattler, secondaryBattler, STAT_SPDEF, DAMAGE_CATEGORY_SPECIAL);
        break;
    case HOLD_EFFECT_SHELL_BELL:
        effect = TryShellBell(primaryBattler);
        break;
    case HOLD_EFFECT_LIFE_ORB:
        effect = TryLifeOrb(primaryBattler);
        break;
    case HOLD_EFFECT_STICKY_BARB:
        if (timing == IsOnTargetHitActivation)
            effect = TryStickyBarbOnTargetHit(primaryBattler, secondaryBattler);
        else
            effect = TryStickyBarbOnEndTurn(primaryBattler);
        break;
    case HOLD_EFFECT_TOXIC_ORB:
        effect = TryToxicOrb(primaryBattler);
        break;
    case HOLD_EFFECT_FLAME_ORB:
        effect = TryFlameOrb(primaryBattler);
        break;
    case HOLD_EFFECT_LEFTOVERS:
        effect = TryLeftovers(primaryBattler);
        break;
    case HOLD_EFFECT_BLACK_SLUDGE:
        if (IS_BATTLER_OF_TYPE(primaryBattler, TYPE_POISON))
            effect = TryLeftovers(primaryBattler);
        else
            effect = TryBlackSludge(primaryBattler);
        break;
    default:
        break;
    }

    if (effect == ITEM_STATUS_CHANGE)
    {
        BtlController_EmitSetMonData(primaryBattler, B_COMM_TO_CONTROLLER, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[primaryBattler].status1);
        MarkBattlerForControllerExec(primaryBattler);
    }

    // TODO: Switch ins use gBattletAttacker. Replace all with scripting / gEffectBattler
    if (effect)
    {
        gLastUsedItem = gBattleMons[primaryBattler].item;
        gBattleScripting.battler = gPotentialItemEffectBattler = primaryBattler;
        RecordItemEffectBattle(primaryBattler, holdEffect); // TODO: Only record the item that is not removed
        if ((item >= FIRST_BERRY_INDEX && item <= LAST_BERRY_INDEX))
            GetBattlerPartyState(primaryBattler)->ateBerry = TRUE;
    }

    return effect;
}
