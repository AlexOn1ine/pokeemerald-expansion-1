#ifndef GUARD_BATTLE_HOLD_EFFECTS
#define GUARD_BATTLE_HOLD_EFFECTS

// #include "constants/item.h"
// #include "constants/item_effects.h"
// #include "constants/items.h"
// #include "constants/moves.h"
// #include "constants/tms_hms.h"
// #include "constants/item_effects.h"
#include "constants/hold_effects.h"

struct HoldEffectInfo
{
    u32 onSwitchIn:1;
    u32 onSwitchInFirstTurn:1;
    u32 mirrorHerb:1;
    u32 mirrorHerbFirstTurn:1;
    u32 whiteHerb:1;
    u32 whiteHerbFirstTurn:1;
    u32 healStatus:1;
    u32 restoreHP:1;
    u32 magicRoomEnds:1;
    u32 bugBiteEats:1;
    u32 KeeMaranga:1;
    u32 MentalHerb:1;

    // u32 padding:4;
};

extern const struct HoldEffectInfo gHoldEffectsInfo[];


#endif // GUARD_BATTLE_HOLD_EFFECTS
