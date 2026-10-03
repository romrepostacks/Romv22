#include "pr_state.h"

#include "bn_common.h"
#include <cstddef>
#include "bn_memory.h"
#include "bn_sram.h"

#include "pr_world_data.h"

namespace pr
{

namespace
{
    constexpr char save_tag[8] = { 'P', 'R', 'O', 'Y', 'A', 'L', 'E', '1' };
    constexpr int save_version = 8;
    // Older saves: game_state up to `visited` (save version 4: GBA 1.0), then `run` (5: GBA 1.1), then `tower` (6:
    // GBA 1.2-1.4). Their bag had 13 kinds of items; version 7 (GBA 1.5) added three and version 8 (GBA 1.6) one
    // more, each moving everything after the bag along.
    constexpr int v4_game_size = 20324;
    constexpr int v5_game_size = 21724;
    constexpr int v6_game_size = 21728;
    constexpr int v7_game_size = 21736;     // GBA 1.5: 16 kinds of items (version 8, GBA 1.6: the MASTER BALL)
    constexpr int old_items_count = 13;
    constexpr int bag_growth = 2 * (items_count - old_items_count);     // items and pc_items
    static_assert(offsetof(game_state, run) == v4_game_size + bag_growth, "save version 4 must be game_state's prefix");
    static_assert(offsetof(game_state, tower) == v5_game_size + bag_growth, "save version 5 must be game_state's prefix");
    static_assert(offsetof(game_state, pc_items) == offsetof(game_state, items) + items_count, "the bag's layout");
    static_assert(offsetof(game_state, party) == offsetof(game_state, pc_items) + items_count, "the bag's layout");

    struct save_block
    {
        char tag[8];
        int version;
        int size;
        game_state game;
        uint32_t checksum;
    };

    static_assert(sizeof(save_block) <= 32 * 1024, "the save must fit in SRAM");
    static_assert(sizeof(game_state) > 0, "");

    uint32_t checksum_of(const save_block& block)
    {
        const auto* bytes = reinterpret_cast<const uint8_t*>(&block);
        int length = reinterpret_cast<const uint8_t*>(&block.checksum) - bytes;
        uint32_t sum = 0x1234;
        for(int i = 0; i < length; ++i)
        {
            sum = (sum * 31) + bytes[i];
        }
        return sum;
    }

    // The save block is ~21 KB, so it lives in EWRAM rather than on the stack.
    BN_DATA_EWRAM_BSS save_block block_buffer;

