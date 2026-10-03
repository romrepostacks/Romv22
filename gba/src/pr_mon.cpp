#include "pr_mon.h"
#include "pr_state.h"

#include "bn_vector.h"

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

    bn::vector<pending_move, 16> pending;
}

void add_pending_move(mon* m, int move)
{
    if(! pending.full())
    {
        pending.push_back({ m, uint16_t(move) });
    }
}

bool take_pending_move(pending_move& out)
{
    if(pending.empty())
    {
        return false;
    }
    out = pending.front();
    pending.erase(pending.begin());
    return true;
}

void clear_pending_moves()
{
    pending.clear();
}

int moves_at(const species& s, int level, uint16_t* out)
{
    // Its last four level-up moves at this level (movesAt); nothing usable yet: its first move, else Struggle.
    if(! s.learnset_count)
    {
        int n = 0;
        for(int i = 0; i < s.fixed_count && n < 4; ++i)
        {
            out[n++] = s.fixed_moves[i];
        }
        if(! n)
        {
            out[n++] = 0;       // STRUGGLE
        }
        return n;
    }
    uint16_t known[64];
    int count = 0;
    for(int i = 0; i < s.learnset_count; ++i)
    {
        const learn_entry& e = s.learnset[i];
        if(e.level > level)
        {
            break;
        }
        for(int k = 0; k < count; ++k)
        {
            if(known[k] == e.move)
            {
                for(int j = k; j < count - 1; ++j)
                {
                    known[j] = known[j + 1];
                }
                --count;
                break;
            }
        }
        if(count == 64)
        {
            for(int j = 0; j < count - 1; ++j)
            {
                known[j] = known[j + 1];
            }
            --count;
        }
        known[count++] = e.move;
    }
    if(! count)
    {
        known[count++] = s.learnset[0].move;
    }
    int first = bn::max(0, count - 4);
    for(int i = first; i < count; ++i)
    {
        out[i - first] = known[i];
    }
    return count - first;
}

mon mon::make(species_id id, int level, held_item item)
{
    mon m;
    m.species_index = uint16_t(id);
    m.level = uint8_t(level);
    m.item = item;
    m.move_count = uint8_t(moves_at(m.data(), level, m.slots));
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
    return nick[0] ? nick : data().name;
}

const char* mon::species_name() const
{
    return data().name;
}

const ability& mon::abil() const
{
    return data().abil;
}

bool mon::has_type(int type) const
{
    return data().type1 == type || data().type2 == type;
}

int stat_min(int base, bool is_hp, int level)
{
    if(is_hp)
    {
        return base == 1 ? 1 : (2 * base * level) / 100 + level + 10;
    }
    return ((2 * base * level) / 100 + 5) * 9 / 10;
}

int stat_max(int base, bool is_hp, int level)
{
    // IV 31 and 252 EVs add 31 + 63 to twice the base.
    if(is_hp)
    {
        return base == 1 ? 1 : ((2 * base + 94) * level) / 100 + level + 10;
    }
    return (((2 * base + 94) * level) / 100 + 5) * 11 / 10;
}

