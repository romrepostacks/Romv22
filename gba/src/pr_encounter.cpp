// Who you face (the web game's startWildBattle / startTrainerBattle), and what happens if you white out
// (sendToCenter, leagueReset).
#include "bn_unique_ptr.h"

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
}

battle_report battle_scene(const encounter& e)
{
    game_state& g = state();
    bn::unique_ptr<battle_setup> s(new battle_setup());
    s->own = g.party.data();
    s->own_count = g.party_count;
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
    if(report.outcome == battle_outcome::WHITED_OUT)
    {
        clear_pending_moves();
        send_to_center();
    }
    save_game();
    return report;
}

}
