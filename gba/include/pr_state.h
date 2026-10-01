// Everything that persists: where you are, your party and bag. Saved to cartridge SRAM.
#ifndef PR_STATE_H
#define PR_STATE_H

#include "bn_array.h"
#include "pr_mon.h"
#include "pr_world_types.h"

namespace pr
{

constexpr int max_party = 6;
constexpr int max_picked_items = 48;

struct picked_item
{
    int8_t area;
    int8_t x;
    int8_t y;
    int8_t used;
};

struct game_state
{
    int8_t area = 0;
    int8_t x = 0;
    int8_t y = 0;
    direction facing = direction::DOWN;
    uint8_t party_count = 0;
    uint8_t poke_balls = 0;
    uint16_t padding = 0;
    uint32_t play_frames = 0;
    bn::array<mon, max_party> party;
    bn::array<picked_item, max_picked_items> picked;

    [[nodiscard]] bool item_picked(int area_index, int tx, int ty) const;
    void pick_item(int area_index, int tx, int ty);
    [[nodiscard]] int first_able() const;       // first party member that can fight, or -1
    void heal_party();
    bool add_to_party(const mon& m);
};

// The one game in progress.
game_state& state();

// The one random generator. It advances every frame on the title screen and in the overworld, so the
// player's timing seeds it (the GBA has no clock to seed from).
bn::random& rng();

// SRAM: a tag and version, then game_state. load() returns false (and leaves state alone) if there's no
// valid save.
bool save_exists();
bool load_game();
void save_game();

}

#endif
