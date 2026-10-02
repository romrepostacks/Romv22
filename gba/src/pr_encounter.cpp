// Who you face (the web game's startWildBattle / startTrainerBattle), and what happens if you white out
// (sendToCenter, leagueReset).
#include <initializer_list>

#include "bn_bg_palettes.h"
#include "bn_unique_ptr.h"

#include "pr_audio.h"
#include "pr_ui.h"

#include "pr_battle.h"
#include "pr_game_data.h"
#include "pr_overworld_impl.h"
#include "pr_state.h"
#include "pr_world_data.h"

namespace pr
{

namespace
{
    // The level cap where you are (advLevel): rooms share their area's.
    int level_cap()
    {
        const map_def& m = world_data::maps[state().map];
        return m.level_cap;
    }

    void trainer_team(battle_setup& s, const trainer& t)
    {
        game_state& g = state();
        int avg = g.average_level();
        int badges = g.badges();
        int cap_level = level_cap();
        bool route = t.role == trainer_role::ROUTE || t.role == trainer_role::JUNIOR || t.role == trainer_role::ELITE;
        bool junior = t.role == trainer_role::JUNIOR;
        bool elite = t.role == trainer_role::ELITE;
        int lv;
        if(junior)
        {
            lv = bn::max(3, bn::min(cap_level, avg - 2));
        }
        else if(elite)
        {
            lv = bn::min(70, avg + t.elite);
        }
        else if(t.role == trainer_role::CHAMPION)
        {
            lv = bn::min(70, avg + 3);
        }
        else
        {
            lv = bn::max(3, bn::min(cap_level, avg - 1));
        }
        species_id names[6];
        int n = 0;
        if(route)
        {
            // Route trainers bring the first `party size` of their team (gym juniors at most 1 + badges);
            // the Elite Four, all six.
            int size = elite ? t.team_count : bn::max(1, junior ? bn::min(int(g.party_count), 1 + badges) : int(g.party_count));
            n = bn::min(bn::min(size, int(t.team_count)), 6);
            for(int i = 0; i < n; ++i)
            {
                names[i] = t.team[i];
            }
        }
        else
        {
            // Leaders, rivals and the Champion: their signature team, at most 2 + badges Pokémon (trimmed keeping
            // the ace, the last one), filled out to your party's size from the gym's juniors or the area.
            int most = bn::min(6, 2 + badges);
            if(t.team_count > most)
            {
                for(int i = 0; i < most - 1; ++i)
                {
                    names[n++] = t.team[i];
                }
                names[n++] = t.team[t.team_count - 1];
            }
            else
            {
                for(int i = 0; i < t.team_count; ++i)
                {
                    names[n++] = t.team[i];
                }
            }
            int size = bn::min(most, bn::max(n, int(g.party_count)));
            for(int i = 0; i < t.fill_count && n < size; ++i)
            {
                bool dup = false;
                for(int k = 0; k < n; ++k)
                {
                    dup |= names[k] == t.fill[i];
                }
                if(! dup)
                {
                    names[n++] = t.fill[i];
                }
            }
            for(int i = 0; n < size && t.fill_count; ++i)
            {
                names[n++] = t.fill[i % t.fill_count];
            }
            // Outnumbered (a small or battered party): 2 levels lower per extra Pokémon.
            int extra = bn::max(0, n - g.able_count());
            lv = bn::max(2, lv - 2 * extra);
        }
        // Leftovers for the leaders, rivals, the Champion and the Elite Four once you have two badges.
        held_item item = (route && ! elite) || badges < 2 ? held_item::NONE : held_item::LEFTOVERS;
        s.foe_count = n;
        for(int i = 0; i < n; ++i)
        {
            s.foes[i] = mon::make(names[i], lv, item);
        }
        s.opponent = &t;
    }

    // sendToCenter(): outside the door of the POKéMON CENTER you last healed at (home by default), healed.
    void send_to_center()
    {
        game_state& g = state();
        g.heal_party();
        g.surfing = false;
        pending_walk_back().map = -1;
        // leagueReset(): the Elite Four's doors shut again until the Champion is beaten.
        for(int i = 0; i < world_data::maps_count; ++i)
        {
            const map_def& m = world_data::maps[i];
            if(m.room && m.room->kind == room_kind::LEAGUE)
            {
                const map_def& league = world_data::maps[m.exit_map];
                if(! (league.leader_id >= 0 && g.beaten.test(league.leader_id)))
                {
                    for(int k = 0; k < m.trainers_count; ++k)
                    {
                        if(m.trainers[k].elite >= 0)
                        {
                            g.beaten.reset(m.trainers[k].id);
                        }
                    }
                }
            }
        }
        int area = g.last_heal;
        if(area < 0 || ! world_data::maps[area].area || ! (world_data::maps[area].area->flags & area_flag::CENTER))
        {
            area = 0;
        }
        const map_def& town = world_data::maps[area];
        g.map = int16_t(area);
        g.x = town.spawn_x;
        g.y = town.spawn_y;
        for(int i = 0; i < town.doors_count; ++i)
        {
            if(town.doors[i].kind == door_kind::CENTER)
            {
                g.x = town.doors[i].x;
                g.y = int16_t(town.doors[i].y + 1);
            }
        }
        g.facing = direction::DOWN;
    }

