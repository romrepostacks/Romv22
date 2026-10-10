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
    MAT,            // a room's exit
    STATUE          // a gym's statue (its name, leader and winners)
};

// Building kinds, as the web game names them (buildTown).
enum class door_kind : uint8_t
{
    HOUSE,
    CENTER,
    MART,
    GYM,
    LEAGUE,
    TOWER           // the CHALLENGE TOWER (SPIRECREST TOWN)
};

// People with a job in the web game's rooms (getInterior / talkTo).
enum class person_role : uint8_t
{
    NONE,
    NURSE,
    CLERK,
    MOM,
    TOWER,          // the CHALLENGE TOWER's guide (ADVENTURE MODE only)
    TRADER,         // TRADEWIND VILLAGE's trader
    DAYCARE,        // CINDERGATE TOWN's DAY CARE (GBA 1.8)
    TUTOR,          // the MOVE TUTOR in every POKéMON CENTER (GBA 1.8)
    FERRY,          // the SAILOR between PORTMERE HARBOUR and PORT CALDER (2.0.0)
    PROF,           // PORT CALDER's professor: the Calderra starter (2.0.0)
    CAPTAIN         // the ship's CAPTAIN between PORT CALDER and PORT KEEL (3.0.0)
};

// 8x8 tiles, their palette banks and 16x16 metatiles (four cells each) for an area (with what can be seen of
// its neighbours) or a kind of room. Water and flowers have a second frame: anim_tiles[i] replaces
// tiles[anim_index[i]] every other half second.
struct tileset
{
    const bn::tile* tiles;
    int tiles_count;
    const bn::color* colors;
    int colors_count;
    const uint16_t (*metatiles)[4];
    int16_t fill_metatile;          // drawn past the map's edges (forest, or black around a room)
    const uint16_t* anim_index;
    const bn::tile* anim_tiles;
    int anim_count;
};

struct sign
{
    int16_t x;
    int16_t y;
    const char* const* lines;
    int8_t lines_count;
};

struct door
{
    int16_t x;
    int16_t y;
    door_kind kind;
    int16_t room;                   // index into world_data::maps
};

struct person
{
    int16_t x;
    int16_t y;
    person_kind kind;
    direction facing;
    person_role role;
    bool wander;
    const char* const* lines;
    int8_t lines_count;
};

// Who a trainer is (startTrainerBattle): route trainers and gym juniors bring their own team; a gym leader
// or rival brings the place's signature team, filled out to your party's size.
enum class trainer_role : uint8_t
{
    ROUTE,
    JUNIOR,
    LEADER,
    RIVAL,
    ELITE,
    CHAMPION,
    TOWER           // a CHALLENGE TOWER floor's trainer (their team is made for each run)
};

// A trainer: watches up to 5 tiles ahead and battles you when they see you.
struct trainer
{
    int16_t x;
    int16_t y;
    person_kind kind;
    direction facing;
    trainer_role role;
    const char* title;
    const species_id* team;
    int8_t team_count;
    const species_id* fill;         // leaders and rivals: extra Pokémon to match your party's size
    int8_t fill_count;
    const char* const* intro;
    int8_t intro_count;
    const char* const* after;       // said when you talk to them after (a rival says it, then leaves)
    int8_t after_count;
    bool vanish;                    // a rival leaves once beaten
    uint16_t id;                    // bit in game_state::beaten
    int8_t elite;                   // the Elite Four's place in line, or -1
    int16_t area;                   // the area it belongs to (its own, or the one its room is in)
    bool scene;                     // only there while a scene puts them there (SCENES.portmere)
};

// What holds a place's exit shut (linkAreas gate): its rival, or its gym's leader.
enum class gate_kind : uint8_t
{
    NONE,
    GYM,
    RIVAL
};

struct item_ball
{
    int16_t x;
    int16_t y;
    item_id item;
    uint16_t id;                    // bit in game_state::picked
    uint16_t ground;                // metatile drawn once it's picked up
};

// A hidden item (GBA only): nothing shows but a glint now and then; A facing the spot finds it.
struct hidden_item
{
    int16_t area;
    int16_t x;
    int16_t y;
    item_id item;
    uint8_t count;
    uint16_t id;                    // bit in game_state::picked
};

