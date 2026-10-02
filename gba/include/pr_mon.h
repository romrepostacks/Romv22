// A Pokémon in your party, a PC box or a battle, with the web game's stat, move and level rules (js/app.js:
// statCalc, movesAt, grantXp, learnMovesAt, tryEvolve, damage).
#ifndef PR_MON_H
#define PR_MON_H

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
}

struct mon
{
    uint16_t species_index = 0;
    uint8_t level = 0;                  // 0: an empty box slot
    uint8_t move_count = 0;
    status st = status::NONE;
    uint8_t sleep_turns = 0;
    held_item item = held_item::NONE;
    uint8_t flags = 0;
    uint16_t moves[4] = {};
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
    [[nodiscard]] const ability& abil() const;

    void recalc_stats();
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

[[nodiscard]] const move& move_data(int index);
[[nodiscard]] int effectiveness_x4(int move_type, const mon& target);
[[nodiscard]] damage_result calc_damage(const mon& user, const move& mv, const mon& target, bn::random& random);
[[nodiscard]] const char* type_name(int type);
// movesAt(): the four moves a species knows at a level.
int moves_at(const species& s, int level, uint16_t* out);

}

#endif
