// Overworld data types; the data itself is generated into pr_world_data.h by tools/build_assets.py.
#ifndef PR_WORLD_TYPES_H
#define PR_WORLD_TYPES_H

#include <cstdint>
#include "bn_color.h"
#include "bn_tile.h"
#include "pr_ids.h"

namespace pr
{

enum class direction : uint8_t
{
    DOWN,
    UP,
    LEFT,
    RIGHT
};

// What a tile does (tools/export.js BEHAVIOUR).
enum class behaviour : uint8_t
{
    WALK,
    SOLID,
    TALL_GRASS,
    WATER,
    LEDGE_DOWN,
    LEDGE_RIGHT,
    LEDGE_LEFT,
    SIGN,
    DOOR,
    ITEM,
    COUNTER,        // talk across it
    PC,
    MAT             // a room's exit
};

// Building kinds, as the web game names them (buildTown).
enum class door_kind : uint8_t
{
    HOUSE,
    CENTER,
    MART,
    GYM,
    LEAGUE
};

// People with a job in the web game's rooms (getInterior / talkTo).
enum class person_role : uint8_t
{
    NONE,
    NURSE,
    CLERK,
    MOM
};

// 8x8 tiles, their palette banks and 16x16 metatiles (four cells each) for a group of maps.
struct tileset
{
    const bn::tile* tiles;
    int tiles_count;
    const bn::color* colors;
    int colors_count;
    const uint16_t (*metatiles)[4];
    int16_t fill_metatile;          // drawn past the map's edges (forest, or black around a room)
    int16_t grass_metatile;         // drawn where an item ball was picked up
};

struct sign
{
    int8_t x;
    int8_t y;
    const char* const* lines;
    int8_t lines_count;
};

struct door
{
    int8_t x;
    int8_t y;
    door_kind kind;
    int8_t room;                    // index into world_data::maps
};

struct person
{
    int8_t x;
    int8_t y;
    person_kind kind;
    direction facing;
    person_role role;
    bool wander;
    const char* const* lines;
    int8_t lines_count;
};

// A route trainer (buildRoute): watches up to 5 tiles ahead and battles with the first `party size`
// Pokémon of their team.
struct trainer
{
    int8_t x;
    int8_t y;
    person_kind kind;
    direction facing;
    const char* title;
    const species_id* team;
    int8_t team_count;
    const char* intro;
    const char* after;
    int8_t id;                      // bit in game_state::beaten
};

struct item_ball
{
    int8_t x;
    int8_t y;
    item_id item;
    int8_t id;                      // bit in game_state::picked
};

// Furniture you can look at (THING_TEXT).
struct thing
{
    int8_t x;
    int8_t y;
    const char* const* lines;
    int8_t lines_count;
};

// A connected area, placed at (ox, oy) in this map's tile coordinates (Emerald-style seamless connections).
// target is the index into world_data::maps, or -1 for an area that isn't in this build yet.
struct link
{
    int8_t target;
    int16_t ox;
    int16_t oy;
    int16_t w;
    int16_t h;
    const char* name;
};

// An area or a room.
struct map_def
{
    const char* name;
    int8_t tileset;
    int16_t w;
    int16_t h;
    const uint16_t* map;            // metatile per tile
    const uint8_t* behaviours;      // behaviour per tile
    const sign* signs;
    int8_t signs_count;
    const door* doors;
    int8_t doors_count;
    const person* people;
    int8_t people_count;
    const trainer* trainers;
    int8_t trainers_count;
    const item_ball* items;
    int8_t items_count;
    const thing* things;
    int8_t things_count;
    const link* links;
    int8_t links_count;
    const species_id* pool;
    int8_t pool_count;
    int8_t spawn_x;
    int8_t spawn_y;
    int8_t level_cap;
    int8_t exit_map;                // rooms: the area outside, and the door you came in by
    int8_t exit_x;
    int8_t exit_y;

    [[nodiscard]] bool is_room() const
    {
        return exit_map >= 0;
    }
};

}

#endif