    // The area a map belongs to (a room's is the one outside).
    int area_of(int map)
    {
        const map_def& m = world_data::maps[map];
        return m.is_room() ? m.exit_map : map;
    }

    // NUZLOCKE: the fallen leave the party for the graveyard (their last 30 kept for the TRAINER CARD).
    void bury_fallen()
    {
        game_state& g = state();
        int kept = 0;
        for(int i = 0; i < g.party_count; ++i)
        {
            mon& m = g.party[i];
            if(m.fainted())
            {
                g.run.graveyard[g.run.grave_next] = m;
                g.run.grave_next = uint8_t((g.run.grave_next + 1) % graveyard_size);
                ++g.run.deaths;
            }
            else
            {
                if(kept != i)
                {
                    g.party[kept] = m;
                }
                ++kept;
            }
        }
        for(int i = kept; i < g.party_count; ++i)
        {
            g.party[i] = mon();
        }
        g.party_count = uint8_t(kept);
    }

    // A few lines on a plain screen, between battles (the CHALLENGE TOWER) or at a run's end.
    void plain_say(std::initializer_list<const char*> lines)
    {
        ui& u = gui();
        bn::bg_palettes::set_transparent_color(bn::color(3, 4, 8));
        ui::fade_in(8);
        for(const char* l : lines)
        {
            u.say(l);
        }
        ui::fade_out(8);
    }

    // ----- The CHALLENGE TOWER (Phase 7) -----
    constexpr const char* tower_names[] = { "ACE TRAINER KAI", "ACE TRAINER MIRA", "VETERAN OSRIC", "VETERAN DELLA",
                                            "COOLTRAINER RHYS", "COOLTRAINER ISLA", "EXPERT BRAM", "EXPERT NOVA",
                                            "DRAGON TAMER VEX", "PSYCHIC LUNE", "BLACK BELT TAO", "SKY TRAINER ARIA" };
    constexpr int tower_names_count = int(sizeof(tower_names) / sizeof(tower_names[0]));
    constexpr uint8_t tower_trainer_id = 255;   // a beaten bit nobody else uses (cleared after each run)

    BN_DATA_EWRAM_BSS trainer tower_trainers[5];

    bool is_legendary(int s)
    {
        for(int i = 0; i < game_data::legendaries_count; ++i)
        {
            if(game_data::legendaries[i] == s)
            {
                return true;
            }
        }
        return false;
    }

    // A themed team: fully evolved, non-legendary Pokémon of one type (any, if the type has too few).
    void tower_team(battle_setup& s, int size, int level, int clears)
    {
        int type = rng().get_int(types_count);
        uint16_t pool[species_count];
        int n = 0;
        for(int pass = 0; pass < 2 && n < size; ++pass)
        {
            n = 0;
            for(int i = 0; i < species_count; ++i)
            {
                const species& sp = game_data::species_list[i];
                if(sp.evolves_to >= 0 || is_legendary(i))
                {
                    continue;
                }
                if(pass == 0 && sp.type1 != type && sp.type2 != type)
                {
                    continue;
                }
                pool[n++] = uint16_t(i);
            }
        }
        s.foe_count = 0;
        while(s.foe_count < size && n)
        {
            int k = rng().get_int(n);
            int sp = pool[k];
            pool[k] = pool[--n];
            // Held items from rank 2 (half of them), all of them from rank 4.
            held_item item = held_item::NONE;
            if(clears >= 3 || (clears >= 1 && rng().get_int(2)))
            {
                item = held_item(1 + rng().get_int(5));
            }
            s.foes[s.foe_count++] = mon::make(species_id(sp), level, item);
        }
    }

