// Party Royale for Game Boy Advance: title -> overworld <-> battles.
#include "bn_core.h"

#include "pr_scenes.h"
#include "pr_state.h"
#include "pr_ui.h"

int main()
{
    bn::core::init();
    pr::ui ui;
    pr::ui::set_faded(true);

    while(true)
    {
        pr::title_scene();
        pr::encounter battle;
        while(pr::overworld_scene(battle))
        {
            pr::battle_scene(battle);
        }
    }
}
