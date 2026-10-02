#ifndef PR_MOVE_FX_H
#define PR_MOVE_FX_H

#include "bn_sprite_ptr.h"

namespace pr
{

// A Pokémon on the battle field, for the move animations: its sprite (may be null) and where it is on the
// screen (centre, size, in screen pixels).
struct fx_body
{
    bn::sprite_ptr* sprite = nullptr;
    int x = 0, y = 0, w = 48, h = 48;
};

// The web game's atkFx(): the move's animation from one Pokémon to another (its MOVE_FX script, step by
// step). Returns when it's done.
void play_move_fx(int move_index, const fx_body& from, const fx_body& to);

// A shiny Pokémon's entrance: stars burst around it, twice.
void play_shiny_sparkle(const fx_body& who);

}

#endif
