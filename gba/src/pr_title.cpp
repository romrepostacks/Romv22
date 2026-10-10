// The title screen (handheldTitle), its menu (CONTINUE with the save's details, NEW GAME, FREE BATTLE,
// WHAT'S NEW), and the new-game intro: the professor's welcome on a dark stage with LOTAD beside him
// (beginNewStory / INTRO_LINES), your name, and his send-off. Your first partner comes later, on ROUTE 1,
// from a trio picked at random (STARTER_TRIOS).
#include "bn_bg_palettes.h"
#include "bn_common.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_unique_ptr.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_intro_bg.h"
#include "bn_regular_bg_items_title_logo.h"
#include "bn_regular_bg_items_title_sky.h"
#include "bn_sprite_items_title_hooh.h"
#include "bn_sprite_items_title_spark.h"
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
    constexpr const char* build_version = "6.0.0";

    BN_DATA_EWRAM game_state saved_preview;

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

    // NEW GAME's choices (Phase 6): NORMAL or NUZLOCKE, then SKIP STORY TEXT. Returns false if backed out.
    bool choose_run(run_state& run)
    {
        ui& u = gui();
        bn::bg_palettes::set_transparent_color(bn::color(7, 15, 21));
        ui::fade_in(8);
        u.show_text("Which kind of adventure would you like?");
        constexpr bn::string_view modes[] = { "NORMAL", "NUZLOCKE" };
        int k = u.list(modes, 2);
        u.clear_text();
        if(k < 0)
        {
            ui::fade_out(8);
            return false;
        }
        run.mode = k == 1 ? run_mode::NUZLOCKE : run_mode::NORMAL;
        if(run.nuzlocke())
        {
            u.say("NUZLOCKE: a POKéMON that faints is gone for good. It can't be used or revived.");
            u.say("Only the first wild POKéMON you meet in each area can be caught, and every catch gets a nickname.");
            u.say("Ones you already have don't count (dupes clause). Your POKéMON can't level past the next GYM LEADER.");
            u.say("If your whole party falls, the run is over. Good luck!");
        }
        u.show_text("SKIP STORY TEXT? Scenes and calls complete themselves.");
        run.skip_story = u.yes_no(false);
        u.clear_text();
        ui::fade_out(8);
        return true;
    }

    void new_game(const run_state& run)
    {
        ui& u = gui();
        reset_state();
        game_state& g = state();
        g.run = run;
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
            for(int i = 0; i < lines && ! g.run.skip_story; ++i)
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
            if(! g.run.skip_story)
            {
                text = g.name;
                text.append(", your very own POKéMON adventure is about to begin!");
                u.say(text);
                u.say("Dreams, friendships, rivals... they're all waiting out there. I'll see you soon!");
            }
            ui::fade_out(20);
        }
        save_game();
    }

    // NEW ADVENTURE MODE (Phase 7): no story. Your name, a draft of 6 at level 50 (the other 4 slots stay empty
    // until you catch more), and the post-game: every gym, rival and the League beaten, SURF, DIVE and the OLD
    // ROD in the bag, starting in SPIRECREST TOWN by the CHALLENGE TOWER. Returns false if backed out.
    bool new_adventure()
    {
        ui& u = gui();
        uint16_t picks[6];
        held_item items[6];
        bn::bg_palettes::set_transparent_color(bn::color(7, 15, 21));
        ui::fade_in(8);
        u.say("NEW ADVENTURE MODE: skip the story and start as a CHAMPION, with the CHALLENGE TOWER open.");
        u.say("Draft a team of 6 POKéMON at level 50. Press START when your team is ready.");
        ui::fade_out(8);
        u.win().clear_all();
        if(! draft_team(6, picks, items))
        {
            return false;
        }
        reset_state();
        game_state& g = state();
        g.run.adventure = true;
        u.keyboard("YOUR NAME?", g.name, name_length);
        if(! g.name[0])
        {
            const char* fallback = "Trainer";
            for(int i = 0; i < 8; ++i)
            {
                g.name[i] = fallback[i];
            }
        }
        for(int i = 0; i < 6; ++i)
        {
            g.party[i] = mon::make(species_id(picks[i]), 50, items[i]);
            g.mark_owned(picks[i]);
        }
        g.party_count = 6;
        g.trainer_id = uint16_t(1 + rng().get_int(65535));
        g.story = story::STARTER | story::CALL1 | story::CALL2 | story::CALL3 | story::SCENE_TEMPEST_RUN |
                  story::SCENE_PORTMERE | story::SCENE_HIDEOUT | story::SCENE_SHRINE | story::STORM_ENDED | story::CHAMPION;
        for(int i = 0; i < world_data::maps_count; ++i)
        {
            const map_def& m = world_data::maps[i];
            if(map_region(i) != 1)
            {
                continue;       // Calderra's story is still ahead
            }
            for(int k = 0; k < m.trainers_count; ++k)
            {
                const trainer& t = m.trainers[k];
                if(t.role == trainer_role::LEADER || t.role == trainer_role::RIVAL || t.role == trainer_role::ELITE ||
                   t.role == trainer_role::CHAMPION)
                {
                    g.beaten.set(t.id);
                }
            }
        }
        g.add_item(item_id::POKEBALL, 20);
        g.add_item(item_id::SUPERPOTION, 10);
        g.add_item(item_id::REVIVE, 3);
        g.add_item(item_id::HM03, 1);
        g.add_item(item_id::HM08, 1);
        g.add_item(item_id::OLDROD, 1);
        g.money = 10000;
        for(int i : world_data::area_maps)
        {
            const map_def& m = world_data::maps[i];
            if(m.area && (m.area->flags & area_flag::TOWER_TOWN))
            {
                g.map = int16_t(i);
                g.x = m.spawn_x;
                g.y = m.spawn_y;
                g.last_heal = int16_t(i);
            }
        }
        g.facing = direction::DOWN;
        save_game();
        return true;
    }

    // A finished NUZLOCKE run (title CONTINUE): its summary, then back to the menu.
    void run_summary(const game_state& saved)
    {
        ui& u = gui();
        bn::bg_palettes::set_transparent_color(bn::color(3, 4, 8));
        ui::fade_in(8);
        bn::string<64> text(saved.name);
        text.append("'s NUZLOCKE run is over.");
        u.say(text);
        text = "Badges: ";
        text.append(bn::to_string<4>(saved.badges()));
        text.append("   POKéMON lost: ");
        text.append(bn::to_string<6>(saved.run.deaths));
        u.say(text);
        text = "Start a NEW GAME to try again!";
        u.say(text);
        ui::fade_out(8);
    }

    // WHAT'S NEW: the message of the day and the changelog, wrapped to the window (nothing cut short), scrolled
    // with Up/Down.
    void news_screen()
    {
        ui& u = gui();
        bn::bg_palettes::set_transparent_color(bn::color(7, 15, 21));
        bn::unique_ptr<bn::vector<bn::string_view, 640>> rows(new bn::vector<bn::string_view, 640>());
        for(int i = 0; i < news_data::lines_count && ! rows->full(); ++i)
        {
            bn::string_view parts[8];
            int n = u.wrap_lines(news_data::lines[i], 212, true, parts, 8);
            if(! n)
            {
                rows->push_back("");
            }
            for(int k = 0; k < n && ! rows->full(); ++k)
            {
                rows->push_back(parts[k]);
            }
        }
        int count = rows->size();
        int top = 0;
        constexpr int visible = 8;
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
                for(int r = 0; r < visible && top + r < count; ++r)
                {
                    u.print_fit(10, 24 + r * 15, (*rows)[top + r], 216, text_color::INK, texts, true);
                }
                u.print(150, 146, "B: back", text_color::INK, texts, true);
                redraw = false;
            }
            frame();
            if(bn::keypad::b_pressed() || bn::keypad::a_pressed() || bn::keypad::start_pressed())
            {
                break;
            }
            if(bn::keypad::down_held() && top + visible < count)
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
        NEW_ADVENTURE,
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
        bool adventure = device_cleared() || (has_save && saved.has(story::CHAMPION));
        title_pick picks[5];
        int n = 0;
        int y = adventure ? 0 : 1;
        int rows_y[5];
        if(has_save)
        {
            u.win().box(window_style::WINDOW, 1, y, 28, 10);
            u.print(24, y * 8 + 4, saved.run.over ? "RUN OVER" : "CONTINUE", saved.run.over ? text_color::RED : text_color::INK,
                    texts);
            // The run's mode, top right.
            const char* mode = saved.run.nuzlocke() ? "NUZLOCKE" : saved.run.adventure ? "ADVENTURE MODE" : "";
            if(mode[0])
            {
                u.print(216 - u.width(mode, true), y * 8 + 7, mode, text_color::RED, texts, true);
            }
            auto row = [&](int i, const char* label, const bn::string_view& value)
            {
                u.print(32, y * 8 + 20 + i * 14, label, text_color::BLUE, texts);
                u.print(208 - u.width(value), y * 8 + 20 + i * 14, value, text_color::BLUE, texts);
            };
            row(0, "PLAYER", saved.name);
            row(1, "TIME", play_time(saved.play_frames));
            row(2, "POKéDEX", bn::to_string<4>(saved.owned.count()));
            row(3, "BADGES", bn::to_string<4>(saved.badges()));
            rows_y[n] = y * 8 + 4;
            picks[n++] = title_pick::CONTINUE;
            y += 10;
        }
        const char* names[4];
        title_pick kinds[4];
        int m = 0;
        names[m] = "NEW GAME"; kinds[m++] = title_pick::NEW_GAME;
        if(adventure)
        {
            names[m] = "NEW ADVENTURE MODE"; kinds[m++] = title_pick::NEW_ADVENTURE;
        }
        names[m] = "FREE BATTLE"; kinds[m++] = title_pick::FREE_BATTLE;
        names[m] = "WHAT'S NEW"; kinds[m++] = title_pick::NEWS;
        u.win().box(window_style::WINDOW, 1, y, 28, 2 + m * 2);
        for(int i = 0; i < m; ++i)
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
        bn::regular_bg_ptr sky = bn::regular_bg_items::title_sky.create_bg(0, 0);
        sky.set_priority(3);
        bn::regular_bg_ptr logo = bn::regular_bg_items::title_logo.create_bg(0, -80);
        logo.set_priority(2);
        // Nine twinkles, one for each land, and HO-OH crossing the dusk now and then.
        constexpr int spark_xy[9][2] = { { -100, -66 }, { -62, -74 }, { -24, -70 }, { 30, -76 }, { 70, -68 },
                                         { 104, -60 }, { -84, 0 }, { 0, -4 }, { 88, 2 } };
        bn::vector<bn::sprite_ptr, 9> sparks;
        int spark_t[9];
        for(int i = 0; i < 9; ++i)
        {
            sparks.push_back(bn::sprite_items::title_spark.create_sprite(spark_xy[i][0], spark_xy[i][1]));
            sparks.back().set_bg_priority(3);
            sparks.back().set_visible(false);
            spark_t[i] = -i * 23;
        }
        bn::sprite_ptr hooh = bn::sprite_items::title_hooh.create_sprite(-200, -40);
        hooh.set_bg_priority(3);
        hooh.set_horizontal_flip(true);
        bn::vector<bn::sprite_ptr, 32> title;
        bn::vector<bn::sprite_ptr, 16> press;
        u.print(120, 100, "PRESS START", text_color::WHITE, press);
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
            // The logo drops in and settles with a bounce.
            if(blink <= 48)
            {
                constexpr int8_t drop[] = { -80, -64, -48, -32, -18, -6, 4, 8, 6, 2, -2, -3, -2, 0 };
                logo.set_y(drop[bn::min(blink / 4, 13)]);
            }
            for(int i = 0; i < 9; ++i)
            {
                int t = ++spark_t[i];
                sparks[i].set_visible(t >= 0 && t < 24);
                if(t >= 0 && t < 24)
                {
                    sparks[i].set_tiles(bn::sprite_items::title_spark.tiles_item(), t < 12 ? t / 4 : 3 - (t - 12) / 4);
                }
                else if(t >= 24)
                {
                    spark_t[i] = -40 - rng().get_int(120);
                }
            }
            int fly = blink % 600;
            hooh.set_visible(fly < 240);
            hooh.set_position(150 - fly * 300 / 240, -50 + (bn::degrees_lut_sin((fly * 3) % 360) * 6).integer());
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
        if(has && saved_preview.has(story::CHAMPION) && ! device_cleared())
        {
            set_device_cleared();       // a game cleared before this version
        }
        title_pick pick = main_menu(has, saved_preview);
        if(pick == title_pick::CONTINUE && has && saved_preview.run.over)
        {
            run_summary(saved_preview);
            continue;
        }
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
        if(pick == title_pick::NEW_GAME || pick == title_pick::NEW_ADVENTURE)
        {
            if(has && ! saved_preview.run.over)
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
            if(pick == title_pick::NEW_ADVENTURE)
            {
                if(! new_adventure())
                {
                    continue;
                }
                set_game_active(true);
                set_just_loaded();
                return true;
            }
            run_state run;
            if(! choose_run(run))
            {
                continue;
            }
            new_game(run);
            set_game_active(true);
            set_new_game_started();
            return true;
        }
    }
}

}
