// A species' 32x32 icon (the party screen, the PC's boxes).
#ifndef PR_ICONS_H
#define PR_ICONS_H

#include "bn_sprite_palette_item.h"
#include "bn_sprite_ptr.h"

#include "pr_game_data.h"
#include "pr_mon.h"
#include "pr_shiny_data.h"

namespace pr
{

inline const bn::sprite_item& icon_item(int species_index)
{
    return game_data::species_list[species_index].icon;
}

enum class mon_view
{
    FRONT,
    BACK,
    ICON
};

// A shiny Pokémon's sprite gets its shiny colours (the same pixels, the shiny palette).
inline void apply_shiny(bn::sprite_ptr& sprite, int species_index, bool shiny, mon_view view)
{
    if(! shiny)
    {
        return;
    }
    const bn::color* colors = view == mon_view::FRONT ? shiny_data::front[species_index] :
                              view == mon_view::BACK ? shiny_data::back[species_index] : shiny_data::icon[species_index];
    sprite.set_palette(bn::sprite_palette_item(bn::span<const bn::color>(colors, 16), bn::bpp_mode::BPP_4));
}

inline void apply_shiny(bn::sprite_ptr& sprite, const mon& m, mon_view view)
{
    apply_shiny(sprite, m.species_index, m.shiny(), view);
}

}

#endif
