// Save converter test: the same digest of everything a save holds, computed in the 1.9 build and the 2.0 build.
#ifndef ITEMS
#define ITEMS 116
#endif
#include "bn_sram.h"
#include "pr_mon.h"
#include "pr_state.h"
static uint32_t H = 0;
static void h(uint32_t v) { H = H * 1000003u ^ v; }
static void hm(const pr::mon& m)
{
    h(m.species_index); h(m.level); h(m.move_count); h(uint32_t(m.st)); h(m.sleep_turns); h(uint32_t(m.item)); h(m.flags); h(m.traits);
    if(m.level) for(int i = 0; i < 4; ++i) { h(m.move(i)); h(i < m.move_count ? m.pp(i) : 0); }
    h(m.hp); h(m.max_hp); h(m.atk); h(m.def); h(m.spa); h(m.spd); h(m.spe); h(m.xp);
    for(int i = 0; i < pr::nick_length; ++i) h(uint8_t(m.nick[i]));
}
static uint32_t digest(const pr::game_state& g)
{
    H = 0;
    h(g.map); h(g.x); h(g.y); h(uint32_t(g.facing)); h(g.party_count); h(g.surfing); h(g.starter_trio);
    h(g.walk_off); h(g.last_heal); h(g.restocked); h(g.trainer_id); h(g.story); h(g.money); h(g.play_frames);
    h(uint32_t(g.opt.speed)); h(g.opt.sound); h(uint32_t(g.opt.music)); h(g.opt.exp_share); h(uint32_t(g.opt.weather));
    for(int i = 0; i < pr::name_length; ++i) h(uint8_t(g.name[i]));
    for(int i = 0; i < ITEMS; ++i) { h(g.items[i]); h(g.pc_items[i]); }
    for(int i = 0; i < pr::max_party; ++i) hm(g.party[i]);
    for(int i = 0; i < pr::box_slots; ++i) hm(g.box[i]);
    for(int b = 0; b < pr::box_count; ++b) { for(int i = 0; i < pr::box_name_length; ++i) h(uint8_t(g.box_names[b][i])); h(g.box_wall[b]); }
    for(int i = 0; i < 256; ++i) { h(g.beaten.test(i)); h(g.picked.test(i)); h(g.extra.rematched.test(i)); }
    for(int i = 0; i < 1024; ++i) { h(g.seen.test(i)); h(g.owned.test(i)); }
    for(int i = 0; i < 64; ++i) { h(g.visited.test(i)); h(g.run.encounter_used.test(i)); }
    h(uint32_t(g.run.mode)); h(g.run.skip_story); h(g.run.adventure); h(g.run.over); h(g.run.deaths); h(g.run.catches);
    h(g.run.tower_clears); h(g.run.tower_best); h(g.run.tower_streak); h(g.run.grave_next);
    for(int i = 0; i < pr::graveyard_size; ++i) hm(g.run.graveyard[i]);
    h(g.tower.active); h(g.tower.legend_shiny); h(g.tower.legend);
    h(g.extra.repel_steps); h(g.extra.rematch_day);
    for(int i = 0; i < 2; ++i) { hm(g.extra.daycare[i]); h(g.extra.daycare_steps[i]); }
    h(g.extra.egg_steps); h(g.extra.egg_waiting); h(g.extra.egg_count);
    for(int i = 0; i < pr::egg_slots; ++i) { h(g.extra.eggs[i].species); h(g.extra.eggs[i].steps); h(g.extra.eggs[i].shiny); }
    return H;
}
