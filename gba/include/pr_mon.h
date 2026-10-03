// A Pokémon in your party, a PC box or a battle, with the web game's stat, move and level rules (js/app.js:
// statCalc, movesAt, grantXp, learnMovesAt, tryEvolve, damage).
#ifndef PR_MON_H
#define PR_MON_H

#include "bn_math.h"
#include "bn_string.h"
#include "bn_random.h"
#include "pr_game_types.h"

namespace pr
{

class ui;

constexpr int nick_length = 12;         // renamePartyMon / nickNext: slice(0, 12)

// Battle-only marks (cleared when a battle starts).
namespace mon_flag
{
    constexpr uint8_t FOUGHT = 1;           // took part (EXP SHARE OFF)
    constexpr uint8_t DISGUISE_USED = 2;
    constexpr uint8_t SASH_USED = 4;
    constexpr uint8_t BERRY_USED = 8;
}

// Kept for good (in the save).
namespace mon_trait
{
    constexpr uint8_t SHINY = 1;
    constexpr uint8_t EDITED = 2;           // its stats were set by hand: they keep their place in the range
}

// Gen 3's range for a stat at a level: IV 0-31, EV 0-252 and a nature's x0.9-x1.1 (HP has no nature). This
// game's own formula (IV 31, no EVs, a neutral nature) sits inside it.
[[nodiscard]] int stat_min(int base, bool is_hp, int level);
[[nodiscard]] int stat_max(int base, bool is_hp, int level);

struct mon
{
    uint16_t species_index = 0;
    uint8_t level = 0;                  // 0: an empty box slot
    uint8_t move_count = 0;
    status st = status::NONE;
    uint8_t sleep_turns = 0;
    held_item item = held_item::NONE;
    uint8_t flags = 0;
    uint16_t slots[4] = {};             // a move each: its index (low 10 bits) and the PP used (high 6 bits)
    uint16_t hp = 0;
    uint16_t max_hp = 0;
    uint16_t atk = 0;
    uint16_t def = 0;
    uint16_t spa = 0;
    uint16_t spd = 0;
    uint16_t spe = 0;
    uint16_t xp = 0;
    char nick[nick_length + 1] = {};
    uint8_t traits = 0;                 // mon_trait bits (this byte was always-0 padding in older saves)

    [[nodiscard]] static mon make(species_id id, int level, held_item item = held_item::NONE);

    [[nodiscard]] bool empty() const
    {
        return level == 0;
    }
    [[nodiscard]] const pr::species& data() const;
    // dname(): the nickname, else the species' name.
    [[nodiscard]] const char* name() const;
    [[nodiscard]] const char* species_name() const;
    [[nodiscard]] bool shiny() const
    {
        return traits & mon_trait::SHINY;
    }
    [[nodiscard]] bool fainted() const
    {
        return hp == 0;
    }
    [[nodiscard]] int xp_next() const
    {
        return level * 8;
    }
    [[nodiscard]] bool has_type(int type) const;

    // The move in a slot, and its PP (the move's base PP; a POKéMON CENTER restores it).
    [[nodiscard]] int move(int slot) const
    {
        return slots[slot] & 0x3ff;
    }
    void set_move(int slot, int move_index)
    {
        slots[slot] = uint16_t(move_index & 0x3ff);
    }
    [[nodiscard]] int max_pp(int slot) const;
    [[nodiscard]] int pp(int slot) const
    {
        return bn::max(0, max_pp(slot) - (slots[slot] >> 10));
    }
    void use_pp(int slot)
    {
        if(pp(slot) > 0)
        {
            slots[slot] = uint16_t(slots[slot] + (1 << 10));
        }
    }
    void restore_pp()
    {
        for(uint16_t& s : slots)
        {
            s &= 0x3ff;
        }
    }
    // Every move is out of PP: it can only Struggle.
    [[nodiscard]] bool out_of_pp() const
    {
        for(int i = 0; i < move_count; ++i)
        {
            if(pp(i) > 0)
            {
                return false;
            }
        }
        return true;
    }
    [[nodiscard]] const ability& abil() const;

    // The stats for its level (and species). A hand-edited one keeps each stat's place between the min and max
    // it had (at old_level, with old_base) instead.
    void recalc_stats(int old_level = 0, const base_stats* old_base = nullptr);
    void heal();
    // Adds XP (grantXp): level-ups, new moves and evolutions are reported through ui.say; moves it has no
    // room for wait in the pending list for after the battle (movePromptNext).
    void grant_xp(int amount, ui& ui);
    void set_species(int index);

private:
    void _learn_moves_at(int at_level, ui& ui);
    void _try_evolve(ui& ui);
};

// "X wants to learn Y" after a battle (pendingMoves).
struct pending_move
{
    mon* m;
    uint16_t move;
};
void add_pending_move(mon* m, int move);
bool take_pending_move(pending_move& out);
void clear_pending_moves();

struct damage_result
{
    int damage;
    int effectiveness_x4;     // 0, 1 (1/4), 2 (1/2), 4 (x1), 8, 16
};

// What a battle adds to the damage formula: stat stages (-6..6), the weather, and a move that hits several.
struct damage_mods
{
    int atk_stage = 0;
    int def_stage = 0;
    battle_weather weather = battle_weather::NONE;
    bool spread = false;
};
// A stat stage's multiplier x100 (2/8 .. 8/2), and accuracy's (3/9 .. 9/3).
[[nodiscard]] int stage_x100(int stage);
[[nodiscard]] int accuracy_stage_x100(int stage);

[[nodiscard]] const move& move_data(int index);
[[nodiscard]] int effectiveness_x4(int move_type, const mon& target);
[[nodiscard]] damage_result calc_damage(const mon& user, const move& mv, const mon& target, bn::random& random,
                                        const damage_mods& mods = damage_mods());
// The type multiplier x4 with the target's ability (Levitate, Volt Absorb...) and the user's Corrosion.
[[nodiscard]] int move_effectiveness_x4(const mon& user, const move& mv, const mon& target);
[[nodiscard]] int type_index(const char* name);
[[nodiscard]] const char* type_name(int type);
// movesAt(): the four moves a species knows at a level.
int moves_at(const species& s, int level, uint16_t* out);

}

#endif
