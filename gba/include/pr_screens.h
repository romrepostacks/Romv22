// Full-screen menus, after the web game's Emerald-style screens: the party screen (partyOpen), the summary
// (summaryDraw), the bag (bagOpen) and the PC's box (usePC).
#ifndef PR_SCREENS_H
#define PR_SCREENS_H

#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"
#include "pr_ids.h"

namespace pr
{

enum class party_mode
{
    FIELD,          // SUMMARY / SWITCH
    BATTLE,         // look only (everyone's already fighting)
    CHOOSE          // pick one (to use an item on); returns the index
};

// Returns the chosen index in CHOOSE mode, else -1.
int party_screen(party_mode mode, const char* prompt = nullptr);
void summary_screen(int index);

enum class bag_mode
{
    FIELD,          // use medicine on the party
    BATTLE          // pick an item to use this turn; returns its id, or -1
};
int bag_screen(bag_mode mode);

void pc_screen();

// An HP bar of `segments` 8 px sprites at screen (x, y), green / yellow / red as hpClass() picks.
void draw_hp_bar(bn::ivector<bn::sprite_ptr>& bar, int x, int y, int segments, int hp, int max_hp);

// Uses a medicine on a party member (useItem): fills message and returns true, or returns false if it
// would do nothing (and nothing is used up).
bool use_item(item_id id, int party_index, bn::string<80>& message);

}

#endif
