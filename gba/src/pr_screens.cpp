// The full-screen menus, after the web game's Emerald-style screens (partyOpen, summaryDraw, bagOpen,
// boxOpen, dexOpen, optionOpen, cardOpen, renderMap, playCredits), at half the web game's 480x320.
#include "pr_screens.h"

#include "bn_bg_palette_ptr.h"
#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_rect_window.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_string.h"
#include "bn_window.h"

#include "bn_regular_bg_items_bag_bg.h"
#include "bn_regular_bg_items_card_bg.h"
#include "bn_regular_bg_items_dex_bg.h"
#include "bn_regular_bg_items_dex_entry_bg.h"
#include "bn_regular_bg_items_party_bg.h"
#include "bn_regular_bg_items_pc_bg.h"
#include "bn_regular_bg_items_summary_bg.h"
#include "bn_regular_bg_items_wallpaper_bg.h"
#include "bn_sprite_items_badge.h"
#include "bn_sprite_items_bag.h"
#include "bn_sprite_items_cell.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_hand.h"
#include "bn_sprite_items_hpbar.h"
#include "bn_sprite_items_link.h"
#include "bn_sprite_items_mon_icon_egg.h"
#include "bn_sprite_items_person_player.h"

#include "pr_audio.h"
#include "pr_game_data.h"
#include "pr_icons.h"
#include "pr_state.h"
#include "pr_ui.h"
#include "pr_world_data.h"

namespace pr
{

namespace
{
    void upper(bn::istring& out, const char* text)
    {
        for(const char* c = text; *c; ++c)
        {
            out.push_back(*c >= 'a' && *c <= 'z' ? char(*c - 32) : *c);
        }
    }

    int hp_colour(const mon& m)
    {
        return m.hp * 2 > m.max_hp ? 0 : m.hp * 5 > m.max_hp ? 1 : 2;
    }

    // Text wrapped by words at (x, y), never cut short (ui::print_wrapped_fit).
    void print_wrapped(ui& u, int x, int y, int width, const bn::string_view& text, text_color color,
                       bn::ivector<bn::sprite_ptr>& out, int max_lines = 2, int line_height = 15, bool small = false)
    {
        u.print_wrapped_fit(x, y, width, text, max_lines, line_height, color, out, small);
    }

    // GBA 1.9: the held items are bag items LEFTOVERS .. SITRUSBERRY, in held_item's order.
    held_item held_of(item_id id)
    {
        int k = int(id) - int(item_id::LEFTOVERS);
        return k >= 0 && k <= int(item_id::SITRUSBERRY) - int(item_id::LEFTOVERS) ? held_item(k + 1) : held_item::NONE;
    }

    item_id item_of(held_item h)
    {
        return item_id(int(item_id::LEFTOVERS) + int(h) - 1);
    }

    // Gives a held item from the bag (one it already holds goes back in the bag).
    bn::string<80> give_held(item_id id, mon& m)
    {
        game_state& g = state();
        bn::string<80> text;
        if(m.item == held_of(id))
        {
            text = m.name();
            text.append(" is already holding that.");
            return text;
        }
        if(m.item != held_item::NONE)
        {
            g.add_item(item_of(m.item), 1);
        }
        g.items[int(id)] = uint8_t(g.items[int(id)] - 1);
        m.item = held_of(id);
        text = m.name();
        text.append(" is now holding the ");
        text.append(game_data::items[int(id)].name);
        text.append(".");
        return text;
    }

    bn::string<80> take_held(mon& m)
    {
        bn::string<80> text;
        if(m.item == held_item::NONE)
        {
            text = m.name();
            text.append(" isn't holding anything.");
            return text;
        }
        state().add_item(item_of(m.item), 1);
        text = "Received the ";
        text.append(game_data::items[int(item_of(m.item))].name);
        text.append(" from ");
        text.append(m.name());
        text.append(".");
        m.item = held_item::NONE;
        return text;
    }
}

void draw_hp_bar(bn::ivector<bn::sprite_ptr>& bar, int x, int y, int segments, int hp, int max_hp)
{
    int length = segments * 8;
    int fill = max_hp ? (hp * length + max_hp - 1) / max_hp : 0;
    if(hp > 0)
    {
        fill = bn::max(fill, 1);
    }
    // hpClass(): green over half, yellow over a fifth, red below.
    int colour = hp * 2 > max_hp ? 0 : hp * 5 > max_hp ? 1 : 2;
    if(bar.size() != segments)
    {
        bar.clear();
        for(int i = 0; i < segments; ++i)
        {
            bar.push_back(bn::sprite_items::hpbar.create_sprite(sx(x + 8 * i + 4), sy(y + 4), 0));
            bar.back().set_bg_priority(0);
        }
    }
    for(int i = 0; i < segments; ++i)
    {
        bar[i].set_position(sx(x + 8 * i + 4), sy(y + 4));
        bar[i].set_tiles(bn::sprite_items::hpbar.tiles_item(), colour * 9 + bn::clamp(fill - 8 * i, 0, 8));
    }
}

// useItem(): medicine on one Pokémon. Returns false (nothing used up) if it would do nothing.
bool use_item_on(item_id id, mon& m, bn::string<80>& message)
{
    game_state& g = state();
    const item_info& it = game_data::items[int(id)];
    if(! g.item_count(id))
    {
        return false;
    }
    if(it.revive)
    {
        if(! m.fainted() || g.run.nuzlocke())
        {
            return false;       // (NUZLOCKE: a fallen Pokémon stays fallen)
        }
        m.hp = uint16_t(bn::max(1, m.max_hp / 2));
        message = m.name();
        message.append(" was revived!");
    }
    else if(m.fainted())
    {
        return false;
    }
    else if(it.full)
    {
        if(m.hp >= m.max_hp && m.st == status::NONE)
        {
            return false;
        }
        m.hp = m.max_hp;
        m.st = status::NONE;
        m.sleep_turns = 0;
        message = m.name();
        message.append(" was fully restored!");
    }
    else if(it.heal)
    {
        if(m.hp >= m.max_hp)
        {
            return false;
        }
        int before = m.hp;
        m.hp = uint16_t(bn::min(int(m.max_hp), m.hp + it.heal));
        message = m.name();
        message.append("'s HP was restored by ");
        message.append(bn::to_string<6>(m.hp - before));
        message.append(" points.");
    }
    else if(it.cure != status::NONE && m.st == it.cure)
    {
        m.st = status::NONE;
        m.sleep_turns = 0;
        message = m.name();
        message.append(" was cured of ");
        message.append(it.cure == status::POISON ? "poisoning." : it.cure == status::PARALYSIS ? "paralysis." :
                       it.cure == status::SLEEP ? "sleep." : "its burn.");
    }
    else
    {
        return false;
    }
    g.items[int(id)] = uint8_t(g.items[int(id)] - 1);
    return true;
}

bool use_item(item_id id, int party_index, bn::string<80>& message)
{
    return use_item_on(id, state().party[party_index], message);
}

// ---------------------------------------------------------------------------------------------------
// Party screen (.pty, Emerald's party_menu): the lead in a big box on the left, the rest in two columns of
// five on the right, the message box and CANCEL along the bottom. The chosen one's icon hops.
namespace
{
    constexpr int cancel_index = max_party;

    struct slot_rect
    {
        int tx, ty, tw, th;
    };

    slot_rect slot_of(int k)
    {
        if(k == 0)
        {
            return { 1, 3, 11, 8 };
        }
        if(k == cancel_index)
        {
            return { 24, 17, 6, 3 };
        }
        return { 12 + ((k - 1) / 5) * 9, 1 + ((k - 1) % 5) * 3, 9, 3 };
    }

    class party_view
    {

    public:
        party_view() :
            _bg(bn::regular_bg_items::party_bg.create_bg(8, 48))
        {
            _bg.set_priority(3);
        }

        void draw(int selected, int swapping)
        {
            game_state& g = state();
            ui& u = gui();
            u.win().clear_all();
            _icons.clear();
            _texts.clear();
            for(int k = 0; k < max_party; ++k)
            {
                _bars[k].clear();
            }
            _selected = selected;
            for(int k = 0; k < max_party; ++k)
            {
                slot_rect r = slot_of(k);
                if(k >= g.party_count)
                {
                    // The EGGS you carry fill the free slots (GBA 1.9): the bar fills as one nears hatching.
                    int e = k - g.party_count;
                    if(k && e < g.extra.egg_count)
                    {
                        int left = g.extra.eggs[e].steps;
                        int x = r.tx * 8, y = r.ty * 8;
                        u.win().box(window_style::EMPTY, r.tx, r.ty, r.tw, r.th);
                        bn::sprite_ptr icon = bn::sprite_items::mon_icon_egg.create_sprite(sx(x + 10), sy(y + 8));
                        icon.set_bg_priority(0);
                        icon.set_z_order(1);
                        _icons.push_back(icon);
                        _icon_y[_icons.size() - 1] = y + 8;
                        _icon_slot[_icons.size() - 1] = k;
                        u.print(x + 22, y + 2, "EGG", text_color::WHITE, _texts, true);
                        u.print(x + 22, y + 12, bn::to_string<8>(left), text_color::WHITE, _texts, true);
                        draw_hp_bar(_bars[k], x + 44, y + 14, 3, egg_hatch_steps - left, egg_hatch_steps);
                        continue;
                    }
                    // Empty slots (and the ones the party can't use yet: partyCap) stay dark.
                    if(k)
                    {
                        u.win().box(k >= g.party_cap() ? window_style::LOCKED : window_style::EMPTY, r.tx, r.ty, r.tw, r.th);
                    }
                    continue;
                }
                const mon& m = g.party[k];
                window_style style = k == selected || k == swapping ? window_style::SLOT_ON :
                                     m.fainted() ? window_style::SLOT_FNT : window_style::SLOT;
                u.win().box(style, r.tx, r.ty, r.tw, r.th);
                int x = r.tx * 8, y = r.ty * 8;
                bn::sprite_ptr icon = icon_item(m.species_index).create_sprite(sx(x + (k ? 10 : 14)), sy(y + (k ? 8 : 12)));
                apply_shiny(icon, m, mon_view::ICON);
                icon.set_bg_priority(0);
                icon.set_z_order(1);        // under the name's text
                _icons.push_back(icon);
                _icon_y[_icons.size() - 1] = y + (k ? 8 : 12);
                _icon_slot[_icons.size() - 1] = k;
                bn::string<16> lv("Lv");
                lv.append(bn::to_string<4>(m.level));
                if(k == 0)
                {
                    u.print_fit_slide(x + 30, x + 5, x + r.tw * 8 - 4, y + 5, m.name(), text_color::WHITE, _texts);
                    u.print(x + 36, y + 22, lv, text_color::WHITE, _texts, true);
                    draw_hp_bar(_bars[k], x + 8, y + 36, 8, m.hp, m.max_hp);
                    bn::string<16> hp(bn::to_string<4>(m.hp));
                    hp.append("/");
                    hp.append(bn::to_string<4>(m.max_hp));
                    u.print(x + 80 - u.width(hp, true), y + 46, hp, text_color::WHITE, _texts, true);
                }
                else
                {
                    u.print_fit_slide(x + 22, x + 3, x + r.tw * 8 - 1, y + 2, m.name(), text_color::WHITE, _texts, true);
                    u.print(x + 22, y + 12, lv, text_color::WHITE, _texts, true);
                    draw_hp_bar(_bars[k], x + 44, y + 14, 3, m.hp, m.max_hp);
                }
            }
            slot_rect c = slot_of(cancel_index);
            u.win().box(selected == cancel_index ? window_style::SLOT_ON : window_style::SLOT, c.tx, c.ty, c.tw, c.th);
            u.print(c.tx * 8 + 4, c.ty * 8 + 4, "CANCEL", text_color::WHITE, _texts, true);
        }

        void message(const bn::string_view& text)
        {
            ui& u = gui();
            _message.clear();
            u.win().box(window_style::WINDOW, 0, 17, 24, 3);
            u.print(8, 140, text, text_color::INK, _message);
        }

        // Drops the sprites (names, icons, bars), which would show through a menu or the text box over them.
        void blank()
        {
            _icons.clear();
            _texts.clear();
            _message.clear();
            for(int k = 0; k < max_party; ++k)
            {
                _bars[k].clear();
            }
        }

        // The chosen one's icon hops (faster when healthier: 6/8/14/22 frames).
        void tick()
        {
            ++_timer;
            game_state& g = state();
            for(int i = 0; i < _icons.size(); ++i)
            {
                int k = _icon_slot[i];
                int dy = 0;
                if(k == _selected)
                {
                    const mon& m = g.party[k];
                    int speed = m.fainted() ? 0 : hp_colour(m) == 0 ? (m.hp >= m.max_hp ? 6 : 8) : hp_colour(m) == 1 ? 14 : 22;
                    dy = speed && (_timer / speed) % 2 ? -3 : 0;
                }
                _icons[i].set_y(sy(_icon_y[i] + dy));
            }
        }

