#include "bn_assert.h"
#include "bn_sram.h"
#include "pr_mon.h"
#include "pr_world_data.h"
#include "pr_game_data.h"
#include "bn_timer.h"
#include "bn_sprite_ptr.h"
// Play test (3.0.0): SRAM 0x7E02 holds a scenario; test_setup() builds a save for it from scratch (Vellorin and,
// unless the scenario says otherwise, Calderra already beaten) and puts you next to whoever it tests.
// An assert writes "CRSH" at 0x7E10.
static void crash_mark()
{
    char tag[4] = { 'C', 'R', 'S', 'H' };
    bn::sram::write_offset(tag, 0x7E10);
}
namespace play
{
namespace wd = pr::world_data;
static bool walkable(const pr::map_def& m, int x, int y, pr::behaviour b = pr::behaviour::WALK)
{
    if(x < 0 || y < 0 || x >= m.w || y >= m.h ||
       (pr::behaviour(m.behaviours[y * m.w + x]) != b && ! (b == pr::behaviour::WALK && pr::behaviour(m.behaviours[y * m.w + x]) == pr::behaviour::TALL_GRASS)))
    {
        return false;
    }
    for(int i = 0; i < m.people_count; ++i)
    {
        if(m.people[i].x == x && m.people[i].y == y) return false;
    }
    for(int i = 0; i < m.trainers_count; ++i)
    {
        if(m.trainers[i].x == x && m.trainers[i].y == y) return false;
    }
    return true;
}
// Stand next to (tx, ty) facing it, below first.
static void stand_by(int map, int tx, int ty)
{
    pr::game_state& g = pr::state();
    const pr::map_def& m = wd::maps[map];
    constexpr int dx[] = { 0, 0, -1, 1 }, dy[] = { 1, -1, 0, 0 };
    constexpr pr::direction face[] = { pr::direction::UP, pr::direction::DOWN, pr::direction::RIGHT, pr::direction::LEFT };
    g.map = int16_t(map);
    for(int k = 0; k < 4; ++k)
    {
        if(walkable(m, tx + dx[k], ty + dy[k]))
        {
            g.x = int16_t(tx + dx[k]); g.y = int16_t(ty + dy[k]); g.facing = face[k];
            return;
        }
    }
    for(int k = 0; k < 4; ++k)
    {
        if(walkable(m, tx + dx[k], ty + dy[k], pr::behaviour::WATER))
        {
            g.x = int16_t(tx + dx[k]); g.y = int16_t(ty + dy[k]); g.facing = face[k]; g.surfing = true;
            return;
        }
    }
    g.x = m.spawn_x; g.y = m.spawn_y;
}
static void beat_region(int region, bool leaders_only = false)
{
    pr::game_state& g = pr::state();
    for(int i = 0; i < wd::maps_count; ++i)
    {
        if(pr::map_region(i) != region) continue;
        const pr::map_def& m = wd::maps[i];
        for(int k = 0; k < m.trainers_count; ++k)
        {
            if(! leaders_only || m.trainers[k].role == pr::trainer_role::LEADER) g.beaten.set(m.trainers[k].id);
        }
        g.visited.set(i);
    }
}
static int find_person(int region, pr::person_role role, int& x, int& y)
{
    for(int i = 0; i < wd::maps_count; ++i)
    {
        if(pr::map_region(i) != region) continue;
        const pr::map_def& m = wd::maps[i];
        for(int k = 0; k < m.people_count; ++k)
        {
            if(m.people[k].role == role) { x = m.people[k].x; y = m.people[k].y; return i; }
        }
    }
    return -1;
}
// The nth trainer of a role in a region (with a kind, if given).
static int find_trainer(int region, pr::trainer_role role, int nth, int& x, int& y, int kind = -1)
{
    for(int i = 0; i < wd::maps_count; ++i)
    {
        if(pr::map_region(i) != region) continue;
        const pr::map_def& m = wd::maps[i];
        for(int k = 0; k < m.trainers_count; ++k)
        {
            const pr::trainer& t = m.trainers[k];
            if(t.role == role && (kind < 0 || int(t.kind) == kind) && nth-- == 0) { x = t.x; y = t.y; return i; }
        }
    }
    return -1;
}
static int find_area(const char* name)
{
    for(int i : wd::area_maps)
    {
        if(bn::string_view(wd::maps[i].name) == bn::string_view(name)) return i;
    }
    return -1;
}
}
static void test_setup()
{
    using namespace pr;
    namespace wd = pr::world_data;
    bn::assert::set_callback(crash_mark);
    int16_t scen = 0;
    bn::sram::read_offset(scen, 0x7E02);
    game_state& g = state();
    if(scen == 99)
    {
        // Timing (timing.py): make and drop every Pokémon's front, back and icon sprite; the timer's ticks
        // (64 CPU cycles each) for each kind go to SRAM 0x7E20.
        int32_t ticks[3] = {};
        for(int kind = 0; kind < 3; ++kind)
        {
            bn::timer t;
            for(int i = 1; i < pr::species_count; ++i)
            {
                const auto& sp = pr::game_data::species_list[i];
                bn::sprite_ptr spr = (kind == 0 ? sp.front : kind == 1 ? sp.back : sp.icon).create_sprite(0, 0);
            }
            ticks[kind] = t.elapsed_ticks();
        }
        bn::sram::write_offset(ticks, 0x7E20);
        int16_t zero = 0;
        bn::sram::write_offset(zero, 0x7E02);
        return;
    }
    if(save_exists() || scen <= 0)
    {
        if(save_exists()) load_game();
        return;
    }
    reset_state();
    g.name[0]='P'; g.name[1]='L'; g.name[2]='A'; g.name[3]='Y';
    g.story = story::STARTER | story::CALL1 | story::CALL2 | story::CALL3 | story::SCENE_TEMPEST_RUN |
              story::SCENE_PORTMERE | story::SCENE_HIDEOUT | story::SCENE_SHRINE | story::STORM_ENDED | story::CHAMPION;
    g.run.skip_story = true;
    g.run.adventure = true;
    constexpr species_id team[] = { species_id::MEWTWO, species_id::ARCEUS, species_id::RAYQUAZA,
                                    species_id::KORAIDON, species_id::MIRAIDON, species_id::ETERNATUS };
    int lv = scen == 4 ? 200 : 300;
    for(int i = 0; i < 6; ++i) g.party[i] = mon::make(team[i], lv);
    if(scen == 12)
    {
        // The CHAMPION's water, steel and ground team: each Pokémon leads with a move that hits it hard.
        constexpr const char* lead[] = { "PSYSTRIKE", "ENERGY BALL", "OUTRAGE", "CLOSE COMBAT", "ELECTRO DRIFT", "DYNAMAX CANNON" };
        for(int i = 0; i < 6; ++i)
            for(int k = 0; k < pr::game_data::moves_count; ++k)
                if(bn::string_view(pr::game_data::moves[k].name) == bn::string_view(lead[i])) g.party[i].moves[0] = uint16_t(k);
    }
    g.party_count = 6;
    g.money = 500000;
    g.add_item(item_id::ULTRABALL, 50);
    g.add_item(item_id::MASTERBALL, 2);
    play::beat_region(1);
    g.flags.set(flag::CALDERRA_STARTER);
    if(scen != 1)
    {
        play::beat_region(2);
        g.flags.set(flag::CALDERRA_CHAMPION);
    }
    g.region = scen <= 2 ? 1 : 2;
    int x = 0, y = 0, map = -1;
    switch(scen)
    {
    case 1:     // Port Calder's CAPTAIN before Calderra's League
    case 2:     // ... and after: sail to PORT KEEL
        map = play::find_person(2, person_role::CAPTAIN, x, y);
        break;
    case 3:     // PORT KEEL's CAPTAIN: sail home
        map = play::find_person(3, person_role::CAPTAIN, x, y);
        break;
    case 4:     // the first SUNDERED gym leader, no badges, your team at 200
        map = play::find_trainer(3, trainer_role::LEADER, 0, x, y);
        break;
    case 5:     // the last gym leader with seven badges
        for(int n = 0; n < 7; ++n)
        {
            int m = play::find_trainer(3, trainer_role::LEADER, n, x, y);
            for(int k = 0; k < wd::maps[m].trainers_count; ++k)
                if(wd::maps[m].trainers[k].role == trainer_role::LEADER) g.beaten.set(wd::maps[m].trainers[k].id);
        }
        map = play::find_trainer(3, trainer_role::LEADER, 7, x, y);
        break;
    case 6:     // TEAM QUAKE's boss on MAGMA ISLE
    case 7:     // TEAM NEPTUNE's boss in THE ABYSS
    {
        map = play::find_area(scen == 6 ? "MAGMA ISLE" : "THE ABYSS");
        const map_def& m = wd::maps[map];
        for(int k = 0; k < m.trainers_count; ++k)
        {
            const trainer& t = m.trainers[k];
            if(t.kind == (scen == 6 ? person_kind::bossQuake : person_kind::bossNeptune)) { x = t.x; y = t.y; }
            else g.beaten.set(t.id);
        }
        break;
    }
    case 15:    // GROUDON again, with just one Pokémon (one action a turn: throwing a ball)
        g.party_count = 1;
        for(int i = 1; i < 6; ++i) g.party[i] = mon();
        [[fallthrough]];
    case 8:     // GROUDON, its boss beaten
    case 9:     // KYOGRE
    {
        map = play::find_area(scen == 9 ? "THE ABYSS" : "MAGMA ISLE");
        const map_def& m = wd::maps[map];
        for(int k = 0; k < m.trainers_count; ++k) g.beaten.set(m.trainers[k].id);
        x = m.area->legend_x; y = m.area->legend_y;
        break;
    }
    case 10:    // PORT KEEL's way up MT. KEEL: eight badges, both bases beaten
    case 11:    // ... eight badges but the bases not
    {
        play::beat_region(3, true);
        if(scen == 10)
        {
            for(const char* n : { "MAGMA ISLE", "THE ABYSS" })
            {
                const map_def& m = wd::maps[play::find_area(n)];
                for(int k = 0; k < m.trainers_count; ++k) g.beaten.set(m.trainers[k].id);
            }
        }
        map = play::find_area("PORT KEEL");
        const map_def& m = wd::maps[map];
        for(int k = 0; k < m.links_count; ++k)
        {
            const pr::link& l = m.links[k];
            if(l.oy < 0)
            {
                int lo = bn::max(0, int(l.ox)), hi = bn::min(int(m.w), l.ox + wd::maps[l.target].w);
                for(int tx = lo; tx < hi; ++tx)
                    if(play::walkable(m, tx, 0) && play::walkable(m, tx, 1)) { x = tx; y = -1; break; }
            }
        }
        break;
    }
    case 12:    // the CHAMPION, the ELITE FOUR beaten
    {
        play::beat_region(3);
        map = play::find_trainer(3, trainer_role::CHAMPION, 0, x, y);
        const map_def& m = wd::maps[map];
        for(int k = 0; k < m.trainers_count; ++k)
            if(m.trainers[k].role == trainer_role::CHAMPION) g.beaten.reset(m.trainers[k].id);
        break;
    }
    case 14:    // the CHAMPION just beaten: the next battle's end brings the HALL OF FAME
    {
        play::beat_region(3);
        map = play::find_trainer(3, trainer_role::ROUTE, 0, x, y);
        const map_def& m = wd::maps[map];
        for(int k = 0; k < m.trainers_count; ++k)
            if(m.trainers[k].x == x && m.trainers[k].y == y) g.beaten.reset(m.trainers[k].id);
        break;
    }
    case 13:    // the first Sundered rival fight (MARLO)
        map = play::find_trainer(3, trainer_role::RIVAL, 0, x, y);
        break;
    }
    if(map >= 0)
    {
        if(y < 0)
        {
            g.map = int16_t(map); g.x = int16_t(x); g.y = 0; g.facing = direction::UP;
        }
        else
        {
            play::stand_by(map, x, y);
        }
    }
    g.visited.set(g.map);
    save_game();
}
