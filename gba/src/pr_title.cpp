// The title screen (handheldTitle), its menu (CONTINUE with the save's details, NEW GAME, FREE BATTLE,
// WHAT'S NEW), and the new-game intro: the professor's welcome on a dark stage with LOTAD beside him
// (beginNewStory / INTRO_LINES), your name, and his send-off. Your first partner comes later, on ROUTE 1,
// from a trio picked at random (STARTER_TRIOS).
#include "bn_bg_palettes.h"
#include "bn_common.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_intro_bg.h"
#include "bn_regular_bg_items_title_bg.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_prof_big.h"

#include "pr_audio.h"
#include "pr_game_data.h"
#include "pr_news_data.h"
#include "pr_scenes.h"
#include "pr_state.h"
#include "pr_ui.h"
#include "pr_world_data.h"

namespace pr
{

namespace
{
    constexpr int start_poke_balls = 10;     // introFinish(): items {pokeball: 10}
    constexpr const char* build_version = "1.0";

    BN_DATA_EWRAM_BSS game_state saved_preview;

    bn::string<16> play_time(uint32_t frames)
    {
        int minutes = int(frames / 3600);
        bn::string<16> t(bn::to_string<6>(minutes / 60));
        t.append(":");
        if(minutes % 60 < 10)
        {
            t.append("0");
        }
        t.append(bn::to_string<4>(minutes % 60));
        return t;
    }

    void new_game()
    {
        ui& u = gui();
        reset_state();
        game_state& g = state();
        const map_def& home = world_data::maps[0];
        g.map = 0;
        g.x = home.spawn_x;
        g.y = home.spawn_y;
        g.facing = direction::DOWN;
        g.add_item(item_id::POKEBALL, start_poke_balls);
        g.pc_items[int(item_id::POTION)] = 1;       // playerPC(): a POTION stored to begin with
        constexpr int trios = int(sizeof(game_data::starter_trios) / sizeof(game_data::starter_trios[0]));
        g.starter_trio = uint8_t(rng().get_int(trios));
        g.trainer_id = uint16_t(1 + rng().get_int(65535));

        {
            // The professor on the web game's dark stage, with LOTAD beside him.
            bn::regular_bg_ptr bg = bn::regular_bg_items::intro_bg.create_bg(8, 48);
            bg.set_priority(3);
            bn::sprite_ptr prof = bn::sprite_items::prof_big.create_sprite(sx(120), sy(52));
            prof.set_bg_priority(2);
            const species& lotad = game_data::species_list[int(species_id::LOTAD)];
            bn::sprite_ptr mon = lotad.front.create_sprite(sx(184), sy(66));
            mon.set_bg_priority(2);
            ui::fade_in(16);
            constexpr int lines = int(sizeof(game_data::intro_lines) / sizeof(game_data::intro_lines[0]));
            for(int i = 0; i < lines; ++i)
            {
                u.say(game_data::intro_lines[i]);
            }
            ui::fade_out(12);
        }
        u.keyboard("YOUR NAME?", g.name, name_length);
        // introFinish(): an empty name becomes "Trainer".
        if(! g.name[0])
        {
            const char* fallback = "Trainer";
            for(int i = 0; i < 8; ++i)
            {
                g.name[i] = fallback[i];
            }
        }
        {
            bn::regular_bg_ptr bg = bn::regular_bg_items::intro_bg.create_bg(8, 48);
            bg.set_priority(3);
            bn::sprite_ptr prof = bn::sprite_items::prof_big.create_sprite(sx(120), sy(52));
            prof.set_bg_priority(2);
            ui::fade_in(12);
            bn::string<96> text(g.name);
            text.append("! That's a fine name.");
            u.say(text);
            text = g.name;
            text.append(", your very own POKéMON adventure is about to begin!");
            u.say(text);
            u.say("Dreams, friendships, rivals... they're all waiting out there. I'll see you soon!");
            ui::fade_out(20);
        }
        save_game();
    }

    // WHAT'S NEW: the message of the day and the changelog, scrolled with Up/Down.
    void news_screen()
    {
        ui& u = gui();
        bn::bg_palettes::set_transparent_color(bn::color(7, 15, 21));
        int top = 0;
        constexpr int rows = 8;
        bool redraw = true;
        bn::vector<bn::sprite_ptr, 96> texts;
        ui::fade_in(8);
        while(true)
        {
            if(redraw)
            {
                texts.clear();
                u.win().box(window_style::WINDOW, 0, 0, 30, 20);
                u.print(8, 6, "WHAT'S NEW", text_color::BLUE, texts);
                for(int r = 0; r < rows && top + r < news_data::lines_count; ++r)
                {
                    bn::string_view line(news_data::lines[top + r]);
                    // Long lines are cut to the window.
                    bn::string<64> shown;
                    for(char c : line)
                    {
                        if(shown.size() >= 60 || u.width(shown, true) > 212)
                        {
                            break;
                        }
                        shown.push_back(c);
                    }
                    u.print(10, 24 + r * 15, shown, text_color::INK, texts, true);
                }
                u.print(150, 146, "B: back", text_color::INK, texts, true);
                redraw = false;
            }
            frame();
            if(bn::keypad::b_pressed() || bn::keypad::a_pressed() || bn::keypad::start_pressed())
            {
                break;
            }
            if(bn::keypad::down_held() && top + rows < news_data::lines_count)
            {
                ++top;
                redraw = true;
                wait(4);
            }
            else if(bn::keypad::up_held() && top > 0)
            {
                --top;
                redraw = true;
                wait(4);
            }
        }
        ui::fade_out(8);
        texts.clear();
        u.win().clear_all();
    }

