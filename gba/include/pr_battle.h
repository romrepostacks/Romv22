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
};

battle_report run_battle(battle_setup& setup);

}

#endif
