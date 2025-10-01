#ifndef GUARD_BATTLE_HOLD_EFFECTS
#define GUARD_BATTLE_HOLD_EFFECTS

// #include "constants/item.h"
// #include "constants/item_effects.h"
// #include "constants/items.h"
// #include "constants/moves.h"
// #include "constants/tms_hms.h"
// #include "constants/item_effects.h"

struct HoldEffectInfo
{
    u32 onSwitchIn:1;
    u32 onSwitchInFirstTurn:1;
    u32 mirrorHerb:1;
    u32 mirrorHerbFirstTurn:1;
    u32 whiteHerb:1;
    u32 whiteHerbFirstTurn:1;
    u32 whiteHerbEndTurn:1;
    u32 healStatus:1;
    u32 berryRestoreHp:1;
    u32 magicRoomEnds:1;
    u32 bugBiteEats:1;
    u32 KeeMaranga:1;
    u32 MentalHerb:1;
    u32 onTargetHit:1;
    u32 onAttackerAfterHit:1;
    u32 afterMove:1;
    u32 kingsRock:1;
    u32 lifeOrbShellBell:1;
    u32 tryHealing:1;
    u32 consumeBerry:1;
    u32 leftovers:1;
    u32 orbs:1;
    u32 normal:1;

    // u32 padding:4;
};

extern const struct HoldEffectInfo gHoldEffectsInfo[];

typedef bool32 (*ActivationTiming)(enum HoldEffect holdEffect);
enum ItemEffect ItemBattleEffects(u32 battler, enum HoldEffect holdEffect, ActivationTiming getActivationTiming);

bool32 IsOnSwitchInActivation(enum HoldEffect holdEffect);
bool32 IsOnSwitchInFirstTurnActivation(enum HoldEffect holdEffect);
bool32 IsMirrorHerbActivation(enum HoldEffect holdEffect);
bool32 IsMirrorHerbFirstTurnActivation(enum HoldEffect holdEffect);
bool32 IsWhiteHerbActivation(enum HoldEffect holdEffect);
bool32 IsWhiteHerbFirstTurnActivation(enum HoldEffect holdEffect);
bool32 IsWhiteHerbEndTurnActivation(enum HoldEffect holdEffect);
bool32 IsHealStatusActivation(enum HoldEffect holdEffect);
bool32 IsBerryRestoreHpActivation(enum HoldEffect holdEffect);
bool32 IsMagicRoomEndsActivation(enum HoldEffect holdEffect);
bool32 IsBugBiteEatsActivation(enum HoldEffect holdEffect);
bool32 IsKeeMarangaActivation(enum HoldEffect holdEffect);
bool32 IsMentalHerbActivation(enum HoldEffect holdEffect);
bool32 IsOnTargetHitActivation(enum HoldEffect holdEffect);
bool32 IsOnAttackerAfterHitActivation(enum HoldEffect holdEffect);
bool32 IsAfterMoveActivation(enum HoldEffect holdEffect);
bool32 IsKingsRockActivation(enum HoldEffect holdEffect);
bool32 IsLifeOrbShellBellActivation(enum HoldEffect holdEffect);
bool32 IsTryHealingActivation(enum HoldEffect holdEffect);
bool32 IsConsumeBerryActivation(enum HoldEffect holdEffect);
bool32 IsLeftoversActivation(enum HoldEffect holdEffect);
bool32 IsOrbsActivation(enum HoldEffect holdEffect);
bool32 IsNormalActivation(enum HoldEffect holdEffect);

#endif // GUARD_BATTLE_HOLD_EFFECTS
