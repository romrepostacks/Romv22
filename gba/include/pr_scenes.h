#ifndef PR_SCENES_H
#define PR_SCENES_H

#include "pr_species_id.h"

namespace pr
{

class ui;

struct encounter
{
    species_id species;
    int level;
};

enum class battle_outcome
{
    WON,
    RAN,
    CAUGHT,
    WHITED_OUT
};

// Title screen; returns once a game is loaded or started.
void title_scene(ui& ui);

// Walks the overworld until a wild Pokémon appears (returns true and fills `wild`) or the player
// quits to the title (returns false).
bool overworld_scene(ui& ui, encounter& wild);

battle_outcome battle_scene(ui& ui, const encounter& wild);

}

#endif
