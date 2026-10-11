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
    int lv = scen >= 40 ? 400 + 100 * ((scen - 20) / 20) : scen == 4 ? 200 : scen >= 20 ? 400 : 300;
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
    if(scen >= 20)
    {
        // The SKYREACH (4.0.0): the SUNDERED ISLES beaten too.
        play::beat_region(3);
        g.flags.set(flag::SUNDERED_CHAMPION);
        g.region = scen == 20 ? 2 : 3;
        if(scen == 22) for(int i = 0; i < 6; ++i) g.party[i] = mon::make(team[i], 300);
    }
    if(scen >= 40)
    {
        // GENOVA (5.0.0) and later: every older region beaten.
        for(int r = 3; r <= 4; ++r) play::beat_region(r);
        g.flags.set(flag::SUNDERED_CHAMPION); g.flags.set(flag::SKYREACH_CHAMPION);
        g.region = 4;
        for(int r = 5; r < 5 + (scen - 40) / 20; ++r)
        {
            play::beat_region(r);
            g.flags.set(champion_flag(r));
            g.region = uint8_t(r);
        }
    }
    int x = 0, y = 0, map = -1;
    // The region these scenarios test (40-59 GENOVA, 60-79 AETERNA, ...), and its first gym leader's level.
    const int rg = scen >= 40 ? 5 + (scen - 40) / 20 : 0, sub = scen >= 40 ? (scen - 40) % 20 : -1;
    if(sub == 2) for(int i = 0; i < 6; ++i) g.party[i] = mon::make(team[i], region_cap(rg) - 100);
    if(rg == 5 && (sub == 2 || sub == 12))
    {
        // Lead with one of GENOVA's experiments (its back sprite), and have one spare to show.
        g.party[0] = mon::make(species_id::HELIXEON, sub == 2 ? 400 : 500);
    }
    if(rg == 6 && ((sub >= 4 && sub <= 7) || sub >= 13))
    {
        // AETERNA: 4 a rift, 5 the seed in the past, 6 the tree it grew into, 7 DIALGA on the TEMPORAL SPIRE,
        // 13 PALKIA in the SPATIAL TEMPLE, 14 the altar with DIALGA in the party.
        if(sub == 6) g.flags.set(flag::ANCIENT_SEED);
        if(sub == 14) g.party[0] = mon::make(species_id::DIALGA, 600);
        if(sub <= 6) map = play::find_person(rg, sub == 4 ? person_role::TWIN : sub == 5 ? person_role::SEED : person_role::TREE, x, y);
        else if(sub == 14) map = play::find_person(rg, person_role::FORM, x, y);
        else
        {
            map = play::find_area(sub == 7 ? "TEMPORAL SPIRE" : "SPATIAL TEMPLE");
            const map_def& m = wd::maps[map];
            for(int k = 0; k < m.trainers_count; ++k) g.beaten.set(m.trainers[k].id);
            x = m.area->legend_x; y = m.area->legend_y;
        }
    }
    else if(rg >= 7 && ((sub >= 4 && sub <= 7) || sub >= 13))
    {
        auto legend_at = [&](const char* area)
        {
            map = play::find_area(area);
            const map_def& m = wd::maps[map];
            for(int k = 0; k < m.trainers_count; ++k) g.beaten.set(m.trainers[k].id);
            x = m.area->legend_x; y = m.area->legend_y;
        };
        auto beat_area = [&](const char* area)
        {
            const map_def& m = wd::maps[play::find_area(area)];
            for(int k = 0; k < m.trainers_count; ++k) g.beaten.set(m.trainers[k].id);
        };
        if(rg == 7)
        {
            // THE HOLLOW LANDS: 4 a shadow to the reverse side, 5 GIRATINA, 6 the altar with GIRATINA.
            if(sub == 4) map = play::find_person(rg, person_role::TWIN, x, y);
            if(sub == 5) legend_at("DISTORTION WORLD");
            if(sub == 6) { g.party[0] = mon::make(species_id::GIRATINA, 700); map = play::find_person(rg, person_role::FORM, x, y); }
        }
        else if(rg == 8)
        {
            // TEMPESTA: 4 the SEASON KEEPER with ZAPDOS free, 5 the DELIBIRD post, 6 LUGIA before the League,
            // 7 ... and after, 13 the festival mirror with ARTICUNO, 14 a road in winter.
            if(sub == 4) { beat_area("THUNDER PEAK"); map = play::find_person(rg, person_role::SEASON, x, y); }
            if(sub == 5) map = play::find_person(rg, person_role::POST, x, y);
            if(sub == 6 || sub == 7) { if(sub == 7) g.flags.set(flag::TEMPESTA_CHAMPION); legend_at("WHIRL ISLE"); }
            if(sub == 13) { g.party[0] = mon::make(species_id::ARTICUNO, 800); map = play::find_person(rg, person_role::FORM, x, y); }
            if(sub == 14) { set_tempesta_season(3); map = play::find_trainer(rg, trainer_role::ROUTE, 0, x, y); }
        }
    }
    else switch(sub)
    {
    case 0:     // the last region's guide on: travel to this region
    case 1:     // this region's guide back
        if(sub == 0) g.region = uint8_t(rg - 1);
        map = play::find_person(sub == 0 ? rg - 1 : rg, sub == 0 ? person_role::GUIDE_ON : person_role::GUIDE_BACK, x, y);
        break;
    case 2:     // the first gym leader, no badges, your team 100 under the cap
        map = play::find_trainer(rg, trainer_role::LEADER, 0, x, y);
        break;
    case 3:     // the badge gate out of the inner part, no badges
    case 8:     // the way up to the League: eight badges, the story bosses beaten
    {
        // Where each region's badge gate is (left of it when `left`, else up from it), and the way up to its League.
        // (side: 0 left, 1 right, -1 up)
        struct gates { const char* gate; int side; const char* league; int league_side; };
        constexpr gates where[] = { {"FOUNDRY WARD", 0, "GENOVA CENTRAL", -1}, {"CLOCKTOWER", -1, "AETERNA VILLAGE", -1},
                                    {"STILLWATER", 1, "CENOTAPH", -1}, {"BLOOMTIDE", 1, "BLOOMTIDE", 1}, {"ORIGIN GATE", -1, "TEMPESTA HALL", -1} };
        const gates& gw = where[bn::min(rg - 5, 4)];
        const char* from = sub == 3 ? gw.gate : gw.league;
        int want = sub == 3 ? gw.side : gw.league_side;
        if(sub == 8)
        {
            play::beat_region(rg, true);
            for(int i : wd::area_maps)
                if(map_region(i) == rg && (wd::maps[i].area->flags & area_flag::SHRINE))
                    for(int k = 0; k < wd::maps[i].trainers_count; ++k) g.beaten.set(wd::maps[i].trainers[k].id);
        }
        map = play::find_area(from);
        const map_def& m = wd::maps[map];
        for(int k = 0; k < m.links_count; ++k)
        {
            const pr::link& l = m.links[k];
            if(want == 0 && l.ox < 0)
            {
                // A link to the left: stand at the left edge on its row, facing it.
                for(int ty = 0; ty < m.h; ++ty)
                    if(play::walkable(m, 0, ty) && play::walkable(m, 1, ty)) { g.map = int16_t(map); g.x = 1; g.y = int16_t(ty); g.facing = direction::LEFT; y = -2; break; }
            }
            else if(want == 1 && l.ox >= m.w)
            {
                // ... to the right.
                for(int ty = 0; ty < m.h; ++ty)
                    if(play::walkable(m, m.w - 1, ty) && play::walkable(m, m.w - 2, ty)) { g.map = int16_t(map); g.x = int16_t(m.w - 2); g.y = int16_t(ty); g.facing = direction::RIGHT; y = -2; break; }
            }
            else if(want < 0 && l.oy < 0)
            {
                int lo = bn::max(0, int(l.ox)), hi = bn::min(int(m.w), l.ox + wd::maps[l.target].w);
                for(int tx = lo; tx < hi; ++tx)
                    if(play::walkable(m, tx, 0) && play::walkable(m, tx, 1)) { x = tx; y = -1; break; }
            }
        }
        if(y == -2) map = -1;
        break;
    }
    case 4:     // a MEW sighting (GENOVA)
        map = play::find_person(rg, person_role::MEW_SIGHT, x, y);
        break;
    case 5:     // MEW on the LOST PIER, its admin beaten, too few sightings
    case 6:     // ... all ten sightings
    case 7:     // MEWTWO in the SEALED CAVE, the Chairman beaten
    {
        if(sub == 6) for(int k = 0; k < flag::mew_sightings; ++k) g.flags.set(flag::MEW_SIGHTING + k);
        map = play::find_area(sub == 7 ? "SEALED CAVE" : "LOST PIER");
        const map_def& m = wd::maps[map];
        for(int k = 0; k < m.trainers_count; ++k) g.beaten.set(m.trainers[k].id);
        x = m.area->legend_x; y = m.area->legend_y;
        break;
    }
    case 9:     // the CHAMPION, the ELITE FOUR beaten
    case 10:    // the CHAMPION just beaten: the next battle's end brings the HALL OF FAME
    {
        play::beat_region(rg);
        map = play::find_trainer(rg, sub == 9 ? trainer_role::CHAMPION : trainer_role::ROUTE, 0, x, y);
        const map_def& m = wd::maps[map];
        for(int k = 0; k < m.trainers_count; ++k)
            if(m.trainers[k].x == x && m.trainers[k].y == y) g.beaten.reset(m.trainers[k].id);
        break;
    }
    case 11:    // the region's rival (the first RIVAL outside a story boss's area)
        for(int n = 0; n < 16; ++n)
        {
            map = play::find_trainer(rg, trainer_role::RIVAL, n, x, y);
            if(map < 0 || ! (wd::maps[map].area->flags & area_flag::BOSS)) break;
        }
        break;
    case 12:    // a route trainer, to see the region's AI and its wild-born POKéMON
        map = play::find_trainer(rg, trainer_role::ROUTE, 0, x, y);
        break;
    }
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
    case 20:    // PORT KEEL's PILOT: fly to WINDWARD
        map = play::find_person(3, person_role::PILOT, x, y);
        break;
    case 21:    // WINDWARD's PILOT: fly back
        map = play::find_person(4, person_role::PILOT, x, y);
        break;
    case 22:    // the first SKYREACH gym leader, no badges, your team at 300
        map = play::find_trainer(4, trainer_role::LEADER, 0, x, y);
        break;
    case 23:    // OCHRE GULCH's updraft up to the mesas, no badges
    case 28:    // WINDWARD's way into PILLAR HEART: eight badges and both legends
    {
        if(scen == 28)
        {
            play::beat_region(4, true);
            g.flags.set(flag::RAYQUAZA); g.flags.set(flag::DEOXYS);
            for(const char* n : { "METEOR CRATER", "SKY PILLAR" })
            {
                const map_def& m = wd::maps[play::find_area(n)];
                for(int k = 0; k < m.trainers_count; ++k) g.beaten.set(m.trainers[k].id);
            }
        }
        map = play::find_area(scen == 23 ? "OCHRE GULCH" : "WINDWARD");
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
    case 24:    // a wind current on CLOUD ROUTE 7: stand left of the first one, then step right onto it
    {
        map = play::find_area("CLOUD ROUTE 7");
        const map_def& m = wd::maps[map];
        for(int i = 0; i < m.w * m.h && y >= 0; ++i)
        {
            behaviour b = behaviour(m.behaviours[i]);
            if(b >= behaviour::WIND_UP && b <= behaviour::WIND_RIGHT && play::walkable(m, i % m.w - 1, i / m.w))
            {
                g.map = int16_t(map); g.x = int16_t(i % m.w - 1); g.y = int16_t(i / m.w); g.facing = direction::RIGHT;
                y = -2;
            }
        }
        if(y == -2) map = -1;
        break;
    }
    case 25:    // RAYQUAZA on the SKY PILLAR, its boss beaten
    case 26:    // DEOXYS in the METEOR CRATER
    {
        map = play::find_area(scen == 25 ? "SKY PILLAR" : "METEOR CRATER");
        const map_def& m = wd::maps[map];
        for(int k = 0; k < m.trainers_count; ++k) g.beaten.set(m.trainers[k].id);
        x = m.area->legend_x; y = m.area->legend_y;
        break;
    }
    case 27:    // OCHRE GULCH's meteorite shard with DEOXYS in the party: it becomes DEOXYS-ATTACK
        g.party[5] = mon::make(species_id::DEOXYS, 350);
        map = play::find_person(4, person_role::SHARD_ATTACK, x, y);
        break;
    case 29:    // the SKYREACH CHAMPION, the ELITE FOUR beaten
    case 30:    // the CHAMPION just beaten: the next battle's end brings the HALL OF FAME
    {
        play::beat_region(4);
        map = play::find_trainer(4, scen == 29 ? trainer_role::CHAMPION : trainer_role::ROUTE, 0, x, y);
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
