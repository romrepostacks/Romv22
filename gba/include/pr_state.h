// Everything that persists (the web game's adv): where you are, your party, the PC's boxes, the bag, the
// story and the options. Saved to cartridge SRAM.
#ifndef PR_STATE_H
#define PR_STATE_H

#include "bn_array.h"
#include "bn_random.h"
#include "pr_mon.h"
#include "pr_world_types.h"

namespace pr
{

constexpr int max_party = 10;           // MAX_PARTY: screens lay out 10 slots
constexpr int small_party = 6;          // partyCap() before the 4th badge
constexpr int box_size = 30;            // one PC box
constexpr int box_count = 14;           // Emerald's 14 boxes
constexpr int box_slots = box_size * box_count;
constexpr int name_length = 12;         // the web game's names (maxlength 12)
constexpr int box_name_length = 8;

// Save version 11 (2.0.0) sizes the save for all nine planned regions, so its layout never has to move again:
// new things go in the spare bytes, and zero always means "not set yet".
constexpr int bag_kinds = 256;          // item kinds the bag can hold (items_count is today's)
constexpr int dex_size = 2048;          // POKéDEX entries (1025 species today, room for more)
constexpr int trainer_slots = 2048;     // trainer::id
constexpr int item_ball_slots = 2048;   // item_ball::id
constexpr int area_slots = 1024;        // areas (visited, NUZLOCKE encounters)
constexpr int flag_slots = 1024;        // story flags beyond `story` (Calderra and later regions)
constexpr int save_spare = 1024;

// Story progress (adv.starterPending / starterThanks / story.*), as bits.
namespace story
{
    constexpr uint32_t STARTER = 1u << 0;         // got a partner from the professor's bag
    constexpr uint32_t THANKS = 1u << 1;          // the professor still has to thank you after the first battle
    constexpr uint32_t CALL1 = 1u << 2;           // PROF_CALLS
    constexpr uint32_t CALL2 = 1u << 3;
    constexpr uint32_t CALL3 = 1u << 4;
    constexpr uint32_t SCENE_TEMPEST_RUN = 1u << 5;   // SCENES, once each
    constexpr uint32_t SCENE_PORTMERE = 1u << 6;
    constexpr uint32_t SCENE_HIDEOUT = 1u << 7;
    constexpr uint32_t SCENE_SHRINE = 1u << 8;
    constexpr uint32_t SURF_GIFT = 1u << 9;       // Sable still has to hand over HM03 (afterStory)
    constexpr uint32_t DIVE_GIFT = 1u << 10;      // Hale, HM08
    constexpr uint32_t LEGEND_FIGHT = 1u << 11;   // just fought the guardian (afterStory)
    constexpr uint32_t STORM_ENDED = 1u << 12;
    constexpr uint32_t LEGEND_CAUGHT = 1u << 13;
    constexpr uint32_t CHAMPION = 1u << 14;       // saw the credits
    constexpr uint32_t OLD_ROD = 1u << 15;        // (unused: the rod is an item)
    constexpr uint32_t MASTER_GIFT = 1u << 16;    // WREN's MASTER BALL at the Sunken Shrine (GBA only)
    // The cheat menu's switches.
    constexpr uint32_t CHEAT_NO_WILD = 1u << 30;     // no wild Pokémon in grass, caves or on the water
    constexpr uint32_t CHEAT_PERFECT_CATCH = 1u << 31;   // every POKé BALL catches
}

enum class text_speed : uint8_t
{
    SLOW,
    MID,
    FAST
};

enum class level4 : uint8_t
{
    OFF,
    LOW,
    MID,
    HIGH
};

// OPTION (optionOpen): TEXT SPEED, SOUND, MUSIC, EXP SHARE, WEATHER.
struct options
{
    text_speed speed = text_speed::MID;
    bool sound = true;
    level4 music = level4::MID;
    bool exp_share = true;
    level4 weather = level4::LOW;
};

template<int Bits>
struct bitset
{
    bn::array<uint8_t, (Bits + 7) / 8> bytes = {};

