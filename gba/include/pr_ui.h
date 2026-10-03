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
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_string_view.h"
#include "bn_vector.h"
#include "bn_fixed_point.h"
#include "pr_ui_data.h"

namespace pr
{

using window_style = ui_data::style;

// Runs one frame: commits window changes, animates the UI, waits for the next VBlank.
void frame();
// LEFT, RIGHT, LEFT, RIGHT, B, A were the last buttons pressed (START completes the cheat code). Forgets them.
bool cheat_code_entered();
void wait(int frames);

enum class text_color
{
    INK,        // dark text with a light shadow (windows)
    WHITE,      // white text with a dark shadow (battle messages, dark screens)
    HUD,        // dark text on the cream HP boxes
    PLANK,      // dark brown on the area name plank
    RED,        // OPTION's chosen value; a super effective move
    BLUE,       // headings on light pages
    YELLOW,     // a move that hits for normal damage
    GRAY        // a move that would have no effect
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
    const text_color* colors = nullptr;     // each option's own colour (instead of color)
    // Called when the cursor moves (to show a description); ctx is passed back.
    void (*on_move)(void* ctx, int index) = nullptr;
    void* ctx = nullptr;
    bool keep_window = false;               // leave the window drawn afterwards
    int rows = 0;                           // visible rows (one column); more options scroll. 0: all
    bool silent = false;                    // no select sound (OPTION)
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
    // The message box, letter by letter, then A. after_typed (if given) runs once the text is all out, before
    // the wait for A (a battle's move animation plays then, as the web game's message box does).
    void say(const bn::string_view& text, void (*after_typed)(void*) = nullptr, void* ctx = nullptr);
    // Prints it all at once and moves on after `frames` without input (SAVING..., the nurse).
    void say_timed(const bn::string_view& text, int frames);
    // Shows text without waiting, until clear_text() (a prompt above a menu). width_px limits the lines.
    void show_text(const bn::string_view& text, int width_px = 0);
    void clear_text();

    int menu(const menu_spec& spec);
    // Emerald's YES/NO box, above the message box on the right. Returns true for YES.
    bool yes_no(bool default_yes = true);
    // A one-column list in a box at the right, sized to fit, standing on the message box (at most six
    // rows show at once; the rest scroll).
    int list(const bn::string_view* options, int count, int start = 0, bool cancel = true,
             void (*on_move)(void*, int) = nullptr, void* ctx = nullptr);
    // The same list in the top left corner (gm-topleft: the PC's menus).
    int list_top_left(const bn::string_view* options, int count, int start = 0, bool cancel = true,
                      void (*on_move)(void*, int) = nullptr, void* ctx = nullptr);

    // The naming screen (names, nicknames, box names): returns false if cancelled with nothing typed.
    bool keyboard(const bn::string_view& title, char* out, int max_length, const bn::string_view& initial = "");

    // Text in a colour; sprites go into out (top-left at screen x, y).
    void print(int x, int y, const bn::string_view& text, text_color color, bn::ivector<bn::sprite_ptr>& out,
               bool small = false);
    [[nodiscard]] int width(const bn::string_view& text, bool small = false);
    // Names are never cut short: print_fit() uses the normal font (or the small one, if small), then smaller
    // fonts down to the condensed one, until the text fits in max_width. Returns the width used.
    int print_fit(int x, int y, const bn::string_view& text, int max_width, text_color color,
                  bn::ivector<bn::sprite_ptr>& out, bool small = false);
    [[nodiscard]] int fit_width(const bn::string_view& text, int max_width, bool small = false);
    // print_fit() from x to right, except that a name too long even condensed slides left (down to min_x,
    // over an icon) rather than being squeezed.
    int print_fit_slide(int x, int min_x, int right, int y, const bn::string_view& text, text_color color,
                        bn::ivector<bn::sprite_ptr>& out, bool small = false);
    [[nodiscard]] int narrow_width(const bn::string_view& text);
    // Word-wrapped text in up to max_lines lines of `width` px. If it doesn't all fit, the condensed font is
    // tried, and the last line is squeezed rather than anything being left out.
    void print_wrapped_fit(int x, int y, int width, const bn::string_view& text, int max_lines, int line_height,
                           text_color color, bn::ivector<bn::sprite_ptr>& out, bool small = false);
    // Splits text into lines of at most `width` px by words (small or normal font). Returns the line count.
    int wrap_lines(const bn::string_view& text, int width, bool small, bn::string_view* lines, int max_lines);

    void tick();       // per-frame animation (called by pr::frame)

    // Emerald's area name on a wooden plank, dropping in from the top left for a couple of seconds.
    void show_place(const bn::string_view& text);

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
    bn::sprite_text_generator _narrow_generator;
    bn::optional<bn::sprite_affine_mat_ptr> _squeeze[8];     // print_fit's last resort: 8/16 .. 15/16 wide
    windows _windows;
    bn::vector<bn::sprite_ptr, 40> _message;
    bn::optional<bn::sprite_ptr> _arrow;
    bn::vector<bn::sprite_ptr, 12> _plank;
    bn::vector<bn::sprite_ptr, 12> _plank_text;
    bn::vector<bn::fixed_point, 12> _plank_base;
    int _plank_timer = 0;
    bool _battle = false;
    bool _box_open = false;
    int _arrow_frame = 0;
    void (*_hook)(void*) = nullptr;
    void* _hook_ctx = nullptr;

    void _open_box();
    void _close_box();
    int _wrap(const bn::string_view& text, bn::string_view* lines, int max_lines, int width) const;
    int _list_at(const bn::string_view* options, int count, int start, bool cancel, void (*on_move)(void*, int), void* ctx,
                 bool top_left);
    void _place_plank(int y);
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
