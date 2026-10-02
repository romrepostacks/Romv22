// FREE BATTLE (showSetup / goToDraft / renderDraft / confirmDraft): choose how many a side, draft yours from
// the whole dex (shuffled; search by name, filter by type, sort by dex order, name or total stats), give each
// a held item, and fight a random side at level 50.
#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "bn_unique_ptr.h"
#include "bn_vector.h"

#include "bn_sprite_items_cursor.h"

#include "pr_audio.h"
#include "pr_battle.h"
#include "pr_game_data.h"
#include "pr_icons.h"
#include "pr_scenes.h"
#include "pr_state.h"
#include "pr_ui.h"

namespace pr
{

namespace
{
    constexpr int free_level = 50;      // LEVEL
    constexpr int held_count = 6;

    struct draft
    {
        uint16_t pool[species_count];
        uint16_t shown[species_count];
        int shown_count = 0;
        int picked[6];
        held_item items[6];
        int picked_count = 0;
        int need_a = 3, need_b = 3;
        int type_filter = -1;
        int sort = 0;               // 0 dex (pool) order, 1 name, 2 total stats
        bn::string<16> search;
        bool adventure = false;     // NEW ADVENTURE MODE's draft: START begins the adventure
    };

    int total_stats(int s)
    {
        const base_stats& b = game_data::species_list[s].base;
        return b.hp + b.atk + b.def + b.spa + b.spd + b.spe;
    }

    bool name_less(int a, int b)
    {
        const char* x = game_data::species_list[a].name;
        const char* y = game_data::species_list[b].name;
        while(*x && *x == *y)
        {
            ++x;
            ++y;
        }
        return uint8_t(*x) < uint8_t(*y);
    }

    bool contains(const char* name, const bn::string<16>& search)
    {
        if(search.empty())
        {
            return true;
        }
        auto lower = [](char c){ return c >= 'A' && c <= 'Z' ? char(c + 32) : c; };
        for(const char* p = name; *p; ++p)
        {
            int k = 0;
            while(k < search.size() && p[k] && lower(p[k]) == lower(search[k]))
            {
                ++k;
            }
            if(k == search.size())
            {
                return true;
            }
        }
        return false;
    }

    void refilter(draft& d)
    {
        d.shown_count = 0;
        for(int i = 0; i < species_count; ++i)
        {
            int s = d.pool[i];
            const species& sp = game_data::species_list[s];
            if(d.type_filter >= 0 && sp.type1 != d.type_filter && sp.type2 != d.type_filter)
            {
                continue;
            }
            if(! contains(sp.name, d.search))
            {
                continue;
            }
            d.shown[d.shown_count++] = uint16_t(s);
        }
        if(d.sort)
        {
            // Insertion sort (a few hundred entries).
            for(int i = 1; i < d.shown_count; ++i)
            {
                uint16_t v = d.shown[i];
                int j = i;
                while(j > 0 && (d.sort == 1 ? name_less(v, d.shown[j - 1]) : total_stats(v) > total_stats(d.shown[j - 1])))
                {
                    d.shown[j] = d.shown[j - 1];
                    --j;
                }
                d.shown[j] = v;
            }
        }
    }

    // The setup page: how many a side (1-6 each), then DRAFT TEAM.
    bool setup(draft& d)
    {
        ui& u = gui();
        int row = 0;
        bn::vector<bn::sprite_ptr, 48> texts;
        bool faded = true;
        while(true)
        {
            texts.clear();
            u.win().box(window_style::WINDOW, 1, 1, 28, 18);
            u.print(16, 14, "FREE BATTLE", text_color::BLUE, texts);
            bn::string<16> a("< ");
            a.append(bn::to_string<4>(d.need_a));
            a.append(" >");
            bn::string<16> b("< ");
            b.append(bn::to_string<4>(d.need_b));
            b.append(" >");
            u.print(32, 40, "Your side size", text_color::INK, texts);
            u.print(200 - u.width(a), 40, a, text_color::RED, texts);
            u.print(32, 58, "Enemy side size", text_color::INK, texts);
            u.print(200 - u.width(b), 58, b, text_color::RED, texts);
            u.print(32, 76, "DRAFT TEAM", text_color::INK, texts);
            u.print(32, 94, "BACK", text_color::INK, texts);
            u.print(16, 120, "806 POKéMON in the pool. Any combo,", text_color::INK, texts, true);
            u.print(16, 132, "e.g. 2 vs 6, 4 vs 1.", text_color::INK, texts, true);
            bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(sx(22 + 4), sy(40 + row * 18 + 8));
            cursor.set_bg_priority(0);
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
            audio::play(audio::sfx::SELECT);
            if(bn::keypad::b_pressed())
            {
                return false;
            }
            if(bn::keypad::up_pressed())
            {
                row = (row + 3) % 4;
            }
            else if(bn::keypad::down_pressed())
            {
                row = (row + 1) % 4;
            }
            else if((bn::keypad::left_pressed() || bn::keypad::right_pressed()) && row < 2)
            {
                int& v = row == 0 ? d.need_a : d.need_b;
                v = (v - 1 + (bn::keypad::left_pressed() ? 5 : 1)) % 6 + 1;
            }
            else if(bn::keypad::a_pressed())
            {
                if(row == 2)
                {
                    return true;
                }
                if(row == 3)
                {
                    return false;
                }
            }
        }
    }

