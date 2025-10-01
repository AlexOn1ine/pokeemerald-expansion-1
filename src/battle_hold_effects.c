#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_hold_effects.h"
#include "battle_scripts.h"
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
bool32 IsKeeMarangaActivation(enum HoldEffect holdEffect)          { return gHoldEffectsInfo[holdEffect].KeeMaranga; }
bool32 IsMentalHerbActivation(enum HoldEffect holdEffect)          { return gHoldEffectsInfo[holdEffect].MentalHerb; }
bool32 IsOnTargetHitActivation(enum HoldEffect holdEffect)         { return gHoldEffectsInfo[holdEffect].onTargetHit; }
bool32 IsOnAttackerAfterHitActivation(enum HoldEffect holdEffect)  { return gHoldEffectsInfo[holdEffect].onAttackerAfterHit; }
bool32 IsAfterMoveActivation(enum HoldEffect holdEffect)           { return gHoldEffectsInfo[holdEffect].afterMove; }
bool32 IsKingsRockActivation(enum HoldEffect holdEffect)           { return gHoldEffectsInfo[holdEffect].kingsRock; }
bool32 IsLifeOrbShellBellActivation(enum HoldEffect holdEffect)    { return gHoldEffectsInfo[holdEffect].lifeOrbShellBell; }
bool32 IsTryHealingActivation(enum HoldEffect holdEffect)          { return gHoldEffectsInfo[holdEffect].tryHealing; }
bool32 IsConsumeBerryActivation(enum HoldEffect holdEffect)        { return gHoldEffectsInfo[holdEffect].consumeBerry; }
bool32 IsLeftoversActivation(enum HoldEffect holdEffect)           { return gHoldEffectsInfo[holdEffect].leftovers; }
bool32 IsOrbsActivation(enum HoldEffect holdEffect)                { return gHoldEffectsInfo[holdEffect].orbs; }
bool32 IsNormalActivation(enum HoldEffect holdEffect)              { return gHoldEffectsInfo[holdEffect].normal; }

static u32 RestoreWhiteHerbStats(u32 battler, ActivationTiming timing)
{
    u32 i, effect = 0;

    for (i = 0; i < NUM_BATTLE_STATS; i++)
    {
        if (gBattleMons[battler].statStages[i] < DEFAULT_STAT_STAGE)
        {
            gBattleMons[battler].statStages[i] = DEFAULT_STAT_STAGE;
            effect = ITEM_STATS_CHANGE;
        }
    }
    if (effect != 0)
    {
        gLastUsedItem = gBattleMons[battler].item;
        gBattleScripting.battler = battler;
        gPotentialItemEffectBattler = battler;
        if (timing == IsWhiteHerbActivation)
            BattleScriptCall(BattleScript_WhiteHerbRet);
        else
            BattleScriptExecute(BattleScript_WhiteHerbEnd2);

    }

    return effect;
}

enum ItemEffect ItemBattleEffects(u32 battler, enum HoldEffect holdEffect, ActivationTiming getActivationTiming)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    u32 item = gBattleMons[battler].item;

    if (!getActivationTiming(holdEffect))
        return effect;

    switch (holdEffect)
    {
    case HOLD_EFFECT_WHITE_HERB:
        effect = RestoreWhiteHerbStats(battler, getActivationTiming);
        break;
    default: break;
    }

    if (effect == ITEM_STATUS_CHANGE)
    {
        // BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
        // MarkBattlerForControllerExec(battler);
    }

    if (effect)
    {
        gLastUsedItem = item;
        if ((item >= FIRST_BERRY_INDEX && item <= LAST_BERRY_INDEX))
            GetBattlerPartyState(battler)->ateBerry = TRUE;
    }

    return effect;
}
