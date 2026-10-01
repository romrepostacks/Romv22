#include "pr_ui.h"

#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sprite_palette_item.h"
#include "bn_sprite_palettes.h"

#include "common_variable_8x16_sprite_font.h"
#include "bn_regular_bg_items_ui_menu.h"
#include "bn_regular_bg_items_ui_menu_tall.h"
#include "bn_regular_bg_items_ui_textbox.h"

namespace pr
{

namespace
{
    // The font draws glyphs in colour 14 and their outline in colour 12: dark text with a light
    // shadow, like the handheld text boxes.
    constexpr bn::color text_colors[] = {
        bn::color(31, 0, 31), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0),
        bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0),
        bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0),
        bn::color(26, 26, 25), bn::color(0, 0, 0), bn::color(8, 8, 9), bn::color(0, 0, 0)
    };
    constexpr bn::sprite_palette_item text_palette(text_colors, bn::bpp_mode::BPP_4);

    constexpr int text_left = 11;          // screen pixels
    constexpr int text_top = 119;
    constexpr int line_height = 16;
    constexpr int text_width = 218;
    constexpr int max_wrapped_lines = 16;
}

ui::ui() :
    _generator(common::variable_8x16_sprite_font, text_palette)
{
    _generator.set_left_alignment();
    _generator.set_bg_priority(0);
}

void ui::set_keep_box(bool keep)
{
    _keep_box = keep;
    if(keep)
    {
        _open_box();
    }
    else
    {
        _close_box();
    }
}

void ui::_open_box()
{
    if(! _box)
    {
        _box = bn::regular_bg_items::ui_textbox.create_bg(8, 48);
        _box->set_priority(0);
    }
}

void ui::_close_box()
{
    _text_sprites.clear();
    if(! _keep_box)
    {
        _box.reset();
    }
}

int ui::_wrap(const bn::string_view& text, bn::string_view* lines, int max_lines) const
{
    int count = 0;
    const char* data = text.data();
    int size = text.size();
    int pos = 0;
    while(pos < size && count < max_lines)
    {
        // Take words while they fit; '\n' always breaks.
        int line_start = pos;
        int best_end = -1;
        int scan = pos;
        while(scan <= size)
        {
            bool at_break = scan == size || data[scan] == ' ' || data[scan] == '\n';
            if(at_break)
            {
                bn::string_view candidate(data + line_start, scan - line_start);
                if(_generator.width(candidate) > text_width && best_end >= 0)
                {
                    break;
                }
                best_end = scan;
                if(scan == size || data[scan] == '\n')
                {
                    break;
                }
            }
            ++scan;
        }
        if(best_end < 0)
        {
            best_end = size;
        }
        lines[count++] = bn::string_view(data + line_start, best_end - line_start);
        pos = best_end;
        if(pos < size && (data[pos] == ' ' || data[pos] == '\n'))
        {
            ++pos;
        }
    }
    return count;
}

void ui::_draw_lines(const bn::string_view* lines, int count)
{
    _text_sprites.clear();
    for(int i = 0; i < count; ++i)
    {
        _generator.generate_top_left(text_left, text_top + i * line_height, lines[i], _text_sprites);
    }
}

void ui::say(const bn::string_view& text)
{
    bn::string_view lines[max_wrapped_lines];
    int count = _wrap(text, lines, max_wrapped_lines);
    _open_box();
    for(int page = 0; page < count; page += 2)
    {
        _draw_lines(lines + page, bn::min(2, count - page));
        // A short pause so a held button doesn't skip pages.
        wait(6);
        while(! bn::keypad::a_pressed() && ! bn::keypad::b_pressed())
        {
            bn::core::update();
        }
    }
    _close_box();
}

void ui::show_text(const bn::string_view& text)
{
    bn::string_view lines[2];
    int count = _wrap(text, lines, 2);
    _open_box();
    _draw_lines(lines, count);
}

void ui::clear_text()
{
    _close_box();
}

int ui::menu(const bn::string_view* options, int count, bool tall, int start, bool cancel,
             const bn::string_view* hints)
{
    // Boxes match the art in build_assets.py: tall = (136,2)-(237,113), short = (152,50)-(237,113).
    int left = tall ? 152 : 166;
    int top = tall ? 9 : 57;
    _menu_box = (tall ? bn::regular_bg_items::ui_menu_tall : bn::regular_bg_items::ui_menu).create_bg(8, 48);
    _menu_box->set_priority(0);

    bn::vector<bn::sprite_ptr, 32> option_sprites;
    for(int i = 0; i < count; ++i)
    {
        _generator.generate_top_left(left, top + i * line_height, options[i], option_sprites);
    }
    bn::vector<bn::sprite_ptr, 2> cursor;
    int index = bn::clamp(start, 0, count - 1);
    int shown_hint = -1;
    int cursor_index = -1;
    int result = -1;
    wait(4);

    while(true)
    {
        if(cursor_index != index)
        {
            cursor_index = index;
            cursor.clear();
            _generator.generate_top_left(left - 9, top + index * line_height, ">", cursor);
        }
        if(hints && shown_hint != index)
        {
            show_text(hints[index]);
            shown_hint = index;
        }
        bn::core::update();

        if(bn::keypad::up_pressed())
        {
            index = (index + count - 1) % count;
        }
        else if(bn::keypad::down_pressed())
        {
            index = (index + 1) % count;
        }
        else if(bn::keypad::a_pressed())
        {
            result = index;
            break;
        }
        else if(cancel && bn::keypad::b_pressed())
        {
            break;
        }
    }
    if(hints)
    {
        clear_text();
    }
    _menu_box.reset();
    return result;
}

void ui::wait(int frames)
{
    for(int i = 0; i < frames; ++i)
    {
        bn::core::update();
    }
}

void ui::set_faded(bool faded)
{
    bn::fixed intensity = faded ? 1 : 0;
    bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);
    bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
}

void ui::fade_out(int frames)
{
    for(int i = 1; i <= frames; ++i)
    {
        bn::fixed intensity = bn::fixed(i) / frames;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::core::update();
    }
}

void ui::fade_in(int frames)
{
    for(int i = frames - 1; i >= 0; --i)
    {
        bn::fixed intensity = bn::fixed(i) / frames;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::core::update();
    }
}

}
