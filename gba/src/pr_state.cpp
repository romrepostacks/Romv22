#include "pr_state.h"

#include "bn_common.h"
#include "bn_sram.h"

namespace pr
{

namespace
{
    constexpr char save_tag[8] = { 'P', 'R', 'O', 'Y', 'A', 'L', 'E', '1' };
    constexpr int save_version = 2;

    struct save_block
    {
        char tag[8];
        int version;
        int size;
        game_state game;
        uint32_t checksum;
    };

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

    // The save block is ~1.5 KB, so it lives in EWRAM rather than on the stack.
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
    bn::random random_generator;
}

game_state& state()
{
    return current;
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

bool game_state::add_mon(const mon& m, bool& to_box)
{
    to_box = false;
    if(party_count < max_party)
    {
        party[party_count++] = m;
        return true;
    }
    if(box_count < box_size)
    {
        box[box_count++] = m;
        to_box = true;
        return true;
    }
    return false;
}

void game_state::add_item(item_id id, int count)
{
    items[int(id)] = uint8_t(bn::min(99, items[int(id)] + count));
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
    return (sum + party_count / 2) / party_count;
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