    battle_report tower_run()
    {
        game_state& g = state();
        int clears = g.run.tower_clears;
        int base = bn::min(100, bn::max(50 + 3 * clears, g.average_level() + clears));
        battle_report last;
        for(int floor = 0; floor < 5; ++floor)
        {
            bool master = floor == 4;
            trainer& t = tower_trainers[floor];
            t = trainer{ 0, 0, person_kind::gentleman, direction::DOWN, trainer_role::ELITE,
                         master ? "TOWER MASTER" : tower_names[rng().get_int(tower_names_count)],
                         nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, false, tower_trainer_id, -1, 0, false };
            bn::unique_ptr<battle_setup> s(new battle_setup());
            s->own = g.party.data();
            s->own_count = g.party_count;
            s->opponent = &t;
            s->smart = master || clears >= 2;
            int size = master ? 6 : bn::min(6, 4 + clears / 2);
            tower_team(*s, size, bn::min(100, base + floor / 2 + (master ? 2 : 0)), clears);
            bn::string<48> head("CHALLENGE TOWER - FLOOR ");
            head.append(bn::to_string<4>(floor + 1));
            plain_say({ head.c_str() });
            g.beaten.reset(tower_trainer_id);
            last = run_battle(*s);
            g.beaten.reset(tower_trainer_id);
            if(last.outcome == battle_outcome::WHITED_OUT || last.outcome == battle_outcome::RAN)
            {
                g.run.tower_streak = 0;
                plain_say({ "The challenge is over. Your streak was reset." });
                return last;
            }
        }
        g.run.tower_clears = uint8_t(bn::min(250, clears + 1));
        g.run.tower_streak = uint8_t(bn::min(250, g.run.tower_streak + 1));
        g.run.tower_best = bn::max(g.run.tower_best, g.run.tower_streak);
        bn::string<64> rank("CHALLENGE TOWER cleared! RANK ");
        rank.append(bn::to_string<4>(g.run.tower_clears + 1));
        rank.append(" unlocked.");
        plain_say({ rank.c_str() });
        // The prize: a battle with a legendary still in the pool (caught ones leave it for good).
        int left[game_data::legendaries_count];
        int n = 0;
        for(int i = 0; i < game_data::legendaries_count; ++i)
        {
            if(! g.owned.test(game_data::legendaries[i]))
            {
                left[n++] = game_data::legendaries[i];
            }
        }
        if(! n)
        {
            plain_say({ "Every legendary POKéMON has been caught!" });
            last.outcome = battle_outcome::WON;
            return last;
        }
        plain_say({ "A legendary POKéMON is drawn to your strength..." });
        bn::unique_ptr<battle_setup> s(new battle_setup());
        s->own = g.party.data();
        s->own_count = g.party_count;
        s->legendary = true;
        s->foe_count = 1;
        s->foes[0] = mon::make(species_id(left[rng().get_int(n)]), bn::min(100, base + 5));
        mon& m = s->foes[0];
        int able = g.able_count();
        if(able > 2)
        {
            m.max_hp = uint16_t((m.max_hp * able + 1) / 2);
            m.hp = m.max_hp;
        }
        battle_report legend = run_battle(*s);
        if(legend.outcome == battle_outcome::RAN || legend.outcome == battle_outcome::WON)
        {
            plain_say({ "The legendary POKéMON returned to the tower's pool." });
        }
        return legend;
    }
}

battle_report battle_scene(const encounter& e)
{
    game_state& g = state();
    if(e.kind == encounter_kind::TOWER)
    {
        clear_pending_moves();
        battle_report report = tower_run();
        if(report.outcome == battle_outcome::WHITED_OUT)
        {
            clear_pending_moves();
            send_to_center();
        }
        save_game();
        return report;
    }
    bn::unique_ptr<battle_setup> s(new battle_setup());
    s->own = g.party.data();
    s->own_count = g.party_count;
    // NUZLOCKE: the area's first wild encounter is its only chance to catch (the guardian and the
    // professor's ZIGZAGOON aside); one that's all dupes doesn't count (dupes clause).
    int area = area_of(g.map);
    bool counts = false;
    if(g.run.nuzlocke() && e.kind != encounter_kind::TRAINER && ! e.legendary && ! e.scripted &&
       ! g.run.encounter_used.test(area))
    {
        for(int i = 0; i < e.count; ++i)
        {
            counts |= ! g.owned.test(int(e.species[i]));
        }
    }
    s->nuzlocke_catch = ! g.run.nuzlocke() || e.legendary || counts;
    if(e.scripted && g.run.nuzlocke())
    {
        s->nuzlocke_catch = false;
    }
    if(e.kind == encounter_kind::TRAINER)
    {
        const map_def& m = world_data::maps[e.map];
        trainer_team(*s, m.trainers[e.trainer_index]);
    }
    else
    {
        s->foe_count = e.count;
        for(int i = 0; i < e.count; ++i)
        {
            s->foes[i] = mon::make(e.species[i], e.level);
        }
        s->legendary = e.legendary;
        if(e.legendary)
        {
            // The guardian is tougher the bigger your party: HP x max(1, able / 2).
            for(int i = 0; i < s->foe_count; ++i)
            {
                mon& m = s->foes[i];
                int able = g.able_count();
                int scaled = able > 2 ? (m.max_hp * able + 1) / 2 : m.max_hp;
                m.max_hp = uint16_t(scaled);
                m.hp = m.max_hp;
            }
        }
    }
    clear_pending_moves();
    battle_report report = run_battle(*s);
    if(counts)
    {
        g.run.encounter_used.set(area);
    }
    if(g.run.nuzlocke())
    {
        bury_fallen();
        if(! g.party_count)
        {
            // Whiting out ends a NUZLOCKE run.
            clear_pending_moves();
            g.run.over = true;
            report.run_over = true;
            save_game();
            bn::string<64> deaths("POKéMON lost: ");
            deaths.append(bn::to_string<6>(g.run.deaths));
            bn::string<64> badges("Badges: ");
            badges.append(bn::to_string<4>(g.badges()));
            audio::play_music("credits");
            plain_say({ "Your whole party has fallen.", "The NUZLOCKE run is over.", badges.c_str(), deaths.c_str() });
            return report;
        }
    }
    if(report.outcome == battle_outcome::WHITED_OUT)
    {
        clear_pending_moves();
        send_to_center();
    }
    save_game();
    return report;
}

}
