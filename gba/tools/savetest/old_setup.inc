#include "common.inc"
static void test_setup()
{
    pr::reset_state();
    pr::game_state& g = pr::state();
    const char* nm = "KYLE"; for(int i = 0; nm[i]; ++i) g.name[i] = nm[i];
    g.map = 5; g.x = 12; g.y = 34; g.facing = pr::direction::UP; g.surfing = true; g.starter_trio = 1;
    g.walk_off = 7; g.last_heal = 3; g.restocked = 2; g.trainer_id = 4242; g.story = 0x1ABCD; g.money = 123456; g.play_frames = 987654;
    g.opt.speed = pr::text_speed::FAST; g.opt.exp_share = false;
    for(int i = 0; i < ITEMS; ++i) { g.items[i] = uint8_t((i * 7) % 50); g.pc_items[i] = uint8_t(i % 9); }
    for(int i = 0; i < 6; ++i)
    {
        pr::mon& m = g.party[i];
        m = pr::mon::make(pr::species_id(i * 37 + 1), 10 + i * 13);
        for(int k = 0; k < i; ++k) m.use_pp(0);
        if(m.move_count > 1) { m.use_pp(1); m.use_pp(1); }
        m.nick[0] = 'N'; m.nick[1] = char('0' + i);
        if(i == 2) m.traits |= pr::mon_trait::SHINY;
        m.hp = uint16_t(m.max_hp / 2); m.xp = uint16_t(i * 5); m.st = pr::status(i % 3);
    }
    g.party[0].item = pr::held_item::LEFTOVERS;
    g.party_count = 6;
    for(int i = 0; i < pr::box_slots; i += 7) g.box[i] = pr::mon::make(pr::species_id((i * 11) % 800 + 1), i % 100 + 1);
    g.box[419] = pr::mon::make(pr::species_id(808), 100); g.box[419].use_pp(0);
    for(int b = 0; b < pr::box_count; ++b) { g.box_names[b][0] = 'B'; g.box_names[b][1] = char('A' + b); g.box_wall[b] = uint8_t(b); }
    for(int i = 0; i < 256; ++i) { if(i % 3 == 0) g.beaten.set(i); if(i % 5 == 0) g.picked.set(i); if(i % 11 == 0) g.extra.rematched.set(i); }
    for(int i = 0; i < 1024; ++i) { if(i % 2) g.seen.set(i); if(i % 4 == 0) g.owned.set(i); }
    for(int i = 0; i < 64; ++i) { if(i % 3) g.visited.set(i); if(i % 7 == 0) g.run.encounter_used.set(i); }
    g.run.adventure = true; g.run.skip_story = true; g.run.deaths = 7; g.run.catches = 99;
    g.run.tower_clears = 3; g.run.tower_best = 2; g.run.tower_streak = 1; g.run.grave_next = 1;
    g.run.graveyard[0] = pr::mon::make(pr::species_id(25), 33);
    g.tower.legend = 150; g.tower.legend_shiny = true;
    g.extra.repel_steps = 50; g.extra.rematch_day = 9;
    g.extra.daycare[0] = pr::mon::make(pr::species_id(133), 40); g.extra.daycare_steps[0] = 777;
    g.extra.egg_steps = 12; g.extra.egg_waiting = true; g.extra.egg_count = 2;
    g.extra.eggs[0].species = 1; g.extra.eggs[0].steps = 300; g.extra.eggs[1].species = 4; g.extra.eggs[1].shiny = true;
    pr::save_game();
    uint32_t d = digest(g);
    bn::sram::write_offset(d, 0x7E20);
    while(true) { }
}
