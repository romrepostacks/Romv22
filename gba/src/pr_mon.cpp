#include "pr_mon.h"

#include "pr_game_data.h"
#include "pr_ui.h"

namespace pr
{

namespace
{
    int stat_calc(int base, bool is_hp, int level)
    {
        int v = ((2 * base + 31) * level) / 100;
        return is_hp ? v + level + 10 : v + 5;
    }
}

mon mon::make(species_id id, int level)
{
    mon m;
    m.species_index = uint8_t(id);
    m.level = uint8_t(level);

    // Its last four level-up moves at this level (movesAt); nothing usable yet: its first move, else Struggle.
    const pr::species& s = m.data();
    for(int i = 0; i < s.learnset_count; ++i)
    {
        const learn_entry& e = s.learnset[i];
        if(e.level > level)
        {
            break;
        }
        int known = -1;
        for(int k = 0; k < m.move_count; ++k)
        {
            if(m.moves[k] == e.move)
            {
                known = k;
            }
        }
        if(known >= 0)
        {
            for(int k = known; k < m.move_count - 1; ++k)
            {
                m.moves[k] = m.moves[k + 1];
            }
            --m.move_count;
        }
        if(m.move_count == 4)
        {
            for(int k = 0; k < 3; ++k)
            {
                m.moves[k] = m.moves[k + 1];
            }
            --m.move_count;
        }
        m.moves[m.move_count++] = e.move;
    }
    if(! m.move_count)
    {
        m.moves[0] = s.learnset_count ? s.learnset[0].move : 0;
        m.move_count = 1;
    }
    m.recalc_stats();
    m.hp = m.max_hp;
    return m;
}

const pr::species& mon::data() const
{
    return game_data::species_list[species_index];
}

const char* mon::name() const
{
    return data().name;
}

bool mon::has_type(int type) const
{
    return data().type1 == type || data().type2 == type;
}

void mon::recalc_stats()
{
    const base_stats& b = data().base;
    int old_max = max_hp;
    max_hp = uint16_t(stat_calc(b.hp, true, level));
    atk = uint16_t(stat_calc(b.atk, false, level));
    def = uint16_t(stat_calc(b.def, false, level));
    spa = uint16_t(stat_calc(b.spa, false, level));
    spd = uint16_t(stat_calc(b.spd, false, level));
    spe = uint16_t(stat_calc(b.spe, false, level));
    // Keep the same fraction of HP (recalcStats).
    if(old_max && hp)
    {
        int scaled = (hp * max_hp + old_max / 2) / old_max;
        hp = uint16_t(bn::max(1, scaled));
    }
}

void mon::heal()
{
    hp = max_hp;
    st = status::NONE;
    sleep_turns = 0;
}

void mon::grant_xp(int amount, ui& ui)
{
    if(fainted())
    {
        return;
    }
    xp = uint16_t(xp + amount);
    while(level < 100 && xp >= xp_next())
    {
        xp = uint16_t(xp - xp_next());
        ++level;
        recalc_stats();
        bn::string<64> text(name());
        text.append(" grew to level ");
        text.append(bn::to_string<4>(level));
        text.append("!");
        ui.say(text);
        _learn_moves_at(level, ui);
        _try_evolve(ui);
    }
}

void mon::_learn_moves_at(int at_level, ui& ui)
{
    const pr::species& s = data();
    for(int i = 0; i < s.learnset_count; ++i)
    {
        const learn_entry& e = s.learnset[i];
        if(e.level != at_level)
        {
            continue;
        }
        bool known = false;
        for(int k = 0; k < move_count; ++k)
        {
            known |= moves[k] == e.move;
        }
        if(known)
        {
            continue;
        }
        bn::string<96> text(name());
        if(move_count < 4)
        {
            moves[move_count++] = e.move;
            text.append(" learned ");
            text.append(move_data(e.move).name);
            text.append("!");
        }
        else
        {
            // The web game asks which move to forget; that screen comes with the full menus.
            text.append(" wants to learn ");
            text.append(move_data(e.move).name);
            text.append(", but already knows four moves.");
        }
        ui.say(text);
    }
}

void mon::_try_evolve(ui& ui)
{
    const pr::species& s = data();
    if(s.evolves_to < 0 || level < s.evolve_level)
    {
        return;
    }
    bn::string<96> text("What? ");
    text.append(name());
    text.append(" is evolving!");
    ui.say(text);
    species_index = uint8_t(s.evolves_to);
    recalc_stats();
    text = "It evolved into ";
    text.append(name());
    text.append("!");
    ui.say(text);
    _learn_moves_at(level, ui);
}

const move& move_data(int index)
{
    return game_data::moves[index];
}

const char* type_name(int type)
{
    return game_data::type_names[type];
}

damage_result calc_damage(const mon& user, const move& mv, const mon& target, bn::random& random)
{
    bool physical = mv.category == move_category::PHYSICAL;
    int atk_stat = physical ? user.atk : user.spa;
    int def_stat = bn::max(1, int(physical ? target.def : target.spd));
    const pr::species& ts = target.data();
    int eff_x4 = 4 * game_data::type_chart[mv.type][ts.type1] / 2;
    if(ts.type2 >= 0)
    {
        eff_x4 = eff_x4 * game_data::type_chart[mv.type][ts.type2] / 2;
    }
    // Same as js damage(): ((2L/5+2) * P * A/D / 50 + 2) * STAB * type * random(0.85-1), in integers.
    int base_x10 = ((4 * user.level + 20) * mv.power * atk_stat / def_stat) / 50 + 20;
    int stab = user.has_type(mv.type) ? 3 : 2;            // /2
    int rand = 85 + random.get_int(16);                   // /100
    int dmg = (base_x10 * stab * eff_x4 * rand) / (10 * 2 * 4 * 100);
    if(user.st == status::BURN && physical)
    {
        dmg /= 2;
    }
    if(eff_x4 == 0)
    {
        dmg = 0;
    }
    return { dmg, eff_x4 };
}

}
