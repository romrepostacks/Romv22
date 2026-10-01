#include "pr_ui.h"

#include "bn_bg_palettes.h"
#include "bn_common.h"
#include "bn_bg_tiles.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_span.h"
#include "bn_sprite_palette_item.h"
#include "bn_sprite_palettes.h"

#include "common_variable_8x16_sprite_font.h"
#include "common_variable_8x8_sprite_font.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_next_arrow.h"

namespace pr
{

namespace
{
    // The fonts draw glyphs in colour 14 and their outline in colour 12 (the web game's text-shadow).
    constexpr bn::color make(int r, int g, int b)
    {
        return bn::color(r >> 3, g >> 3, b >> 3);
    }
    constexpr bn::color ink_colors[] = {
        bn::color(31, 0, 31), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0xd0, 0xd0, 0xc8), make(0, 0, 0), make(0x38, 0x38, 0x40), make(0, 0, 0)
    };
    constexpr bn::color white_colors[] = {
        bn::color(31, 0, 31), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0x10, 0x20, 0x30), make(0, 0, 0), make(0xf8, 0xf8, 0xf8), make(0, 0, 0)
    };
    constexpr bn::color hud_colors[] = {
        bn::color(31, 0, 31), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0xd8, 0xd8, 0xb8), make(0, 0, 0), make(0x38, 0x38, 0x40), make(0, 0, 0)
    };
    constexpr bn::sprite_palette_item ink_palette(ink_colors, bn::bpp_mode::BPP_4);
    constexpr bn::sprite_palette_item white_palette(white_colors, bn::bpp_mode::BPP_4);
    constexpr bn::sprite_palette_item hud_palette(hud_colors, bn::bpp_mode::BPP_4);

    const bn::sprite_palette_item& palette_of(text_color c)
    {
        return c == text_color::WHITE ? white_palette : c == text_color::HUD ? hud_palette : ink_palette;
    }

    // Message box: the whole bottom of the screen (Emerald's field and battle windows).
    constexpr int box_tx = 0, box_ty = 14, box_tw = 30, box_th = 6;
    constexpr int text_left = 12;
    constexpr int text_top = 117;
    constexpr int line_height = 16;
    constexpr int text_width = 212;
    constexpr int max_wrapped_lines = 16;
    constexpr int text_frames = 4;       // Emerald's MID text speed: a letter every 4 frames

    alignas(int) BN_DATA_EWRAM_BSS bn::regular_bg_map_cell window_cells[32 * 32];
    ui* instance = nullptr;

    int utf8_length(char lead)
    {
        auto c = uint8_t(lead);
        return c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
    }
}

// ---------------------------------------------------------------------------------------------------
windows::windows() :
    _bg([]{
        bn::bg_tiles::set_allow_offset(false);
        bn::regular_bg_tiles_item tiles_item(bn::span<const bn::tile>(ui_data::tiles), bn::bpp_mode::BPP_4);
        bn::bg_palette_item palette_item(bn::span<const bn::color>(ui_data::colors), bn::bpp_mode::BPP_4);
        bn::regular_bg_map_item map_item(window_cells[0], bn::size(32, 32));
        bn::regular_bg_ptr bg = bn::regular_bg_item(tiles_item, palette_item, map_item).create_bg(8, 48);
        bn::bg_tiles::set_allow_offset(true);
        return bg;
    }()),
    _map(_bg.map())
{
    _bg.set_priority(0);
}

void windows::box(window_style style, int tx, int ty, int tw, int th)
{
    int base = 1 + int(style) * 9;
    for(int y = 0; y < th; ++y)
    {
        int row = y == 0 ? 0 : y == th - 1 ? 2 : 1;
        for(int x = 0; x < tw; ++x)
        {
            int col = x == 0 ? 0 : x == tw - 1 ? 2 : 1;
            int cx = tx + x, cy = ty + y;
            if(cx >= 0 && cy >= 0 && cx < 32 && cy < 32)
            {
                window_cells[cy * 32 + cx] = bn::regular_bg_map_cell(base + row * 3 + col);
            }
        }
    }
    _dirty = true;
}

