// Full-screen menus, after the web game's Emerald-style screens: the party (partyOpen), the summary
// (summaryDraw), the bag (bagOpen), the PC's boxes (boxOpen), the POKéDEX (dexOpen), OPTION, the TRAINER
// CARD and the region map (POKéNAV).
#ifndef PR_SCREENS_H
#define PR_SCREENS_H

#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"
#include "pr_ids.h"

namespace pr
{

struct mon;

enum class party_mode
{
    FIELD,          // SUMMARY / SWITCH
    BATTLE,         // look only (everyone's already fighting)
    CHOOSE          // pick one (to use an item on); returns the index
};

// Returns the chosen index in CHOOSE mode, else -1.
int party_screen(party_mode mode, const char* prompt = nullptr);
void summary_screen(const mon* mons, int count, int index);

enum class bag_mode
{
    FIELD,          // use medicine on the party
    BATTLE          // pick an item to use this turn; returns its id, or -1
};
int bag_screen(bag_mode mode);

// The hidden editor (see pr_field: one tree, then A, B, A at another): a party Pokémon's level, shininess,
// stats within their legal range, and moves.
void secret_editor_screen();

enum class pc_mode
{
    WITHDRAW,
    DEPOSIT,
    MOVE
};
void pc_box_screen(pc_mode mode);

// The POKéDEX from START; or, with a species, its entry headed "POKéDEX registration completed."
void dex_screen(int register_species = -1);
void option_screen();
void card_screen();
void region_map_screen();
// The Hall of Fame and the credits roll (playCredits).
void credits_screen();

// An HP bar of `segments` 8 px sprites at screen (x, y), green / yellow / red as hpClass() picks.
void draw_hp_bar(bn::ivector<bn::sprite_ptr>& bar, int x, int y, int segments, int hp, int max_hp);

// Uses a medicine on a party member (useItem): fills message and returns true, or returns false if it
// would do nothing (and nothing is used up).
bool use_item(item_id id, int party_index, bn::string<80>& message);
bool use_item_on(item_id id, mon& m, bn::string<80>& message);

}

#endif
