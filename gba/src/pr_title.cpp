// Title screen, Emerald's main menu (CONTINUE with the save's details, NEW GAME), and the new-game intro: the
// professor's welcome on a dark stage with LOTAD beside him (the web game's beginNewStory / INTRO_LINES),
// choosing your name, and his send-off. Your first partner comes later, on ROUTE 1 (starterEvent).
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

    // Emerald's naming screen, simplified: a name box and a letter board. A types, B deletes, SELECT
    // switches case, START jumps to OK.
    void name_entry(char* out)
    {
        ui& u = gui();
        constexpr const char* pages[2][3] = {
            { "ABCDEFGHI", "JKLMNOPQR", "STUVWXYZ " },
            { "abcdefghi", "jklmnopqr", "stuvwxyz " }
        };
        constexpr int columns = 9, rows = 3;
        int page = 0, cx = 0, cy = 0;      // cy == rows: the bottom row (CASE / DEL / OK)
        bn::string<name_length> name;
        bn::vector<bn::sprite_ptr, 48> board;
        bn::vector<bn::sprite_ptr, 8> field;
        bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(0, 0);
        cursor.set_bg_priority(0);
        auto draw_board = [&]()
        {
            board.clear();
            u.win().box(window_style::WINDOW, 1, 1, 28, 5);
            u.print(16, 12, "YOUR NAME?", text_color::INK, board);
            u.win().box(window_style::WINDOW, 1, 7, 28, 12);
            for(int r = 0; r < rows; ++r)
            {
                for(int c = 0; c < columns; ++c)
                {
                    char ch[2] = { pages[page][r][c], 0 };
                    if(ch[0] != ' ')
                    {
                        u.print(32 + c * 22, 68 + r * 18, ch, text_color::INK, board);
                    }
                }
            }
            u.print(32, 128, page ? "UPPER" : "lower", text_color::INK, board);
            u.print(108, 128, "DEL", text_color::INK, board);
            u.print(172, 128, "OK", text_color::INK, board);
        };
        auto draw_field = [&]()
        {
            field.clear();
            bn::string<24> shown(name);
            for(int i = name.size(); i < name_length; ++i)
            {
                shown.append("_");
            }
            u.print(120, 26, shown, text_color::INK, field);
        };
        draw_board();
        draw_field();
        ui::fade_in(12);
        while(true)
        {
            int px = cy == rows ? (cx == 0 ? 32 : cx == 1 ? 108 : 172) : 32 + cx * 22;
            int py = cy == rows ? 128 : 68 + cy * 18;
            cursor.set_position(sx(px - 10 + 4), sy(py + 8));
            frame();
            if(bn::keypad::left_pressed())
            {
                cx = (cx + (cy == rows ? 2 : columns - 1)) % (cy == rows ? 3 : columns);
            }
            else if(bn::keypad::right_pressed())
            {
                cx = (cx + 1) % (cy == rows ? 3 : columns);
            }
            else if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
            {
                bool to_bottom_row = cy != rows;
                cy = (cy + (bn::keypad::up_pressed() ? rows : 1)) % (rows + 1);
                if(cy == rows && to_bottom_row)
                {
                    cx = cx < 3 ? 0 : cx < 6 ? 1 : 2;
                }
                else if(cy != rows && ! to_bottom_row)
                {
                    cx = cx * 3 + 1;
                }
            }
            else if(bn::keypad::select_pressed())
            {
                page ^= 1;
                draw_board();
            }
            else if(bn::keypad::start_pressed())
            {
                cy = rows;
                cx = 2;
            }
            else if(bn::keypad::b_pressed())
            {
                if(! name.empty())
                {
                    name.pop_back();
                    draw_field();
                }
            }
            else if(bn::keypad::a_pressed())
            {
                if(cy == rows)
                {
                    if(cx == 0)
                    {
                        page ^= 1;
                        draw_board();
                    }
                    else if(cx == 1 && ! name.empty())
                    {
                        name.pop_back();
                        draw_field();
                    }
                    else if(cx == 2)
                    {
                        break;
                    }
                }
                else
                {
                    char ch = pages[page][cy][cx];
                    if(ch != ' ' && name.size() < name_length)
                    {
                        name.push_back(ch);
                        draw_field();
                        if(name.size() == name_length)
                        {
                            cy = rows;
                            cx = 2;
                        }
                    }
                }
            }
        }
        // introFinish(): an empty name becomes "Trainer".
        if(name.empty())
        {
            name = "Trainer";
        }
        for(int i = 0; i <= name.size(); ++i)
        {
            out[i] = i < name.size() ? name[i] : 0;
        }
        ui::fade_out(12);
        board.clear();
        field.clear();
        u.win().clear_all();
    }

    void new_game()
    {
        ui& u = gui();
        game_state& g = state();
        g = game_state();
        const map_def& home = world_data::maps[0];
        g.map = 0;
        g.x = home.spawn_x;
        g.y = home.spawn_y;
        g.facing = direction::DOWN;
        g.add_item(item_id::POKEBALL, start_poke_balls);

        {
            // The professor on the web game's dark stage, with LOTAD beside him.
            bn::regular_bg_ptr bg = bn::regular_bg_items::intro_bg.create_bg(8, 48);
            bg.set_priority(3);
            bn::sprite_ptr prof = bn::sprite_items::prof_big.create_sprite(sx(120), sy(52));
            prof.set_bg_priority(2);
            bn::optional<bn::sprite_ptr> lotad;
            ui::fade_in(16);
            constexpr int lines = int(sizeof(game_data::intro_lines) / sizeof(game_data::intro_lines[0]));
            for(int i = 0; i < lines; ++i)
            {
                if(i == 3)
                {
                    // "This world is home to creatures called POKéMON": LOTAD hops out.
                    for(const species& s : game_data::species_list)
                    {
                        if(s.dex_number == 270)
                        {
                            lotad = s.front.create_sprite(sx(184), sy(66));
                            lotad->set_bg_priority(2);
                        }
                    }
                }
                u.say(game_data::intro_lines[i]);
            }
            ui::fade_out(12);
        }
        name_entry(g.name);
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
    }

    // Emerald's main menu: a CONTINUE window with the save's details, and NEW GAME.
    int main_menu(const game_state& saved)
    {
        ui& u = gui();
        bn::bg_palettes::set_transparent_color(bn::color(7, 15, 21));
        bn::vector<bn::sprite_ptr, 32> texts;
        u.win().box(window_style::WINDOW, 1, 1, 28, 10);
        u.print(24, 12, "CONTINUE", text_color::INK, texts);
        auto row = [&](int i, const char* label, const bn::string_view& value)
        {
            u.print(32, 32 + i * 16, label, text_color::INK, texts);
            u.print(208 - u.width(value), 32 + i * 16, value, text_color::INK, texts);
        };
        row(0, "PLAYER", saved.name);
        row(1, "TIME", play_time(saved.play_frames));
        row(2, "POKéDEX", bn::to_string<4>(saved.owned.count()));
        u.win().box(window_style::WINDOW, 1, 12, 28, 4);
        u.print(24, 108, "NEW GAME", text_color::INK, texts);
        bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(0, 0);
        cursor.set_bg_priority(0);
        int index = 0;
        ui::fade_in(12);
        while(true)
        {
            cursor.set_position(sx(14 + 4), sy((index ? 108 : 12) + 8));
            frame();
            if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
            {
                index ^= 1;
            }
            else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
            {
                break;
            }
        }
        ui::fade_out(12);
        u.win().clear_all();
        return index;
    }
}

void title_scene()
{
    ui& u = gui();
    bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));
    {
        bn::regular_bg_ptr bg = bn::regular_bg_items::title_bg.create_bg(8, 48);
        bg.set_priority(3);
        bn::vector<bn::sprite_ptr, 16> title;
        u.win().box(window_style::WINDOW, 3, 2, 24, 6);
        u.text().set_center_alignment();
        u.print(120, 26, "PARTY ROYALE", text_color::INK, title);
        u.print(120, 42, "GBA test build 0.3", text_color::INK, title);
        bn::vector<bn::sprite_ptr, 16> press;
        u.print(120, 124, "PRESS START", text_color::WHITE, press);
        u.text().set_left_alignment();
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
            frame();
        }
        ui::fade_out(12);
        u.win().clear_all();
    }
    if(peek_save(saved_preview))
    {
        if(main_menu(saved_preview) == 0 && load_game())
        {
            return;
        }
    }
    new_game();
}

}