// An item ball in a neighbour's strip: where it is (in the neighbour) and its picked-up metatile here.
struct strip_item
{
    int16_t x;
    int16_t y;
    uint16_t ground;
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
// What else a link needs before it opens (Calderra's CRATER RIM).
enum class link_need : uint8_t
{
    NONE,
    VISITED,                        // you've been there (walked that branch from PORT CALDER)
    SHRINES                         // every beast shrine in the region is cleared
};

// The part of the neighbour this area's tileset can draw is its strip (sx, sy, sw, sh in the neighbour's
// tiles); past it is forest.
struct link
{
    int16_t target;
    int16_t ox;
    int16_t oy;
    int16_t w;
    int16_t h;
    const char* name;
    bool gate;                      // shut until this place's leader or rival is beaten
    int8_t badges;                  // badges needed to go there (Victory Road)
    link_need need;
    int16_t sx;
    int16_t sy;
    int16_t sw;
    int16_t sh;
    const uint16_t* strip;
    const strip_item* items;
    int8_t items_count;
};

enum class area_kind : uint8_t
{
    TOWN,
    ROUTE,
    GYM,
    TRAINER
};

enum class area_theme : uint8_t
{
    PLAIN,
    FOREST,
    LAKE,
    ROCKY,
    SEA,
    DEEP,
    CAVE
};

enum class area_weather : uint8_t
{
    NONE,
    RAIN,
    SNOW,
    ASH,
    FOG,
    DEEP,
    CAVE
};

namespace area_flag
{
    constexpr uint16_t CENTER = 1;
    constexpr uint16_t LEAGUE = 2;
    constexpr uint16_t CHAMPION = 4;
    constexpr uint16_t BOSS = 8;
    constexpr uint16_t OWN_POOL = 16;     // its own wild Pokémon (the POKéDEX's AREA), not a neighbour's
    constexpr uint16_t TOWER_TOWN = 32;   // SPIRECREST TOWN: shut until you're CHAMPION
    constexpr uint16_t SAFARI = 64;       // the SAFARI ZONE: $5000 to enter, any species at all in the grass
    constexpr uint16_t TRADE_TOWN = 128;  // TRADEWIND VILLAGE (the TRADER)
    constexpr uint16_t SHRINE = 256;      // a beast's shrine (Calderra's three)
}

// What an area is (LOCATIONS): its kind, look and weather, where it sits on the region map, the layer
// below or above it (DIVE), its scene, and the guardian sleeping there.
struct area_info
{
    area_kind kind;
    area_theme theme;
    area_weather weather;
    uint16_t flags;
    int8_t tier;
    uint8_t region;                 // 1 Vellorin, 2 Calderra...
    int16_t at_x;                   // the region map's grid (DIVE's areas sit apart, at x 100 and up)
    int16_t at_y;
    int16_t dive;                   // the area beneath (map index), or -1
    int16_t surface;                // the area above, or -1
    const char* scene;
    const uint8_t* dive_spots;      // x, y pairs
    int16_t dive_count;
    const uint8_t* shafts;
    int16_t shaft_count;
    species_id legend;
    int16_t legend_x;               // -1: none
    int16_t legend_y;
    int8_t water_count;
    int8_t fish_count;
    const uint16_t* clean;          // ash areas: metatiles once the ash is swept off
};

enum class room_kind : uint8_t
{
    CENTER,
    MART,
    HOUSE,
    GYM,
    LEAGUE,
    TOWER,          // a CHALLENGE TOWER floor (one trainer)
    SUMMIT,         // the tower's top: the SUMMONING STONE
    CHAMBER         // where the summoned legendary waits (one per theme)
};

enum class gym_theme : uint8_t
{
    NONE,
    FIRE,
    WATER,
    GROUND,
    GHOST,
    ELECTRIC,
    GRASS,
    ICE,
    DRAGON,
    LEAGUE,         // (a tower room in the League's look)
    FLYING,         // Calderra's three new gyms (2.0.0)
    NORMAL,
    ROCK,
    BUG,            // the Sundered Isles' six new gyms (3.0.0)
    STEEL,
    FAIRY,
    POISON,
    FIGHTING,
    PSYCHIC
};

// A League gate: tiles x0..x1 of row y, open once that Elite Four trainer is beaten.
struct league_gate
{
    int8_t y;
    int8_t x0;
    int8_t x1;
    int8_t elite;
};

struct room_info
{
    room_kind kind;
    gym_theme theme;
    const league_gate* gates;
    int8_t gates_count;
    int16_t gate_metatile;          // the floor shown in an open gate
    bool home;
    int8_t floor;                   // CHALLENGE TOWER floors: 0-4, else -1
};

// An area or a room.
struct map_def
{
    const char* name;
    uint16_t tileset;
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
    int16_t spawn_x;
    int16_t spawn_y;
    int16_t level_cap;
    int16_t exit_map;                // rooms: the area outside, and the door you came in by
    int16_t exit_x;
    int16_t exit_y;
    int16_t leader_id;              // the trainer id of this place's rival or gym leader, or -1
    gate_kind gate;
    const char* place_name;         // gyms: the town, for the statues
    const char* leader_name;
    const area_info* area;          // areas only
    const species_id* water;        // areas: SURF and fishing pools
    const species_id* fish;
    const char* desc;
    const room_info* room;          // rooms only

    [[nodiscard]] bool is_room() const
    {
        return exit_map >= 0;
    }
};

}

#endif