    bool read_block(save_block& block)
    {
        bn::sram::read(block);
        for(int i = 0; i < 8; ++i)
        {
            if(block.tag[i] != save_tag[i])
            {
                return false;
            }
        }
        if((block.version == 4 && block.size == v4_game_size) || (block.version == 5 && block.size == v5_game_size) ||
           (block.version == 6 && block.size == v6_game_size) || (block.version == 7 && block.size == v7_game_size))
        {
            // An older save: its checksum right after its game_state.
            int old_size = block.size;
            const auto* bytes = reinterpret_cast<const uint8_t*>(&block);
            int length = int(offsetof(save_block, game)) + old_size;
            uint32_t sum = 0x1234;
            for(int i = 0; i < length; ++i)
            {
                sum = (sum * 31) + bytes[i];
            }
            uint32_t stored = uint32_t(bytes[length]) | (uint32_t(bytes[length + 1]) << 8) |
                              (uint32_t(bytes[length + 2]) << 16) | (uint32_t(bytes[length + 3]) << 24);
            if(stored != sum)
            {
                return false;
            }
            // The bag grew: move everything after it along, then the PC's items, and clear the new kinds.
            auto* g = reinterpret_cast<uint8_t*>(&block.game);
            int items_at = int(offsetof(game_state, items)), pc_at = int(offsetof(game_state, pc_items));
            int party_at = int(offsetof(game_state, party));
            int old_items = block.version == 7 ? 16 : old_items_count;
            int old_pc_at = items_at + old_items, old_party_at = old_pc_at + old_items;
            int tail = old_size - old_party_at;
            for(int i = tail - 1; i >= 0; --i)
            {
                g[party_at + i] = g[old_party_at + i];
            }
            for(int i = old_items - 1; i >= 0; --i)
            {
                g[pc_at + i] = g[old_pc_at + i];
            }
            for(int i = items_at + old_items; i < pc_at; ++i)
            {
                g[i] = 0;
            }
            for(int i = pc_at + old_items; i < party_at; ++i)
            {
                g[i] = 0;
            }
            for(int i = party_at + tail; i < int(sizeof(game_state)); ++i)
            {
                g[i] = 0;
            }
            if(block.version < 5)
            {
                block.game.run = run_state();
            }
            if(block.version < 6)
            {
                block.game.tower = tower_state();
                // SPIRECREST TOWN came in as area 34, so every room moved up one.
                if(block.game.map >= 34)
                {
                    block.game.map = int16_t(block.game.map + 1);
                }
            }
            // TRADEWIND VILLAGE and the SAFARI ZONE came in as areas 35 and 36: the rooms moved up two more.
            if(block.version < 7 && block.game.map >= 35)
            {
                block.game.map = int16_t(block.game.map + 2);
            }
            block.version = save_version;
            block.size = int(sizeof(game_state));
            return true;
        }
        return block.version == save_version && block.size == int(sizeof(game_state)) && block.checksum == checksum_of(block);
    }

    // The device unlock, in the SRAM's last 16 bytes.
    struct clear_mark
    {
        char tag[8];
        uint32_t cleared;
        uint32_t check;
    };
    constexpr int clear_mark_offset = 32 * 1024 - int(sizeof(clear_mark));
    static_assert(sizeof(save_block) <= clear_mark_offset, "the save must leave room for the clear mark");
    constexpr char clear_tag[8] = { 'P', 'R', 'C', 'L', 'E', 'A', 'R', '1' };