void mon::recalc_stats(int old_level, const base_stats* old_base)
{
    const base_stats& b = data().base;
    int old_max = max_hp;
    if(traits & mon_trait::EDITED && old_max)
    {
        // Hand-set stats: the same place between this level's min and max as they had between the old ones.
        int from = old_level ? old_level : level;
        const base_stats& ob = old_base ? *old_base : b;
        auto keep = [&](uint16_t& v, int old_b, int new_b, bool hp)
        {
            int lo = stat_min(old_b, hp, from), hi = stat_max(old_b, hp, from);
            int pos = hi > lo ? bn::clamp((int(v) - lo) * 1024 / (hi - lo), 0, 1024) : 512;
            int nlo = stat_min(new_b, hp, level), nhi = stat_max(new_b, hp, level);
            v = uint16_t(nlo + (nhi - nlo) * pos / 1024);
        };
        keep(max_hp, ob.hp, b.hp, true);
        keep(atk, ob.atk, b.atk, false);
        keep(def, ob.def, b.def, false);
        keep(spa, ob.spa, b.spa, false);
        keep(spd, ob.spd, b.spd, false);
        keep(spe, ob.spe, b.spe, false);
    }
    else
    {
        max_hp = uint16_t(stat_calc(b.hp, true, level));
        atk = uint16_t(stat_calc(b.atk, false, level));
        def = uint16_t(stat_calc(b.def, false, level));
        spa = uint16_t(stat_calc(b.spa, false, level));
        spd = uint16_t(stat_calc(b.spd, false, level));
        spe = uint16_t(stat_calc(b.spe, false, level));
    }
    // Keep the same fraction of HP (recalcStats: Math.round, at least 1 unless fainted).
    if(old_max)
    {
        if(hp)
        {
            int scaled = (2 * hp * max_hp + old_max) / (2 * old_max);
            hp = uint16_t(bn::max(1, scaled));
        }
    }
}

int mon::max_pp(int slot) const
{
    return move_data(move(slot)).pp;
}

void mon::heal()
{
    restore_pp();
    hp = max_hp;
    st = status::NONE;
    sleep_turns = 0;
    flags = 0;
}

void mon::set_species(int index)
{
    base_stats old = data().base;
    species_index = uint16_t(index);
    recalc_stats(level, &old);
}

void mon::grant_xp(int amount, ui& ui)
{
    if(fainted())
    {
        return;
    }
    // NUZLOCKE's level cap: no EXP past the next gym leader's level.
    int cap = level_cap_now();
    if(level >= cap)
    {
        xp = 0;
        return;
    }
    xp = uint16_t(xp + amount);
    while(level < cap && xp >= xp_next())
    {
        xp = uint16_t(xp - xp_next());
        ++level;
        recalc_stats(level - 1);
        bn::string<64> text(name());
        text.append(" grew to level ");
        text.append(bn::to_string<4>(level));
        text.append("!");
        ui.say(text);
        _learn_moves_at(level, ui);
        _try_evolve(ui);
    }
    if(level >= cap)
    {
        xp = 0;
    }
}