    // The draft list. Returns true with the side picked (START when it's full), false to go back.
    bool draft_list(draft& d)
    {
        ui& u = gui();
        // goToDraft(): the pool is shuffled each time.
        for(int i = 0; i < species_count; ++i)
        {
            d.pool[i] = uint16_t(i);
        }
        for(int i = species_count - 1; i > 0; --i)
        {
            int j = rng().get_int(i + 1);
            uint16_t t = d.pool[i];
            d.pool[i] = d.pool[j];
            d.pool[j] = t;
        }
        d.picked_count = 0;
        d.type_filter = -1;
        d.sort = 0;
        d.search.clear();
        refilter(d);
        constexpr int rows = 6;
        constexpr int header = 3;      // SEARCH, TYPE, SORT
        int index = header;
        int top = 0;
        bool redraw = true;
        bn::vector<bn::sprite_ptr, 96> texts;
        bn::vector<bn::sprite_ptr, rows> icons;
        bn::optional<bn::sprite_ptr> cursor;
        constexpr const char* sort_names[] = { "DEX ORDER", "NAME A-Z", "TOTAL STATS" };
        ui::fade_in(8);
        while(true)
        {
            int count = header + d.shown_count;
            if(redraw)
            {
                texts.clear();
                icons.clear();
                u.win().clear_all();
                u.win().box(window_style::WINDOW, 0, 0, 30, 3);
                bn::string<48> counter("Pick ");
                counter.append(bn::to_string<4>(d.need_a));
                counter.append(" for your side (");
                counter.append(bn::to_string<4>(d.picked_count));
                counter.append("/");
                counter.append(bn::to_string<4>(d.need_a));
                counter.append(")");
                u.print(8, 4, counter, text_color::INK, texts, true);
                const char* start_label = d.adventure ? "START: go" : "START: battle";
                u.print(234 - u.width(start_label, true), 13, start_label, text_color::BLUE, texts, true);
                u.win().box(window_style::WINDOW, 0, 3, 30, 14);
                if(index < top)
                {
                    top = index;
                }
                if(index >= top + rows)
                {
                    top = index - rows + 1;
                }
                for(int r = 0; r < rows && top + r < count; ++r)
                {
                    int i = top + r;
                    int y = 30 + r * 16;
                    if(i == 0)
                    {
                        bn::string<32> s("SEARCH: ");
                        s.append(d.search.empty() ? bn::string_view("(any)") : bn::string_view(d.search));
                        u.print(20, y, s, text_color::BLUE, texts, true);
                        continue;
                    }
                    if(i == 1)
                    {
                        bn::string<32> s("TYPE: ");
                        s.append(d.type_filter < 0 ? "ALL TYPES" : type_name(d.type_filter));
                        u.print(20, y, s, text_color::BLUE, texts, true);
                        continue;
                    }
                    if(i == 2)
                    {
                        bn::string<32> s("SORT: ");
                        s.append(sort_names[d.sort]);
                        u.print(20, y, s, text_color::BLUE, texts, true);
                        continue;
                    }
                    int s = d.shown[i - header];
                    const species& sp = game_data::species_list[s];
                    bn::sprite_ptr icon = sp.icon.create_sprite(sx(28), sy(y + 4));
                    icon.set_scale(bn::fixed(0.5));
                    icon.set_bg_priority(0);
                    icons.push_back(icon);
                    u.print_fit(40, y, sp.name, 80, text_color::INK, texts, true);
                    bn::string<24> t(type_name(sp.type1));
                    if(sp.type2 >= 0)
                    {
                        t.append("/");
                        t.append(type_name(sp.type2));
                    }
                    u.print_fit(124, y, t, 84, text_color::INK, texts, true);
                    for(int k = 0; k < d.picked_count; ++k)
                    {
                        if(d.picked[k] == s)
                        {
                            u.print(212, y, "OK", text_color::RED, texts, true);
                        }
                    }
                }
                u.win().box(window_style::WINDOW, 0, 17, 30, 3);
                if(index >= header && index - header < d.shown_count)
                {
                    const species& sp = game_data::species_list[d.shown[index - header]];
                    bn::string<160> ab(sp.abil.name);
                    ab.append(" - ");
                    ab.append(sp.abil.desc);
                    u.print_wrapped_fit(8, 139, 224, ab, 2, 9, text_color::INK, texts, true);
                }
                else
                {
                    u.print(8, 142, "A: change   B: back", text_color::INK, texts, true);
                }
                cursor = bn::sprite_items::cursor.create_sprite(sx(10 + 4), sy(30 + (index - top) * 16 + 6));
                cursor->set_bg_priority(0);
                redraw = false;
            }
            frame();
            if(bn::keypad::b_pressed())
            {
                audio::play(audio::sfx::SELECT);
                ui::fade_out(8);
                u.win().clear_all();
                return false;
            }
            if(bn::keypad::start_pressed())
            {
                if(d.picked_count == d.need_a)
                {
                    audio::play(audio::sfx::SELECT);
                    ui::fade_out(8);
                    u.win().clear_all();
                    return true;
                }
                audio::play(audio::sfx::BUMP);
                continue;
            }
            int step = bn::keypad::up_pressed() ? -1 : bn::keypad::down_pressed() ? 1 : bn::keypad::left_pressed() ? -rows :
                       bn::keypad::right_pressed() ? rows : 0;
            if(step)
            {
                index = bn::clamp(index + step, 0, count - 1);
                redraw = true;
                audio::play(audio::sfx::SELECT);
                continue;
            }
            if(! bn::keypad::a_pressed())
            {
                continue;
            }
            audio::play(audio::sfx::SELECT);
            if(index == 0)
            {
                texts.clear();
                icons.clear();
                cursor.reset();
                ui::fade_out(8);
                char text[16];
                if(u.keyboard("Search by name...", text, 12, d.search))
                {
                    d.search = text;
                }
                refilter(d);
                index = header;
                top = 0;
                redraw = true;
                ui::fade_in(8);
                continue;
            }
            if(index == 1)
            {
                d.type_filter = d.type_filter + 1 >= types_count ? -1 : d.type_filter + 1;
                refilter(d);
                redraw = true;
                continue;
            }
            if(index == 2)
            {
                d.sort = (d.sort + 1) % 3;
                refilter(d);
                redraw = true;
                continue;
            }
            // toggleDraft(): pick (with a held item) or unpick.
            int s = d.shown[index - header];
            int at = -1;
            for(int k = 0; k < d.picked_count; ++k)
            {
                if(d.picked[k] == s)
                {
                    at = k;
                }
            }
            if(at >= 0)
            {
                for(int k = at; k < d.picked_count - 1; ++k)
                {
                    d.picked[k] = d.picked[k + 1];
                    d.items[k] = d.items[k + 1];
                }
                --d.picked_count;
            }
            else if(d.picked_count < d.need_a)
            {
                bn::string_view options[held_count];
                for(int k = 0; k < held_count; ++k)
                {
                    options[k] = game_data::held_items[k].name;
                }
                u.show_text("Hold an item?");
                int item = u.list(options, held_count, 0, true);
                u.clear_text();
                d.picked[d.picked_count] = s;
                d.items[d.picked_count] = held_item(item < 0 ? 0 : item);
                ++d.picked_count;
            }
            redraw = true;
        }
    }
}

// NEW ADVENTURE MODE's draft (Phase 7): the same screen, `count` Pokémon with their items. False if B backs out.
bool draft_team(int count, uint16_t* species_out, held_item* items_out)
{
    bn::unique_ptr<draft> d(new draft());
    d->need_a = count;
    d->adventure = true;
    bn::bg_palettes::set_transparent_color(bn::color(7, 15, 21));
    if(! draft_list(*d))
    {
        return false;
    }
    for(int i = 0; i < d->picked_count; ++i)
    {
        species_out[i] = uint16_t(d->picked[i]);
        items_out[i] = d->items[i];
    }
    return true;
}

void free_battle_scene()
{
    bn::unique_ptr<draft> d(new draft());
    bn::bg_palettes::set_transparent_color(bn::color(7, 15, 21));
    while(true)
    {
        if(! setup(*d))
        {
            ui::fade_out(8);
            gui().win().clear_all();
            return;
        }
        ui::fade_out(8);
        gui().win().clear_all();
        if(draft_list(*d))
        {
            break;
        }
        ui::fade_in(8);
    }
    // confirmDraft(): your side at level 50 with their items; theirs drawn from the rest, with random items.
    bn::unique_ptr<battle_setup> s(new battle_setup());
    bn::unique_ptr<bn::array<mon, 6>> own(new bn::array<mon, 6>());
    for(int i = 0; i < d->picked_count; ++i)
    {
        (*own)[i] = mon::make(species_id(d->picked[i]), free_level, d->items[i]);
    }
    s->own = own->data();
    s->own_count = d->picked_count;
    s->free = true;
    int n = 0;
    while(n < d->need_b)
    {
        int c = rng().get_int(species_count);
        bool used = false;
        for(int i = 0; i < d->picked_count; ++i)
        {
            used |= d->picked[i] == c;
        }
        for(int i = 0; i < n; ++i)
        {
            used |= s->foes[i].species_index == c;
        }
        if(! used)
        {
            s->foes[n++] = mon::make(species_id(c), free_level, held_item(rng().get_int(held_count)));
        }
    }
    s->foe_count = n;
    run_battle(*s);
}

}
