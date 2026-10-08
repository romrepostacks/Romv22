// Poké Legends: Lands of Nine for Game Boy Advance: the title, then the overworld and its battles (or FREE BATTLE).
#include "bn_core.h"

#include "pr_audio.h"
#include "pr_scenes.h"
#include "pr_state.h"
#include "pr_ui.h"

int main()
{
    bn::core::init();
    pr::audio::init();
    pr::ui ui;
    pr::ui::set_faded(true);

    while(true)
    {
        if(! pr::title_scene())
        {
            continue;       // FREE BATTLE is over: back to the title
        }
        pr::encounter battle;
        pr::battle_report report;
        bool fought = false;
        while(pr::overworld_scene(battle, fought ? &report : nullptr))
        {
            report = pr::battle_scene(battle);
            fought = true;
            if(report.run_over)
            {
                break;      // NUZLOCKE: the run has ended
            }
        }
        pr::set_game_active(false);
    }
}