    BN_DATA_EWRAM_BSS game_state current;
    bool active = false;
    bn::random random_generator;
}

game_state& state()
{
    return current;
}

void reset_state()
{
    auto* bytes = reinterpret_cast<uint8_t*>(&current);
    for(int i = 0; i < int(sizeof(game_state)); ++i)
    {
        bytes[i] = 0;
    }
    current.walk_off = -1;
    current.last_heal = -1;
    current.restocked = -1;
    current.money = 3000;
    current.opt = options();
    current.starter_trio = 2;
    current.run = run_state();
    current.tower = tower_state();
}

bool game_active()
{
    return active;
}

void set_game_active(bool on)
{
    active = on;
}

time_of_day current_time_of_day()
{
    // Each time of day lasts 7.5 minutes of play (450 s), so a full day is 30 minutes.
    return time_of_day((current.play_frames / 60 / 450) % 4);
}

const char* time_of_day_name(time_of_day t)
{
    constexpr const char* names[] = { "MORNING", "DAY", "EVENING", "NIGHT" };
    return names[int(t)];
}

bn::random& rng()
{
    return random_generator;
}

int game_state::able_count() const
{
    int n = 0;
    for(int i = 0; i < party_count; ++i)
    {
        n += ! party[i].fainted();
    }
    return n;
}

int game_state::first_able() const
{
    for(int i = 0; i < party_count; ++i)
    {
        if(! party[i].fainted())
        {
            return i;
        }
    }
    return -1;
}

void game_state::heal_party()
{
    for(int i = 0; i < party_count; ++i)
    {
        party[i].heal();
    }
}

int game_state::party_cap() const
{
    return badges() >= 4 ? max_party : small_party;
}

bool game_state::add_mon(const mon& m, int& box_slot)
{
    box_slot = -1;
    if(party_count < party_cap())
    {
        party[party_count++] = m;
        return true;
    }
    for(int i = 0; i < box_slots; ++i)
    {
        if(box[i].empty())
        {
            box[i] = m;
            box[i].heal();      // the Box heals (saveAdv)
            box_slot = i;
            return true;
        }
    }
    return false;
}

int game_state::box_used() const
{
    int n = 0;
    for(const mon& m : box)
    {
        n += ! m.empty();
    }
    return n;
}

void game_state::add_item(item_id id, int count)
{
    items[int(id)] = uint8_t(bn::min(255, items[int(id)] + count));
}

int game_state::average_level() const
{
    if(! party_count)
    {
        return 5;
    }
    int sum = 0;
    for(int i = 0; i < party_count; ++i)
    {
        sum += party[i].level;
    }
    // Math.round(sum / n)
    return (2 * sum + party_count) / (2 * party_count);
}

int game_state::badges() const
{
    int n = 0;
    for(int i = 0; i < world_data::areas_count; ++i)
    {
        const map_def& m = world_data::maps[i];
        if(m.gate == gate_kind::GYM && m.leader_id >= 0)
        {
            n += beaten.test(m.leader_id);
        }
    }
    return n;
}

void game_state::mark_seen(int species)
{
    seen.set(species);
}

bool game_state::mark_owned(int species)
{
    seen.set(species);
    bool first = ! owned.test(species);
    owned.set(species);
    return first;
}

bool device_cleared()
{
    clear_mark m;
    bn::sram::read_offset(m, clear_mark_offset);
    for(int i = 0; i < 8; ++i)
    {
        if(m.tag[i] != clear_tag[i])
        {
            return false;
        }
    }
    return m.cleared == 1 && m.check == 0x5eed1234u;
}

void set_device_cleared()
{
    clear_mark m;
    for(int i = 0; i < 8; ++i)
    {
        m.tag[i] = clear_tag[i];
    }
    m.cleared = 1;
    m.check = 0x5eed1234u;
    bn::sram::write_offset(m, clear_mark_offset);
}

bool roll_shiny()
{
    // Out of 16384: 4 (1/4096), 8 in ADVENTURE MODE, 5 in a NUZLOCKE run.
    const run_state& run = state().run;
    int chances = run.adventure ? 8 : run.nuzlocke() ? 5 : 4;
    return rng().get_int(16384) < chances;
}

int level_cap_now()
{
    const game_state& g = state();
    if(! g.run.nuzlocke())
    {
        return 100;
    }
    // The first gym (in the region's order) whose leader you haven't beaten; after the eighth, the League.
    for(int i = 0; i < world_data::areas_count; ++i)
    {
        const map_def& m = world_data::maps[i];
        if(m.area && m.area->kind == area_kind::GYM && m.leader_id >= 0 && ! g.beaten.test(m.leader_id))
        {
            return m.level_cap;
        }
    }
    for(int i = 0; i < world_data::areas_count; ++i)
    {
        const map_def& m = world_data::maps[i];
        if(m.area && (m.area->flags & area_flag::CHAMPION) && ! (m.leader_id >= 0 && g.beaten.test(m.leader_id)))
        {
            return m.level_cap;
        }
    }
    return 100;
}

bool save_exists()
{
    return read_block(block_buffer);
}

bool peek_save(game_state& out)
{
    if(! read_block(block_buffer))
    {
        return false;
    }
    out = block_buffer.game;
    return true;
}

bool load_game()
{
    if(! read_block(block_buffer))
    {
        return false;
    }
    current = block_buffer.game;
    return true;
}

void save_game()
{
    save_block& block = block_buffer;
    // The Box heals (saveAdv, tester #27).
    for(mon& m : current.box)
    {
        if(! m.empty())
        {
            m.heal();
        }
    }
    for(int i = 0; i < 8; ++i)
    {
        block.tag[i] = save_tag[i];
    }
    block.version = save_version;
    block.size = int(sizeof(game_state));
    block.game = current;
    block.checksum = checksum_of(block);
    bn::sram::write(block);
}

}
