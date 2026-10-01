// A species' 32x32 icon (the party screen, the PC's boxes).
#ifndef PR_ICONS_H
#define PR_ICONS_H

#include "pr_game_data.h"

namespace pr
{

inline const bn::sprite_item& icon_item(int species_index)
{
    return game_data::species_list[species_index].icon;
}

}

#endif
