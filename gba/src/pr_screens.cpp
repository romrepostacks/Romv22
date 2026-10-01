#include "pr_screens.h"

#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_string.h"

#include "bn_regular_bg_items_bag_bg.h"
#include "bn_regular_bg_items_party_bg.h"
#include "bn_regular_bg_items_summary_bg.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_hpbar.h"

#include "pr_game_data.h"
#include "pr_state.h"
#include "pr_ui.h"

namespace pr
{

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

bool use_item(item_id id, int party_index, bn::string<80>& message)
{
    game_state& g = state();
    const item_info& it = game_data::items[int(id)];
    mon& m = g.party[party_index];
    if(! g.item_count(id))
    {
        return false;
    }
    if(it.revive)
    {
        if(! m.fainted())
        {
            return false;
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

// ---------------------------------------------------------------------------------------------------
// Party screen (the web game's .pty, at GBA scale): the lead in a big box on the left, the rest in a column
// on the right, a message box and CANCEL along the bottom.
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
            return { 1, 3, 10, 8 };
        }
        if(k == cancel_index)
        {
            return { 23, 17, 7, 3 };
        }
        return { 12, 1 + (k - 1) * 3, 18, 3 };
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
            for(int k = 0; k < g.party_count; ++k)
            {
                const mon& m = g.party[k];
                slot_rect r = slot_of(k);
                window_style style = k == selected || k == swapping ? window_style::SLOT_ON :
                                     m.fainted() ? window_style::SLOT_FNT : window_style::SLOT;
                u.win().box(style, r.tx, r.ty, r.tw, r.th);
                int x = r.tx * 8, y = r.ty * 8;
                // Icon: the front sprite at half size.
                // The web game's party icons: half-size front sprites, top left of each box.
                bn::sprite_ptr icon = m.data().front.create_sprite(sx(x + (k ? 14 : 16)), sy(y + (k ? 10 : 14)));
                icon.set_scale(bn::fixed(0.5));
                icon.set_bg_priority(0);
                _icons.push_back(icon);
                bn::string<16> lv("Lv");
                lv.append(bn::to_string<4>(m.level));
                bn::string<16> hp(bn::to_string<4>(m.hp));
                hp.append("/");
                hp.append(bn::to_string<4>(m.max_hp));
                if(k == 0)
                {
                    u.print(x + 31, y + 5, m.name(), text_color::WHITE, _texts);
                    u.print(x + 37, y + 22, lv, text_color::WHITE, _texts, true);
                    draw_hp_bar(_bars[k], x + 8, y + 36, 8, m.hp, m.max_hp);
                    u.print(x + 74 - u.width(hp, true), y + 46, hp, text_color::WHITE, _texts, true);
                }
                else
                {
                    u.print(x + 30, y + 1, m.name(), text_color::WHITE, _texts);
                    u.print(x + 34, y + 14, lv, text_color::WHITE, _texts, true);
                    draw_hp_bar(_bars[k], x + 88, y + 6, 6, m.hp, m.max_hp);
                    u.print(x + 136 - u.width(hp, true), y + 13, hp, text_color::WHITE, _texts, true);
                }
            }
            slot_rect c = slot_of(cancel_index);
            u.win().box(selected == cancel_index ? window_style::SLOT_ON : window_style::SLOT, c.tx, c.ty, c.tw, c.th);
            u.print(c.tx * 8 + 8, c.ty * 8 + 4, "CANCEL", text_color::WHITE, _texts);
        }

        void message(const bn::string_view& text)
        {
            ui& u = gui();
            _message.clear();
            u.win().box(window_style::WINDOW, 0, 17, 23, 3);
            u.print(8, 140, text, text_color::INK, _message);
        }

    private:
        bn::regular_bg_ptr _bg;
        bn::vector<bn::sprite_ptr, max_party> _icons;
        bn::vector<bn::sprite_ptr, 48> _texts;
        bn::vector<bn::sprite_ptr, 12> _message;
        bn::vector<bn::sprite_ptr, 8> _bars[max_party];
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
        const char* base_prompt = prompt ? prompt : "Choose a POKéMON.";
        ui::fade_in(8);
        while(true)
        {
            if(redraw)
            {
                view->draw(selected, swapping);
                view->message(swapping >= 0 ? "Move to where?" : base_prompt);
                redraw = false;
            }
            frame();
            int n = g.party_count;
            if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
            {
                // Up/Down step through the Pokémon and CANCEL.
                int order_pos = selected == cancel_index ? n : selected;
                order_pos = (order_pos + (bn::keypad::up_pressed() ? n : 1)) % (n + 1);
                selected = order_pos == n ? cancel_index : order_pos;
                redraw = true;
            }
            else if(bn::keypad::left_pressed() && selected != 0 && selected != cancel_index)
            {
                selected = 0;
                redraw = true;
            }
            else if(bn::keypad::right_pressed() && selected == 0 && n > 1)
            {
                selected = 1;
                redraw = true;
            }
            else if(bn::keypad::b_pressed())
            {
                if(swapping >= 0)
                {
                    swapping = -1;
                    redraw = true;
                    continue;
                }
                break;
            }
            else if(bn::keypad::a_pressed())
            {
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
                bn::string_view options[] = { "SUMMARY", "SWITCH", "CANCEL" };
                bn::string_view battle_options[] = { "SUMMARY", "CANCEL" };
                menu_spec s;
                s.options = mode == party_mode::FIELD ? options : battle_options;
                s.count = mode == party_mode::FIELD ? 3 : 2;
                s.tw = 10;
                s.th = s.count * 2 + 2;
                s.tx = 20;
                s.ty = 17 - s.th;
                int pick = u.menu(s);
                if(pick == 0)
                {
                    view.reset();
                    summary_screen(selected);
                    view.emplace();
                    view->draw(selected, swapping);
                    view->message(base_prompt);
                    ui::fade_in(8);
                }
                else if(pick == 1 && mode == party_mode::FIELD && n > 1)
                {
                    swapping = selected;
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
// Summary (the web game's .sm): three pages, Left/Right to turn, Up/Down to change Pokémon.
void summary_screen(int index)
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
        bn::vector<bn::sprite_ptr, 64> texts;
        bn::vector<bn::sprite_ptr, 8> bar;
        constexpr const char* titles[] = { "POKéMON INFO", "POKéMON SKILLS", "BATTLE MOVES" };
        bool faded = true;
        while(true)
        {
            if(redraw)
            {
                const mon& m = g.party[index];
                texts.clear();
                u.win().clear_all();
                u.win().box(window_style::PAGE, 12, 3, 18, 17);
                u.print(8, 1, titles[page], text_color::WHITE, texts);
                u.print(180, 4, "< PAGE >", text_color::WHITE, texts, true);
                mon_sprite = m.data().front.create_sprite(sx(44), sy(56));
                mon_sprite->set_bg_priority(1);
                u.print(6, 90, m.name(), text_color::INK, texts);
                bn::string<16> lv("Lv");
                lv.append(bn::to_string<4>(m.level));
                u.print(6, 108, lv, text_color::INK, texts);
                draw_hp_bar(bar, 14, 128, 8, m.hp, m.max_hp);
                bn::string<24> row;
                auto line = [&](int i, const char* label, const bn::string_view& value)
                {
                    u.print(106, 30 + i * 18, label, text_color::INK, texts);
                    u.print(230 - u.width(value), 30 + i * 18, value, text_color::INK, texts);
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
                    line(1, "OT", g.name);
                    line(2, "EXP. POINTS", bn::to_string<8>(m.xp));
                    line(3, "NEXT LV.", bn::to_string<8>(bn::max(0, m.xp_next() - m.xp)));
                    line(4, "STATUS", m.fainted() ? "FAINTED" : m.st == status::NONE ? "OK" :
                         m.st == status::POISON ? "PSN" : m.st == status::BURN ? "BRN" :
                         m.st == status::PARALYSIS ? "PAR" : m.st == status::SLEEP ? "SLP" : "FRZ");
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
                        const move& mv = move_data(m.moves[i]);
                        u.print(106, 28 + i * 30, mv.name, text_color::INK, texts);
                        row = type_name(mv.type);
                        u.print(110, 43 + i * 30, row, text_color::INK, texts, true);
                        if(mv.power)
                        {
                            row = "PWR ";
                            row.append(bn::to_string<4>(mv.power));
                        }
                        else
                        {
                            row = "-";
                        }
                        u.print(230 - u.width(row, true), 43 + i * 30, row, text_color::INK, texts, true);
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
            else if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
            {
                index = (index + (bn::keypad::up_pressed() ? g.party_count - 1 : 1)) % g.party_count;
                redraw = true;
            }
        }
        ui::fade_out(8);
        u.win().clear_all();
    }
}

// ---------------------------------------------------------------------------------------------------
// Bag (the web game's .bag): pocket name top left, the list on the right, the description bottom left.
namespace
{
    constexpr const char* pocket_names[] = { "ITEMS", "POKé BALLS" };
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
        static int pocket = 1;
        int index = 0;
        bool redraw = true;
        bool faded = true;
        bn::vector<bn::sprite_ptr, 64> texts;
        bn::vector<bn::sprite_ptr, 16> desc;
        bn::optional<bn::sprite_ptr> cursor;
        bn::string<96> said;
        while(true)
        {
            int ids[items_count];
            int count = 0;
            for(int i = 0; i < items_count; ++i)
            {
                if(g.items[i] && game_data::items[i].pocket == pocket)
                {
                    ids[count++] = i;
                }
            }
            index = bn::min(index, count);     // count = CLOSE BAG
            if(redraw)
            {
                texts.clear();
                u.win().clear_all();
                u.win().box(window_style::BAG, 1, 1, 12, 3);
                u.win().box(window_style::BAG, 13, 1, 17, 14);
                u.win().box(window_style::BAG, 1, 15, 29, 5);
                bn::string<24> title("< ");
                title.append(pocket_names[pocket]);
                title.append(" >");
                u.print(52 - u.width(title) / 2, 12, title, text_color::INK, texts);
                for(int i = 0; i <= count && i < 6; ++i)
                {
                    int y = 14 + i * 16;
                    if(i == count)
                    {
                        u.print(122, y, "CLOSE BAG", text_color::INK, texts);
                        break;
                    }
                    const item_info& it = game_data::items[ids[i]];
                    u.print(122, y, it.name, text_color::INK, texts);
                    bn::string<8> n("x");
                    n.append(bn::to_string<4>(g.items[ids[i]]));
                    u.print(230 - u.width(n), y, n, text_color::INK, texts);
                }
                cursor = bn::sprite_items::cursor.create_sprite(sx(112 + 4), sy(14 + index * 16 + 8));
                cursor->set_bg_priority(0);
                desc.clear();
                bn::string_view lines[3];
                bn::string_view text = said.empty() ? (index < count ? game_data::items[ids[index]].desc : "Close the BAG.")
                                                    : bn::string_view(said);
                // In battle the web game's tip shows for POKé BALLS (renderCmd's bag view).
                if(said.empty() && mode == bag_mode::BATTLE && index < count && ids[index] == int(item_id::POKEBALL))
                {
                    text = "Weaken it first for a better catch rate!";
                }
                // Two lines of description, wrapped by words.
                int line = 0;
                const char* data = text.data();
                int start = 0, last_space = -1;
                for(int i = 0; i <= text.size() && line < 2; ++i)
                {
                    if(i == text.size() || data[i] == ' ')
                    {
                        if(u.width(bn::string_view(data + start, i - start)) > 216 && last_space > start)
                        {
                            lines[line++] = bn::string_view(data + start, last_space - start);
                            start = last_space + 1;
                        }
                        last_space = i;
                    }
                }
                if(line < 2 && start < text.size())
                {
                    lines[line++] = bn::string_view(data + start, text.size() - start);
                }
                for(int i = 0; i < line; ++i)
                {
                    u.print(14, 126 + i * 15, lines[i], text_color::INK, desc);
                }
                said.clear();
                redraw = false;
                if(faded)
                {
                    ui::fade_in(8);
                    faded = false;
                }
            }
            frame();
            if(bn::keypad::left_pressed() || bn::keypad::right_pressed())
            {
                pocket ^= 1;
                index = 0;
                redraw = true;
            }
            else if(bn::keypad::up_pressed() && index > 0)
            {
                --index;
                redraw = true;
            }
            else if(bn::keypad::down_pressed() && index < bn::min(count, 5))
            {
                ++index;
                redraw = true;
            }
            else if(bn::keypad::b_pressed() || (bn::keypad::a_pressed() && index == count))
            {
                break;
            }
            else if(bn::keypad::a_pressed())
            {
                item_id id = item_id(ids[index]);
                const item_info& it = game_data::items[int(id)];
                if(mode == bag_mode::BATTLE)
                {
                    result = int(id);
                    break;
                }
                if(it.pocket != 0)
                {
                    said = "This can't be used here.";
                    redraw = true;
                    continue;
                }
                // Medicine from the field: choose who (bagOpen).
                ui::fade_out(8);
                cursor.reset();
                texts.clear();
                desc.clear();
                bg.set_visible(false);
                int who = party_screen(party_mode::CHOOSE, "Use on which POKéMON?");
                bg.set_visible(true);
                bn::string<80> message;
                if(who >= 0)
                {
                    if(! use_item(id, who, message))
                    {
                        message = "It won't have any effect.";
                    }
                    said = message;
                }
                faded = true;
                redraw = true;
            }
        }
        ui::fade_out(8);
        u.win().clear_all();
    }
    return result;
}

// ---------------------------------------------------------------------------------------------------
// The PC: Emerald's storage, in one box.
void pc_screen()
{
    game_state& g = state();
    ui& u = gui();
    u.say("Booted up the PC.");
    while(true)
    {
        u.show_text("Which would you like to do?");
        constexpr bn::string_view options[] = { "WITHDRAW POKéMON", "DEPOSIT POKéMON", "LOG OFF" };
        int pick = u.list(options, 3);
        u.clear_text();
        if(pick == 0)
        {
            if(! g.box_count)
            {
                u.say("There are no POKéMON in the BOX.");
                continue;
            }
            if(g.party_count >= max_party)
            {
                u.say("Your party is full!");
                continue;
            }
            bn::string<24> labels[box_size];
            bn::string_view views[box_size];
            int shown = bn::min(int(g.box_count), 6);
            for(int i = 0; i < shown; ++i)
            {
                labels[i] = g.box[i].name();
                labels[i].append(" Lv");
                labels[i].append(bn::to_string<4>(g.box[i].level));
                views[i] = labels[i];
            }
            u.show_text("Withdraw which POKéMON?");
            int which = u.list(views, shown);
            u.clear_text();
            if(which >= 0)
            {
                g.party[g.party_count++] = g.box[which];
                for(int i = which; i < g.box_count - 1; ++i)
                {
                    g.box[i] = g.box[i + 1];
                }
                --g.box_count;
                bn::string<48> text(g.party[g.party_count - 1].name());
                text.append(" was taken out of the BOX.");
                u.say(text);
            }
        }
        else if(pick == 1)
        {
            if(g.party_count <= 1)
            {
                u.say("You can't deposit your last POKéMON!");
                continue;
            }
            if(g.box_count >= box_size)
            {
                u.say("The BOX is full.");
                continue;
            }
            bn::string<24> labels[max_party];
            bn::string_view views[max_party];
            for(int i = 0; i < g.party_count; ++i)
            {
                labels[i] = g.party[i].name();
                labels[i].append(" Lv");
                labels[i].append(bn::to_string<4>(g.party[i].level));
                views[i] = labels[i];
            }
            u.show_text("Deposit which POKéMON?");
            int which = u.list(views, g.party_count);
            u.clear_text();
            if(which >= 0)
            {
                mon m = g.party[which];
                if(g.able_count() - (m.fainted() ? 0 : 1) <= 0)
                {
                    u.say("That's your last POKéMON that can battle!");
                    continue;
                }
                for(int i = which; i < g.party_count - 1; ++i)
                {
                    g.party[i] = g.party[i + 1];
                }
                --g.party_count;
                g.box[g.box_count++] = m;
                bn::string<48> text(m.name());
                text.append(" was stored in the BOX.");
                u.say(text);
            }
        }
        else
        {
            return;
        }
    }
}

}
