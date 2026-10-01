#ifndef PR_SCENES_H
#define PR_SCENES_H

#include "pr_ids.h"

namespace pr
{

// What the overworld hands to a battle: a wild pack, or a route trainer.
struct encounter
{
    bool trainer = false;
    int map = 0;                    // trainer: where they stand, and which one
    int trainer_index = 0;
    species_id species[4] = {};     // wild: the pack (startWildBattle)
    int count = 0;
    int level = 2;
};

enum class battle_outcome
{
    WON,
    RAN,
    CAUGHT,
    WHITED_OUT
};

// Title screen and new game; returns once a game is loaded or started.
void title_scene();

// Walks the overworld until a battle starts (returns true and fills `battle`) or the player quits to the
// title (returns false).
bool overworld_scene(encounter& battle);

battle_outcome battle_scene(const encounter& battle);

}

#endif
