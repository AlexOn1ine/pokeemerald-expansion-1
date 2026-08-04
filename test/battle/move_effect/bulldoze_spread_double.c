#include "global.h"
#include "test/battle.h"

DOUBLE_BATTLE_TEST("Bulldoze lowers each target's Speed only once")
{
    GIVEN {
        ASSUME(GetMoveTarget(MOVE_BULLDOZE) == TARGET_FOES_AND_ALLY);
        ASSUME(GetMovePower(MOVE_BULLDOZE) > 0);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WYNAUT);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_BULLDOZE); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet's Speed fell!");
        MESSAGE("The opposing Wobbuffet's Speed fell!");
        NOT MESSAGE("The opposing Wobbuffet's Speed fell!");
    } THEN {
        EXPECT_EQ(opponentLeft->statStages[STAT_SPEED], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(opponentRight->statStages[STAT_SPEED], DEFAULT_STAT_STAGE - 1);
    }
}
