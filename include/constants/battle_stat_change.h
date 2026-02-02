#ifndef GUARD_CONSTANTS_BATTLE_STAT_CHANGE
#define GUARD_CONSTANTS_BATTLE_STAT_CHANGE

#include "constants/pokemon.h"

#define STAT_CHANGE_WORKED      0
#define STAT_CHANGE_DIDNT_WORK  1

enum
{
    STAT_CHANGE_ALLOW_PTR            = (1 << 0),  // If set, allow use of jumpptr. If not set and unable to raise/lower stats, jump to failInstr.
    STAT_CHANGE_MIRROR_ARMOR         = (1 << 1),  // Stat change redirection caused by Mirror Armor ability.
    STAT_CHANGE_ONLY_CHECKING        = (1 << 2),  // Checks if the stat change can occur. Does not change stats or play stat change animation.
    STAT_CHANGE_NOT_PROTECT_AFFECTED = (1 << 3),
    STAT_CHANGE_UPDATE_MOVE_EFFECT   = (1 << 4),
    STAT_CHANGE_CHECK_PREVENTION     = (1 << 5),
    STAT_CHANGE_CERTAIN              = (1 << 6),
};

// stat flags for TryPlayStatChangeAnimation
enum StatBits
{
    BIT_HP      = (1 << STAT_HP),
    BIT_ATK     = (1 << STAT_ATK),
    BIT_DEF     = (1 << STAT_DEF),
    BIT_SPEED   = (1 << STAT_SPEED),
    BIT_SPATK   = (1 << STAT_SPATK),
    BIT_SPDEF   = (1 << STAT_SPDEF),
    BIT_ACC     = (1 << STAT_ACC),
    BIT_EVASION = (1 << STAT_EVASION),
};

// wider stag flags for setstatchanger
enum StatBuffBits
{
    STAT_BUFF_NEGATIVE = 1,
    STAT_BUFF_ATK      = (1 << (4 * STAT_ATK)),
    STAT_BUFF_DEF      = (1 << (4 * STAT_DEF)),
    STAT_BUFF_SPEED    = (1 << (4 * STAT_SPEED)),
    STAT_BUFF_SPATK    = (1 << (4 * STAT_SPATK)),
    STAT_BUFF_SPDEF    = (1 << (4 * STAT_SPDEF)),
    STAT_BUFF_ACC      = (1 << (4 * STAT_ACC)),
    STAT_BUFF_EVASION  = (1 << (4 * STAT_EVASION)),
};

#endif // GUARD_CONSTANTS_BATTLE_STAT_CHANGE
