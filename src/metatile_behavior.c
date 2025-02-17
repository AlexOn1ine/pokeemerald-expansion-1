#include "global.h"
#include "metatile_behavior.h"
#include "constants/metatile_behaviors.h"

static const bool8 sBehaviorSurfable[NUM_METATILE_BEHAVIORS] =
{
    [MB_POND_WATER]             = TRUE,
    [MB_DEEP_WATER]             = TRUE,
    [MB_WATERFALL]              = TRUE,
    [MB_SOOTOPOLIS_DEEP_WATER]  = TRUE,
    [MB_OCEAN_WATER]            = TRUE,
    [MB_NO_SURFACING]           = TRUE,
    [MB_SEAWEED]                = TRUE,
    [MB_SEAWEED_NO_SURFACING]   = TRUE,
    [MB_EASTWARD_CURRENT]       = TRUE,
    [MB_WESTWARD_CURRENT]       = TRUE,
    [MB_NORTHWARD_CURRENT]      = TRUE,
    [MB_SOUTHWARD_CURRENT]      = TRUE,
    [MB_WATER_DOOR]             = TRUE,
    [MB_WATER_SOUTH_ARROW_WARP] = TRUE,
};

static const u8 sTileBitAttributes[32] = {
    [0] = 0,
    [1] = 1 << 0,
    [2] = 1 << 1,
    [3] = 1 << 2,
    [4] = 1 << 3,
};

bool8 MetatileBehavior_IsATile(u8 metatileBehavior)
{
    return TRUE;
}