void windows::clear(int tx, int ty, int tw, int th)
{
    for(int y = ty; y < ty + th; ++y)
    {
        for(int x = tx; x < tx + tw; ++x)
        {
            if(x >= 0 && y >= 0 && x < 32 && y < 32)
            {
                window_cells[y * 32 + x] = 0;
            }
        }
    }
    _dirty = true;
}

void windows::clear_all()
{
    clear(0, 0, 32, 32);
}

void windows::commit()
{
    if(_dirty)
    {
        _map.reload_cells_ref();
        _dirty = false;
    }
}

// ---------------------------------------------------------------------------------------------------
void frame()
{
    if(instance)
    {
        instance->tick();
    }
    bn::core::update();
}

void wait(int frames)
{
    for(int i = 0; i < frames; ++i)
    {
        frame();
    }
}

ui& gui()
{
    return *instance;
}

ui::ui() :
    _generator(common::variable_8x16_sprite_font, ink_palette),
    _small_generator(common::variable_8x8_sprite_font, hud_palette)
{
    instance = this;
    _generator.set_left_alignment();
    _generator.set_bg_priority(0);
    _small_generator.set_left_alignment();
    _small_generator.set_bg_priority(0);
}

void ui::tick()
{
    if(_hook)
    {
        _hook(_hook_ctx);
    }
    _windows.commit();
    if(_arrow)
    {
        ++_arrow_frame;
        _arrow->set_tiles(bn::sprite_items::next_arrow.tiles_item(), (_arrow_frame / 18) % 2);
    }
}

void ui::set_battle_style(bool battle)
{
    _battle = battle;
    if(battle)
    {
        _open_box();
    }
    else
    {
        _close_box();
    }
}

int ui::width(const bn::string_view& text, bool small)
{
    return small ? _small_generator.width(text) : _generator.width(text);
}

void ui::print(int x, int y, const bn::string_view& text, text_color color, bn::ivector<bn::sprite_ptr>& out, bool small)
{
    bn::sprite_text_generator& gen = small ? _small_generator : _generator;
    gen.set_palette_item(palette_of(color));
    gen.generate_top_left(x, y, text, out);
}

void ui::_open_box()
{
    _windows.box(_battle ? window_style::DARK : window_style::WINDOW, box_tx, box_ty, box_tw, box_th);
    _box_open = true;
}

void ui::_close_box()
{
    _message.clear();
    _arrow.reset();
    if(! _battle)
    {
        _windows.clear(box_tx, box_ty, box_tw, box_th);
        _box_open = false;
    }
    else
    {
        _open_box();
    }
}

int ui::_wrap(const bn::string_view& text, bn::string_view* lines, int max_lines, int width) const
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
                if(_generator.width(candidate) > width && best_end >= 0)
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

void ui::_draw_lines(const bn::string_view* lines, int count, int last_chars)
{
    _message.clear();
    int budget = last_chars;
    for(int i = 0; i < count; ++i)
    {
        bn::string_view line = lines[i];
        if(budget >= 0)
        {
            if(budget < line.size())
            {
                line = bn::string_view(line.data(), budget);
            }
            budget = bn::max(0, budget - lines[i].size());
        }
        if(! line.empty())
        {
            print(text_left, text_top + i * line_height, line, _battle ? text_color::WHITE : text_color::INK, _message);
        }
    }
}

void ui::say(const bn::string_view& text)
{
    bn::string_view lines[max_wrapped_lines];
    int count = _wrap(text, lines, max_wrapped_lines, text_width);
    _open_box();
    for(int page = 0; page < count; page += 2)
    {
        int n = bn::min(2, count - page);
        int total = 0;
        for(int i = 0; i < n; ++i)
        {
            total += lines[page + i].size();
        }
        // Letter by letter; A or B shows the rest of the page at once.
        int shown = 0;
        int timer = 0;
        _draw_lines(lines + page, n, 0);
        frame();
        while(shown < total)
        {
            if(bn::keypad::a_pressed() || bn::keypad::b_pressed())
            {
                shown = total;
                break;
            }
            if(++timer >= text_frames)
            {
                timer = 0;
                // Step a whole character (é is two bytes), skipping spaces.
                int line_index = 0, offset = shown;
                while(line_index < n && offset >= lines[page + line_index].size())
                {
                    offset -= lines[page + line_index].size();
                    ++line_index;
                }
                if(line_index < n)
                {
                    shown += utf8_length(lines[page + line_index][offset]);
                }
                _draw_lines(lines + page, n, shown);
            }
            frame();
        }
        _draw_lines(lines + page, n);
        _arrow = bn::sprite_items::next_arrow.create_sprite(sx(222 + 4), sy(147 + 4));
        _arrow->set_bg_priority(0);
        frame();
        while(! bn::keypad::a_pressed() && ! bn::keypad::b_pressed())
        {
            frame();
        }
        _arrow.reset();
    }
    _close_box();
}