    private:
        bn::regular_bg_ptr _bg;
        bn::vector<bn::sprite_ptr, max_party> _icons;
        int _icon_y[max_party] = {};
        int _icon_slot[max_party] = {};
        bn::vector<bn::sprite_ptr, 64> _texts;
        bn::vector<bn::sprite_ptr, 12> _message;
        bn::vector<bn::sprite_ptr, 8> _bars[max_party];
        int _selected = 0;
        int _timer = 0;
    };
}

int party_screen(party_mode mode, const char* prompt)
{
    game_state& g = state();
    ui& u = gui();
    ui::fade_out(8);
    u.win().clear_all();
    int result = -1;
    {
        bn::optional<party_view> view;
        view.emplace();
        int selected = 0;
        int swapping = -1;
        bool redraw = true;
        bn::string<48> base_prompt(prompt ? prompt : "Choose a POKéMON.");
        // EGGS that don't fit in the free slots: the soonest one's steps go in the message instead.
        const extras_state& x = g.extra;
        if(! prompt && g.party_count + x.egg_count > max_party)
        {
            int soonest = egg_hatch_steps;
            for(int i = 0; i < x.egg_count; ++i)
            {
                soonest = bn::min(soonest, int(x.eggs[i].steps));
            }
            base_prompt = "Next EGG hatches in ";
            base_prompt.append(bn::to_string<8>(soonest));
            base_prompt.append(" steps.");
        }
        bool faded = true;
        while(true)
        {
            if(redraw)
            {
                view->draw(selected, swapping);
                view->message(swapping >= 0 ? bn::string_view("Move to where?") : bn::string_view(base_prompt));
                redraw = false;
                if(faded)
                {
                    ui::fade_in(8);
                    faded = false;
                }
            }
            view->tick();
            frame();
            int n = g.party_count;
            int before = selected;
            if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
            {
                // Up/Down step through the Pokémon and CANCEL.
                int order_pos = selected == cancel_index ? n : selected;
                order_pos = (order_pos + (bn::keypad::up_pressed() ? n : 1)) % (n + 1);
                selected = order_pos == n ? cancel_index : order_pos;
            }
            else if(bn::keypad::left_pressed())
            {
                // Left/Right move between the lead and the two columns.
                selected = selected >= 6 && selected < cancel_index ? selected - 5 : selected == cancel_index ? selected : 0;
            }
            else if(bn::keypad::right_pressed())
            {
                selected = selected == 0 ? (n > 1 ? 1 : 0) : selected >= 1 && selected <= 5 && selected + 5 < n ? selected + 5 : selected;
            }
            if(selected != before)
            {
                audio::play(audio::sfx::SELECT);
                redraw = true;
                continue;
            }
            if(bn::keypad::b_pressed())
            {
                audio::play(audio::sfx::SELECT);
                if(swapping >= 0)
                {
                    swapping = -1;
                    redraw = true;
                    continue;
                }
                break;
            }
            if(bn::keypad::a_pressed())
            {
                audio::play(audio::sfx::SELECT);
                if(selected == cancel_index)
                {
                    if(swapping >= 0)
                    {
                        swapping = -1;
                        redraw = true;
                        continue;
                    }
                    break;
                }
                if(swapping >= 0)
                {
                    mon t = g.party[swapping];
                    g.party[swapping] = g.party[selected];
                    g.party[selected] = t;
                    swapping = -1;
                    redraw = true;
                    continue;
                }
                if(mode == party_mode::CHOOSE)
                {
                    result = selected;
                    break;
                }
                bn::string<48> text("Do what with ");
                text.append(g.party[selected].name());
                text.append("?");
                view->message(text);
                bn::string_view options[] = { "SUMMARY", "SWITCH", "ITEM", "CANCEL" };
                bn::string_view battle_options[] = { "SUMMARY", "CANCEL" };
                menu_spec s;
                s.options = mode == party_mode::FIELD ? options : battle_options;
                s.count = mode == party_mode::FIELD ? 4 : 2;
                s.tw = 10;
                s.th = s.count * 2 + 2;
                s.tx = 20;
                s.ty = 17 - s.th;
                int pick = u.menu(s);
                if(pick == 0)
                {
                    view.reset();
                    summary_screen(g.party.data(), g.party_count, selected);
                    view.emplace();
                    faded = true;
                }
                else if(pick == 1 && mode == party_mode::FIELD && n > 1)
                {
                    swapping = selected;
                }
                else if(pick == 2 && mode == party_mode::FIELD)
                {
                    // GBA 1.9: ITEM: GIVE (from the held items in the bag) / TAKE.
                    mon& m = g.party[selected];
                    view->blank();
                    bn::string_view item_options[] = { "GIVE", "TAKE", "CANCEL" };
                    menu_spec is;
                    is.options = item_options;
                    is.count = 3;
                    is.tw = 8;
                    is.th = 8;
                    is.tx = 22;
                    is.ty = 9;
                    int action = u.menu(is);
                    bn::string<80> said;
                    if(action == 0)
                    {
                        item_id ids[5];
                        bn::string<32> labels[6];
                        bn::string_view views[6];
                        int k = 0;
                        for(int h = 1; h <= 5; ++h)
                        {
                            item_id id = item_of(held_item(h));
                            if(g.item_count(id))
                            {
                                ids[k] = id;
                                labels[k] = game_data::items[int(id)].name;
                                labels[k].append(" x");
                                labels[k].append(bn::to_string<4>(g.item_count(id)));
                                views[k] = labels[k];
                                ++k;
                            }
                        }
                        if(! k)
                        {
                            said = "You don't have any items to hold.";
                        }
                        else
                        {
                            views[k] = "CANCEL";
                            menu_spec gs;
                            gs.options = views;
                            gs.count = k + 1;
                            gs.tw = 17;
                            gs.th = gs.count * 2 + 2;
                            gs.tx = 13;
                            gs.ty = 17 - gs.th;
                            int which = u.menu(gs);
                            if(which >= 0 && which < k)
                            {
                                said = give_held(ids[which], m);
                            }
                        }
                    }
                    else if(action == 1)
                    {
                        said = take_held(m);
                    }
                    if(! said.empty())
                    {
                        u.say(said);
                        u.clear_text();
                    }
                }
                redraw = true;
            }
        }
        ui::fade_out(8);
        u.win().clear_all();
    }
    return result;
}

// ---------------------------------------------------------------------------------------------------
// Summary (.sm): POKéMON INFO / POKéMON SKILLS / BATTLE MOVES. Left/Right change page, Up/Down Pokémon.
void summary_screen(const mon* mons, int count, int index)
{
    game_state& g = state();
    ui& u = gui();
    ui::fade_out(8);
    u.win().clear_all();
    {
        bn::regular_bg_ptr bg = bn::regular_bg_items::summary_bg.create_bg(8, 48);
        bg.set_priority(3);
        int page = 0;
        bool redraw = true;
        bn::optional<bn::sprite_ptr> mon_sprite;
        bn::vector<bn::sprite_ptr, 72> texts;
        bn::vector<bn::sprite_ptr, 8> bar;
        constexpr const char* titles[] = { "POKéMON INFO", "POKéMON SKILLS", "BATTLE MOVES" };
        bool faded = true;
        while(true)
        {
            if(redraw)
            {
                const mon& m = mons[index];
                texts.clear();
                u.win().clear_all();
                u.win().box(window_style::PAGE, 11, 3, 19, 17);
                u.print(8, 1, titles[page], text_color::WHITE, texts);
                u.print(196, 4, "< PAGE >", text_color::WHITE, texts, true);
                mon_sprite = m.data().front.create_sprite(sx(44), sy(56));
                apply_shiny(*mon_sprite, m, mon_view::FRONT);
                mon_sprite->set_bg_priority(1);
                u.print_fit(6, 92, m.name(), 76, text_color::INK, texts);
                bn::string<16> lv("Lv");
                lv.append(bn::to_string<4>(m.level));
                u.print(6, 108, lv, text_color::INK, texts);
                if(m.shiny())
                {
                    u.print(44, 111, "SHINY", text_color::RED, texts, true);
                }
                draw_hp_bar(bar, 14, 128, 8, m.hp, m.max_hp);
                bn::string<32> row;
                auto line = [&](int i, const char* label, const bn::string_view& value)
                {
                    u.print(98, 30 + i * 17, label, text_color::INK, texts);
                    // The value right-aligned after its label, in a smaller font if it's long (ability names).
                    int room = 232 - (98 + u.width(label) + 6);
                    int w = u.fit_width(value, room);
                    u.print_fit(232 - w, 30 + i * 17, value, room, text_color::INK, texts);
                };
                if(page == 0)
                {
                    row = type_name(m.data().type1);
                    if(m.data().type2 >= 0)
                    {
                        row.append("/");
                        row.append(type_name(m.data().type2));
                    }
                    line(0, "TYPE", row);
                    row.clear();
                    upper(row, m.abil().name[0] ? m.abil().name : "-");
                    line(1, "ABILITY", row);
                    line(2, "OT", g.name);
                    line(3, "EXP. POINTS", bn::to_string<8>(m.xp));
                    line(4, "NEXT LV.", bn::to_string<8>(bn::max(0, m.xp_next() - m.xp)));
                    if(m.item != held_item::NONE)
                    {
                        line(5, "ITEM", game_data::held_items[int(m.item)].name);
                    }
                }
                else if(page == 1)
                {
                    row = bn::to_string<4>(m.hp);
                    row.append("/");
                    row.append(bn::to_string<4>(m.max_hp));
                    line(0, "HP", row);
                    line(1, "ATTACK", bn::to_string<4>(m.atk));
                    line(2, "DEFENSE", bn::to_string<4>(m.def));
                    line(3, "SP. ATK", bn::to_string<4>(m.spa));
                    line(4, "SP. DEF", bn::to_string<4>(m.spd));
                    line(5, "SPEED", bn::to_string<4>(m.spe));
                }
                else
                {
                    for(int i = 0; i < m.move_count; ++i)
                    {
                        const move& mv = move_data(m.move(i));
                        u.print_fit(98, 28 + i * 30, mv.name, 136, text_color::INK, texts);
                        u.print(100, 43 + i * 30, type_name(mv.type), text_color::BLUE, texts, true);
                        if(mv.power)
                        {
                            row = bn::to_string<4>(mv.power);
                        }
                        else
                        {
                            row = "-";
                        }
                        u.print(232 - u.width(row, true), 43 + i * 30, row, text_color::INK, texts, true);
                        bn::string<16> pp("PP ");
                        pp.append(bn::to_string<4>(m.pp(i)));
                        pp.append("/");
                        pp.append(bn::to_string<4>(m.max_pp(i)));
                        // (Right of the type, left of the power: "ELECTRIC" ran into it at a fixed x.)
                        int pp_right = 232 - u.width(row, true) - 8;
                        u.print(pp_right - u.width(pp, true), 43 + i * 30, pp, m.pp(i) ? text_color::INK : text_color::RED,
                                texts, true);
                    }
                }
                redraw = false;
                if(faded)
                {
                    ui::fade_in(8);
                    faded = false;
                }
            }
            frame();
            if(bn::keypad::a_pressed() || bn::keypad::b_pressed())
            {
                audio::play(audio::sfx::SELECT);
                break;
            }
            if(bn::keypad::left_pressed() && page > 0)
            {
                --page;
                redraw = true;
            }
            else if(bn::keypad::right_pressed() && page < 2)
            {
                ++page;
                redraw = true;
            }
            else if((bn::keypad::up_pressed() || bn::keypad::down_pressed()) && count > 1)
            {
                index = (index + (bn::keypad::up_pressed() ? count - 1 : 1)) % count;
                redraw = true;
            }
            if(redraw)
            {
                audio::play(audio::sfx::SELECT);
            }
        }
        ui::fade_out(8);
        u.win().clear_all();
    }
}

// ---------------------------------------------------------------------------------------------------
// Bag (.bag): five pockets; Left/Right change pocket (the bag hops), Up/Down move the cursor (it shakes).
// From the field only medicine can be used, on a Pokémon picked from a list.
namespace
{
    constexpr const char* pocket_names[] = { "ITEMS", "POKé BALLS", "TMs & HMs", "BERRIES", "KEY ITEMS" };
    int bag_pocket = 1;

    // The TM an item is (game_data::tms index), or -1.
    int tm_of(item_id id)
    {
        for(int i = 0; i < game_data::tms_count; ++i)
        {
            if(game_data::tms[i].item == id)
            {
                return i;
            }
        }
        return -1;
    }

    // GBA 1.8: a TM on a party member. It's never used up; a fifth move waits for the "forget a move" prompt
    // the overworld runs when the bag closes.
    bn::string<96> teach_tm(int tm, mon& m)
    {
        const tm_info& t = game_data::tms[tm];
        bn::string<96> text(m.name());
        if(! (game_data::tm_compat[m.species_index][tm >> 3] & (1 << (tm & 7))))
        {
            text.append(" can't learn ");
            text.append(move_data(t.move).name);
            text.append(".");
            return text;
        }
        for(int k = 0; k < m.move_count; ++k)
        {
            if(m.move(k) == t.move)
            {
                text.append(" already knows ");
                text.append(move_data(t.move).name);
                text.append(".");
                return text;
            }
        }
        // Struggle goes once it knows something real.
        for(int k = 0; k < m.move_count; ++k)
        {
            if(m.move(k) == 0)
            {
                m.remove_move(k);
                --k;
            }
        }
        if(m.move_count < 4)
        {
            m.set_move(m.move_count++, t.move);
            audio::play(audio::sfx::OBTAIN);
            text.append(" learned ");
            text.append(move_data(t.move).name);
            text.append("!");
            return text;
        }
        add_pending_move(&m, t.move);
        text.append(" wants to learn ");
        text.append(move_data(t.move).name);
        text.append("...");
        return text;
    }