bool8 MetatileBehavior_IsJumpEast(u8 metatileBehavior)
{
    if (metatileBehavior == MB_JUMP_EAST)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsJumpWest(u8 metatileBehavior)
{
    if (metatileBehavior == MB_JUMP_WEST)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsJumpNorth(u8 metatileBehavior)
{
    if (metatileBehavior == MB_JUMP_NORTH)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsJumpSouth(u8 metatileBehavior)
{
    if (metatileBehavior == MB_JUMP_SOUTH)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPokeGrass(u8 metatileBehavior)
{
    if (metatileBehavior == MB_TALL_GRASS || metatileBehavior == MB_LONG_GRASS)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSandOrDeepSand(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SAND || metatileBehavior == MB_DEEP_SAND)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsDeepSand(u8 metatileBehavior)
{
    if (metatileBehavior == MB_DEEP_SAND)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsReflective(u8 metatileBehavior)
{
    if (metatileBehavior == MB_POND_WATER
     || metatileBehavior == MB_PUDDLE
     || metatileBehavior == MB_ICE
     || metatileBehavior == MB_SOOTOPOLIS_DEEP_WATER
     || metatileBehavior == MB_REFLECTION_UNDER_BRIDGE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsIce(u8 metatileBehavior)
{
    if (metatileBehavior == MB_ICE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsWarpDoor(u8 metatileBehavior)
{
    if (metatileBehavior == MB_ANIMATED_DOOR)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsDoor(u8 metatileBehavior)
{
    if (metatileBehavior == MB_PETALBURG_GYM_DOOR
     || metatileBehavior == MB_ANIMATED_DOOR)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsEscalator(u8 metatileBehavior)
{
    if (metatileBehavior == MB_UP_ESCALATOR
     || metatileBehavior == MB_DOWN_ESCALATOR)
        return TRUE;
    else
        return FALSE;
}

bool8 Unref_MetatileBehavior_IsUnused04(u8 metatileBehavior)
{
    if (metatileBehavior == MB_UNUSED_04)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsLadder(u8 metatileBehavior)
{
    if (metatileBehavior == MB_LADDER)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsNonAnimDoor(u8 metatileBehavior)
{
    if (metatileBehavior == MB_NON_ANIMATED_DOOR
     || metatileBehavior == MB_WATER_DOOR
     || metatileBehavior == MB_DEEP_SOUTH_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsDeepSouthWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_DEEP_SOUTH_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSurfableWaterOrUnderwater(u8 metatileBehavior)
{
    if ((sBehaviorSurfable[metatileBehavior] & 1))
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsEastArrowWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_EAST_ARROW_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsWestArrowWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_WEST_ARROW_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsNorthArrowWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_NORTH_ARROW_WARP
     || metatileBehavior == MB_STAIRS_OUTSIDE_ABANDONED_SHIP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSouthArrowWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SOUTH_ARROW_WARP
     || metatileBehavior == MB_WATER_SOUTH_ARROW_WARP
     || metatileBehavior == MB_SHOAL_CAVE_ENTRANCE)
        return TRUE;
    else
        return FALSE;
}

bool8 Unref_MetatileBehavior_IsArrowWarp(u8 metatileBehavior)
{
    u8 isArrowWarp = FALSE;

    if (MetatileBehavior_IsEastArrowWarp(metatileBehavior)
     || MetatileBehavior_IsWestArrowWarp(metatileBehavior)
     || MetatileBehavior_IsNorthArrowWarp(metatileBehavior)
     || MetatileBehavior_IsSouthArrowWarp(metatileBehavior))
    {
        isArrowWarp = TRUE;
    }

    return isArrowWarp;
}

bool8 MetatileBehavior_IsForcedMovementTile(u8 metatileBehavior)
{
    if ((metatileBehavior >= MB_WALK_EAST && metatileBehavior <= MB_TRICK_HOUSE_PUZZLE_8_FLOOR)
     || (metatileBehavior >= MB_EASTWARD_CURRENT && metatileBehavior <= MB_SOUTHWARD_CURRENT)
     || metatileBehavior == MB_MUDDY_SLOPE
     || metatileBehavior == MB_CRACKED_FLOOR
     || metatileBehavior == MB_WATERFALL
     || metatileBehavior == MB_ICE
     || metatileBehavior == MB_SECRET_BASE_JUMP_MAT
     || metatileBehavior == MB_SECRET_BASE_SPIN_MAT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsIce_2(u8 metatileBehavior)
{
    if (metatileBehavior == MB_ICE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsTrickHouseSlipperyFloor(u8 metatileBehavior)
{
    if (metatileBehavior == MB_TRICK_HOUSE_PUZZLE_8_FLOOR)
        return TRUE;
    else
        return FALSE;
}

bool8 Unref_MetatileBehavior_IsUnused05(u8 metatileBehavior)
{
    if (metatileBehavior == MB_UNUSED_05)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsWalkNorth(u8 metatileBehavior)
{
    if (metatileBehavior == MB_WALK_NORTH)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsWalkSouth(u8 metatileBehavior)
{
    if (metatileBehavior == MB_WALK_SOUTH)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsWalkWest(u8 metatileBehavior)
{
    if (metatileBehavior == MB_WALK_WEST)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsWalkEast(u8 metatileBehavior)
{
    if (metatileBehavior == MB_WALK_EAST)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsNorthwardCurrent(u8 metatileBehavior)
{
    if (metatileBehavior == MB_NORTHWARD_CURRENT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSouthwardCurrent(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SOUTHWARD_CURRENT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsWestwardCurrent(u8 metatileBehavior)
{
    if (metatileBehavior == MB_WESTWARD_CURRENT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsEastwardCurrent(u8 metatileBehavior)
{
    if (metatileBehavior == MB_EASTWARD_CURRENT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSlideNorth(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SLIDE_NORTH)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSlideSouth(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SLIDE_SOUTH)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSlideWest(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SLIDE_WEST)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSlideEast(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SLIDE_EAST)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsCounter(u8 metatileBehavior)
{
    if (metatileBehavior == MB_COUNTER)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPlayerFacingTVScreen(u8 metatileBehavior, u8 playerDir)
{
    if (playerDir != DIR_NORTH)
        return FALSE;
    else if (metatileBehavior == MB_TELEVISION)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPC(u8 metatileBehavior)
{
    if (metatileBehavior == MB_PC)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsCableBoxResults1(u8 metatileBehavior)
{
    if (metatileBehavior == MB_CABLE_BOX_RESULTS_1)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsOpenSecretBaseDoor(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_SPOT_RED_CAVE_OPEN
     || metatileBehavior == MB_SECRET_BASE_SPOT_BROWN_CAVE_OPEN
     || metatileBehavior == MB_SECRET_BASE_SPOT_YELLOW_CAVE_OPEN
     || metatileBehavior == MB_SECRET_BASE_SPOT_TREE_LEFT_OPEN
     || metatileBehavior == MB_SECRET_BASE_SPOT_SHRUB_OPEN
     || metatileBehavior == MB_SECRET_BASE_SPOT_BLUE_CAVE_OPEN
     || metatileBehavior == MB_SECRET_BASE_SPOT_TREE_RIGHT_OPEN)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseCave(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_SPOT_RED_CAVE
     || metatileBehavior == MB_SECRET_BASE_SPOT_BROWN_CAVE
     || metatileBehavior == MB_SECRET_BASE_SPOT_YELLOW_CAVE
     || metatileBehavior == MB_SECRET_BASE_SPOT_BLUE_CAVE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseTree(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_SPOT_TREE_LEFT
     || metatileBehavior == MB_SECRET_BASE_SPOT_TREE_RIGHT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseShrub(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_SPOT_SHRUB)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBasePC(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_PC)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsRecordMixingSecretBasePC(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_REGISTER_PC)
        return TRUE;
    else
        return FALSE;
}

// Used by the rock/grass floor spaces that the secret base trainer is not standing on
bool8 MetatileBehavior_IsSecretBaseScenery1(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_SCENERY)
        return TRUE;
    else
        return FALSE;
}

// Used by the rock/grass floor space that the secret base trainer stands on
bool8 MetatileBehavior_IsSecretBaseTrainerSpot(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_TRAINER_SPOT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseImpassable(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_IMPASSABLE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseDecorationBase(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_DECORATION_BASE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBasePoster(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_POSTER)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsNormal(u8 metatileBehavior)
{
    if (metatileBehavior == MB_NORMAL)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseNorthWall(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_NORTH_WALL)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseScenery2(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_SCENERY)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_HoldsSmallDecoration(u8 metatileBehavior)
{
    if (metatileBehavior == MB_HOLDS_SMALL_DECORATION)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_HoldsLargeDecoration(u8 metatileBehavior)
{
    if (metatileBehavior == MB_HOLDS_LARGE_DECORATION)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseHole(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_HOLE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseBalloon(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_BALLOON)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseBreakableDoor(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_BREAKABLE_DOOR)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseSoundMat(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_SOUND_MAT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseGlitterMat(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_GLITTER_MAT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseSandOrnament(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_SAND_ORNAMENT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseShieldOrToyTV(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_TV_SHIELD)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPlayerRoomPCOn(u8 metatileBehavior)
{
    if (metatileBehavior == MB_PLAYER_ROOM_PC_ON)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_HasRipples(u8 metatileBehavior)
{
    if (metatileBehavior == MB_POND_WATER
     || metatileBehavior == MB_PUDDLE
     || metatileBehavior == MB_SOOTOPOLIS_DEEP_WATER)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPuddle(u8 metatileBehavior)
{
    if (metatileBehavior == MB_PUDDLE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsTallGrass(u8 metatileBehavior)
{
    if (metatileBehavior == MB_TALL_GRASS)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsLongGrass(u8 metatileBehavior)
{
    if (metatileBehavior == MB_LONG_GRASS)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsAshGrass(u8 metatileBehavior)
{
    if (metatileBehavior == MB_ASHGRASS)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsFootprints(u8 metatileBehavior)
{
    // MB_FOOTPRINTS is not used by any metatiles.
    if (metatileBehavior == MB_FOOTPRINTS)
        return TRUE;
    else
        return FALSE;
}

// For the sections of log bridges that span water / water's edge.
// Note that the rest of the metatiles for these bridges use MB_NORMAL.
// This is used to allow encounters on the water below the bridge.
bool8 MetatileBehavior_IsBridgeOverWater(u8 metatileBehavior)
{
    if ((metatileBehavior == MB_BRIDGE_OVER_OCEAN
      || metatileBehavior == MB_BRIDGE_OVER_POND_LOW
      || metatileBehavior == MB_BRIDGE_OVER_POND_MED
      || metatileBehavior == MB_BRIDGE_OVER_POND_HIGH)
      || (metatileBehavior == MB_BRIDGE_OVER_POND_HIGH_EDGE_1
       || metatileBehavior == MB_BRIDGE_OVER_POND_HIGH_EDGE_2
       || metatileBehavior == MB_UNUSED_BRIDGE
       || metatileBehavior == MB_BIKE_BRIDGE_OVER_BARRIER))
        return TRUE;
    else
        return FALSE;
}

u8 MetatileBehavior_GetBridgeType(u8 metatileBehavior)
{
    // MB_BRIDGE_OVER_OCEAN     --> BRIDGE_TYPE_OCEAN     (Routes 110/119)
    // MB_BRIDGE_OVER_POND_LOW  --> BRIDGE_TYPE_POND_LOW  (Unused)
    // MB_BRIDGE_OVER_POND_MED  --> BRIDGE_TYPE_POND_MED  (Route 120, south)
    // MB_BRIDGE_OVER_POND_HIGH --> BRIDGE_TYPE_POND_HIGH (Route 120, north)
    if (metatileBehavior >= MB_BRIDGE_OVER_OCEAN
     && metatileBehavior <= MB_BRIDGE_OVER_POND_HIGH)
        return metatileBehavior - MB_BRIDGE_OVER_OCEAN;

    if (metatileBehavior >= MB_BRIDGE_OVER_POND_MED_EDGE_1
     && metatileBehavior <= MB_BRIDGE_OVER_POND_MED_EDGE_2)
        return BRIDGE_TYPE_POND_MED;

    if (metatileBehavior >= MB_BRIDGE_OVER_POND_HIGH_EDGE_1
     && metatileBehavior <= MB_BRIDGE_OVER_POND_HIGH_EDGE_2)
        return BRIDGE_TYPE_POND_HIGH;

    return BRIDGE_TYPE_OCEAN;
}

// Used to allow fishing below the bridge metatiles.
bool8 MetatileBehavior_IsBridgeOverWaterNoEdge(u8 metatileBehavior)
{
    if (metatileBehavior >= MB_BRIDGE_OVER_OCEAN
     && metatileBehavior <= MB_BRIDGE_OVER_POND_HIGH)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsIndoorEncounter(u8 metatileBehavior)
{
    if (metatileBehavior == MB_INDOOR_ENCOUNTER)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsMountain(u8 metatileBehavior)
{
    if (metatileBehavior == MB_MOUNTAIN_TOP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsDiveable(u8 metatileBehavior)
{
    if (metatileBehavior == MB_DEEP_WATER
     || metatileBehavior == MB_SOOTOPOLIS_DEEP_WATER)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsUnableToEmerge(u8 metatileBehavior)
{
    // BUG: The player is unintentionally able to emerge on water doors.
    // Also the narrower underwater door in the underwater tileset has the wrong metatile behavior. This causes the dive glitch.
    // To fix change the metatile behavior of the narrower water door with porymap's tileset editor.
    if (metatileBehavior == MB_NO_SURFACING
     || metatileBehavior == MB_SEAWEED_NO_SURFACING
     #ifdef BUGFIX
     || metatileBehavior == MB_WATER_DOOR
     #endif
     )
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsShallowFlowingWater(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SHALLOW_WATER
     || metatileBehavior == MB_STAIRS_OUTSIDE_ABANDONED_SHIP
     || metatileBehavior == MB_SHOAL_CAVE_ENTRANCE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsThinIce(u8 metatileBehavior)
{
    if (metatileBehavior == MB_THIN_ICE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsCrackedIce(u8 metatileBehavior)
{
    if (metatileBehavior == MB_CRACKED_ICE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsDeepOrOceanWater(u8 metatileBehavior)
{
    if (metatileBehavior == MB_OCEAN_WATER
     || metatileBehavior == MB_DEEP_WATER)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSurfableAndNotWaterfall(u8 metatileBehavior)
{
    if (MetatileBehavior_IsSurfableWaterOrUnderwater(metatileBehavior)
     && MetatileBehavior_IsWaterfall(metatileBehavior) == FALSE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsEastBlocked(u8 metatileBehavior)
{
    if (metatileBehavior == MB_IMPASSABLE_EAST
     || metatileBehavior == MB_IMPASSABLE_NORTHEAST
     || metatileBehavior == MB_IMPASSABLE_SOUTHEAST
     || metatileBehavior == MB_IMPASSABLE_WEST_AND_EAST
     || metatileBehavior == MB_SECRET_BASE_BREAKABLE_DOOR)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsWestBlocked(u8 metatileBehavior)
{
    if (metatileBehavior == MB_IMPASSABLE_WEST
     || metatileBehavior == MB_IMPASSABLE_NORTHWEST
     || metatileBehavior == MB_IMPASSABLE_SOUTHWEST
     || metatileBehavior == MB_IMPASSABLE_WEST_AND_EAST
     || metatileBehavior == MB_SECRET_BASE_BREAKABLE_DOOR)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsNorthBlocked(u8 metatileBehavior)
{
    if (metatileBehavior == MB_IMPASSABLE_NORTH
     || metatileBehavior == MB_IMPASSABLE_NORTHEAST
     || metatileBehavior == MB_IMPASSABLE_NORTHWEST
     || metatileBehavior == MB_IMPASSABLE_SOUTH_AND_NORTH)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSouthBlocked(u8 metatileBehavior)
{
    if (metatileBehavior == MB_IMPASSABLE_SOUTH
     || metatileBehavior == MB_IMPASSABLE_SOUTHEAST
     || metatileBehavior == MB_IMPASSABLE_SOUTHWEST
     || metatileBehavior == MB_IMPASSABLE_SOUTH_AND_NORTH)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsShortGrass(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SHORT_GRASS)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsHotSprings(u8 metatileBehavior)
{
    if (metatileBehavior == MB_HOT_SPRINGS)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsWaterfall(u8 metatileBehavior)
{
    if (metatileBehavior == MB_WATERFALL)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsFortreeBridge(u8 metatileBehavior)
{
    if (metatileBehavior == MB_FORTREE_BRIDGE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPacifidlogVerticalLogTop(u8 metatileBehavior)
{
    if (metatileBehavior == MB_PACIFIDLOG_VERTICAL_LOG_TOP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPacifidlogVerticalLogBottom(u8 metatileBehavior)
{
    if (metatileBehavior == MB_PACIFIDLOG_VERTICAL_LOG_BOTTOM)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPacifidlogHorizontalLogLeft(u8 metatileBehavior)
{
    if (metatileBehavior == MB_PACIFIDLOG_HORIZONTAL_LOG_LEFT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPacifidlogHorizontalLogRight(u8 metatileBehavior)
{
    if (metatileBehavior == MB_PACIFIDLOG_HORIZONTAL_LOG_RIGHT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPacifidlogLog(u8 metatileBehavior)
{
    if (metatileBehavior == MB_PACIFIDLOG_VERTICAL_LOG_TOP
     || metatileBehavior == MB_PACIFIDLOG_VERTICAL_LOG_BOTTOM
     || metatileBehavior == MB_PACIFIDLOG_HORIZONTAL_LOG_LEFT
     || metatileBehavior == MB_PACIFIDLOG_HORIZONTAL_LOG_RIGHT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsTrickHousePuzzleDoor(u8 metatileBehavior)
{
    if (metatileBehavior == MB_TRICK_HOUSE_PUZZLE_DOOR)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsRegionMap(u8 metatileBehavior)
{
    if (metatileBehavior == MB_REGION_MAP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsClosedSootopolisDoor(u8 metatileBehavior)
{
    if (metatileBehavior == MB_CLOSED_SOOTOPOLIS_DOOR)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSkyPillarClosedDoor(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SKY_PILLAR_CLOSED_DOOR)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsRoulette(u8 metatileBehavior) // unused
{
    if (metatileBehavior == MB_ROULETTE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPokeblockFeeder(u8 metatileBehavior)
{
    if (metatileBehavior == MB_POKEBLOCK_FEEDER)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseJumpMat(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_JUMP_MAT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSecretBaseSpinMat(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SECRET_BASE_SPIN_MAT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsLavaridgeB1FWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_LAVARIDGE_GYM_B1F_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsLavaridge1FWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_LAVARIDGE_GYM_1F_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsAquaHideoutWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_AQUA_HIDEOUT_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsUnionRoomWarp(u8 metatileBehavior)
{
    // This metatile behavior is re-used for some reason by
    // the Union Room exit metatile. This function is used to
    // initiate a teleport-style warp.
    if (metatileBehavior == MB_BRIDGE_OVER_OCEAN)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsMossdeepGymWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_MOSSDEEP_GYM_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsMtPyreHole(u8 metatileBehavior)
{
    if (metatileBehavior == MB_MT_PYRE_HOLE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsCrackedFloorHole(u8 metatileBehavior)
{
    if (metatileBehavior == MB_CRACKED_FLOOR_HOLE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsCrackedFloor(u8 metatileBehavior)
{
    if (metatileBehavior == MB_CRACKED_FLOOR)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsMuddySlope(u8 metatileBehavior)
{
    if (metatileBehavior == MB_MUDDY_SLOPE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsBumpySlope(u8 metatileBehavior)
{
    if (metatileBehavior == MB_BUMPY_SLOPE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsIsolatedVerticalRail(u8 metatileBehavior)
{
    if (metatileBehavior == MB_ISOLATED_VERTICAL_RAIL)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsIsolatedHorizontalRail(u8 metatileBehavior)
{
    if (metatileBehavior == MB_ISOLATED_HORIZONTAL_RAIL)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsVerticalRail(u8 metatileBehavior)
{
    if (metatileBehavior == MB_VERTICAL_RAIL)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsHorizontalRail(u8 metatileBehavior)
{
    if (metatileBehavior == MB_HORIZONTAL_RAIL)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSeaweed(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SEAWEED || metatileBehavior == MB_SEAWEED_NO_SURFACING)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsRunningDisallowed(u8 metatileBehavior)
{
    if (metatileBehavior == MB_NO_RUNNING
     || metatileBehavior == MB_LONG_GRASS
     || metatileBehavior == MB_HOT_SPRINGS
     || MetatileBehavior_IsPacifidlogLog(metatileBehavior) != FALSE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsRunningShoesManual(u8 metatileBehavior)
{
    if (metatileBehavior == MB_RUNNING_SHOES_INSTRUCTION)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPictureBookShelf(u8 metatileBehavior)
{
    if (metatileBehavior == MB_PICTURE_BOOK_SHELF)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsBookShelf(u8 metatileBehavior)
{
    if (metatileBehavior == MB_BOOKSHELF)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPokeCenterBookShelf(u8 metatileBehavior)
{
    if (metatileBehavior == MB_POKEMON_CENTER_BOOKSHELF)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsVase(u8 metatileBehavior)
{
    if (metatileBehavior == MB_VASE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsTrashCan(u8 metatileBehavior)
{
    if (metatileBehavior == MB_TRASH_CAN)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsShopShelf(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SHOP_SHELF)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsBlueprint(u8 metatileBehavior)
{
    if (metatileBehavior == MB_BLUEPRINT)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsBattlePyramidWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_BATTLE_PYRAMID_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsPlayerFacingWirelessBoxResults(u8 tile, u8 playerDir)
{
    if (playerDir != CONNECTION_NORTH) // if the player isn't facing north, forget about it.
        return FALSE;
    else if (tile == MB_WIRELESS_BOX_RESULTS) // is the player's north tile the monitor with results?
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsCableBoxResults2(u8 tile, u8 playerDir)
{
    if (playerDir != CONNECTION_NORTH) // if the player isn't facing north, forget about it.
        return FALSE;
    else if (tile == MB_CABLE_BOX_RESULTS_2) // is the player's north tile the monitor with results?
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsQuestionnaire(u8 metatileBehavior)
{
    if (metatileBehavior == MB_QUESTIONNAIRE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsLongGrass_Duplicate(u8 metatileBehavior)
{
    if (metatileBehavior == MB_LONG_GRASS)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsLongGrassSouthEdge(u8 metatileBehavior)
{
    if (metatileBehavior == MB_LONG_GRASS_SOUTH_EDGE)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsTrainerHillTimer(u8 metatileBehavior)
{
    if (metatileBehavior == MB_TRAINER_HILL_TIMER)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsDirectionalUpRightStairWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_UP_RIGHT_STAIR_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsDirectionalUpLeftStairWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_UP_LEFT_STAIR_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsDirectionalDownRightStairWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_DOWN_RIGHT_STAIR_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsDirectionalDownLeftStairWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_DOWN_LEFT_STAIR_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsDirectionalStairWarp(u8 metatileBehavior)
{
    if (metatileBehavior == MB_UP_RIGHT_STAIR_WARP
     || metatileBehavior == MB_UP_LEFT_STAIR_WARP
     || metatileBehavior == MB_DOWN_RIGHT_STAIR_WARP
     || metatileBehavior == MB_DOWN_LEFT_STAIR_WARP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSignpost(u32 metatileBehavior)
{
    return (metatileBehavior == MB_SIGNPOST);
}

bool8 MetatileBehavior_IsPokemonCenterSign(u32 metatileBehavior)
{
    return (metatileBehavior == MB_POKEMON_CENTER_SIGN);
}

bool8 MetatileBehavior_IsPokeMartSign(u32 metatileBehavior)
{
    return (metatileBehavior == MB_POKEMART_SIGN);
}

bool8 MetatileBehavior_IsSidewaysStairsRightSide(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SIDEWAYS_STAIRS_RIGHT_SIDE || metatileBehavior == MB_SIDEWAYS_STAIRS_RIGHT_SIDE_BOTTOM)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSidewaysStairsLeftSide(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SIDEWAYS_STAIRS_LEFT_SIDE || metatileBehavior == MB_SIDEWAYS_STAIRS_LEFT_SIDE_BOTTOM)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSidewaysStairsRightSideTop(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SIDEWAYS_STAIRS_RIGHT_SIDE_TOP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSidewaysStairsLeftSideTop(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SIDEWAYS_STAIRS_LEFT_SIDE_TOP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSidewaysStairsRightSideBottom(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SIDEWAYS_STAIRS_RIGHT_SIDE_BOTTOM)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSidewaysStairsLeftSideBottom(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SIDEWAYS_STAIRS_LEFT_SIDE_BOTTOM)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSidewaysStairsRightSideAny(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SIDEWAYS_STAIRS_RIGHT_SIDE
     || metatileBehavior == MB_SIDEWAYS_STAIRS_RIGHT_SIDE_BOTTOM
     || metatileBehavior == MB_SIDEWAYS_STAIRS_RIGHT_SIDE_TOP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsSidewaysStairsLeftSideAny(u8 metatileBehavior)
{
    if (metatileBehavior == MB_SIDEWAYS_STAIRS_LEFT_SIDE
     || metatileBehavior == MB_SIDEWAYS_STAIRS_LEFT_SIDE_BOTTOM
     || metatileBehavior == MB_SIDEWAYS_STAIRS_LEFT_SIDE_TOP)
        return TRUE;
    else
        return FALSE;
}

bool8 MetatileBehavior_IsRockStairs(u8 metatileBehavior)
{
    if (metatileBehavior == MB_ROCK_STAIRS)
        return TRUE;
    else
        return FALSE;
}

bool32 TestMetatileAttributeBit(u32 attr, u32 bitmask)
{
    return sTileBitAttributes[attr] & bitmask;
}

