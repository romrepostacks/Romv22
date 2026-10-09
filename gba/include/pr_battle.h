// What a battle is fought with: your side (the party, or FREE BATTLE's draft) and theirs.
#ifndef PR_BATTLE_H
#define PR_BATTLE_H

#include "pr_mon.h"
#include "pr_scenes.h"
#include "pr_world_types.h"

namespace pr
{

struct battle_setup
{
    mon* own = nullptr;
    int own_count = 0;
    mon foes[6];
    int foe_count = 0;
    const pr::trainer* opponent = nullptr;
    bool legendary = false;
    bool free = false;              // FREE BATTLE: no EXP, money, catching or items
    // NUZLOCKE: whether this wild battle may still be caught from (the area's first encounter; scripted
    // battles never). Dupes are refused one by one (dupes clause), and only one catch per battle.
    bool nuzlocke_catch = true;
    bool smart = false;             // CHALLENGE TOWER from rank 3: foes pick their best move and target
    int8_t boss_heals = 0;          // FULL RESTOREs a battle: Calderra's leaders, rival, Elite Four and Champion
                                    // one, the Sundered Isles' two (3.0.0)
    bool sharp = false;             // the Sundered Isles' trainers (3.0.0): smart, and they read held items too
    battle_weather weather = battle_weather::NONE;  // the weather at the start (3.0.0: the islands' storms)
};

battle_report run_battle(battle_setup& setup);

}

#endif