    // GBA 1.8: a stone (or the LINKING CORD) on a party member: it evolves if that's its item. A Pokémon
    // with two ways (CLAMPERL) asks which.
    bn::string<96> use_evolver(item_id id, mon& m)
    {
        game_state& g = state();
        ui& u = gui();
        int targets[4];
        int n = 0;
        for(int k = 0; k < game_data::evo_items_count && n < 4; ++k)
        {
            const evo_item& e = game_data::evo_items[k];
            if(e.item == id && e.from == m.species_index)
            {
                targets[n++] = e.to;
            }
        }
        if(! n || m.fainted())
        {
            return bn::string<96>("It won't have any effect.");
        }
        int to = targets[0];
        if(n > 1)
        {
            bn::string_view views[5];
            for(int k = 0; k < n; ++k)
            {
                views[k] = game_data::species_list[targets[k]].name;
            }
            views[n] = "CANCEL";
            u.show_text("Evolve into which?");
            int k = u.list(views, n + 1);
            u.clear_text();
            if(k < 0 || k >= n)
            {
                return bn::string<96>();
            }
            to = targets[k];
        }
        g.items[int(id)] = uint8_t(g.items[int(id)] - 1);
        audio::play(audio::sfx::OBTAIN);
        bn::string<96> text("What? ");
        text.append(m.name());
        text.append(" is evolving!");
        u.say(text);
        m.evolve_into(to, u);
        g.mark_seen(to);
        g.mark_owned(to);
        u.clear_text();
        return bn::string<96>();
    }
}

int bag_screen(bag_mode mode)
{
    game_state& g = state();
    ui& u = gui();
    ui::fade_out(8);
    u.win().clear_all();
    int result = -1;
    {
        bn::regular_bg_ptr bg = bn::regular_bg_items::bag_bg.create_bg(8, 48);
        bg.set_priority(3);
        int index = 0, top = 0;
        bool redraw = true;
        bool faded = true;
        bn::vector<bn::sprite_ptr, 64> texts;
        bn::vector<bn::sprite_ptr, 16> desc;
        bn::optional<bn::sprite_ptr> cursor;
        bn::sprite_ptr bag = bn::sprite_items::bag.create_sprite(sx(60), sy(68), bag_pocket);
        bag.set_bg_priority(2);
        bn::vector<bn::sprite_ptr, 5> dots;
        int anim = 0, anim_kind = 0;
        bn::string<96> said;
        while(true)
        {
            int ids[items_count];
            int count = 0;
            for(int i = 0; i < items_count; ++i)
            {
                if(g.items[i] && game_data::items[i].pocket == bag_pocket)
                {
                    ids[count++] = i;
                }
            }
            index = bn::min(index, count);     // count = CLOSE BAG
            if(redraw)
            {
                texts.clear();
                u.win().clear_all();
                u.win().box(window_style::BAG, 0, 0, 13, 3);
                u.win().box(window_style::BAG, 14, 1, 16, 16);
                u.win().box(window_style::BAG, 0, 17, 14, 3);
                bn::string<24> title("< ");
                title.append(pocket_names[bag_pocket]);
                title.append(" >");
                u.print(52 - u.width(title) / 2, 4, title, text_color::INK, texts);
                // Seven rows at a time; the list scrolls with the cursor (the TMs make a long pocket).
                top = bn::clamp(top, bn::max(0, index - 6), index);
                for(int k = 0; k < 7 && top + k <= count; ++k)
                {
                    int i = top + k;
                    int y = 13 + k * 15;
                    if(i == count)
                    {
                        u.print(126, y, "CLOSE BAG", text_color::INK, texts);
                        break;
                    }
                    const item_info& it = game_data::items[ids[i]];
                    bn::string<8> n("x");
                    n.append(bn::to_string<4>(g.items[ids[i]]));
                    bool tm = it.pocket == 2 && it.price;       // TMs are kept for good: no count
                    int room = 232 - (tm ? 0 : u.width(n, true) + 4) - 126;
                    u.print_fit(126, y, it.name, room, text_color::INK, texts, true);
                    if(! tm)
                    {
                        u.print(232 - u.width(n, true), y, n, text_color::INK, texts, true);
                    }
                }
                if(top > 0)
                {
                    u.print(230, 4, "^", text_color::INK, texts, true);
                }
                if(top + 7 <= count)
                {
                    u.print(230, 13 + 7 * 15 - 6, "v", text_color::INK, texts, true);
                }
                cursor = bn::sprite_items::cursor.create_sprite(sx(118 + 4), sy(13 + (index - top) * 15 + 6));
                cursor->set_bg_priority(0);
                dots.clear();
                for(int k = 0; k < 5; ++k)
                {
                    // The pocket dots: the current one lit.
                    bn::sprite_ptr d = bn::sprite_items::badge.create_sprite(sx(24 + k * 10), sy(30), k == bag_pocket ? 1 : 0);
                    d.set_scale(bn::fixed(0.5));
                    d.set_bg_priority(1);
                    dots.push_back(d);
                }
                bag.set_tiles(bn::sprite_items::bag.tiles_item(), bag_pocket);
                desc.clear();
                bn::string_view text = said.empty() ? (index < count ? game_data::items[ids[index]].desc : "CLOSE BAG")
                                                    : bn::string_view(said);
                if(said.empty() && mode == bag_mode::BATTLE && index < count && game_data::items[ids[index]].pocket == 1)
                {
                    text = "Weaken it first for a better catch rate!";
                }
                print_wrapped(u, 4, 139, 104, text, text_color::INK, desc, 2, 9, true);
                said.clear();
                redraw = false;
                if(faded)
                {
                    ui::fade_in(8);
                    faded = false;
                }
            }
            // The bag hops when the pocket changes, and shakes when the cursor moves.
            if(anim > 0)
            {
                --anim;
                int t = anim;
                if(anim_kind == 0)
                {
                    bag.set_y(sy(68 - (t > 5 ? 10 - t : t)));
                }
                else
                {
                    int a = t % 12 < 3 ? -6 : t % 12 < 6 ? 0 : t % 12 < 9 ? 6 : 0;
                    bag.set_rotation_angle(a < 0 ? 360 + a : a);
                }
                if(! anim)
                {
                    bag.set_y(sy(68));
                    bag.set_rotation_angle(0);
                }
            }
            frame();
            if(bn::keypad::left_pressed() || bn::keypad::right_pressed())
            {
                bag_pocket = (bag_pocket + (bn::keypad::left_pressed() ? 4 : 1)) % 5;
                index = 0;
                top = 0;
                redraw = true;
                anim = 10;
                anim_kind = 0;
                audio::play(audio::sfx::SELECT);
            }
            else if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
            {
                index = (index + (bn::keypad::up_pressed() ? count : 1)) % (count + 1);
                redraw = true;
                anim = 12;
                anim_kind = 1;
                audio::play(audio::sfx::SELECT);
            }
            else if(bn::keypad::b_pressed() || (bn::keypad::a_pressed() && index == count))
            {
                audio::play(audio::sfx::SELECT);
                break;
            }
            else if(bn::keypad::a_pressed())
            {
                audio::play(audio::sfx::SELECT);
                item_id id = item_id(ids[index]);
                const item_info& it = game_data::items[int(id)];
                if(mode == bag_mode::BATTLE)
                {
                    result = int(id);
                    break;
                }
                bool medicine = it.heal || it.cure != status::NONE || it.revive || it.full;
                bool candy = id == item_id::RARECANDY;
                int tm = tm_of(id);
                bool evolver = false;
                for(int k = 0; k < game_data::evo_items_count; ++k)
                {
                    evolver |= game_data::evo_items[k].item == id;
                }
                if(id == item_id::ESCAPEROPE)
                {
                    result = int(id);   // the overworld knows whether it works here
                    break;
                }
                if(id == item_id::REPEL || id == item_id::SUPERREPEL || id == item_id::MAXREPEL)
                {
                    if(g.extra.repel_steps)
                    {
                        said = "The last REPEL is still working.";   // Emerald: it lingers, nothing used up
                        redraw = true;
                        continue;
                    }
                    g.items[int(id)] = uint8_t(g.items[int(id)] - 1);
                    g.extra.repel_steps = uint16_t(id == item_id::REPEL ? 100 : id == item_id::SUPERREPEL ? 200 : 250);
                    audio::play(audio::sfx::BALL);
                    said = it.name;
                    said.append(" was used. Wild POKéMON will stay away.");
                    redraw = true;
                    continue;
                }
                bool held = held_of(id) != held_item::NONE;
                if(! medicine && ! candy && tm < 0 && ! evolver && ! held)
                {
                    continue;   // only medicine, the RARE CANDY, evolution items, TMs and held items go on a Pokémon
                }
                // Who on (bagOpen's list: "NAME hp/max", CANCEL).
                bn::string<32> labels[max_party + 1];
                bn::string_view views[max_party + 1];
                for(int i = 0; i < g.party_count; ++i)
                {
                    labels[i] = g.party[i].name();
                    labels[i].append(" ");
                    labels[i].append(bn::to_string<4>(g.party[i].hp));
                    labels[i].append("/");
                    labels[i].append(bn::to_string<4>(g.party[i].max_hp));
                    views[i] = labels[i];
                }
                views[g.party_count] = "CANCEL";
                texts.clear();      // the item list's text would show through the party list
                cursor.reset();
                int who = u.list(views, g.party_count + 1);
                if(who >= 0 && who < g.party_count && held)
                {
                    said = give_held(id, g.party[who]);
                }
                else if(who >= 0 && who < g.party_count && tm >= 0)
                {
                    said = teach_tm(tm, g.party[who]);
                }
                else if(who >= 0 && who < g.party_count && evolver)
                {
                    said = use_evolver(id, g.party[who]);
                }
                else if(who >= 0 && who < g.party_count && candy)
                {
                    // One level up (grantXp's level-up: new moves, evolution); not past the NUZLOCKE cap.
                    mon& m = g.party[who];
                    if(m.fainted() || m.level >= level_cap_now())
                    {
                        said = "It won't have any effect.";
                    }
                    else
                    {
                        g.items[int(id)] = uint8_t(g.items[int(id)] - 1);
                        m.grant_xp(bn::max(1, m.xp_next() - m.xp), u);
                        u.clear_text();
                    }
                }
                else if(who >= 0 && who < g.party_count)
                {
                    bn::string<80> message;
                    if(! use_item(id, who, message))
                    {
                        message = "It won't have any effect.";
                    }
                    said = message;
                }
                redraw = true;
            }
        }
        ui::fade_out(8);
        u.win().clear_all();
    }
    return result;
}

// ---------------------------------------------------------------------------------------------------
// The PC's boxes (boxOpen): 14 boxes of 30 (6 to a row). WITHDRAW and MOVE show the box; DEPOSIT shows the
// party. A on a Pokémon: WITHDRAW / STORE / MOVE, RELEASE, CANCEL; A on the box's name: WALLPAPER / NAME.
namespace
{
    struct box_cursor
    {
        int box = 0;
        int r = 0, c = 0;      // r = -1: the box title; c = -1: the party (MOVE)
        int p = 0;              // party row
    };

    class box_screen
    {

    public:
        explicit box_screen(pc_mode mode) :
            _mode(mode),
            _bg(bn::regular_bg_items::pc_bg.create_bg(8, 48)),
            _wall(bn::regular_bg_items::wallpaper_bg.create_bg(8, 48)),
            _hand(bn::sprite_items::hand.create_sprite(0, 0, 0))
        {
            _bg.set_priority(3);
            _wall.set_priority(3);
            _wall.set_z_order(-1);
            _hand.set_bg_priority(0);
            _hand.set_z_order(-10);
            // The wallpaper shows only inside the box's frame.
            bn::rect_window inside = bn::rect_window::internal();
            inside.set_boundaries(sy(25), sx(90), sy(137), sx(236));
            bn::window::outside().set_show_bg(_wall, false);
            inside.set_show_bg(_wall, true);
            _wall.set_visible(_mode != pc_mode::DEPOSIT);
        }

        ~box_screen()
        {
            bn::window::outside().restore();
            bn::rect_window::internal().restore();
        }

        void run();

    private:
        pc_mode _mode;
        bn::regular_bg_ptr _bg;
        bn::regular_bg_ptr _wall;
        bn::sprite_ptr _hand;
        box_cursor _k;
        bn::optional<mon> _held;
        bn::vector<bn::sprite_ptr, 30> _icons;
        bn::vector<bn::sprite_ptr, max_party> _party_icons;
        bn::optional<bn::sprite_ptr> _big;
        bn::optional<bn::sprite_ptr> _held_icon;
        bn::vector<bn::sprite_ptr, 80> _texts;
        bn::vector<bn::sprite_ptr, 24> _info;
        bn::string<64> _msg;
        int _timer = 0;
        int _shown_box = -1;

        [[nodiscard]] bool in_party() const
        {
            return _mode == pc_mode::DEPOSIT || _k.c < 0;
        }
        [[nodiscard]] mon* selected()
        {
            game_state& g = state();
            if(in_party())
            {
                return _k.p < g.party_count ? &g.party[_k.p] : nullptr;
            }
            if(_k.r < 0)
            {
                return nullptr;
            }
            mon& m = g.box[_k.box * box_size + _k.r * 6 + _k.c];
            return m.empty() ? nullptr : &m;
        }
        void draw_box();
        void draw_info();
        void place_hand();
        void say(const bn::string_view& text);
        bool ask_yes_no(const bn::string_view& text);
        void action();
        void title_menu();
        void store(mon& m);
        void withdraw(int slot);
        void release(mon& m);
        void grab();
        void place();
        void compact_party_after_remove(int index);
    };

