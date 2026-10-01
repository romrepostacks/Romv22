// A Pokémon in your party or in a battle, with the web game's stat, move and level rules (js/app.js:
// statCalc, movesAt, grantXp, tryEvolve, damage).
#ifndef PR_MON_H
#define PR_MON_H

#include "bn_string.h"
#include "bn_random.h"
#include "pr_game_types.h"

namespace pr
{

class ui;

struct mon
{
    uint8_t species_index = 0;
    uint8_t level = 1;
    uint8_t move_count = 0;
    status st = status::NONE;
    uint8_t moves[4] = {};
    uint8_t sleep_turns = 0;
    uint8_t padding = 0;
    uint16_t hp = 0;
    uint16_t max_hp = 0;
    uint16_t atk = 0;
    uint16_t def = 0;
    uint16_t spa = 0;
    uint16_t spd = 0;
    uint16_t spe = 0;
    uint16_t xp = 0;

    [[nodiscard]] static mon make(species_id id, int level);

    [[nodiscard]] const pr::species& data() const;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] bool fainted() const
    {
        return hp == 0;
    }
    [[nodiscard]] int xp_next() const
    {
        return level * 8;
    }
    [[nodiscard]] bool has_type(int type) const;

    void recalc_stats();
    void heal();
    // Adds XP; level-ups, new moves and evolutions are reported through ui.say.
    void grant_xp(int amount, ui& ui);

private:
    void _learn_moves_at(int at_level, ui& ui);
    void _try_evolve(ui& ui);
};

struct damage_result
{
    int damage;
    int effectiveness_x4;     // 0, 1 (1/4), 2 (1/2), 4 (x1), 8, 16
};

[[nodiscard]] const move& move_data(int index);
[[nodiscard]] damage_result calc_damage(const mon& user, const move& mv, const mon& target, bn::random& random);
[[nodiscard]] const char* type_name(int type);

}

#endif
