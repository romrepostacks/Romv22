#include "pr_state.h"

#include "bn_common.h"
#include "bn_memory.h"
#include "bn_sram.h"

#include "pr_world_data.h"

namespace pr
{

namespace
{
    constexpr char save_tag[8] = { 'P', 'R', 'O', 'Y', 'A', 'L', 'E', '1' };
    constexpr int save_version = 4;

    struct save_block
    {
        char tag[8];
        int version;
        int size;
        game_state game;
        uint32_t checksum;
    };

    static_assert(sizeof(save_block) <= 32 * 1024, "the save must fit in SRAM");

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
        return block.version == save_version && block.size == int(sizeof(game_state)) && block.checksum == checksum_of(block);
    }

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
    // Each time of day lasts 30 minutes of play (1800 s), so a full day is 2 hours.
    return time_of_day((current.play_frames / 60 / 1800) % 4);
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
