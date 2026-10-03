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
#include "pr_narrow_font.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_next_arrow.h"
#include "bn_sprite_items_plank.h"

#include "pr_audio.h"
#include "pr_state.h"

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
    constexpr bn::color plank_colors[] = {
        bn::color(31, 0, 31), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0xe8, 0xb0, 0x70), make(0, 0, 0), make(0x40, 0x20, 0x10), make(0, 0, 0)
    };
    constexpr bn::color red_colors[] = {
        bn::color(31, 0, 31), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0xf8, 0xc8, 0xc0), make(0, 0, 0), make(0xe0, 0x38, 0x30), make(0, 0, 0)
    };
    constexpr bn::color blue_colors[] = {
        bn::color(31, 0, 31), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0xc0, 0xd8, 0xf0), make(0, 0, 0), make(0x30, 0x58, 0xb0), make(0, 0, 0)
    };
    constexpr bn::color yellow_colors[] = {
        bn::color(31, 0, 31), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0xf0, 0xe0, 0xa0), make(0, 0, 0), make(0xc8, 0x90, 0x00), make(0, 0, 0)
    };
    constexpr bn::color gray_colors[] = {
        bn::color(31, 0, 31), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0), make(0, 0, 0),
        make(0xe8, 0xe8, 0xe8), make(0, 0, 0), make(0xa0, 0xa0, 0xa8), make(0, 0, 0)
    };
    constexpr bn::sprite_palette_item yellow_palette(yellow_colors, bn::bpp_mode::BPP_4);
    constexpr bn::sprite_palette_item gray_palette(gray_colors, bn::bpp_mode::BPP_4);
    constexpr bn::sprite_palette_item ink_palette(ink_colors, bn::bpp_mode::BPP_4);
    constexpr bn::sprite_palette_item white_palette(white_colors, bn::bpp_mode::BPP_4);
    constexpr bn::sprite_palette_item hud_palette(hud_colors, bn::bpp_mode::BPP_4);
    constexpr bn::sprite_palette_item plank_palette(plank_colors, bn::bpp_mode::BPP_4);
    constexpr bn::sprite_palette_item red_palette(red_colors, bn::bpp_mode::BPP_4);
    constexpr bn::sprite_palette_item blue_palette(blue_colors, bn::bpp_mode::BPP_4);

    const bn::sprite_palette_item& palette_of(text_color c)
    {
        switch(c)
        {
        case text_color::WHITE: return white_palette;
        case text_color::HUD: return hud_palette;
        case text_color::PLANK: return plank_palette;
        case text_color::RED: return red_palette;
        case text_color::BLUE: return blue_palette;
        case text_color::YELLOW: return yellow_palette;
        case text_color::GRAY: return gray_palette;
        default: return ink_palette;
        }
    }

    // Message box: the whole bottom of the screen (Emerald's field and battle windows).
    constexpr int box_tx = 0, box_ty = 14, box_tw = 30, box_th = 6;
    constexpr int text_left = 12;
    constexpr int text_top = 117;
    constexpr int line_height = 16;
    constexpr int text_width = 212;
    constexpr int max_wrapped_lines = 16;

    // OPTION > TEXT SPEED (TEXT_FRAMES): frames per letter, SLOW / MID / FAST.
    int text_frames()
    {
        constexpr int frames[] = { 9, 5, 2 };
        return frames[int(state().opt.speed)];
    }

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
    audio::tick();
    if(game_active())
    {
        ++state().play_frames;
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
    _small_generator(common::variable_8x8_sprite_font, hud_palette),
    _narrow_generator(narrow_sprite_font, hud_palette)
{
    instance = this;
    _generator.set_left_alignment();
    _generator.set_bg_priority(0);
    _small_generator.set_left_alignment();
    _small_generator.set_bg_priority(0);
    _narrow_generator.set_left_alignment();
    _narrow_generator.set_bg_priority(0);
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
    if(_plank_timer > 0)
    {
        // Down over 18 frames, a couple of seconds' rest, back up over 18 (.ow-popup: .3 s each way).
        --_plank_timer;
        int shown = 132 - _plank_timer;
        int drop = shown < 18 ? shown : _plank_timer < 18 ? _plank_timer : 18;
        _place_plank(-26 + drop * 26 / 18);
        if(! _plank_timer)
        {
            _plank.clear();
            _plank_text.clear();
        }
    }
}

void ui::show_place(const bn::string_view& text)
{
    _plank.clear();
    _plank_text.clear();
    // The plank is at most 224 px (192 px of text): a longer name drops to the small font.
    bool small = width(text) > 192;
    int w = bn::max(64, ((width(text, small) + 32 + 31) / 32) * 32);
    for(int x = 0; x < w; x += 32)
    {
        int piece = x == 0 ? 0 : x + 32 >= w ? 2 : 1;
        bn::sprite_ptr p = bn::sprite_items::plank.create_sprite(0, 0, piece);
        p.set_bg_priority(0);
        p.set_z_order(10);
        _plank.push_back(p);
    }
    print(0, small ? 3 : 0, text, text_color::PLANK, _plank_text, small);
    _plank_base.clear();
    for(bn::sprite_ptr& t : _plank_text)
    {
        t.set_z_order(0);
        _plank_base.push_back(t.position());
    }
    _plank_timer = 132;
    _place_plank(-26);
}

void ui::_place_plank(int y)
{
    for(int i = 0; i < _plank.size(); ++i)
    {
        _plank[i].set_position(sx(8 + i * 32 + 16), sy(y + 16));
    }
    // The text was made at the screen's top left; it rides 16 px in and 5 px down on the plank.
    for(int i = 0; i < _plank_text.size(); ++i)
    {
        _plank_text[i].set_position(_plank_base[i].x() + 8 + 16, _plank_base[i].y() + y + 5);
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

int ui::narrow_width(const bn::string_view& text)
{
    return _narrow_generator.width(text);
}

int ui::fit_width(const bn::string_view& text, int max_width, bool small)
{
    if(! small && width(text) <= max_width)
    {
        return width(text);
    }
    if(width(text, true) <= max_width)
    {
        return width(text, true);
    }
    return bn::min(narrow_width(text), max_width);
}

namespace
{
    // Word wrap with any width measure; returns the lines used (stops at max_lines, the rest in the last).
    template<typename Measure>
    int wrap_by(const bn::string_view& text, int width, Measure measure, bn::string_view* lines, int max_lines,
                bool& all_fit)
    {
        const char* data = text.data();
        int size = text.size();
        int start = 0, count = 0;
        all_fit = true;
        while(start < size && count < max_lines)
        {
            while(start < size && data[start] == ' ')
            {
                ++start;
            }
            if(count == max_lines - 1)
            {
                lines[count++] = bn::string_view(data + start, size - start);
                all_fit = measure(lines[count - 1]) <= width;
                break;
            }
            int end = start, last_space = -1;
            while(end < size)
            {
                int next = end;
                while(next < size && data[next] != ' ')
                {
                    ++next;
                }
                if(measure(bn::string_view(data + start, next - start)) > width && last_space > start)
                {
                    break;
                }
                last_space = next;
                end = next;
                if(end < size)
                {
                    ++end;
                }
            }
            int stop = last_space > start ? last_space : end;
            if(end >= size && measure(bn::string_view(data + start, size - start)) <= width)
            {
                stop = size;
            }
            lines[count++] = bn::string_view(data + start, stop - start);
            if(measure(lines[count - 1]) > width)
            {
                all_fit = false;
            }
            start = stop;
        }
        return count;
    }
}

int ui::wrap_lines(const bn::string_view& text, int width, bool small, bn::string_view* lines, int max_lines)
{
    bool all_fit;
    return wrap_by(text, width, [&](const bn::string_view& t){ return this->width(t, small); }, lines, max_lines, all_fit);
}

void ui::print_wrapped_fit(int x, int y, int width, const bn::string_view& text, int max_lines, int line_height,
                           text_color color, bn::ivector<bn::sprite_ptr>& out, bool small)
{
    bn::string_view lines[8];
    max_lines = bn::min(max_lines, 8);
    bool all_fit;
    int n = wrap_by(text, width, [&](const bn::string_view& t){ return this->width(t, small); }, lines, max_lines, all_fit);
    if(all_fit)
    {
        for(int i = 0; i < n; ++i)
        {
            print(x, y + i * line_height, lines[i], color, out, small);
        }
        return;
    }
    // The condensed font, each line fitted (the last one squeezed if it must).
    n = wrap_by(text, width, [&](const bn::string_view& t){ return narrow_width(t); }, lines, max_lines, all_fit);
    for(int i = 0; i < n; ++i)
    {
        print_fit(x, y + i * line_height, lines[i], width, color, out, true);
    }
}

int ui::print_fit_slide(int x, int min_x, int right, int y, const bn::string_view& text, text_color color,
                        bn::ivector<bn::sprite_ptr>& out, bool small)
{
    if(narrow_width(text) > right - x)
    {
        x = bn::max(min_x, right - narrow_width(text));
    }
    return print_fit(x, y, text, right - x, color, out, small);
}

int ui::print_fit(int x, int y, const bn::string_view& text, int max_width, text_color color,
                  bn::ivector<bn::sprite_ptr>& out, bool small)
{
    if(! small && width(text) <= max_width)
    {
        print(x, y, text, color, out);
        return width(text);
    }
    if(width(text, true) <= max_width)
    {
        print(x, y + (small ? 0 : 3), text, color, out, true);
        return width(text, true);
    }
    bn::sprite_text_generator& gen = _narrow_generator;
    gen.set_palette_item(palette_of(color));
    int first = out.size();
    gen.generate_top_left(x, y + (small ? 0 : 4), text, out);
    int w = gen.width(text);
    if(w > max_width && max_width > 0)
    {
        // Still too wide (a 12-letter nickname of Ms): squeezed to fit, in sixteenths.
        int k16 = bn::clamp(max_width * 16 / w, 8, 15);
        bn::optional<bn::sprite_affine_mat_ptr>& mat = _squeeze[k16 - 8];
        if(! mat)
        {
            mat = bn::sprite_affine_mat_ptr::create();
            mat->set_horizontal_scale(bn::fixed(k16) / 16);
        }
        bn::fixed left = sx(x);
        for(int i = first; i < out.size(); ++i)
        {
            bn::sprite_ptr& sp = out[i];
            sp.set_affine_mat(*mat);
            sp.set_x(left + (sp.x() - left) * k16 / 16);
        }
        w = w * k16 / 16;
    }
    return w;
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

void ui::say(const bn::string_view& text, void (*after_typed)(void*), void* ctx)
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
            if(++timer >= text_frames())
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
        if(after_typed && page + 2 >= count)
        {
            after_typed(ctx);
        }
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
    bn::string_view lines[max_wrapped_lines];
    int count = _wrap(text, lines, max_wrapped_lines, width_px ? width_px : text_width);
    _open_box();
    // A question longer than the box: the earlier pages wait for A, the last one stays up for the answer.
    int last = ((count - 1) / 2) * 2;
    for(int page = 0; page < last; page += 2)
    {
        _draw_lines(lines + page, 2);
        _arrow = bn::sprite_items::next_arrow.create_sprite(sx(222 + 4), sy(147 + 4));
        _arrow->set_bg_priority(0);
        frame();
        while(! bn::keypad::a_pressed() && ! bn::keypad::b_pressed())
        {
            frame();
        }
        _arrow.reset();
        frame();
    }
    _draw_lines(lines + last, count - last);
}

void ui::clear_text()
{
    _close_box();
}

int ui::menu(const menu_spec& s)
{
    _windows.box(s.style, s.tx, s.ty, s.tw, s.th);
    bn::vector<bn::sprite_ptr, 64> texts;
    int rows = s.rows > 0 && s.columns == 1 ? bn::min(s.rows, s.count) : s.count;
    int top = 0;
    auto option_x = [&](int i){ return s.tx * 8 + 16 + (i % s.columns) * s.column_width; };
    auto option_y = [&](int i){ return s.ty * 8 + 8 + (i / s.columns) * line_height; };
    bn::optional<bn::sprite_ptr> more_up, more_down;
    auto draw = [&]()
    {
        texts.clear();
        for(int k = 0; k < (s.columns == 1 ? rows : s.count); ++k)
        {
            int i = s.columns == 1 ? top + k : k;
            if(i < s.count)
            {
                // Too long for its column (FIRST IMPRESSION): a smaller font, never cut short.
                // The last column runs to the window's edge; the others stop short of the next one's cursor.
                bool last_column = s.columns == 1 || k % s.columns == s.columns - 1;
                int room = last_column ? (s.tx + s.tw) * 8 - 4 - option_x(k) - (rows < s.count ? 10 : 0)
                                       : s.column_width - 9;
                print_fit(option_x(k), option_y(k), s.options[i], room, s.colors ? s.colors[i] : s.color, texts);
            }
        }
        // Scroll marks when there's more above or below.
        if(rows < s.count)
        {
            if(top > 0)
            {
                print(s.tx * 8 + s.tw * 8 - 14, s.ty * 8 + 2, "^", s.color, texts, true);
            }
            if(top + rows < s.count)
            {
                print(s.tx * 8 + s.tw * 8 - 14, s.ty * 8 + s.th * 8 - 10, "v", s.color, texts, true);
            }
        }
    };
    bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(0, 0);
    cursor.set_bg_priority(0);
    int index = bn::clamp(s.start, 0, s.count - 1);
    if(index >= rows)
    {
        top = index - rows + 1;
    }
    draw();
    int shown = -1;
    int result = -1;
    frame();
    auto click = [&]()
    {
        if(! s.silent)
        {
            audio::play(audio::sfx::SELECT);
        }
    };
    while(true)
    {
        if(shown != index)
        {
            shown = index;
            int k = s.columns == 1 ? index - top : index;
            cursor.set_position(sx(option_x(k) - 10 + 4), sy(option_y(k) + 4 + 4));
            if(s.on_move)
            {
                s.on_move(s.ctx, index);
            }
        }
        frame();
        int col = index % s.columns;
        int before = index;
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
            click();
            result = index;
            break;
        }
        else if(s.cancel && bn::keypad::b_pressed())
        {
            click();
            break;
        }
        if(index != before)
        {
            click();
            if(s.columns == 1 && rows < s.count)
            {
                if(index < top)
                {
                    top = index;
                    draw();
                }
                else if(index >= top + rows)
                {
                    top = index - rows + 1;
                    draw();
                }
            }
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

int ui::_list_at(const bn::string_view* options, int count, int start, bool cancel, void (*on_move)(void*, int), void* ctx,
                 bool top_left)
{
    int widest = 0;
    for(int i = 0; i < count; ++i)
    {
        widest = bn::max(widest, width(options[i]));
    }
    menu_spec s;
    s.options = options;
    s.count = count;
    s.rows = 6;
    s.tw = bn::min(30, (widest + 16 + 12 + 7) / 8 + (count > 6 ? 1 : 0));
    s.th = bn::min(count, 6) * 2 + 2;
    s.tx = top_left ? 0 : 30 - s.tw;
    s.ty = top_left ? 0 : bn::max(0, 14 - s.th);
    s.start = start;
    s.cancel = cancel;
    s.on_move = on_move;
    s.ctx = ctx;
    return menu(s);
}

int ui::list(const bn::string_view* options, int count, int start, bool cancel, void (*on_move)(void*, int), void* ctx)
{
    return _list_at(options, count, start, cancel, on_move, ctx, false);
}

int ui::list_top_left(const bn::string_view* options, int count, int start, bool cancel, void (*on_move)(void*, int), void* ctx)
{
    return _list_at(options, count, start, cancel, on_move, ctx, true);
}

// The naming screen (Emerald's, simplified): the name on top, a letter board below. A types, B deletes,
// SELECT switches the board, START jumps to OK.
bool ui::keyboard(const bn::string_view& title, char* out, int max_length, const bn::string_view& initial)
{
    constexpr const char* pages[3][3] = {
        { "ABCDEFGHI", "JKLMNOPQR", "STUVWXYZ " },
        { "abcdefghi", "jklmnopqr", "stuvwxyz " },
        { "012345678", "9.,!?-'&/", ":;()     " }
    };
    constexpr const char* page_names[] = { "UPPER", "lower", "OTHER" };
    constexpr int columns = 9, rows = 3;
    int page = 0, cx = 0, cy = 0;      // cy == rows: the bottom row (CASE / DEL / OK)
    bn::string<32> name(initial);
    if(name.size() > max_length)
    {
        name.shrink(max_length);
    }
    bn::vector<bn::sprite_ptr, 48> board;
    bn::vector<bn::sprite_ptr, 16> field;
    bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(0, 0);
    cursor.set_bg_priority(0);
    _windows.clear_all();
    auto draw_board = [&]()
    {
        board.clear();
        _windows.box(window_style::WINDOW, 1, 1, 28, 5);
        print(16, 12, title, text_color::INK, board);
        _windows.box(window_style::WINDOW, 1, 7, 28, 12);
        for(int r = 0; r < rows; ++r)
        {
            for(int c = 0; c < columns; ++c)
            {
                char ch[2] = { pages[page][r][c], 0 };
                if(ch[0] && ch[0] != ' ')
                {
                    print(32 + c * 22, 68 + r * 18, ch, text_color::INK, board);
                }
            }
        }
        print(32, 128, page_names[(page + 1) % 3], text_color::INK, board);
        print(108, 128, "DEL", text_color::INK, board);
        print(172, 128, "OK", text_color::INK, board);
    };
    auto draw_field = [&]()
    {
        field.clear();
        bn::string<40> shown(name);
        for(int i = name.size(); i < max_length; ++i)
        {
            shown.append("_");
        }
        print(16, 30, shown, text_color::INK, field);
    };
    draw_board();
    draw_field();
    ui::fade_in(12);
    bool ok = true;
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
            page = (page + 1) % 3;
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
            else if(initial.empty())
            {
                continue;
            }
            else
            {
                ok = false;
                break;
            }
        }
        else if(bn::keypad::a_pressed())
        {
            audio::play(audio::sfx::SELECT);
            if(cy == rows)
            {
                if(cx == 0)
                {
                    page = (page + 1) % 3;
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
                if(ch && (ch != ' ' || ! name.empty()) && name.size() < max_length)
                {
                    name.push_back(ch);
                    draw_field();
                    if(name.size() == max_length)
                    {
                        cy = rows;
                        cx = 2;
                    }
                }
            }
        }
    }
    // Trailing spaces go (trim()).
    while(! name.empty() && name.back() == ' ')
    {
        name.pop_back();
    }
    for(int i = 0; i <= name.size(); ++i)
    {
        out[i] = i < name.size() ? name[i] : 0;
    }
    ui::fade_out(12);
    board.clear();
    field.clear();
    _windows.clear_all();
    return ok;
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
