// Text box, menus and fades, shared by every screen. Calls block (running bn::core::update) until the
// player answers, which keeps the screen code linear.
#ifndef PR_UI_H
#define PR_UI_H

#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string_view.h"
#include "bn_vector.h"

namespace pr
{

class ui
{

public:
    ui();

    [[nodiscard]] bn::sprite_text_generator& text_generator()
    {
        return _generator;
    }

    // Keep the message box up between messages (battles); otherwise it closes after each say().
    void set_keep_box(bool keep);

    // Shows text in the message box, word-wrapped, two lines per page; A or B turns the page.
    void say(const bn::string_view& text);

    // Shows text without waiting (a prompt above a menu). clear_text() removes it.
    void show_text(const bn::string_view& text);
    void clear_text();

    // A list in a box on the right. Returns the chosen index, or -1 for B when cancel is allowed.
    // hints (optional, one per option) appear in the message box as the cursor moves.
    int menu(const bn::string_view* options, int count, bool tall, int start = 0, bool cancel = true,
             const bn::string_view* hints = nullptr);

    // Waits n frames.
    static void wait(int frames);

    static void fade_out(int frames = 16);
    static void fade_in(int frames = 16);
    static void set_faded(bool faded);

private:
    bn::sprite_text_generator _generator;
    bn::optional<bn::regular_bg_ptr> _box;
    bn::optional<bn::regular_bg_ptr> _menu_box;
    bn::vector<bn::sprite_ptr, 48> _text_sprites;
    bool _keep_box = false;

    void _open_box();
    void _close_box();
    int _wrap(const bn::string_view& text, bn::string_view* lines, int max_lines) const;
    void _draw_lines(const bn::string_view* lines, int count);
};

}

#endif