    const char* box_name_of(int b)
    {
        game_state& g = state();
        static bn::string<16> name;
        if(g.box_names[b][0])
        {
            name = g.box_names[b];
        }
        else
        {
            name = "BOX";
            name.append(bn::to_string<4>(b + 1));
        }
        return name.c_str();
    }

    void box_screen::draw_box()
    {
        game_state& g = state();
        ui& u = gui();
        _texts.clear();
        _icons.clear();
        _party_icons.clear();
        u.win().clear_all();
        // Wallpaper colours (boxWall: the box's default is its number's).
        int wall = g.box_wall[_k.box] ? g.box_wall[_k.box] - 1 : _k.box % 16;
        bn::bg_palette_ptr pal = _wall.palette();
        bn::color c1(game_data::wallpapers[wall][0] & 31, (game_data::wallpapers[wall][0] >> 5) & 31, game_data::wallpapers[wall][0] >> 10);
        bn::color c2(game_data::wallpapers[wall][1] & 31, (game_data::wallpapers[wall][1] >> 5) & 31, game_data::wallpapers[wall][1] >> 10);
        pal.set_color(1, c1);
        pal.set_color(2, c2);
        if(_mode == pc_mode::DEPOSIT)
        {
            // The party in two columns of five, CANCEL along the bottom.
            for(int i = 0; i <= max_party; ++i)
            {
                int tx = i == max_party ? 15 : 11 + (i / 5) * 10, ty = i == max_party ? 15 : 1 + (i % 5) * 3;
                int tw = i == max_party ? 8 : 9;
                u.win().box(i == _k.p ? window_style::SLOT_ON : window_style::PAGE, tx, ty, tw, 3);
                if(i == max_party)
                {
                    u.print(tx * 8 + 10, ty * 8 + 4, "CANCEL", text_color::INK, _texts, true);
                }
                else if(i < g.party_count)
                {
                    const mon& m = g.party[i];
                    bn::sprite_ptr icon = icon_item(m.species_index).create_sprite(sx(tx * 8 + 10), sy(ty * 8 + 10));
                    apply_shiny(icon, m, mon_view::ICON);
                    icon.set_scale(bn::fixed(0.75));
                    icon.set_bg_priority(0);
                    _party_icons.push_back(icon);
                    u.print_fit_slide(tx * 8 + 22, tx * 8 + 3, tx * 8 + tw * 8 - 1, ty * 8 + 3, m.name(), text_color::INK, _texts, true);
                    bn::string<8> lv("Lv");
                    lv.append(bn::to_string<4>(m.level));
                    u.print(tx * 8 + 22, ty * 8 + 12, lv, text_color::INK, _texts, true);
                }
            }
        }
        else
        {
            // The title bar: < BOX1 >, and the box's 30 slots.
            u.win().box(window_style::PAGE, 11, 0, 19, 3);
            bn::string<24> title(box_name_of(_k.box));
            u.print(163 - u.width(title) / 2, 4, title, text_color::INK, _texts);
            u.print(96, 4, "<", text_color::INK, _texts);
            u.print(226, 4, ">", text_color::INK, _texts);
            for(int s = 0; s < box_size; ++s)
            {
                const mon& m = g.box[_k.box * box_size + s];
                if(m.empty())
                {
                    continue;
                }
                int x = 92 + (s % 6) * 24 + 12, y = 26 + (s / 6) * 22 + 11;
                bn::sprite_ptr icon = icon_item(m.species_index).create_sprite(sx(x), sy(y));
                apply_shiny(icon, m, mon_view::ICON);
                icon.set_bg_priority(1);
                _icons.push_back(icon);
            }
            if(_mode == pc_mode::MOVE)
            {
                // MOVE: the party as a grid in the left panel.
                for(int i = 0; i < max_party; ++i)
                {
                    int x = 4 + (i % 2) * 40, y = 84 + (i / 2) * 15;
                    window_style st = _k.c < 0 && _k.p == i ? window_style::SLOT_ON : window_style::PAGE;
                    u.win().box(st, x / 8, y / 8, 5, 2);
                    if(i < g.party_count)
                    {
                        bn::sprite_ptr icon = icon_item(g.party[i].species_index).create_sprite(sx(x + 20), sy(y + 7));
                        apply_shiny(icon, g.party[i], mon_view::ICON);
                        icon.set_scale(bn::fixed(0.5));
                        icon.set_bg_priority(0);
                        _party_icons.push_back(icon);
                    }
                }
            }
        }
        _shown_box = _k.box;
        draw_info();
    }

    void box_screen::draw_info()
    {
        game_state& g = state();
        ui& u = gui();
        _info.clear();
        mon* m = selected();
        const mon* shown = m ? m : (_held ? &*_held : nullptr);
        u.win().box(shown ? window_style::SLOT_ON : window_style::DARK, 0, 0, 11, 2);
        u.print(6, 4, "POKéMON DATA", shown ? text_color::INK : text_color::WHITE, _info, true);
        if(shown)
        {
            _big = shown->data().front.create_sprite(sx(43), sy(48));
            apply_shiny(*_big, *shown, mon_view::FRONT);
            _big->set_bg_priority(1);
        }
        else
        {
            _big.reset();
        }
        if(_mode != pc_mode::MOVE)
        {
            u.win().box(window_style::WINDOW, 0, 11, 11, 6);
            if(shown)
            {
                u.print_fit(6, 92, shown->name(), 76, text_color::INK, _info, true);
                bn::string<24> sp("/");
                upper(sp, shown->species_name());
                u.print_fit(6, 104, sp, 76, text_color::INK, _info, true);
                bn::string<8> lv("Lv");
                lv.append(bn::to_string<4>(shown->level));
                u.print(6, 116, lv, text_color::INK, _info, true);
            }
        }
        // The message line.
        bn::string<64> msg = _msg;
        if(msg.empty())
        {
            if(_mode == pc_mode::DEPOSIT)
            {
                msg = "Which POKéMON will you deposit?";
            }
            else if(_held)
            {
                msg = "Holding ";
                msg.append(_held->name());
                msg.append(". Where to?");
            }
            else
            {
                int used = 0;
                for(int s = 0; s < box_size; ++s)
                {
                    used += ! g.box[_k.box * box_size + s].empty();
                }
                msg = bn::to_string<4>(used);
                msg.append("/30");
            }
        }
        u.print_fit(92, 145, msg, 144, text_color::INK, _info, true);
        place_hand();
    }

    void box_screen::place_hand()
    {
        int x, y;
        if(_mode == pc_mode::DEPOSIT)
        {
            x = _k.p == max_party ? 128 : 96 + (_k.p / 5) * 80;
            y = _k.p == max_party ? 116 : 6 + (_k.p % 5) * 24;
        }
        else if(_k.c < 0)
        {
            x = 18 + (_k.p % 2) * 40;
            y = 72 + (_k.p / 2) * 15;
        }
        else if(_k.r < 0)
        {
            x = 155;
            y = -2;
        }
        else
        {
            x = 96 + _k.c * 24 + 4;
            y = 14 + _k.r * 22;
        }
        int bob = _held ? 0 : (_timer / 30) % 2;
        _hand.set_position(sx(x + 8), sy(y + 8 + bob));
        _hand.set_tiles(bn::sprite_items::hand.tiles_item(), _held ? 1 : 0);
        if(_held)
        {
            if(! _held_icon)
            {
                _held_icon = icon_item(_held->species_index).create_sprite(0, 0);
                apply_shiny(*_held_icon, *_held, mon_view::ICON);
                _held_icon->set_bg_priority(0);
                _held_icon->set_z_order(-5);
            }
            _held_icon->set_position(sx(x + 8), sy(y + 22));
        }
        else
        {
            _held_icon.reset();
        }
    }

    void box_screen::say(const bn::string_view& text)
    {
        _msg = text;
        draw_info();
        frame();
        while(! bn::keypad::a_pressed() && ! bn::keypad::b_pressed())
        {
            ++_timer;
            place_hand();
            frame();
        }
        audio::play(audio::sfx::SELECT);
        _msg.clear();
        draw_info();
    }

    bool box_screen::ask_yes_no(const bn::string_view& text)
    {
        _msg = text;
        draw_info();
        bool yes = gui().yes_no(false);
        _msg.clear();
        draw_info();
        return yes;
    }

    void box_screen::compact_party_after_remove(int index)
    {
        game_state& g = state();
        for(int i = index; i < g.party_count - 1; ++i)
        {
            g.party[i] = g.party[i + 1];
        }
        g.party[g.party_count - 1] = mon();
        --g.party_count;
    }

    void box_screen::store(mon& m)
    {
        game_state& g = state();
        if(g.party_count <= 1)
        {
            say("That's your last POKéMON!");
            return;
        }
        int slot = -1;
        for(int i = 0; i < box_slots; ++i)
        {
            if(g.box[i].empty())
            {
                slot = i;
                break;
            }
        }
        if(slot < 0)
        {
            say("The BOXES are full.");
            return;
        }
        int index = int(&m - g.party.data());
        g.box[slot] = m;
        g.box[slot].heal();
        bn::string<64> text(m.name());
        compact_party_after_remove(index);
        _k.p = bn::min(_k.p, g.party_count - 1);
        text.append(" was deposited in BOX");
        text.append(bn::to_string<4>(slot / box_size + 1));
        text.append(".");
        draw_box();
        say(text);
    }

    void box_screen::withdraw(int slot)
    {
        game_state& g = state();
        if(g.party_count >= g.party_cap())
        {
            say("Your party's full!");
            return;
        }
        g.party[g.party_count++] = g.box[slot];
        bn::string<64> text(g.box[slot].name());
        g.box[slot] = mon();
        text.append(" was withdrawn.");
        draw_box();
        say(text);
    }

    void box_screen::release(mon& m)
    {
        game_state& g = state();
        bool from_party = in_party();
        if(from_party && g.party_count <= 1)
        {
            say("That's your last POKéMON!");
            return;
        }
        if(! ask_yes_no("Release this POKéMON?"))
        {
            return;
        }
        // The icon shrinks away over 120 frames, then the goodbyes.
        if(_big)
        {
            for(int f = 120; f > 0; f -= 2)
            {
                _big->set_scale(bn::fixed(f) / 120 + bn::fixed(0.01));
                frame();
            }
        }
        bn::string<32> name(m.name());
        if(from_party)
        {
            compact_party_after_remove(int(&m - g.party.data()));
            _k.p = bn::min(_k.p, g.party_count - 1);
        }
        else
        {
            m = mon();
        }
        draw_box();
        bn::string<48> text(name);
        text.append(" was released.");
        say(text);
        text = "Bye-bye, ";
        text.append(name);
        text.append("!");
        say(text);
    }

    // MOVE: the hand takes the Pokémon out of its slot (the party closes up behind it)...
    void box_screen::grab()
    {
        game_state& g = state();
        mon* m = selected();
        if(! m)
        {
            return;
        }
        if(in_party())
        {
            if(g.party_count <= 1)
            {
                say("That's your last POKéMON!");
                return;
            }
            _held = *m;
            compact_party_after_remove(_k.p);
        }
        else
        {
            _held = *m;
            *m = mon();
        }
        draw_box();
    }

    // ...and puts it down: an empty spot takes it, an occupied one swaps (you then hold the other).
    void box_screen::place()
    {
        game_state& g = state();
        mon m = *_held;
        if(in_party())
        {
            if(_k.p < g.party_count)
            {
                mon other = g.party[_k.p];
                g.party[_k.p] = m;
                _held = other;
            }
            else
            {
                if(g.party_count >= g.party_cap())
                {
                    say("Your party's full!");
                    return;
                }
                g.party[g.party_count++] = m;
                _held.reset();
            }
        }
        else
        {
            mon& slot = g.box[_k.box * box_size + _k.r * 6 + _k.c];
            mon other = slot;
            slot = m;
            slot.heal();
            if(other.empty())
            {
                _held.reset();
            }
            else
            {
                _held = other;
            }
        }
        _held_icon.reset();
        draw_box();
    }

    // A on the box title: WALLPAPER (four theme groups) / NAME / CANCEL.
    void box_screen::title_menu()
    {
        game_state& g = state();
        ui& u = gui();
        _msg = "What do you want to do?";
        draw_info();
        constexpr bn::string_view options[] = { "WALLPAPER", "NAME", "CANCEL" };
        int k = u.list(options, 3);
        _msg.clear();
        draw_info();
        if(k == 0)
        {
            _msg = "Pick a theme.";
            draw_info();
            bn::string_view groups[5];
            for(int i = 0; i < 4; ++i)
            {
                groups[i] = game_data::wall_groups[i];
            }
            groups[4] = "CANCEL";
            int grp = u.list(groups, 5);
            if(grp >= 0 && grp < 4)
            {
                _msg = "Pick the wallpaper.";
                draw_info();
                bn::string_view names[4];
                for(int i = 0; i < 4; ++i)
                {
                    names[i] = game_data::wall_names[grp * 4 + i];
                }
                int w = u.list(names, 4);
                if(w >= 0)
                {
                    g.box_wall[_k.box] = uint8_t(grp * 4 + w + 1);
                    // Fades through white.
                    bn::bg_palette_ptr pal = _wall.palette();
                    for(int f = 16; f >= 0; --f)
                    {
                        pal.set_fade(bn::color(31, 31, 31), bn::fixed(f) / 16);
                        draw_box();
                        frame();
                    }
                }
            }
            _msg.clear();
            draw_box();
        }
        else if(k == 1)
        {
            bn::string<32> title(box_name_of(_k.box));
            title.append("'s name?");
            char name[box_name_length + 1];
            _texts.clear();
            _info.clear();
            _icons.clear();
            _party_icons.clear();
            _big.reset();
            _hand.set_visible(false);
            _wall.set_visible(false);
            _bg.set_visible(false);
            if(u.keyboard(title, name, box_name_length, box_name_of(_k.box)))
            {
                for(int i = 0; i <= box_name_length; ++i)
                {
                    g.box_names[_k.box][i] = name[i];
                }
            }
            _hand.set_visible(true);
            _wall.set_visible(true);
            _bg.set_visible(true);
            draw_box();
            ui::fade_in(8);
        }
    }

