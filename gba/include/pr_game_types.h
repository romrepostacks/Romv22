// Species and move data types; the data itself is generated into pr_game_data.h by tools/build_assets.py.
#ifndef PR_GAME_TYPES_H
#define PR_GAME_TYPES_H

#include <cstdint>
#include "bn_sprite_item.h"
#include "pr_species_id.h"

namespace pr
{

enum class move_category : uint8_t
{
    PHYSICAL,
    SPECIAL,
    STATUS
};

enum class status : uint8_t
{
    NONE,
    POISON,
    BURN,
    PARALYSIS,
    SLEEP,
    FREEZE
};

struct move
{
    const char* name;
    int8_t type;
    uint8_t power;
    move_category category;
    uint8_t accuracy;
    status inflicts;          // status moves
    status secondary;         // damaging moves' side effect
    uint8_t secondary_chance;
};

struct learn_entry
{
    uint8_t level;
    uint8_t move;
};

struct base_stats
{
    uint8_t hp;
    uint8_t atk;
    uint8_t def;
    uint8_t spa;
    uint8_t spd;
    uint8_t spe;
};

struct species
{
    const char* name;
    int16_t dex_number;
    int8_t type1;
    int8_t type2;             // -1: single type
    base_stats base;
    const learn_entry* learnset;
    int16_t learnset_count;
    int8_t evolves_to;        // index into species_list, -1 if none in this build
    uint8_t evolve_level;
    const bn::sprite_item& front;
    const bn::sprite_item& back;
};

}

#endif