    [[nodiscard]] bool test(int i) const
    {
        return i >= 0 && i < Bits && (bytes[i >> 3] >> (i & 7)) & 1;
    }
    void set(int i, bool on = true)
    {
        if(i >= 0 && i < Bits)
        {
            if(on)
            {
                bytes[i >> 3] = uint8_t(bytes[i >> 3] | (1 << (i & 7)));
            }
            else
            {
                bytes[i >> 3] = uint8_t(bytes[i >> 3] & ~(1 << (i & 7)));
            }
        }
    }
    void reset(int i)
    {
        set(i, false);
    }
    [[nodiscard]] int count() const
    {
        int n = 0;
        for(uint8_t b : bytes)
        {
            for(; b; b &= uint8_t(b - 1))
            {
                ++n;
            }
        }
        return n;
    }
};

// Phase 6 and 7: how this run is played. NUZLOCKE (chosen at NEW GAME) enforces the classic rules; ADVENTURE
// MODE follows the credits (or NEW ADVENTURE MODE) with the CHALLENGE TOWER as the goal.
enum class run_mode : uint8_t
{
    NORMAL,
    NUZLOCKE
};

constexpr int graveyard_size = 30;      // the latest who fell (the trainer card counts them all)

struct run_state
{
    run_mode mode = run_mode::NORMAL;
    bool skip_story = false;            // SKIP STORY TEXT: scenes and calls complete themselves
    bool adventure = false;             // ADVENTURE MODE (never in a Nuzlocke run)
    bool over = false;                  // NUZLOCKE: the party whited out, the run has ended
    uint16_t deaths = 0;
    uint16_t catches = 0;
    bitset<area_slots> encounter_used;  // NUZLOCKE: areas whose one encounter is spent
    uint8_t tower_clears = 0;           // CHALLENGE TOWER: rank = clears + 1
    uint8_t tower_best = 0;             // best streak of clears in a row
    uint8_t tower_streak = 0;
    uint8_t grave_next = 0;             // where the next one goes (it wraps)
    mon graveyard[graveyard_size];

    [[nodiscard]] bool nuzlocke() const
    {
        return mode == run_mode::NUZLOCKE;
    }
};

// The CHALLENGE TOWER run in progress (save version 6).
struct tower_state
{
    bool active = false;                // inside, on a challenge (healing only from the bag)
    bool legend_shiny = false;          // the summoned legendary's shiny roll
    int16_t legend = -1;                // the species the SUMMONING STONE called, waiting in its chamber
};

// GBA 1.8 (save version 9): REPEL, trainer rematches, the DAY CARE and the EGGS you carry. All zero to start.
constexpr int egg_slots = 6;
constexpr int egg_hatch_steps = 600;
constexpr int egg_lay_steps = 256;      // the DAY CARE checks for an EGG this often

struct egg
{
    uint16_t species = 0;
    uint16_t steps = 0;                 // left before it hatches
    bool shiny = false;
};

struct extras_state
{
    uint16_t repel_steps = 0;           // wild POKéMON stay away while this counts down
    uint16_t rematch_day = 0;           // the day (of play time) `rematched` is for
    bitset<trainer_slots> rematched;    // route trainers beaten again today
    mon daycare[2];                     // level 0: empty
    uint32_t daycare_steps[2] = {};     // steps walked since it was left (its EXP)
    uint16_t egg_steps = 0;             // toward the DAY CARE's next check for an EGG
    bool egg_waiting = false;           // the DAY CARE lady has an EGG for you
    uint8_t egg_count = 0;
    egg eggs[egg_slots];
};

struct game_state
{
    int16_t map = 0;                    // world_data::maps index (an area or a room)
    int16_t x = 0;
    int16_t y = 0;
    direction facing = direction::DOWN;
    uint8_t party_count = 0;
    bool surfing = false;
    uint8_t starter_trio = 2;           // STARTER_TRIOS index, picked at random for each new game
    int16_t walk_off = -1;              // a beaten rival (trainer id) who still has to say goodbye and leave
    int16_t last_heal = -1;             // the area whose POKéMON CENTER (or home) you last healed at
    int16_t restocked = -1;             // the area where the clerk last gave you free POKé BALLS
    uint16_t trainer_id = 0;            // TRAINER CARD IDNo.
    uint32_t story = 0;                 // story:: bits
    uint32_t money = 3000;              // introFinish(): ₽3000
    uint32_t play_frames = 0;
    options opt;
    char name[name_length + 1] = {};
    bn::array<uint8_t, bag_kinds> items = {};
    bn::array<uint8_t, bag_kinds> pc_items = {};
    bn::array<mon, max_party> party;
    bn::array<mon, box_slots> box;      // box b holds b * 30 ... b * 30 + 29; empty slots have level 0
    char box_names[box_count][box_name_length + 1] = {};
    uint8_t box_wall[box_count] = {};   // 0: the box's default
    bitset<trainer_slots> beaten;       // trainers (trainer::id); a rival or leader beaten = the place cleared
    bitset<item_ball_slots> picked;     // item balls (item_ball::id)
    bitset<dex_size> seen;              // POKéDEX, by species index
    bitset<dex_size> owned;
    bitset<area_slots> visited;         // areas you've been to (the region map, the TRAINER CARD)
    run_state run;
    tower_state tower;
    extras_state extra;
    // Save version 11 (2.0.0).
    uint8_t region = 0;                 // 0 Vellorin, 1 Calderra, ...
    uint8_t spare_bytes[3] = {};
    bitset<flag_slots> flags;           // story flags for Calderra and later regions
    int16_t roam[4] = {};               // Calderra's roaming beasts (legend_flag): the map each is on + 1, 0 if not
    uint16_t roam_hp[4] = {};           // 2.0.1: a roaming beast's HP left from your last battle, 0 if unhurt
    uint8_t spare[save_spare - 16] = {}; // zero; later versions add fields here without moving anything

