#include "bn_assert.h"
#include "bn_sram.h"
#include "pr_mon.h"
#include "pr_world_data.h"
// Walk test: SRAM 0x7E00 holds a request (map + 1); the game goes there. An assert writes "CRSH" at 0x7E10.
static void crash_mark()
{
    char tag[4] = { 'C', 'R', 'S', 'H' };
    bn::sram::write_offset(tag, 0x7E10);
}
static void test_setup()
{
    bn::assert::set_callback(crash_mark);
    int16_t req = 0;
    bn::sram::read_offset(req, 0x7E00);
    pr::game_state& g = pr::state();
    if(! pr::save_exists())
    {
        pr::reset_state();
        g.name[0]='W'; g.name[1]='A'; g.name[2]='L'; g.name[3]='K';
        g.story = pr::story::STARTER | pr::story::CALL1 | pr::story::CALL2 | pr::story::CALL3;
        g.run.skip_story = true;
        g.party[0] = pr::mon::make(pr::species_id::PIKACHU, 100);
        g.party[1] = pr::mon::make(pr::species_id::CHARMANDER, 100);
        g.party[2] = pr::mon::make(pr::species_id::EEVEE, 100);
        g.party[0].item = pr::held_item::LEFTOVERS;
        g.party[1].item = pr::held_item::SITRUS_BERRY;
        g.party[2].item = pr::held_item::CHOICE_SCARF;
        g.party_count = 3;
        g.money = 50000;
        for(int i = 0; i < 64; ++i) g.visited.set(i);
        g.extra.egg_count = 1; g.extra.eggs[0].species = 1; g.extra.eggs[0].steps = 300;
    }
    else
    {
        pr::load_game();
    }
    if(req > 0)
    {
        const pr::map_def& m = pr::world_data::maps[req - 1];
        g.map = int16_t(req - 1); g.x = m.spawn_x; g.y = m.spawn_y; g.surfing = false;
        int16_t zero = 0;
        bn::sram::write_offset(zero, 0x7E00);
    }
    pr::save_game();
}