// learnMovesAt(): an empty slot takes a new move straight away; with four, it waits for after the battle.
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
        // Struggle goes once it knows something real.
        for(int k = 0; k < move_count; ++k)
        {
            if(move(k) == 0)
            {
                for(int j = k; j < move_count - 1; ++j)
                {
                    slots[j] = slots[j + 1];
                }
                --move_count;
                --k;
            }
        }
        bool known = false;
        for(int k = 0; k < move_count; ++k)
        {
            known |= move(k) == e.move;
        }
        if(known)
        {
            continue;
        }
        bn::string<96> text(name());
        if(move_count >= 4)
        {
            add_pending_move(this, e.move);
            text.append(" wants to learn ");
            text.append(move_data(e.move).name);
            text.append("!");
        }
        else
        {
            set_move(move_count++, e.move);
            text.append(" learned ");
            text.append(move_data(e.move).name);
            text.append("!");
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
    bn::string<96> text(name());
    base_stats old = s.base;
    species_index = uint16_t(s.evolves_to);
    recalc_stats(level, &old);
    text.append(" evolved into ");
    text.append(data().name);
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

int effectiveness_x4(int move_type, const mon& target)
{
    const pr::species& ts = target.data();
    int eff = 4 * game_data::type_chart[move_type][ts.type1] / 2;
    if(ts.type2 >= 0)
    {
        eff = eff * game_data::type_chart[move_type][ts.type2] / 2;
    }
    return eff;
}

int stage_x100(int stage)
{
    stage = bn::clamp(stage, -6, 6);
    return stage >= 0 ? (2 + stage) * 100 / 2 : 200 / (2 - stage);
}

int accuracy_stage_x100(int stage)
{
    stage = bn::clamp(stage, -6, 6);
    return stage >= 0 ? (3 + stage) * 100 / 3 : 300 / (3 - stage);
}

int type_index(const char* name)
{
    for(int i = 0; i < int(sizeof(game_data::type_names) / sizeof(game_data::type_names[0])); ++i)
    {
        if(bn::string_view(game_data::type_names[i]) == name)
        {
            return i;
        }
    }
    return -1;
}

int move_effectiveness_x4(const mon& user, const move& mv, const mon& target)
{
    int eff_x4 = effectiveness_x4(mv.type, target);
    const ability& ta = target.abil();
    const ability& ua = user.abil();
    if(ta.kind == ability_kind::IMMUNE && ta.type == mv.type)
    {
        eff_x4 = 0;
    }
    if(ua.kind == ability_kind::CORROSION && mv.type == 7 && eff_x4 == 0)
    {
        // Poison moves (type 7) ignore Steel and Poison: the other type's multiplier, else neutral.
        const pr::species& ts = target.data();
        int e = 4;
        for(int t : { int(ts.type1), int(ts.type2) })
        {
            if(t >= 0 && t != 16 && t != 7)
            {
                e = e * game_data::type_chart[mv.type][t] / 2;
            }
        }
        eff_x4 = e ? e : 4;
    }
    return eff_x4;
}

// damage(): ((2L/5+2) * P * A/D / 50 + 2) * STAB * type * random(0.85-1), with the abilities and held
// items that change it, and the battle's stat stages, weather and spread. Computed in floating point, as the
// browser does.
damage_result calc_damage(const mon& user, const move& mv, const mon& target, bn::random& random, const damage_mods& mods)
{
    bool physical = mv.category == move_category::PHYSICAL;
    double atk_stat = (physical ? user.atk : user.spa) * stage_x100(mods.atk_stage) / 100.0;
    double def_stat = bn::max(1, int(physical ? target.def : target.spd)) * stage_x100(mods.def_stage) / 100.0;
    double stab = user.has_type(mv.type) ? 1.5 : 1;
    int eff_x4 = move_effectiveness_x4(user, mv, target);
    const ability& ua = user.abil();
    static const int water = type_index("WATER"), fire = type_index("FIRE"), rock = type_index("ROCK");
    double weather = 1;
    if(mods.weather == battle_weather::RAIN)
    {
        weather = mv.type == water ? 1.5 : mv.type == fire ? 0.5 : 1;
    }
    else if(mods.weather == battle_weather::SUN)
    {
        weather = mv.type == fire ? 1.5 : mv.type == water ? 0.5 : 1;
    }
    else if(mods.weather == battle_weather::SAND && ! physical && target.has_type(rock))
    {
        def_stat *= 1.5;    // a sandstorm raises Rock types' SP. DEF
    }
    double rand = 0.85 + (random.get_int(65536) / 65536.0) * 0.15;
    double power = mv.power;
    if(ua.kind == ability_kind::BOOST && ua.type == mv.type && user.hp * 3 <= user.max_hp)
    {
        power *= 1.5;
    }
    if(ua.kind == ability_kind::PUNCH && mv.punch)
    {
        power *= 1.2;
    }
    double level = user.level;
    double base = (((2 * level / 5 + 2) * power * (atk_stat / def_stat)) / 50 + 2) * stab * (eff_x4 / 4.0) * rand * weather;
    if(mods.spread)
    {
        base *= 0.75;   // a move that hits more than one
    }
    int dmg = int(base);
    if(user.st == status::BURN && physical)
    {
        dmg /= 2;
    }
    if(user.item == held_item::LIFE_ORB)
    {
        dmg = int(dmg * 1.3);
    }
    if(ua.kind == ability_kind::MERCILESS && target.st == status::POISON)
    {
        dmg = int(dmg * 1.5);
    }
    if(eff_x4 == 0)
    {
        dmg = 0;
    }
    return { dmg, eff_x4 };
}

}
