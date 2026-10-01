// Windows, text and menus in Emerald's layout, styled after the web game (css: .gba-box, .bdark, .ow-text).
// Windows are tiles drawn on one always-on BG layer; text and cursors are sprites. Calls that wait for the
// player run frames (pr::frame) until they answer, which keeps the screen code linear.
#ifndef PR_UI_H
#define PR_UI_H

#include "bn_optional.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string_view.h"
#include "bn_vector.h"
#include "pr_ui_data.h"

namespace pr
{

using window_style = ui_data::style;

// Runs one frame: commits window changes, animates the UI, waits for the next VBlank.
void frame();
void wait(int frames);

enum class text_color
{
    INK,        // dark text with a light shadow (windows)
    WHITE,      // white text with a dark shadow (battle messages, dark screens)
    HUD         // dark text on the cream HP boxes
};

// The window layer: a 32x32 tile map over everything; boxes are drawn in 8x8 tiles.
class windows
{

public:
    windows();

    void box(window_style style, int tx, int ty, int tw, int th);
    void clear(int tx, int ty, int tw, int th);
    void clear_all();
    void commit();

private:
    bn::regular_bg_ptr _bg;
    bn::regular_bg_map_ptr _map;
    bool _dirty = false;
};

// A menu: options in a window, in one or more columns, with the ▶ cursor.
struct menu_spec
{
    const bn::string_view* options = nullptr;
    int count = 0;
    int tx = 0, ty = 0, tw = 0, th = 0;     // window, in tiles
    int columns = 1;
    int column_width = 0;                   // pixels between columns
    int start = 0;
    bool cancel = true;                     // B returns -1
    window_style style = window_style::WINDOW;
    text_color color = text_color::INK;
    // Called when the cursor moves (to show a description); ctx is passed back.
    void (*on_move)(void* ctx, int index) = nullptr;
    void* ctx = nullptr;
    bool keep_window = false;               // leave the window drawn afterwards
};

class ui
{

public:
    ui();

    [[nodiscard]] bn::sprite_text_generator& text()
    {
        return _generator;
    }
    [[nodiscard]] bn::sprite_text_generator& small_text()
    {
        return _small_generator;
    }
    [[nodiscard]] windows& win()
    {
        return _windows;
    }

    // Battle messages use the dark box and white text; the overworld uses the white window.
    void set_battle_style(bool battle);

    // Prints text into the message box letter by letter (A or B finishes the line, then turns the page),
    // two lines per page, word-wrapped.
    void say(const bn::string_view& text);
    // Prints it all at once and moves on after `frames` without input (SAVING..., the nurse).
    void say_timed(const bn::string_view& text, int frames);
    // Shows text without waiting, until clear_text() (a prompt above a menu). width_px limits the lines.
    void show_text(const bn::string_view& text, int width_px = 0);
    void clear_text();

    int menu(const menu_spec& spec);
    // Emerald's YES/NO box, above the message box on the right. Returns true for YES.
    bool yes_no(bool default_yes = true);
    // A one-column list in a box at the right, sized to fit, standing on the message box.
    int list(const bn::string_view* options, int count, int start = 0, bool cancel = true,
             void (*on_move)(void*, int) = nullptr, void* ctx = nullptr);

    // Text in a colour; sprites go into out (top-left at screen x, y).
    void print(int x, int y, const bn::string_view& text, text_color color, bn::ivector<bn::sprite_ptr>& out,
               bool small = false);
    [[nodiscard]] int width(const bn::string_view& text, bool small = false);

    void tick();       // per-frame animation (called by pr::frame)

    // Something to animate every frame while the UI waits (the battle's active Pokémon bobbing).
    void set_frame_hook(void (*hook)(void*), void* ctx)
    {
        _hook = hook;
        _hook_ctx = ctx;
    }

    static void fade_out(int frames = 16);
    static void fade_in(int frames = 16);
    static void set_faded(bool faded);

private:
    bn::sprite_text_generator _generator;
    bn::sprite_text_generator _small_generator;
    windows _windows;
    bn::vector<bn::sprite_ptr, 40> _message;
    bn::optional<bn::sprite_ptr> _arrow;
    bool _battle = false;
    bool _box_open = false;
    int _arrow_frame = 0;
    void (*_hook)(void*) = nullptr;
    void* _hook_ctx = nullptr;

    void _open_box();
    void _close_box();
    int _wrap(const bn::string_view& text, bn::string_view* lines, int max_lines, int width) const;
    void _draw_lines(const bn::string_view* lines, int count, int last_chars = -1);
};

ui& gui();

// Screen helpers.
constexpr int sx(int x)
{
    return x - 120;
}
constexpr int sy(int y)
{
    return y - 80;
}

}

#endif
