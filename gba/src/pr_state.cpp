#include "pr_state.h"

#include "bn_sram.h"

namespace pr
{

namespace
{
    constexpr char save_tag[8] = { 'P', 'R', 'O', 'Y', 'A', 'L', 'E', '1' };
    constexpr int save_version = 1;

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

    game_state current;
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

bool game_state::item_picked(int area_index, int tx, int ty) const
{
    for(const picked_item& p : picked)
    {
        if(p.used && p.area == area_index && p.x == tx && p.y == ty)
        {
            return true;
        }
    }
    return false;
}

void game_state::pick_item(int area_index, int tx, int ty)
{
    for(picked_item& p : picked)
    {
        if(! p.used)
        {
            p = { int8_t(area_index), int8_t(tx), int8_t(ty), 1 };
            return;
        }
    }
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

bool game_state::add_to_party(const mon& m)
{
    if(party_count >= max_party)
    {
        return false;
    }
    party[party_count++] = m;
    return true;
}

bool save_exists()
{
    save_block block;
    return read_block(block);
}

bool load_game()
{
    save_block block;
    if(! read_block(block))
    {
        return false;
    }
    current = block.game;
    return true;
}

void save_game()
{
    save_block block;
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
