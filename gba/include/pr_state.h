// Everything that persists: where you are, your party, PC box and bag. Saved to cartridge SRAM.
#ifndef PR_STATE_H
#define PR_STATE_H

#include "bn_array.h"
#include "bn_random.h"
#include "pr_mon.h"
#include "pr_world_types.h"

namespace pr
{

constexpr int max_party = 6;            // partyCap() before the 4th badge
constexpr int box_size = 30;            // one PC box
constexpr int name_length = 7;          // Emerald's player names

// Story progress (the web game's adv.starterPending / adv.starterThanks).
constexpr uint8_t story_starter = 1;    // got a partner from the professor's bag
constexpr uint8_t story_thanks = 2;     // the professor still has to thank you after the first battle
constexpr uint8_t story_call1 = 4;      // the professor's call after the first badge (PROF_CALLS)

template<int Bits>
struct bitset
{
    bn::array<uint8_t, (Bits + 7) / 8> bytes = {};

    [[nodiscard]] bool test(int i) const
    {
        return i >= 0 && i < Bits && (bytes[i >> 3] >> (i & 7)) & 1;
    }
    void set(int i)
    {
        if(i >= 0 && i < Bits)
        {
            bytes[i >> 3] = uint8_t(bytes[i >> 3] | (1 << (i & 7)));
        }
    }
    [[nodiscard]] int count() const
    {
        int n = 0;
        for(int i = 0; i < Bits; ++i)
        {
            n += test(i);
        }
        return n;
    }
};

struct game_state
{
    int8_t map = 0;                     // world_data::maps index (an area or a room)
    int8_t x = 0;
    int8_t y = 0;
    direction facing = direction::DOWN;
    uint8_t party_count = 0;
    uint8_t box_count = 0;
    bool mart_gift = false;             // the clerk's free POKé BALLS (talkTo: restockedLoc)
    uint8_t story = 0;                  // story_* bits
    int8_t walk_off = -1;               // a beaten rival (trainer id) who still has to say goodbye and leave
    uint8_t padding[3] = {};
    uint32_t money = 3000;              // introFinish(): ₽3000
    uint32_t play_frames = 0;
    char name[name_length + 1] = {};
    bn::array<uint8_t, items_count> items = {};
    bn::array<mon, max_party> party;
    bn::array<mon, box_size> box;
    bitset<64> beaten;                  // route trainers (trainer::id)
    bitset<64> picked;                  // item balls (item_ball::id)
    bitset<256> seen;                   // POKéDEX, by species index
    bitset<256> owned;

    [[nodiscard]] int able_count() const;
    [[nodiscard]] int first_able() const;       // first party member that can fight, or -1
    void heal_party();
    // Party first, then the PC box. Returns false if both are full.
    bool add_mon(const mon& m, bool& to_box);
    void add_item(item_id id, int count);
    [[nodiscard]] int item_count(item_id id) const
    {
        return items[int(id)];
    }
    [[nodiscard]] int average_level() const;
    [[nodiscard]] int badges() const;           // gym leaders beaten (badgeCount)
};

// The one game in progress.
game_state& state();

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

}

#endif