void ui::say_timed(const bn::string_view& text, int frames)
{
    show_text(text);
    wait(frames);
    clear_text();
}

void ui::show_text(const bn::string_view& text, int width_px)
{
    bn::string_view lines[2];
    int count = _wrap(text, lines, 2, width_px ? width_px : text_width);
    _open_box();
    _draw_lines(lines, count);
}

void ui::clear_text()
{
    _close_box();
}

int ui::menu(const menu_spec& s)
{
    _windows.box(s.style, s.tx, s.ty, s.tw, s.th);
    bn::vector<bn::sprite_ptr, 48> texts;
    auto option_x = [&](int i){ return s.tx * 8 + 16 + (i % s.columns) * s.column_width; };
    auto option_y = [&](int i){ return s.ty * 8 + 8 + (i / s.columns) * line_height; };
    for(int i = 0; i < s.count; ++i)
    {
        print(option_x(i), option_y(i), s.options[i], s.color, texts);
    }
    bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(0, 0);
    cursor.set_bg_priority(0);
    int index = bn::clamp(s.start, 0, s.count - 1);
    int shown = -1;
    int result = -1;
    frame();
    while(true)
    {
        if(shown != index)
        {
            shown = index;
            cursor.set_position(sx(option_x(index) - 10 + 4), sy(option_y(index) + 4 + 4));
            if(s.on_move)
            {
                s.on_move(s.ctx, index);
            }
        }
        frame();
        int col = index % s.columns;
        if(bn::keypad::up_pressed())
        {
            if(s.columns == 1)
            {
                index = (index + s.count - 1) % s.count;
            }
            else if(index - s.columns >= 0)
            {
                index -= s.columns;
            }
        }
        else if(bn::keypad::down_pressed())
        {
            if(s.columns == 1)
            {
                index = (index + 1) % s.count;
            }
            else if(index + s.columns < s.count)
            {
                index += s.columns;
            }
        }
        else if(bn::keypad::left_pressed() && col > 0)
        {
            --index;
        }
        else if(bn::keypad::right_pressed() && col < s.columns - 1 && index + 1 < s.count)
        {
            ++index;
        }
        else if(bn::keypad::a_pressed())
        {
            result = index;
            break;
        }
        else if(s.cancel && bn::keypad::b_pressed())
        {
            break;
        }
    }
    if(! s.keep_window)
    {
        _windows.clear(s.tx, s.ty, s.tw, s.th);
        if(_box_open)
        {
            _open_box();
        }
    }
    return result;
}

bool ui::yes_no(bool default_yes)
{
    constexpr bn::string_view options[] = { "YES", "NO" };
    menu_spec s;
    s.options = options;
    s.count = 2;
    s.tx = 23;
    s.ty = 8;
    s.tw = 7;
    s.th = 6;
    s.start = default_yes ? 0 : 1;
    return menu(s) == 0;
}

int ui::list(const bn::string_view* options, int count, int start, bool cancel, void (*on_move)(void*, int), void* ctx)
{
    int widest = 0;
    for(int i = 0; i < count; ++i)
    {
        widest = bn::max(widest, width(options[i]));
    }
    menu_spec s;
    s.options = options;
    s.count = count;
    s.tw = bn::min(30, (widest + 16 + 12 + 7) / 8);
    s.th = count * 2 + 2;
    s.tx = 30 - s.tw;
    s.ty = bn::max(0, 14 - s.th);
    s.start = start;
    s.cancel = cancel;
    s.on_move = on_move;
    s.ctx = ctx;
    return menu(s);
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
        frame();
    }
}

void ui::fade_in(int frames)
{
    for(int i = frames - 1; i >= 0; --i)
    {
        bn::fixed intensity = bn::fixed(i) / frames;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
        frame();
    }
}

}