    void box_screen::action()
    {
        game_state& g = state();
        ui& u = gui();
        if(_mode == pc_mode::DEPOSIT && _k.p == max_party)
        {
            return;
        }
        if(_mode != pc_mode::DEPOSIT && _k.r < 0 && _k.c >= 0)
        {
            title_menu();
            return;
        }
        if(_held)
        {
            place();
            return;
        }
        mon* m = selected();
        if(! m)
        {
            return;
        }
        bn::string<48> text(m->name());
        text.append(" is selected.");
        _msg = text;
        draw_info();
        bn::string_view options[] = { _mode == pc_mode::DEPOSIT ? "STORE" : _mode == pc_mode::WITHDRAW ? "WITHDRAW" : "MOVE",
                                      "RELEASE", "CANCEL" };
        int k = u.list(options, 3);
        _msg.clear();
        draw_info();
        if(k == 0)
        {
            if(_mode == pc_mode::DEPOSIT)
            {
                store(*m);
            }
            else if(_mode == pc_mode::WITHDRAW)
            {
                withdraw(_k.box * box_size + _k.r * 6 + _k.c);
            }
            else
            {
                grab();
            }
        }
        else if(k == 1)
        {
            release(*m);
        }
        (void) g;
    }

    void box_screen::run()
    {
        game_state& g = state();
        ui& u = gui();
        draw_box();
        ui::fade_in(8);
        while(true)
        {
            ++_timer;
            place_hand();
            frame();
            bool moved = false;
            if(bn::keypad::b_pressed())
            {
                audio::play(audio::sfx::SELECT);
                if(_held)
                {
                    say("You're holding a POKéMON!");
                    continue;
                }
                if(ask_yes_no("Exit from the BOX?"))
                {
                    break;
                }
                continue;
            }
            if(bn::keypad::a_pressed())
            {
                audio::play(audio::sfx::SELECT);
                if(_mode == pc_mode::DEPOSIT && _k.p == max_party)
                {
                    break;
                }
                action();
                continue;
            }
            if(_mode == pc_mode::DEPOSIT)
            {
                if(bn::keypad::up_pressed()) { _k.p = (_k.p + max_party) % (max_party + 1); moved = true; }
                if(bn::keypad::down_pressed()) { _k.p = (_k.p + 1) % (max_party + 1); moved = true; }
                if((bn::keypad::left_pressed() || bn::keypad::right_pressed()) && _k.p < max_party) { _k.p = (_k.p + 5) % max_party; moved = true; }
            }
            else if(_k.c < 0)
            {
                // MOVE, the party grid (two columns); right from its right column goes back to the box.
                if(bn::keypad::up_pressed()) { _k.p = (_k.p + max_party - 2) % max_party; moved = true; }
                if(bn::keypad::down_pressed()) { _k.p = (_k.p + 2) % max_party; moved = true; }
                if(bn::keypad::left_pressed()) { _k.p -= _k.p % 2; moved = true; }
                if(bn::keypad::right_pressed())
                {
                    if(_k.p % 2)
                    {
                        _k.c = 0;
                        _k.r = bn::clamp((_k.p / 2) * 2, 0, 4);
                    }
                    else
                    {
                        ++_k.p;
                    }
                    moved = true;
                }
            }
            else
            {
                if(_k.r < 0 && (bn::keypad::left_pressed() || bn::keypad::right_pressed()))
                {
                    _k.box = (_k.box + (bn::keypad::left_pressed() ? box_count - 1 : 1)) % box_count;
                    audio::play(audio::sfx::SELECT);
                    draw_box();
                    continue;
                }
                if(bn::keypad::left_pressed())
                {
                    if(_k.c == 0 && _mode == pc_mode::MOVE)
                    {
                        _k.c = -1;
                        _k.p = bn::clamp(_k.r * 2 + 1, 0, max_party - 1);
                    }
                    else
                    {
                        _k.c = (_k.c + 5) % 6;
                    }
                    moved = true;
                }
                if(bn::keypad::right_pressed()) { _k.c = (_k.c + 1) % 6; moved = true; }
                if(bn::keypad::up_pressed()) { _k.r = _k.r < 0 ? 4 : _k.r - 1; moved = true; }
                if(bn::keypad::down_pressed()) { _k.r = _k.r >= 4 ? -1 : _k.r + 1; moved = true; }
            }
            if(moved)
            {
                audio::play(audio::sfx::SELECT);
                if(_mode == pc_mode::DEPOSIT || _mode == pc_mode::MOVE)
                {
                    draw_box();
                }
                else
                {
                    draw_info();
                }
            }
        }
        ui::fade_out(8);
        _texts.clear();
        _info.clear();
        u.win().clear_all();
        (void) g;
    }
}

void pc_box_screen(pc_mode mode)
{
    ui::fade_out(8);
    gui().win().clear_all();
    bn::bg_palettes::set_transparent_color(bn::color(25, 26, 29));
    {
        box_screen screen(mode);
        screen.run();
    }
}

// ---------------------------------------------------------------------------------------------------
// POKéDEX (dexOpen, pokedex.c): the list (No. 1 to the highest seen) with the selected row always 6th,
// three sprites in the viewer, SEEN / OWN; an entry has INFO / AREA / SIZE / CANCEL.
namespace
{
    int dex_index = -1;

    int seen_count()
    {
        // Every species seen, regional forms included (the web game's Object.keys(adv.seen)).
        return state().seen.count();
    }

