#include "global.h"
#include "test/battle.h"

ASSUMPTIONS
{
    ASSUME(gItemsInfo[ITEM_KINGS_ROCK].holdEffect == HOLD_EFFECT_FLINCH);
}

DOUBLE_BATTLE_TEST("Kings Rock flinch applies only once to each target of a spread move")
{
    GIVEN {
        ASSUME(GetMovePower(MOVE_EARTHQUAKE) > 0);
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_KINGS_ROCK); }
        PLAYER(SPECIES_WYNAUT);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_EARTHQUAKE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EARTHQUAKE, playerLeft);
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight);
        MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        NOT MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
    }
}
