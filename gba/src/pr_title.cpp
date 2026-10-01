// Title screen, CONTINUE / NEW GAME, and the new-game intro: the professor's welcome (the web game's
// INTRO_LINES) and choosing your first partner.
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_battle_bg_plain.h"
#include "bn_regular_bg_items_title_bg.h"

#include "pr_game_data.h"
#include "pr_scenes.h"
#include "pr_state.h"
#include "pr_ui.h"
#include "pr_world_data.h"

namespace pr
{

namespace
{
    constexpr int start_poke_balls = 10;     // introFinish(): items {pokeball: 10}

    void new_game(ui& ui)
    {
        game_state& g = state();
        g = game_state();
        const pr::area& home = world_data::areas[0];
        g.area = 0;
        g.x = home.spawn_x;
        g.y = home.spawn_y;
        g.facing = direction::DOWN;
        g.poke_balls = start_poke_balls;

        bn::bg_palettes::set_transparent_color(bn::color(26, 30, 24));
        bn::regular_bg_ptr bg = bn::regular_bg_items::battle_bg_plain.create_bg(8, 48);
        bg.set_priority(3);
        ui::fade_in(16);

        bn::string<96> line(game_data::prof_name);
        constexpr const char* intro[] = {
            "Hi there! Sorry to keep you waiting!",
            "Welcome to the world of POKéMON!",
            nullptr,
            "This world is home to creatures called POKéMON. People and POKéMON live side by side here in the VELLORIN region.",
            "Some play with them, some work with them, and some battle alongside them.",
            "Me? I study POKéMON for a living.",
        };
        for(const char* text : intro)
        {
            if(! text)
            {
                line = "I'm ";
                line.append(game_data::prof_name);
                line.append(". Around here, people call me the POKéMON PROFESSOR.");
                ui.say(line);
            }
            else
            {
                ui.say(text);
            }
        }
        ui.say("Now, every trainer needs a partner. I have three POKéMON here. Go on, choose one!");

        // The three starters on the battle platforms' backdrop.
        bn::vector<bn::sprite_ptr, 3> mons;
        constexpr int xs[] = { -84, -24, 36 };
        for(int i = 0; i < 3; ++i)
        {
            mons.push_back(game_data::species_list[int(game_data::starters[i])].front.create_sprite(xs[i], -56));
            mons.back().set_bg_priority(2);
        }
        int pick = 0;
        while(true)
        {
            bn::string_view names[3];
            bn::string<48> hints[3];
            bn::string_view hint_views[3];
            for(int i = 0; i < 3; ++i)
            {
                const species& s = game_data::species_list[int(game_data::starters[i])];
                names[i] = s.name;
                hints[i] = "The ";
                hints[i].append(type_name(s.type1));
                hints[i].append(" POKéMON ");
                hints[i].append(s.name);
                hints[i].append(".");
                hint_views[i] = hints[i];
            }
            pick = ui.menu(names, 3, false, pick, false, hint_views);
            const species& s = game_data::species_list[int(game_data::starters[pick])];
            line = "Do you choose ";
            line.append(s.name);
            line.append("?");
            ui.show_text(line);
            constexpr bn::string_view yes_no[] = { "YES", "NO" };
            int yes = ui.menu(yes_no, 2, false, 0);
            ui.clear_text();
            if(yes == 0)
            {
                break;
            }
        }
        mon partner = mon::make(game_data::starters[pick], 5);
        g.add_to_party(partner);
        line = "You chose ";
        line.append(partner.name());
        line.append("!");
        ui.say(line);
        ui.say("Your very own POKéMON adventure is about to begin!");
        ui.say("ROUTE 1 is east of town. Wild POKéMON live in the tall grass. Press START for your menu and to save.");
        ui::fade_out(16);
    }
}

void title_scene(ui& ui)
{
    bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));
    bool has_save = save_exists();
    {
        bn::regular_bg_ptr bg = bn::regular_bg_items::title_bg.create_bg(8, 48);
        bg.set_priority(3);
        bn::sprite_text_generator& gen = ui.text_generator();
        bn::vector<bn::sprite_ptr, 16> title;
        gen.set_center_alignment();
        gen.generate(0, -50, "PARTY ROYALE", title);
        gen.generate(0, -34, "GBA test build", title);
        bn::vector<bn::sprite_ptr, 16> press;
        gen.generate(0, 50, "PRESS START", press);
        gen.set_left_alignment();
        ui::fade_in(20);

        int blink = 0;
        while(! bn::keypad::start_pressed() && ! bn::keypad::a_pressed())
        {
            ++blink;
            rng().update();
            for(bn::sprite_ptr& s : press)
            {
                s.set_visible((blink / 30) % 2 == 0);
            }
            bn::core::update();
        }
        press.clear();

        int choice = 1;
        if(has_save)
        {
            constexpr bn::string_view options[] = { "CONTINUE", "NEW GAME" };
            choice = ui.menu(options, 2, false, 0, false);
            if(choice == 1)
            {
                ui.show_text("Start a new game? Your saved game is kept until you save over it.");
                constexpr bn::string_view yes_no[] = { "YES", "NO" };
                if(ui.menu(yes_no, 2, false, 1, false) != 0)
                {
                    choice = 0;
                }
                ui.clear_text();
            }
        }
        if(choice == 0 && load_game())
        {
            ui::fade_out(16);
            return;
        }
        ui::fade_out(16);
    }
    new_game(ui);
}

}