    enum class title_pick
    {
        CONTINUE,
        NEW_GAME,
        FREE_BATTLE,
        NEWS
    };

    // The title's menu (titleKey): CONTINUE (with the save's details, Emerald-style), NEW GAME, FREE BATTLE,
    // WHAT'S NEW.
    title_pick main_menu(bool has_save, const game_state& saved)
    {
        ui& u = gui();
        bn::bg_palettes::set_transparent_color(bn::color(7, 15, 21));
        bn::vector<bn::sprite_ptr, 40> texts;
        title_pick picks[4];
        int n = 0;
        int y = 1;
        int rows_y[4];
        if(has_save)
        {
            u.win().box(window_style::WINDOW, 1, y, 28, 10);
            u.print(24, y * 8 + 4, "CONTINUE", text_color::INK, texts);
            auto row = [&](int i, const char* label, const bn::string_view& value)
            {
                u.print(32, y * 8 + 22 + i * 15, label, text_color::BLUE, texts);
                u.print(208 - u.width(value), y * 8 + 22 + i * 15, value, text_color::BLUE, texts);
            };
            row(0, "PLAYER", saved.name);
            row(1, "TIME", play_time(saved.play_frames));
            row(2, "POKéDEX", bn::to_string<4>(saved.owned.count()));
            row(3, "BADGES", bn::to_string<4>(saved.badges()));
            rows_y[n] = y * 8 + 4;
            picks[n++] = title_pick::CONTINUE;
            y += 10;
        }
        constexpr const char* names[] = { "NEW GAME", "FREE BATTLE", "WHAT'S NEW" };
        constexpr title_pick kinds[] = { title_pick::NEW_GAME, title_pick::FREE_BATTLE, title_pick::NEWS };
        u.win().box(window_style::WINDOW, 1, y, 28, 2 + 3 * 2);
        for(int i = 0; i < 3; ++i)
        {
            u.print(24, y * 8 + 4 + i * 16, names[i], text_color::INK, texts);
            rows_y[n] = y * 8 + 4 + i * 16;
            picks[n++] = kinds[i];
        }
        bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(0, 0);
        cursor.set_bg_priority(0);
        int index = 0;
        ui::fade_in(12);
        while(true)
        {
            cursor.set_position(sx(14 + 4), sy(rows_y[index] + 8));
            rng().update();
            frame();
            if(bn::keypad::up_pressed())
            {
                index = (index + n - 1) % n;
                audio::play(audio::sfx::SELECT);
            }
            else if(bn::keypad::down_pressed())
            {
                index = (index + 1) % n;
                audio::play(audio::sfx::SELECT);
            }
            else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
            {
                audio::play(audio::sfx::SELECT);
                break;
            }
        }
        ui::fade_out(12);
        u.win().clear_all();
        return picks[index];
    }
}

bool title_scene()
{
    ui& u = gui();
    set_game_active(false);
    audio::play_music("credits");
    bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));
    {
        bn::regular_bg_ptr bg = bn::regular_bg_items::title_bg.create_bg(8, 48);
        bg.set_priority(3);
        bn::vector<bn::sprite_ptr, 32> title;
        u.win().box(window_style::WINDOW, 3, 2, 24, 6);
        u.text().set_center_alignment();
        u.print(120, 26, "PARTY ROYALE", text_color::RED, title);
        u.print(120, 42, "Vellorin Version", text_color::INK, title);
        bn::vector<bn::sprite_ptr, 16> press;
        u.print(120, 112, "PRESS START", text_color::WHITE, press);
        u.text().set_left_alignment();
        // The message of the day (news.json motd): centred, or scrolling by if it's too wide.
        bn::vector<bn::sprite_ptr, 40> motd;
        u.print(0, 136, news_data::motd, text_color::WHITE, motd, true);
        int motd_w = u.width(news_data::motd, true);
        bn::vector<bn::fixed, 40> motd_x;
        for(bn::sprite_ptr& s : motd)
        {
            motd_x.push_back(s.x());
        }
        bn::string<24> build("build ");
        build.append(build_version);
        u.print(236 - u.width(build, true), 150, build, text_color::WHITE, title, true);
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
            int offset = motd_w <= 232 ? 120 - motd_w / 2 : 240 - (blink / 2) % (motd_w + 240);
            for(int i = 0; i < motd.size(); ++i)
            {
                bn::fixed x = motd_x[i] + offset;
                motd[i].set_x(x);
                motd[i].set_visible(x > -152 && x < 152);
            }
            frame();
        }
        audio::play(audio::sfx::SELECT);
        ui::fade_out(12);
        u.win().clear_all();
    }
    while(true)
    {
        bool has = peek_save(saved_preview);
        title_pick pick = main_menu(has, saved_preview);
        if(pick == title_pick::CONTINUE && load_game())
        {
            set_game_active(true);
            set_just_loaded();
            return true;
        }
        if(pick == title_pick::NEWS)
        {
            news_screen();
            continue;
        }
        if(pick == title_pick::FREE_BATTLE)
        {
            free_battle_scene();
            return false;
        }
        if(pick == title_pick::NEW_GAME)
        {
            if(has)
            {
                // newAdventurePrompt(): this overwrites the saved game.
                bn::bg_palettes::set_transparent_color(bn::color(7, 15, 21));
                ui::fade_in(8);
                u.show_text("Start a new adventure? This will overwrite your current saved game.");
                bool yes = u.yes_no(false);
                u.clear_text();
                ui::fade_out(8);
                if(! yes)
                {
                    continue;
                }
            }
            new_game();
            set_game_active(true);
            set_new_game_started();
            return true;
        }
    }
}

}
