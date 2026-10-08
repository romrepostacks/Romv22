// Species and move data types; the data itself is generated into pr_game_data.h by tools/build_assets.py.
#ifndef PR_GAME_TYPES_H
#define PR_GAME_TYPES_H

#include <cstdint>
#include "bn_sprite_item.h"
#include "pr_ids.h"

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

// Who a move hits: the chosen foe, every foe (Earthquake, Growl), the user (Swords Dance) or no one (weather).
enum class move_target : uint8_t
{
    ONE,
    FOES,
    SELF,
    FIELD
};

enum class battle_weather : uint8_t
{
    NONE,
    SUN,
    RAIN,
    SAND,
    HAIL
};

// The battle stats a move can raise or lower.
namespace battle_stat
{
    enum : int8_t { ATK, DEF, SPA, SPD, SPE, ACC, EVA, COUNT };
}

struct stat_change
{
    int8_t stat;
    int8_t delta;
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
    bool punch;               // Iron Fist boosts it
    uint8_t pp;
    move_target target;
    int8_t drain;             // % of the damage dealt: + heals the user, - is recoil
    uint8_t heal;             // % of the user's max HP it restores
    stat_change stats[3];     // stage changes (to the target, or the user if stat_self)
    uint8_t stat_count;
    uint8_t stat_chance;      // % (damaging moves' side effect; status moves always)
    bool stat_self;
    battle_weather weather;
};

struct learn_entry
{
    uint8_t level;
    uint16_t move;
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

// What an ability does in battle (damage()): the rest are flavour.
enum class ability_kind : uint8_t
{
    FLAVOR,
    BOOST,          // its type's moves x1.5 at 1/3 HP or less (Blaze, Torrent...)
    IMMUNE,         // takes no damage from one type (Levitate, Flash Fire...)
    PUNCH,          // punching moves x1.2 (Iron Fist)
    MERCILESS,      // x1.5 against a poisoned target
    CORROSION,      // Poison moves hit Steel and Poison types
    DISGUISE        // the first hit does nothing
};

struct ability
{
    const char* name;
    const char* desc;
    ability_kind kind;
    int8_t type;              // BOOST / IMMUNE: the type, else -1
};

// ITEM_INFO: pocket 0 ITEMS, 1 POKé BALLS, 2 TMs & HMs, 3 BERRIES, 4 KEY ITEMS.
struct item_info
{
    const char* name;
    uint8_t pocket;
    const char* desc;
    uint16_t price;
    uint16_t heal;            // HP restored
    status cure;              // status cured
    bool revive;              // revives a fainted Pokémon to half HP
    bool full;                // full HP and status
};

// Held items (ITEMS, FREE BATTLE's draft and the trainers' Leftovers).
enum class held_item : uint8_t
{
    NONE,
    LEFTOVERS,
    LIFE_ORB,
    CHOICE_SCARF,
    FOCUS_SASH,
    SITRUS_BERRY
};

struct held_item_info
{
    const char* name;
    const char* desc;
};

// GBA 1.8: an item that evolves one species into another when it's used on it (stones, the LINKING CORD).
struct evo_item
{
    uint16_t from;
    uint16_t to;
    item_id item;
};

// GBA 1.8: a TM (it can be used again and again): the move it teaches, its item, and the badges the MART
// wants to see before it sells it.
struct tm_info
{
    uint16_t move;
    item_id item;
    uint8_t badges;
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
    const uint16_t* fixed_moves;    // hand-made entries without a learnset know these
    int8_t fixed_count;
    int16_t evolves_to;       // index into species_list, -1 if none
    uint8_t evolve_level;
    uint8_t capture_rate;     // Emerald's catch rate (PokeAPI), 3-255
    const ability& abil;
    uint16_t height;          // decimetres (DEXINFO)
    uint16_t weight;          // hectograms
    const char* genus;        // "Seed"
    const bn::sprite_item& front;
    const bn::sprite_item& back;
    const bn::sprite_item& icon;    // 32x32, the party screen and the PC
};

}

#endif
