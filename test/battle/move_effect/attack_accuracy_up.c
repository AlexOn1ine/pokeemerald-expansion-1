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
        TURN { MOVE(player, MOVE_ROTOTILLER); }
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
        PLAYER(SPECIES_BULBASAUR);
        OPPONENT(SPECIES_IVYSAUR) { Ability(ABILITY_MIRROR_ARMOR); };
        OPPONENT(SPECIES_BULBASAUR);
    } WHEN {
        // TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        // TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        // TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(playerLeft, MOVE_ROTOTILLER); }
    } SCENE {
    } THEN {
        // EXPECT_EQ(playerLeft->statStages[STAT_ATK], 7);
        // EXPECT_EQ(playerRight->statStages[STAT_ATK], 7);
        // EXPECT_EQ(opponentLeft->statStages[STAT_ATK], 7);
        // EXPECT_EQ(opponentRight->statStages[STAT_ATK], 7);
    }
}