    // An entry page. Returns when B (or A on CANCEL) is pressed; Up/Down move to other seen species.
    void dex_entry(int& index, int list_count, bool registering)
    {
        game_state& g = state();
        ui& u = gui();
        int tab = 0;
        int page = 0;          // 0 INFO, 1 AREA, 2 SIZE
        constexpr const char* tabs[] = { "INFO", "AREA", "SIZE", "CANCEL" };
        bn::regular_bg_ptr bg = bn::regular_bg_items::dex_entry_bg.create_bg(8, 48);
        bg.set_priority(3);
        bn::vector<bn::sprite_ptr, 64> texts;
        bn::vector<bn::sprite_ptr, 40> cells;
        bn::vector<bn::sprite_ptr, 48> links;
        bn::vector<int, 40> habitats;      // indexes into cells
        bn::vector<int, 40> habitat_base;  // their unlit frame
        bn::optional<bn::sprite_ptr> sprite, you;
        bool redraw = true;
        int timer = 0;
        while(true)
        {
            int s = game_data::dex_order[index];
            const species& sp = game_data::species_list[s];
            bool owned = g.owned.test(s);
            if(redraw)
            {
                texts.clear();
                cells.clear();
                links.clear();
                habitats.clear();
                habitat_base.clear();
                sprite.reset();
                you.reset();
                u.win().clear_all();
                if(registering)
                {
                    u.print(12, 2, "POKéDEX registration completed.", text_color::WHITE, texts);
                }
                else
                {
                    for(int t = 0; t < 4; ++t)
                    {
                        u.print(12 + t * 58, 2, tabs[t], t == tab ? text_color::RED : text_color::WHITE, texts);
                    }
                }
                if(page == 0)
                {
                    sprite = sp.front.create_sprite(sx(44), sy(60));
                    sprite->set_bg_priority(2);
                    bn::string<32> line("No");
                    if(sp.dex_number < 100) line.append("0");
                    if(sp.dex_number < 10) line.append("0");
                    line.append(bn::to_string<4>(sp.dex_number));
                    line.append(" ");
                    line.append(sp.name);
                    u.print_fit(88, 30, line, 144, text_color::INK, texts);
                    bn::string<32> genus;
                    if(owned && sp.genus[0])
                    {
                        upper(genus, sp.genus);
                    }
                    else
                    {
                        genus = "?????";
                    }
                    genus.append(" POKéMON");
                    u.print_fit(88, 46, genus, 144, text_color::INK, texts);
                    bn::string<24> ht("HT  ");
                    bn::string<24> wt("WT  ");
                    if(owned && sp.height)
                    {
                        int inches = (sp.height * 3937 + 500) / 1000;
                        ht.append(bn::to_string<4>(inches / 12));
                        ht.append("'");
                        if(inches % 12 < 10) ht.append("0");
                        ht.append(bn::to_string<4>(inches % 12));
                        ht.append("\"");
                    }
                    else
                    {
                        ht.append("??'??\"");
                    }
                    if(owned && sp.weight)
                    {
                        int tenths = (sp.weight * 220462 + 50000) / 100000;     // hg -> lbs x10
                        wt.append(bn::to_string<6>(tenths / 10));
                        wt.append(".");
                        wt.append(bn::to_string<2>(tenths % 10));
                        wt.append(" lbs.");
                    }
                    else
                    {
                        wt.append("????.? lbs.");
                    }
                    u.print(88, 62, ht, text_color::INK, texts);
                    u.print(88, 76, wt, text_color::INK, texts);
                    if(owned)
                    {
                        print_wrapped(u, 18, 108, 204, sp.abil.desc, text_color::INK, texts, 3, 14);
                    }
                }
                else if(page == 1)
                {
                    // AREA: the region's grid, every area where it lives wild lit up; or AREA UNKNOWN.
                    u.win().box(window_style::WINDOW, 1, 3, 28, 16);
                    bn::string<24> cap;
                    upper(cap, sp.name);
                    u.print(120 - u.width(cap) / 2, 28, cap, text_color::INK, texts);
                    bool any = false;
                    for(int i : world_data::area_maps)
                    {
                        const map_def& m = world_data::maps[i];
                        const area_info& a = *m.area;
                        if(a.at_x >= 100 || a.region != map_region(state().map))
                        {
                            continue;
                        }
                        bool hab = false;
                        if(a.flags & area_flag::OWN_POOL)
                        {
                            for(int k = 0; k < m.pool_count; ++k)
                            {
                                hab |= int(m.pool[k]) == s;
                            }
                        }
                        any |= hab;
                    }
                    // With nowhere to show, AREA UNKNOWN sits in a box over the middle of the map.
                    constexpr int box_x0 = 64, box_x1 = 176, box_y0 = 104, box_y1 = 136;
                    auto covered = [&](int x, int y, int w, int h)
                    {
                        return ! any && x + w > box_x0 && x < box_x1 && y + h > box_y0 && y < box_y1;
                    };
                    for(int i : world_data::area_maps)
                    {
                        const map_def& m = world_data::maps[i];
                        const area_info& a = *m.area;
                        if(a.at_x >= 100 || a.region != map_region(state().map))
                        {
                            continue;
                        }
                        bool hab = false;
                        if(a.flags & area_flag::OWN_POOL)
                        {
                            for(int k = 0; k < m.pool_count; ++k)
                            {
                                hab |= int(m.pool[k]) == s;
                            }
                        }
                        int x = 36 + a.at_x * 24, y = 84 + a.at_y * 20;
                        bool town = a.kind == area_kind::TOWN || a.kind == area_kind::GYM;
                        if(! covered(x, y, 16, 16) && ! cells.full())
                        {
                            bn::sprite_ptr c = bn::sprite_items::cell.create_sprite(sx(x + 8), sy(y + 8), hab ? 8 : town ? 7 : 6);
                            c.set_bg_priority(0);
                            if(hab)
                            {
                                habitats.push_back(cells.size());
                                habitat_base.push_back(town ? 7 : 6);
                            }
                            cells.push_back(c);
                        }
                        // Its connections (each once: to the right and down).
                        for(int k = 0; k < m.links_count && ! links.full(); ++k)
                        {
                            const link& l = m.links[k];
                            if(l.target < 0)
                            {
                                continue;
                            }
                            const area_info& b = *world_data::maps[l.target].area;
                            bool right = b.at_x > a.at_x, down = b.at_y > a.at_y;
                            if(b.at_x >= 100 || (! right && ! down))
                            {
                                continue;
                            }
                            int lx = right ? x + 17 : x + 4, ly = right ? y + 4 : y + 14;
                            if(! covered(lx, ly, 8, 8))
                            {
                                bn::sprite_ptr ln = bn::sprite_items::link.create_sprite(sx(lx + 4), sy(ly + 4), right ? 0 : 2);
                                ln.set_bg_priority(0);
                                ln.set_z_order(2);
                                links.push_back(ln);
                            }
                        }
                    }
                    if(! any)
                    {
                        u.win().box(window_style::WINDOW, box_x0 / 8, box_y0 / 8, (box_x1 - box_x0) / 8, (box_y1 - box_y0) / 8);
                        u.print(120 - u.width("AREA UNKNOWN") / 2, 113, "AREA UNKNOWN", text_color::INK, texts);
                    }
                }
                else
                {
                    // SIZE: its silhouette beside yours (1.6 m), the taller one filling the box.
                    u.win().box(window_style::WINDOW, 1, 3, 28, 16);
                    bn::string<40> cap("SIZE COMPARED TO ");
                    upper(cap, g.name);
                    u.print(120 - u.width(cap) / 2, 28, cap, text_color::INK, texts);
                    int mh = bn::max(int(sp.height), 1), ph = 16, big = bn::max(mh, ph);
                    sprite = sp.front.create_sprite(sx(76), sy(140 - 32 * mh / big));
                    sprite->set_scale(bn::max(bn::fixed(0.1), bn::fixed(mh) / big * bn::fixed(1.4)));
                    sprite->set_bg_priority(0);
                    bn::sprite_palette_ptr sp_pal = sprite->palette();
                    sp_pal.set_fade(bn::color(0, 0, 0), 1);
                    you = bn::sprite_items::person_player.create_sprite(sx(168), sy(140 - 32 * ph / big));
                    you->set_scale(bn::max(bn::fixed(0.2), bn::fixed(ph) / big * 3));
                    you->set_bg_priority(0);
                    bn::sprite_palette_ptr you_pal = you->palette();
                    you_pal.set_fade(bn::color(0, 0, 0), 1);
                }
                redraw = false;
            }
            // Habitats pulse (the web's 1.07 s daPulse): lit, then dim, every 32 frames.
            if(page == 1)
            {
                ++timer;
                for(int h = 0; h < habitats.size(); ++h)
                {
                    cells[habitats[h]].set_tiles(bn::sprite_items::cell.tiles_item(), (timer / 32) % 2 ? habitat_base[h] : 8);
                }
            }
            frame();
            if(registering)
            {
                if(bn::keypad::a_pressed() || bn::keypad::b_pressed())
                {
                    audio::play(audio::sfx::SELECT);
                    break;
                }
                continue;
            }
            if(bn::keypad::b_pressed())
            {
                audio::play(audio::sfx::SELECT);
                break;
            }
            if(bn::keypad::left_pressed() || bn::keypad::right_pressed())
            {
                tab = (tab + (bn::keypad::left_pressed() ? 3 : 1)) % 4;
                audio::play(audio::sfx::SELECT);
                redraw = true;
            }
            else if(bn::keypad::a_pressed())
            {
                if(tab == 3)
                {
                    audio::play(audio::sfx::SELECT);
                    break;
                }
                // SIZE needs the Pokémon to be caught; otherwise the failure buzz.
                if(tab == 2 && ! owned)
                {
                    audio::play(audio::sfx::BUMP);
                    continue;
                }
                audio::play(audio::sfx::SELECT);
                page = tab;
                redraw = true;
            }
            else if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
            {
                // Up/Down jump to the previous / next species seen.
                int step = bn::keypad::up_pressed() ? -1 : 1;
                for(int i = index + step; i >= 0 && i < list_count; i += step)
                {
                    if(g.seen.test(game_data::dex_order[i]))
                    {
                        index = i;
                        redraw = true;
                        audio::play(audio::sfx::SELECT);
                        break;
                    }
                }
            }
        }
        texts.clear();
        cells.clear();
        u.win().clear_all();
    }
}

void dex_screen(int register_species)
{
    game_state& g = state();
    ui& u = gui();
    ui::fade_out(8);
    u.win().clear_all();
    // The list runs from No. 1 to the highest species seen.
    int list_count = 0;
    for(int i = 0; i < game_data::dex_order_count; ++i)
    {
        if(g.seen.test(game_data::dex_order[i]))
        {
            list_count = i + 1;
        }
    }
    if(! list_count)
    {
        return;
    }
    if(register_species >= 0)
    {
        for(int i = 0; i < list_count; ++i)
        {
            if(game_data::dex_order[i] == register_species)
            {
                ui::fade_in(8);
                int index = i;
                dex_entry(index, list_count, true);
                ui::fade_out(8);
                return;
            }
        }
        return;
    }
    if(dex_index < 0 || dex_index >= list_count)
    {
        for(int i = 0; i < list_count; ++i)
        {
            if(g.seen.test(game_data::dex_order[i]))
            {
                dex_index = i;
                break;
            }
        }
    }
    bool faded = true;
    while(true)
    {
        bool leave = false;
        {
            bn::regular_bg_ptr bg = bn::regular_bg_items::dex_bg.create_bg(8, 48);
            bg.set_priority(3);
            bn::vector<bn::sprite_ptr, 80> texts;
            bn::vector<bn::sprite_ptr, 12> balls;
            bn::optional<bn::sprite_ptr> prev, cur, next;
            bool redraw = true;
            while(true)
            {
                if(redraw)
                {
                    texts.clear();
                    balls.clear();
                    for(int r = 0; r < 11; ++r)
                    {
                        int i = dex_index - 5 + r;
                        if(i < 0 || i >= list_count)
                        {
                            continue;
                        }
                        int s = game_data::dex_order[i];
                        const species& sp = game_data::species_list[s];
                        int y = -8 + r * 16 + 4;
                        bn::string<8> number("No");
                        if(sp.dex_number < 100) number.append("0");
                        if(sp.dex_number < 10) number.append("0");
                        number.append(bn::to_string<4>(sp.dex_number));
                        u.print(148, y, number, text_color::INK, texts, true);
                        int nx = 148 + u.width(number, true) + 3;
                        u.print_fit(nx, y, g.seen.test(s) ? bn::string_view(sp.name) : bn::string_view("----------"), 238 - nx,
                                    text_color::INK, texts, true);
                        if(g.owned.test(s) && ! balls.full())
                        {
                            bn::sprite_ptr b = bn::sprite_items::badge.create_sprite(sx(143), sy(y + 4), 1);
                            b.set_scale(bn::fixed(0.5));
                            b.set_bg_priority(0);
                            balls.push_back(b);
                        }
                    }
                    // In Calderra, its own POKéDEX: the Gen 8 and 9 species (species_v1 on).
                    bn::string<16> seen("SEEN ");
                    bn::string<16> own("OWN ");
                    if(map_region(g.map) >= 2)
                    {
                        // The SUNDERED ISLES' (3.0.0): its island forms (species_v2 on).
                        const int lo = map_region(g.map) == 2 ? species_v1 : species_v2;
                        const int hi = map_region(g.map) == 2 ? species_v2 : species_count;
                        int sn = 0, on = 0;
                        for(int i = lo; i < hi; ++i)
                        {
                            sn += g.seen.test(i);
                            on += g.owned.test(i);
                        }
                        seen.append(bn::to_string<4>(sn));
                        seen.append("/");
                        seen.append(bn::to_string<4>(hi - lo));
                        own.append(bn::to_string<4>(on));
                        own.append("/");
                        own.append(bn::to_string<4>(hi - lo));
                    }
                    else
                    {
                        seen.append(bn::to_string<4>(seen_count()));
                        own.append(bn::to_string<4>(g.owned.count()));
                    }
                    u.print(4, 132, seen, text_color::WHITE, texts, true);
                    u.print(4, 144, own, text_color::WHITE, texts, true);
                    auto viewer = [&](bn::optional<bn::sprite_ptr>& spr, int i, int y, bool small)
                    {
                        spr.reset();
                        if(i < 0 || i >= list_count)
                        {
                            return;
                        }
                        if(! g.seen.test(game_data::dex_order[i]))
                        {
                            // Not seen yet: a question mark (.dex-unk).
                            u.print(94, y - 8, "?", text_color::INK, texts);
                            return;
                        }
                        spr = game_data::species_list[game_data::dex_order[i]].front.create_sprite(sx(97), sy(y));
                        spr->set_bg_priority(2);
                        if(small)
                        {
                            spr->set_vertical_scale(bn::fixed(0.5));
                        }
                    };
                    viewer(prev, dex_index - 1, 30, true);
                    viewer(cur, dex_index, 80, false);
                    viewer(next, dex_index + 1, 130, true);
                    redraw = false;
                    if(faded)
                    {
                        ui::fade_in(8);
                        faded = false;
                    }
                }
                frame();
                int step = bn::keypad::up_pressed() ? -1 : bn::keypad::down_pressed() ? 1 : bn::keypad::left_pressed() ? -7 :
                           bn::keypad::right_pressed() ? 7 : 0;
                if(step)
                {
                    int to = bn::clamp(dex_index + step, 0, list_count - 1);
                    if(to != dex_index)
                    {
                        dex_index = to;
                        redraw = true;
                        audio::play(audio::sfx::SELECT);
                    }
                }
                else if(bn::keypad::b_pressed())
                {
                    audio::play(audio::sfx::SELECT);
                    leave = true;
                    break;
                }
                else if(bn::keypad::a_pressed() && g.seen.test(game_data::dex_order[dex_index]))
                {
                    audio::play(audio::sfx::SELECT);
                    break;
                }
            }
            ui::fade_out(6);
        }
        if(leave)
        {
            break;
        }
        ui::fade_in(6);
        dex_entry(dex_index, list_count, false);
        ui::fade_out(6);
        faded = true;
    }
    u.win().clear_all();
}

// ---------------------------------------------------------------------------------------------------
// OPTION: TEXT SPEED, SOUND, MUSIC, EXP SHARE, WEATHER; Left/Right change the value (the chosen one in red);
// B or CANCEL closes. Emerald plays no sounds on this page.
void option_screen()
{
    game_state& g = state();
    ui& u = gui();
    ui::fade_out(8);
    u.win().clear_all();
    bn::bg_palettes::set_transparent_color(bn::color(31, 31, 31));
    bn::vector<bn::sprite_ptr, 64> texts;
    bn::optional<bn::sprite_ptr> cursor;
    int row = 0;
    constexpr int rows = 5;
    constexpr const char* labels[] = { "TEXT SPEED", "SOUND", "MUSIC", "EXP SHARE", "WEATHER" };
    bool faded = true;
    while(true)
    {
        texts.clear();
        u.win().box(window_style::PAGE, 1, 1, 28, 4);
        u.print(16, 13, "OPTION", text_color::INK, texts);
        u.win().box(window_style::PAGE, 1, 6, 28, 13);
        auto values = [&](int r, const char* const* names, int count, int chosen)
        {
            int y = 56 + r * 16;
            if(count > 3)
            {
                bn::string<16> v("< ");
                v.append(names[chosen]);
                v.append(" >");
                u.print(220 - u.width(v), y, v, text_color::RED, texts);
                return;
            }
            int x = 220;
            for(int i = count - 1; i >= 0; --i)
            {
                x -= u.width(names[i]) + 8;
                u.print(x, y, names[i], i == chosen ? text_color::RED : text_color::INK, texts);
            }
        };
        constexpr const char* speeds[] = { "SLOW", "MID", "FAST" };
        constexpr const char* on_off[] = { "ON", "OFF" };
        constexpr const char* music[] = { "OFF", "LOW", "MID", "HIGH" };
        constexpr const char* weather[] = { "OFF", "LOW", "MED", "HIGH" };
        for(int r = 0; r < rows; ++r)
        {
            u.print(24, 56 + r * 16, labels[r], text_color::INK, texts);
        }
        values(0, speeds, 3, int(g.opt.speed));
        values(1, on_off, 2, g.opt.sound ? 0 : 1);
        values(2, music, 4, int(g.opt.music));
        values(3, on_off, 2, g.opt.exp_share ? 0 : 1);
        values(4, weather, 4, int(g.opt.weather));
        u.print(24, 56 + rows * 16, "CANCEL", text_color::INK, texts);
        cursor = bn::sprite_items::cursor.create_sprite(sx(14 + 4), sy(56 + row * 16 + 8));
        cursor->set_bg_priority(0);
        if(faded)
        {
            ui::fade_in(8);
            faded = false;
        }
        frame();
        while(! bn::keypad::any_pressed())
        {
            frame();
        }
        if(bn::keypad::b_pressed() || (bn::keypad::a_pressed() && row == rows))
        {
            break;
        }
        if(bn::keypad::up_pressed())
        {
            row = (row + rows) % (rows + 1);
        }
        else if(bn::keypad::down_pressed())
        {
            row = (row + 1) % (rows + 1);
        }
        else if(bn::keypad::left_pressed() || bn::keypad::right_pressed())
        {
            int d = bn::keypad::left_pressed() ? -1 : 1;
            switch(row)
            {
            case 0: g.opt.speed = text_speed((int(g.opt.speed) + 3 + d) % 3); break;
            case 1: g.opt.sound = ! g.opt.sound; break;
            case 2: g.opt.music = level4((int(g.opt.music) + 4 + d) % 4); break;
            case 3: g.opt.exp_share = ! g.opt.exp_share; break;
            case 4: g.opt.weather = level4((int(g.opt.weather) + 4 + d) % 4); break;
            default: break;
            }
        }
    }
    ui::fade_out(8);
    texts.clear();
    u.win().clear_all();
}

// ---------------------------------------------------------------------------------------------------
// TRAINER CARD (cardOpen): name, IDNo., money, POKéDEX, play time with a blinking colon, you, and the eight
// badges; A flips it over (areas visited, rival battles won, Pokémon in the boxes); B closes.
void card_screen()
{
    game_state& g = state();
    ui& u = gui();
    if(! g.trainer_id)
    {
        g.trainer_id = uint16_t(1 + rng().get_int(65535));
    }
    audio::play(audio::sfx::PC_LOGIN);
    ui::fade_out(8);
    u.win().clear_all();
    bn::regular_bg_ptr bg = bn::regular_bg_items::card_bg.create_bg(8, 48);
    bg.set_priority(3);
    bn::vector<bn::sprite_ptr, 48> texts;
    bn::vector<bn::sprite_ptr, 8> badges;
    bn::optional<bn::sprite_ptr> you;
    bool back = false;
    bool redraw = true;
    bool faded = true;
    int last_second = -1;
    while(true)
    {
        int seconds = int(g.play_frames / 60);
        if(redraw || (! back && seconds != last_second))
        {
            last_second = seconds;
            texts.clear();
            badges.clear();
            you.reset();
            u.print(18, 12, back ? g.name : "TRAINER CARD", text_color::INK, texts);
            auto row = [&](int i, const char* label, const bn::string_view& value)
            {
                int y = 34 + i * 18;
                u.print(22, y, label, text_color::INK, texts);
                u.print((back ? 214 : 138) - u.width(value), y, value, text_color::INK, texts);
            };
            if(! back)
            {
                bn::string<16> id("IDNo.");
                bn::string<8> digits = bn::to_string<6>(g.trainer_id);
                for(int pad = digits.size(); pad < 5; ++pad)
                {
                    id.append("0");
                }
                id.append(digits);
                u.print(214 - u.width(id, true), 14, id, text_color::INK, texts, true);
                row(0, "NAME", g.name);
                bn::string<16> money("$");
                money.append(bn::to_string<8>(g.money));
                row(1, "MONEY", money);
                row(2, "POKéDEX", bn::to_string<4>(g.owned.count()));
                bn::string<16> time(bn::to_string<6>(seconds / 3600));
                time.append(seconds % 2 ? " " : ":");
                int m = (seconds / 60) % 60;
                if(m < 10)
                {
                    time.append("0");
                }
                time.append(bn::to_string<4>(m));
                row(3, "TIME", time);
                you = bn::sprite_items::person_player.create_sprite(sx(186), sy(70));
                you->set_scale(2);
                you->set_bg_priority(1);
                // The eight gyms' badges.
                int k = 0;
                for(int i : world_data::area_maps)
                {
                    const map_def& a = world_data::maps[i];
                    if(a.gate != gate_kind::GYM || a.area->region != map_region(g.map))
                    {
                        continue;
                    }
                    bool on = a.leader_id >= 0 && g.beaten.test(a.leader_id);
                    bn::sprite_ptr b = bn::sprite_items::badge.create_sprite(sx(48 + k * 20), sy(134), on ? 1 : 0);
                    b.set_bg_priority(1);
                    badges.push_back(b);
                    ++k;
                }
            }
            else
            {
                int rivals = 0;
                for(int i : world_data::area_maps)
                {
                    const map_def& a = world_data::maps[i];
                    if(a.gate == gate_kind::RIVAL && a.leader_id >= 0 && g.beaten.test(a.leader_id))
                    {
                        ++rivals;
                    }
                    else if(a.area && (a.area->flags & area_flag::BOSS) && a.leader_id >= 0 && g.beaten.test(a.leader_id))
                    {
                        ++rivals;
                    }
                }
                row(0, "AREAS VISITED", bn::to_string<4>(g.visited.count()));
                row(1, "RIVAL BATTLES WON", bn::to_string<4>(rivals));
                row(2, "POKéMON IN BOXES", bn::to_string<4>(g.box_used()));
                if(g.run.nuzlocke())
                {
                    // The NUZLOCKE run so far.
                    row(3, "NUZLOCKE DEATHS", bn::to_string<6>(g.run.deaths));
                    row(4, "CATCHES", bn::to_string<6>(g.run.catches));
                    row(5, "ENCOUNTERS USED", bn::to_string<4>(g.run.encounter_used.count()));
                }
                else if(g.run.adventure)
                {
                    int left = 0;
                    for(int i = 0; i < game_data::legendaries_count; ++i)
                    {
                        left += ! g.owned.test(game_data::legendaries[i]);
                    }
                    row(3, "TOWER RANK", bn::to_string<4>(g.run.tower_clears + 1));
                    row(4, "BEST STREAK", bn::to_string<4>(g.run.tower_best));
                    row(5, "LEGENDARIES LEFT", bn::to_string<4>(left));
                }
            }
            redraw = false;
            if(faded)
            {
                ui::fade_in(8);
                faded = false;
            }
        }
        frame();
        if(bn::keypad::b_pressed())
        {
            break;
        }
        if(bn::keypad::a_pressed())
        {
            // A vertical squash, the other side, and back.
            for(int f = 8; f >= 0; --f)
            {
                bg.set_y(48 + (8 - f));
                frame();
            }
            back = ! back;
            redraw = true;
            bg.set_y(48);
        }
    }
    ui::fade_out(8);
    texts.clear();
    u.win().clear_all();
}

// ---------------------------------------------------------------------------------------------------
// The region map (renderMap, POKéNAV): every area on its grid cell with its connections; places you haven't
// been to show as unseen; where you are blinks. Move over the cells to read their names. With travel, A on
// any town or route you've been to (but where you are) offers to go there.
int region_map_screen(bool travel)
{
    int dest = -1;
    game_state& g = state();
    ui& u = gui();
    ui::fade_out(8);
    u.win().clear_all();
    bn::vector<bn::sprite_ptr, 64> cells;
    bn::vector<bn::sprite_ptr, 60> links;
    bn::vector<bn::sprite_ptr, 24> texts;
    bn::vector<bn::sprite_ptr, 24> used_marks;
    // The grid: x 0..7, y -2..3 (Calderra's: x 0..11, closer together).
    int here = g.map;
    if(world_data::maps[here].is_room())
    {
        here = world_data::maps[here].exit_map;
    }
    const int region = world_data::maps[here].area->region;
    // The SUNDERED ISLES' (3.0.0): x 0..5, drawn on a parchment treasure map.
    const int ox = region == 1 ? 30 : region == 3 ? 36 : 12, oy = 10, cw = region == 1 ? 24 : region == 3 ? 30 : 18, ch = 20;
    bn::bg_palettes::set_transparent_color(region == 3 ? bn::color(29, 25, 17) : bn::color(26, 28, 31));
    auto elsewhere = [&](const area_info& a)
    {
        return a.at_x >= 100 || a.region != region;     // under the sea, or another region
    };
    int cursor = here;
    auto cell_xy = [&](const area_info& a, int& x, int& y)
    {
        x = ox + a.at_x * cw;
        y = oy + (a.at_y + 2) * ch;
    };
    for(int i : world_data::area_maps)
    {
        const map_def& m = world_data::maps[i];
        const area_info& a = *m.area;
        if(elsewhere(a))
        {
            continue;
        }
        int x, y;
        cell_xy(a, x, y);
        bool seen = g.visited.test(i) || i == here;
        int frame_index = ! seen ? 4 : a.kind == area_kind::ROUTE ? 0 : a.kind == area_kind::GYM ? 1 : a.kind == area_kind::TOWN ? 2 : 3;
        bn::sprite_ptr c = bn::sprite_items::cell.create_sprite(sx(x + 8), sy(y + 8), frame_index);
        c.set_bg_priority(1);
        cells.push_back(c);
        if(g.run.nuzlocke() && g.run.encounter_used.test(i) && ! used_marks.full())
        {
            // NUZLOCKE: this area's encounter is spent.
            u.print(x + 6, y + 4, "x", text_color::RED, used_marks, true);
        }
        // Each link once (to the right and down), dashed while shut.
        for(int k = 0; k < m.links_count && ! links.full(); ++k)
        {
            const link& l = m.links[k];
            if(l.target < 0)
            {
                continue;
            }
            const area_info& b = *world_data::maps[l.target].area;
            if(elsewhere(b))
            {
                continue;
            }
            bool right = b.at_x > a.at_x, down = b.at_y > a.at_y;
            if(! right && ! down)
            {
                continue;
            }
            bool locked = l.gate && m.leader_id >= 0 && ! g.beaten.test(m.leader_id);
            int lx = right ? x + 17 : x + 4, ly = right ? y + 4 : y + 14;
            bn::sprite_ptr ln = bn::sprite_items::link.create_sprite(sx(lx + 4), sy(ly + 4), (right ? 0 : 2) + (locked ? 1 : 0));
            ln.set_bg_priority(2);
            links.push_back(ln);
        }
    }
    bn::sprite_ptr mark = bn::sprite_items::cell.create_sprite(0, 0, 5);
    mark.set_bg_priority(0);
    bn::sprite_ptr pick = bn::sprite_items::cursor.create_sprite(0, 0);
    pick.set_bg_priority(0);
    int timer = 0;
    bool redraw = true;
    ui::fade_in(8);
    while(true)
    {
        const area_info& ha = *world_data::maps[here].area;
        int hx, hy;
        cell_xy(ha, hx, hy);
        mark.set_position(sx(hx + 8), sy(hy + 8));
        mark.set_visible((timer / 30) % 2 == 0);
        const area_info& ca = *world_data::maps[cursor].area;
        int cx, cy;
        cell_xy(ca, cx, cy);
        pick.set_position(sx(cx - 4), sy(cy + 8));
        if(redraw)
        {
            texts.clear();
            u.win().box(window_style::WINDOW, 0, 17, 30, 3);
            const map_def& m = world_data::maps[cursor];
            bool seen = g.visited.test(cursor) || cursor == here;
            bn::string<64> name(seen ? bn::string_view(m.name) : bn::string_view("???"));
            bool cleared = (m.gate == gate_kind::GYM || m.gate == gate_kind::RIVAL) && m.leader_id >= 0 && g.beaten.test(m.leader_id);
            if(seen && cleared)
            {
                name.append("  (cleared)");
            }
            if(g.run.nuzlocke() && g.run.encounter_used.test(cursor))
            {
                name.append("  (encounter used)");
            }
            bool can_go = travel && seen && cursor != here;
            u.print_fit(8, 140, name, can_go ? 216 - u.width("A: TRAVEL", true) : 224, text_color::INK, texts);
            if(can_go)
            {
                u.print(232 - u.width("A: TRAVEL", true), 142, "A: TRAVEL", text_color::BLUE, texts, true);
            }
            redraw = false;
        }
        ++timer;
        frame();
        if(bn::keypad::a_pressed() && travel && cursor != here && g.visited.test(cursor))
        {
            bn::string<80> ask("Travel to ");
            ask.append(world_data::maps[cursor].name);
            ask.append("?");
            u.show_text(ask);
            bool yes = u.yes_no();
            u.clear_text();
            if(yes)
            {
                dest = cursor;
                break;
            }
            redraw = true;
            continue;
        }
        if(bn::keypad::b_pressed() || bn::keypad::start_pressed() || bn::keypad::select_pressed() || bn::keypad::a_pressed())
        {
            break;
        }
        int dx = bn::keypad::left_pressed() ? -1 : bn::keypad::right_pressed() ? 1 : 0;
        int dy = bn::keypad::up_pressed() ? -1 : bn::keypad::down_pressed() ? 1 : 0;
        if(dx || dy)
        {
            // The nearest area in that direction.
            int best = -1, best_d = 1 << 30;
            for(int i : world_data::area_maps)
            {
                const area_info& a = *world_data::maps[i].area;
                if(elsewhere(a) || i == cursor)
                {
                    continue;
                }
                int ddx = a.at_x - ca.at_x, ddy = a.at_y - ca.at_y;
                if((dx && ddx * dx <= 0) || (dy && ddy * dy <= 0))
                {
                    continue;
                }
                int d = (dx ? bn::abs(ddx) * 2 + bn::abs(ddy) * 5 : bn::abs(ddy) * 2 + bn::abs(ddx) * 5);
                if(d < best_d)
                {
                    best_d = d;
                    best = i;
                }
            }
            if(best >= 0)
            {
                cursor = best;
                redraw = true;
                audio::play(audio::sfx::SELECT);
            }
        }
    }
    ui::fade_out(8);
    texts.clear();
    u.win().clear_all();
    return dest;
}

// ---------------------------------------------------------------------------------------------------
// The Hall of Fame, then the credits roll (A continues at THE END).
void hall_of_fame_screen(const char* region)
{
    game_state& g = state();
    ui& u = gui();
    ui::fade_out(8);
    u.win().clear_all();
    bn::bg_palettes::set_transparent_color(bn::color(1, 2, 3));
    {
        bn::vector<bn::sprite_ptr, 6> hof;
        bn::vector<bn::sprite_ptr, 64> texts;
        u.text().set_center_alignment();
        u.print(120, 10, "HALL OF FAME", text_color::RED, texts);
        int n = bn::min(int(g.party_count), 6);
        for(int i = 0; i < n; ++i)
        {
            const mon& m = g.party[i];
            int x = 120 + (2 * i - (n - 1)) * 19;
            bn::sprite_ptr s = m.data().front.create_sprite(sx(x), sy(62));
            apply_shiny(s, m, mon_view::FRONT);
            s.set_scale(bn::fixed(0.6));
            s.set_bg_priority(1);
            hof.push_back(s);
            // (The small font is always drawn from the left: centre it by hand.)
            // Each name gets its column (38 px with six): smaller fonts when it's long, never cut.
            int column = n > 1 ? 36 : 120;
            int nw = u.fit_width(m.name(), column, true);
            u.print_fit(x - nw / 2, 86, m.name(), column, text_color::WHITE, texts, true);
            bn::string<8> lv("Lv");
            lv.append(bn::to_string<4>(m.level));
            u.print(x - u.width(lv, true) / 2, 96, lv, text_color::WHITE, texts, true);
        }
        bn::string<64> line;
        upper(line, g.name);
        line.append(" became the CHAMPION of ");
        line.append(region);
        line.append("!");
        int lw = bn::min(u.width(line, true), 232);
        u.print_fit(120 - lw / 2, 120, line, lw, text_color::WHITE, texts, true);
        u.text().set_left_alignment();
        ui::fade_in(16);
        wait(300);
        ui::fade_out(24);
    }
}

void credits_screen()
{
    game_state& g = state();
    ui& u = gui();
    audio::play_music("credits");
    hall_of_fame_screen("VELLORIN");
    {
        // The roll: up from the bottom, over 30 seconds.
        bn::string<64> champion;
        upper(champion, g.name);
        champion.append(", the new CHAMPION");
        bn::string<48> prof(game_data::prof_name);
        const char* roll[] = { "POKé LEGENDS", "LANDS OF NINE", "Vellorin", "", "STARRING", champion.c_str(), "WREN, rival and friend", prof.c_str(), "",
                               "THE GYM LEADERS", "RELL - SABLE - ORIN - ISKA", "JUNO - BRYN - HALE - CORVIN", "", "THE ELITE FOUR",
                               "MORROW - BRAKK - FERRIN - AURELLE", "", "ADMIN VESPER and TEAM TEMPEST", "and LUGIA, guardian of the sea and sky",
                               "", "Made by the Lands of Nine team", "", "Thank you to every playtester", "who pressed FEEDBACK." };
        constexpr int lines = int(sizeof(roll) / sizeof(roll[0]));
        constexpr int spacing = 18;
        bn::vector<bn::sprite_ptr, 96> texts;
        bn::vector<int, lines> line_first;
        for(int i = 0; i < lines; ++i)
        {
            line_first.push_back(texts.size());
            if(roll[i][0])
            {
                u.print(120 - u.width(roll[i], true) / 2, 0, roll[i], i == 0 ? text_color::RED : text_color::WHITE, texts, true);
            }
        }
        // Each line's sprites start at y 0; move them all with the roll.
        bn::vector<int, 96> line_of;
        for(int i = 0; i < lines; ++i)
        {
            int end = i + 1 < lines ? line_first[i + 1] : texts.size();
            for(int k = line_first[i]; k < end; ++k)
            {
                line_of.push_back(i);
            }
        }
        bn::vector<bn::fixed, 96> base_y;
        for(bn::sprite_ptr& t : texts)
        {
            base_y.push_back(t.y());
        }
        ui::fade_in(8);
        int total = 160 + lines * spacing;
        for(int f = 0; f <= 1800; ++f)
        {
            int offset = 160 - total * f / 1800;
            for(int k = 0; k < texts.size(); ++k)
            {
                int y = offset + line_of[k] * spacing;
                texts[k].set_y(base_y[k] + y);
                texts[k].set_visible(y > -16 && y < 170);
            }
            frame();
            if(bn::keypad::a_pressed() && f > 120)
            {
                break;
            }
        }
        texts.clear();
        u.text().set_center_alignment();
        u.print(120, 66, "THE END", text_color::RED, texts);
        bn::vector<bn::sprite_ptr, 8> press;
        u.print(120 - u.width("Press A", true) / 2, 92, "Press A", text_color::WHITE, press, true);
        u.text().set_left_alignment();
        int t = 0;
        while(! bn::keypad::a_pressed() && ! bn::keypad::b_pressed() && ! bn::keypad::start_pressed())
        {
            ++t;
            for(bn::sprite_ptr& p : press)
            {
                p.set_visible((t / 30) % 2 == 0);
            }
            frame();
        }
        ui::fade_out(16);
    }
    u.win().clear_all();
}

// ---------------------------------------------------------------------------------------------------
// The hidden editor (the tree by DUSKMERE HOLLOW's MART, then A, B, A at the lone tree above it): a party
// Pokémon's level, shininess, stats (each kept to Gen 3's range for its level: IV 0-31, EV 0-252, nature
// x0.9-x1.1) and moves (from what its species can learn). DONE keeps the changes; B leaves them.
namespace
{
    constexpr int editor_rows = 10;     // LEVEL, SHINY, six stats, MOVES, DONE

