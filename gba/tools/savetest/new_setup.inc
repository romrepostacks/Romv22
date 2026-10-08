#include "common.inc"
static void test_setup()
{
    pr::game_state& g = pr::state();
    uint32_t a = pr::load_game() ? digest(g) : 1;
    bn::sram::write_offset(a, 0x7E24);
    pr::save_game();                    // now a version 11 save
    pr::reset_state();
    uint32_t b = pr::load_game() ? digest(g) : 2;
    bn::sram::write_offset(b, 0x7E28);
    while(true) { }
}
