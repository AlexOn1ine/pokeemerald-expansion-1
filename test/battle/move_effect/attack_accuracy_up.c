#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Hone Claws increases Attack and Accuracy by one stage each")
{
    GIVEN {
        ASSUME_STAT_CHANGE(MOVE_HONE_CLAWS, attack: +1, accuracy: +1);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_HONE_CLAWS); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_HONE_CLAWS, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
        MESSAGE("Wobbuffet's Attack rose!");
        MESSAGE("Wobbuffet's accuracy rose!");
    }
}

SINGLE_BATTLE_TEST("Dragon Dance test")
{
    GIVEN {
        PLAYER(SPECIES_BULBASAUR);
        PLAYER(SPECIES_BULBASAUR);
        OPPONENT(SPECIES_BULBASAUR);
        OPPONENT(SPECIES_BULBASAUR);
    } WHEN {
        // TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        // TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        // TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_SHIFT_GEAR); }
    } SCENE {
    } THEN {
        // EXPECT_EQ(player->statStages[STAT_ATK], 7);
        // EXPECT_EQ(player->statStages[STAT_SPEED], 7);
    }
}

DOUBLE_BATTLE_TEST("Dragon Dance double test")
{
    GIVEN {
        PLAYER(SPECIES_IVYSAUR);
        PLAYER(SPECIES_BULBASAUR) { Ability(ABILITY_CLEAR_BODY); }
        OPPONENT(SPECIES_IVYSAUR);
        OPPONENT(SPECIES_BULBASAUR) { Ability(ABILITY_MIRROR_ARMOR); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_ROTOTILLER); }
    } SCENE {
    } THEN {
    }
}

DOUBLE_BATTLE_TEST("Cotton Down test")
{
    GIVEN {
        PLAYER(SPECIES_BAYLEEF) { Ability(ABILITY_INTIMIDATE); }
        PLAYER(SPECIES_CHIKORITA) { Ability(ABILITY_COTTON_DOWN); }
        OPPONENT(SPECIES_IVYSAUR) { Ability(ABILITY_INTIMIDATE); }
        OPPONENT(SPECIES_BULBASAUR) { Ability(ABILITY_COTTON_DOWN); }
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_POUND, target: opponentRight);
            MOVE(playerRight, MOVE_ROTOTILLER, target: opponentRight);
            MOVE(opponentLeft, MOVE_POUND, target: playerRight);
            MOVE(opponentRight, MOVE_ROTOTILLER, target: playerRight);
        }
    } SCENE {
    } THEN {
    }
}