    [[nodiscard]] int able_count() const;
    [[nodiscard]] int first_able() const;       // first party member that can fight, or -1
    void heal_party();
    [[nodiscard]] int party_cap() const;        // 6, then 10 from the 4th badge (partyCap)
    // Party first (up to party_cap), then the first free box slot. Returns false if both are full;
    // box_slot gets the slot used, or -1 for the party.
    bool add_mon(const mon& m, int& box_slot);
    [[nodiscard]] int box_used() const;
    void add_item(item_id id, int count);
    [[nodiscard]] int item_count(item_id id) const
    {
        return items[int(id)];
    }
    [[nodiscard]] int average_level() const;    // partyAvgLevel, rounded
    [[nodiscard]] int badges() const;           // Vellorin's gym leaders beaten (badgeCount)
    [[nodiscard]] int region_badges(int region) const;  // a region's gym leaders beaten (1 Vellorin, 2 Calderra)
    [[nodiscard]] bool has(uint32_t bits) const
    {
        return (story & bits) == bits;
    }
    void mark_seen(int species);
    bool mark_owned(int species);               // true the first time
};

// The one game in progress.
game_state& state();
// A fresh game (introFinish), in place: the state is too big for a temporary on the stack.
void reset_state();
bool game_active();
void set_game_active(bool active);

// The time of day (timeOfDay): 30 minutes of play each, morning first.
enum class time_of_day
{
    MORNING,
    DAY,
    EVENING,
    NIGHT
};
time_of_day current_time_of_day();
const char* time_of_day_name(time_of_day t);

// The one random generator. It advances every frame on the title screen and in the overworld, so the
// player's timing seeds it (the GBA has no clock to seed from).
bn::random& rng();

// SRAM: a tag and version, then game_state and a checksum. load_game() returns false (and leaves the state
// alone) if there's no valid save.
bool save_exists();
bool load_game();
void save_game();
// For the CONTINUE window: the saved game's name, play time and POKéDEX count.
bool peek_save(game_state& out);

// The device unlock (partyroyale_cleared): set by the first clear of the main story on this cartridge, kept
// apart from the save so a new game doesn't lose it. It offers NEW ADVENTURE MODE on the title.
bool device_cleared();
void set_device_cleared();

// NUZLOCKE's level cap: the next gym leader's (the League's after the eighth badge); otherwise 100, or 200 once
// you've reached Calderra.
int level_cap_now();

// The region a map is in (1 Vellorin, 2 Calderra...): a room's is its area's.
int map_region(int map);

// The level cap where a map is (advLevel). Vellorin's is the place's own; Calderra's branches go in any order,
// so its levels follow your Calderra badges instead: 100, then 12 more per badge.
int map_level_cap(int map);

// Story flags beyond `story` (bits in game_state::flags); 0-3 are Calderra's legendaries (legend_flag).
namespace flag
{
    constexpr int CALDERRA_CHAMPION = 4;
    constexpr int CALDERRA_STARTER = 5;
    constexpr int GROUDON = 6;              // 3.0.0: the Sundered Isles' legendaries caught (legend_flag)
    constexpr int KYOGRE = 7;
    constexpr int SUNDERED_CHAMPION = 8;
    constexpr int RAYQUAZA = 9;             // 4.0.0: the SKYREACH's legendaries caught (legend_flag)
    constexpr int DEOXYS = 10;
    constexpr int SKYREACH_CHAMPION = 11;
}

// Calderra's legendaries caught (bits in game_state::flags): the three beasts and HO-OH, and the Sundered Isles'
// GROUDON and KYOGRE; -1 for any other.
int legend_flag(species_id legend);
bool legend_caught(species_id legend);

// Calderra's roaming beasts (2.0.0): each runs from its shrine and roams its branch's routes, moving every
// time you change area, until you catch it.
species_id roamer_species(int f);
void start_roaming(int f);
void roamers_move();

// Every beast shrine (a BOSS area with a guardian, other than the story legendary's) in a region cleared.
bool shrines_cleared(int region);
int calderra_level(int badges);

// 3.0.0: a region's top level (100 Vellorin, 200 Calderra, 300 the Sundered Isles), and the level its badges
// bring from Calderra on: 100 under the top, then 12 more per badge.
int region_cap(int region);
int region_level(int region, int badges);

// A wild (or summoned) Pokémon's shiny roll: 1 in 4096, x2 in ADVENTURE MODE, x1.25 in a NUZLOCKE run.
bool roll_shiny();

}

#endif