    uint16_t& stat_ref(mon& m, int k)
    {
        switch(k)
        {
        case 0: return m.max_hp;
        case 1: return m.atk;
        case 2: return m.def;
        case 3: return m.spa;
        case 4: return m.spd;
        default: return m.spe;
        }
    }

    int base_of(const mon& m, int k)
    {
        const base_stats& b = m.data().base;
        return k == 0 ? b.hp : k == 1 ? b.atk : k == 2 ? b.def : k == 3 ? b.spa : k == 4 ? b.spd : b.spe;
    }

    // The moves its species can know: its learnset at any level, and its hand-made moves.
    int learnable_moves(const mon& m, uint16_t* out, int max)
    {
        const species& s = m.data();
        int n = 0;
        auto add = [&](uint16_t mv)
        {
            for(int i = 0; i < n; ++i)
            {
                if(out[i] == mv)
                {
                    return;
                }
            }
            if(n < max)
            {
                out[n++] = mv;
            }
        };
        for(int i = 0; i < s.learnset_count; ++i)
        {
            add(s.learnset[i].move);
        }
        for(int i = 0; i < s.fixed_count; ++i)
        {
            add(s.fixed_moves[i]);
        }
        for(int i = 0; i < m.move_count; ++i)
        {
            add(m.move(i));
        }
        return n;
    }

