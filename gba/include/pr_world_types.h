// Overworld data types; the data itself is generated into pr_world_data.h by tools/build_assets.py.
#ifndef PR_WORLD_TYPES_H
#define PR_WORLD_TYPES_H

#include <cstdint>
#include "pr_species_id.h"

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
    ITEM
};

struct sign
{
    int8_t x;
    int8_t y;
    const char* text;
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

struct door
{
    int8_t x;
    int8_t y;
    door_kind kind;
};

struct person
{
    int8_t x;
    int8_t y;
    person_kind kind;
    direction facing;
    const char* const* lines;
    int8_t lines_count;
};

// A connected area, placed at (ox, oy) in this area's tile coordinates (Emerald-style seamless connections).
// target is the index into world_data::areas, or -1 for an area that isn't in this build yet.
struct link
{
    int8_t target;
    int16_t ox;
    int16_t oy;
    int16_t w;
    int16_t h;
    const char* name;
};

struct area
{
    const char* name;
    int16_t w;
    int16_t h;
    const uint16_t* map;          // metatile per tile
    const uint8_t* behaviours;    // behaviour per tile
    const sign* signs;
    int8_t signs_count;
    const door* doors;
    int8_t doors_count;
    const person* people;
    int8_t people_count;
    const link* links;
    int8_t links_count;
    const species_id* pool;
    int8_t pool_count;
    int8_t spawn_x;
    int8_t spawn_y;
    int8_t level_cap;
};

}

#endif