    // MOVES: four slots; A on one picks from what it can learn, or (none) to clear it (one always stays).
    void edit_moves(mon& m)
    {
        ui& u = gui();
        static uint16_t pool[256];
        int pool_n = learnable_moves(m, pool, 256);
        while(true)
        {
            bn::string<24> labels[5];
            bn::string_view views[5];
            for(int i = 0; i < 4; ++i)
            {
                labels[i] = i < m.move_count ? bn::string_view(move_data(m.move(i)).name) : bn::string_view("-");
                views[i] = labels[i];
            }
            views[4] = "DONE";
            u.show_text("Which move slot?");
            int slot = u.list(views, 5);
            u.clear_text();
            if(slot < 0 || slot == 4)
            {
                return;
            }
            static bn::string_view names[257];
            names[0] = "(none)";
            for(int i = 0; i < pool_n; ++i)
            {
                names[i + 1] = move_data(pool[i]).name;
            }
            u.show_text("Teach which move?");
            menu_spec s;
            s.options = names;
            s.count = pool_n + 1;
            s.rows = 6;
            s.tx = 14; s.ty = 0; s.tw = 16; s.th = 14;
            int k = u.menu(s);
            u.clear_text();
            u.win().clear(14, 0, 16, 14);
            if(k < 0)
            {
                continue;
            }
            if(k == 0)
            {
                // Clear the slot (the others close up), but never the last move.
                if(slot < m.move_count && m.move_count > 1)
                {
                    m.remove_move(slot);
                }
                continue;
            }
            uint16_t mv = pool[k - 1];
            bool known = false;
            for(int i = 0; i < m.move_count; ++i)
            {
                known |= m.move(i) == mv && i != slot;
            }
            if(known)
            {
                continue;
            }
            if(slot < m.move_count)
            {
                m.set_move(slot, mv);
            }
            else
            {
                m.set_move(m.move_count++, mv);
            }
        }
    }
}

namespace
{
    void edit_one(int index);
}

// Pick a party Pokémon, edit it, and back to the party for the next, until CANCEL (or B).
void secret_editor_screen()
{
    while(true)
    {
        int index = party_screen(party_mode::CHOOSE, "Edit which POKéMON?");
        if(index < 0 || index >= state().party_count)
        {
            return;
        }
        edit_one(index);
    }
}

namespace
{
void edit_one(int index)
{
    game_state& g = state();
    ui& u = gui();
    mon m = g.party[index];
    for(int k = 0; k < 6; ++k)
    {
        uint16_t& v = stat_ref(m, k);
        v = uint16_t(bn::clamp(int(v), stat_min(base_of(m, k), k == 0, m.level), stat_max(base_of(m, k), k == 0, m.level)));
    }
    m.hp = bn::min(m.hp, m.max_hp);
    ui::fade_out(8);
    u.win().clear_all();
    bn::bg_palettes::set_transparent_color(bn::color(4, 6, 12));
    bn::vector<bn::sprite_ptr, 96> texts;
    bn::optional<bn::sprite_ptr> sprite;
    bn::optional<bn::sprite_ptr> cursor;
    int row = 0;
    bool redraw = true;
    bool faded = true;
    bool keep = false;
    constexpr const char* stat_names[] = { "HP", "ATTACK", "DEFENSE", "SP. ATK", "SP. DEF", "SPEED" };
    while(true)
    {
        if(redraw)
        {
            texts.clear();
            u.win().box(window_style::WINDOW, 0, 0, 30, 20);
            bn::string<32> title;
            upper(title, m.name());
            u.print(8, 5, "EDITOR", text_color::RED, texts);
            u.print_fit(8, 20, title, 84, text_color::INK, texts);
            sprite = m.data().front.create_sprite(sx(48), sy(68));
            apply_shiny(*sprite, m, mon_view::FRONT);
            sprite->set_bg_priority(0);
            // Its moves, in full, under it.
            for(int i = 0; i < m.move_count; ++i)
            {
                u.print_fit(8, 104 + i * 9, move_data(m.move(i)).name, 86, text_color::BLUE, texts, true);
            }
            u.print_fit(8, 146, "Left/Right: change   L/R: 10 at a time   A: open   B: leave", 224, text_color::INK, texts, true);
            for(int r = 0; r < editor_rows; ++r)
            {
                int y = 5 + r * 14;
                bn::string<32> value;
                bn::string<24> range;
                const char* label = "";
                if(r == 0)
                {
                    label = "LEVEL";
                    value = bn::to_string<4>(m.level);
                    range = "1-100";
                }
                else if(r == 1)
                {
                    label = "SHINY";
                    value = m.shiny() ? "YES" : "NO";
                }
                else if(r < 8)
                {
                    int k = r - 2;
                    label = stat_names[k];
                    value = bn::to_string<6>(stat_ref(m, k));
                    range = bn::to_string<6>(stat_min(base_of(m, k), k == 0, m.level));
                    range.append("-");
                    range.append(bn::to_string<6>(stat_max(base_of(m, k), k == 0, m.level)));
                }
                else if(r == 8)
                {
                    label = "MOVES";
                    value = "EDIT";
                }
                else
                {
                    label = "DONE";
                }
                u.print(103, y, label, row == r ? text_color::RED : text_color::INK, texts, true);
                if(r < 8)
                {
                    bn::string<32> shown("<");
                    shown.append(value);
                    shown.append(">");
                    u.print(154, y, shown, text_color::BLUE, texts, true);
                    if(! range.empty())
                    {
                        u.print_fit(192, y, range, 40, text_color::INK, texts, true);
                    }
                }
                else if(r == 8)
                {
                    u.print(156, y, value, text_color::BLUE, texts, true);
                }
            }
            cursor = bn::sprite_items::cursor.create_sprite(sx(97), sy(5 + row * 14 + 4));
            cursor->set_bg_priority(0);
            redraw = false;
            if(faded)
            {
                ui::fade_in(8);
                faded = false;
            }
        }
        frame();
        if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
        {
            row = (row + (bn::keypad::up_pressed() ? editor_rows - 1 : 1)) % editor_rows;
            audio::play(audio::sfx::SELECT);
            redraw = true;
            continue;
        }
        int step = bn::keypad::left_pressed() ? -1 : bn::keypad::right_pressed() ? 1 : bn::keypad::l_pressed() ? -10 :
                   bn::keypad::r_pressed() ? 10 : 0;
        if(step && row < 8)
        {
            if(row == 0)
            {
                int old_level = m.level;
                m.level = uint16_t(bn::clamp(int(m.level) + step, 1, 200));
                if(m.level != old_level)
                {
                    m.xp = 0;
                    m.recalc_stats(old_level);
                }
            }
            else if(row == 1)
            {
                m.traits = uint8_t(m.traits ^ mon_trait::SHINY);
            }
            else
            {
                int k = row - 2;
                int lo = stat_min(base_of(m, k), k == 0, m.level), hi = stat_max(base_of(m, k), k == 0, m.level);
                uint16_t& v = stat_ref(m, k);
                v = uint16_t(bn::clamp(int(v) + step, lo, hi));
                m.traits = uint8_t(m.traits | mon_trait::EDITED);
                if(k == 0)
                {
                    m.hp = bn::min(m.hp, m.max_hp);
                }
            }
            audio::play(audio::sfx::SELECT);
            redraw = true;
            continue;
        }
        if(bn::keypad::a_pressed())
        {
            audio::play(audio::sfx::SELECT);
            if(row == 8)
            {
                sprite.reset();
                cursor.reset();
                texts.clear();
                edit_moves(m);
                u.win().clear_all();
                redraw = true;
            }
            else if(row == 9)
            {
                keep = true;
                break;
            }
            continue;
        }
        if(bn::keypad::b_pressed())
        {
            u.show_text("Leave without saving the changes?");
            bool yes = u.yes_no(false);
            u.clear_text();
            if(yes)
            {
                break;
            }
            redraw = true;
        }
    }
    ui::fade_out(8);
    texts.clear();
    sprite.reset();
    cursor.reset();
    u.win().clear_all();
    if(keep)
    {
        // Stats inside the range at its level; hit points no more than the max.
        for(int k = 0; k < 6; ++k)
        {
            uint16_t& v = stat_ref(m, k);
            v = uint16_t(bn::clamp(int(v), stat_min(base_of(m, k), k == 0, m.level), stat_max(base_of(m, k), k == 0, m.level)));
        }
        m.hp = bn::min(m.hp, m.max_hp);
        g.party[index] = m;
        g.mark_owned(m.species_index);
        save_game();
    }
}
}

}
