// Battles, on the web game's rules (js/app.js startWildBattle / startTrainerBattle / renderCmd / submitTurn /
// checkEnd): your whole party against a wild pack or a trainer's team. Every Pokémon of yours that can fight
// gets a command (FIGHT and a target, the BAG, a look at the party, or RUN), then everyone acts in speed
// order (the BAG first): type chart and STAB, abilities, held items, accuracy, status, POKé BALLS, EXP (to
// everyone, or with EXP SHARE off to those who fought), level-ups, new moves, evolution and prize money.
// Layout: Emerald's (foes top right, yours bottom left, the dark message box with the white command box).
// As in the web game, only the Pokémon of yours that's acting is on screen; every one has an HP box.
#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_unique_ptr.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_battle_bg.h"
#include "bn_sprite_items_ball.h"

#include "pr_audio.h"
#include "pr_battle.h"
#include "pr_move_fx.h"
#include "pr_game_data.h"
#include "pr_scenes.h"
#include "pr_screens.h"
#include "pr_state.h"
#include "pr_ui.h"
#include "pr_world_data.h"

namespace pr
{

namespace
{
    constexpr int max_foes = 6;
    constexpr int max_own = 10;

    // Sprite sizes by count, from the web game's spriteSize() (foes 120..70 px).
    constexpr int foe_scale_x100[] = { 100, 87, 77, 70, 63, 58 };

    struct fighter
    {
        mon* m = nullptr;
        bool own = false;
        bool caught = false;
        int shown_hp = 0;
        int base_x = 0, base_y = 0;     // sprite centre (screen)
        bn::fixed scale = 1;
        bn::optional<bn::sprite_ptr> sprite;
        bn::vector<bn::sprite_ptr, 16> hud_text;
        bn::vector<bn::sprite_ptr, 8> bar;
        int hud_tx = 0, hud_ty = 0, hud_tw = 0, hud_th = 0;
        bool hud_big = false;

        [[nodiscard]] bool out() const
        {
            return m->fainted() || caught;
        }
    };

    enum class choice_kind
    {
        MOVE,
        BALL,
        ITEM
    };

    struct choice
    {
        choice_kind kind = choice_kind::MOVE;
        int move = 0;          // index into the user's moves
        int target = 0;        // foe index (MOVE / BALL)
        item_id item = item_id::POTION;
    };

    struct action
    {
        fighter* user;
        fighter* target;
        choice c;
        bool own;
    };

    const char* status_word(status s)
    {
        switch(s)
        {
        case status::POISON: return " was poisoned!";
        case status::BURN: return " was burned!";
        case status::PARALYSIS: return " is paralyzed! It may be unable to move!";
        case status::SLEEP: return " fell asleep!";
        case status::FREEZE: return " was frozen solid!";
        default: return "";
        }
    }

    const char* status_tag(status s)
    {
        switch(s)
        {
        case status::POISON: return "PSN";
        case status::BURN: return "BRN";
        case status::PARALYSIS: return "PAR";
        case status::SLEEP: return "SLP";
        case status::FREEZE: return "FRZ";
        default: return "";
        }
    }

    // Integer square root (pokeemerald's Sqrt).
    int isqrt(int x)
    {
        int r = 0;
        while((r + 1) * (r + 1) <= x)
        {
            ++r;
        }
        return r;
    }

    bool is_medicine(int id)
    {
        const item_info& it = game_data::items[id];
        return it.heal || it.cure != status::NONE || it.revive || it.full;
    }

    // musicWanted() in battle.
    const char* battle_music(const battle_setup& b)
    {
        if(b.legendary)
        {
            return "legend";
        }
        if(! b.opponent)
        {
            return b.free ? "trainer" : "wild";
        }
        const trainer& t = *b.opponent;
        switch(t.role)
        {
        case trainer_role::CHAMPION: return "champion";
        case trainer_role::ELITE:
        {
            constexpr const char* themes[] = { "e_dark", "e_fight", "e_steel", "e_psy" };
            return themes[bn::clamp(int(t.elite), 0, 3)];
        }
        case trainer_role::ROUTE:
        case trainer_role::JUNIOR:
            return "trainer";
        case trainer_role::LEADER:
        {
            const map_def& m = world_data::maps[t.area];
            for(int i = 0; i < m.doors_count; ++i)
            {
                const map_def& room = world_data::maps[m.doors[i].room];
                if(room.room && room.room->kind == room_kind::GYM)
                {
                    constexpr const char* themes[] = { "leader", "g_fire", "g_water", "g_ground", "g_ghost", "g_electric", "g_grass",
                                                       "g_ice", "g_dragon" };
                    return themes[int(room.room->theme)];
                }
            }
            return "leader";
        }
        default:
            return "leader";
        }
    }

    class battle
    {

    public:
        explicit battle(battle_setup& setup) :
            _s(setup),
            _bg(bn::regular_bg_items::battle_bg.create_bg(8, 48))
        {
            _bg.set_priority(3);
        }

        battle_report run();

    private:
        battle_setup& _s;
        bn::regular_bg_ptr _bg;
        fighter _own[max_own];
        fighter _foes[max_foes];
        int _own_count = 0;
        int _foe_count = 0;
        int _focus = 0;             // which of yours is on screen (setFocusA)
        int _active = -1;           // own fighter being commanded (it bobs)
        int _bob = 0;
        int _last_command = 0;
        int _last_move[max_own] = {};
        battle_report _report;
        bn::optional<bn::sprite_ptr> _own_sprite;
        int _own_sprite_index = -1;
        bool _animating = false;    // a move animation is moving the sprites (no bobbing)

        [[nodiscard]] bn::string<32> label(const fighter& f) const
        {
            bn::string<32> s(f.own ? "" : (_s.opponent || _s.free) ? "Foe " : "Wild ");
            s.append(f.m->name());
            return s;
        }
        [[nodiscard]] int alive(bool own) const
        {
            int n = 0;
            const fighter* side = own ? _own : _foes;
            for(int i = 0; i < (own ? _own_count : _foe_count); ++i)
            {
                n += ! side[i].out();
            }
            return n;
        }

        void layout();
        void show_focus(int index);
        void create_foe_sprite(fighter& f);
        void draw_hud(fighter& f);
        void bob();
        static void bob_hook(void* self)
        {
            static_cast<battle*>(self)->bob();
        }

        bool command(choice* choices, bool& ran);
        int pick_target(const char* what);
        int pick_move(int own_index);
        int pick_bag_item();
        void party_view();
        void turn(choice* choices);
        void use_move(fighter& user, int move_index, fighter& target);
        [[nodiscard]] fx_body body_of(fighter& f);

        // What plays once "X used MOVE!" is out (the web's atkFx, then the hit and the HP bar).
        struct used_ctx
        {
            battle* self;
            fighter* user;
            fighter* target;
            int move_index;
            int damage;         // -1: a status move
        };
        static void used_hook(void* p)
        {
            auto* c = static_cast<used_ctx*>(p);
            c->self->used(*c);
        }
        void used(const used_ctx& c);
        bool can_act(fighter& f);
        void apply_status(fighter& target, status s, int chance);
        void throw_ball(fighter& target);
        void ball_animation(fighter& target, int wobbles, bool caught);
        struct ball_ctx
        {
            battle* self;
            fighter* target;
            int wobbles;
            bool caught;
        };
        static void ball_hook(void* p)
        {
            auto* c = static_cast<ball_ctx*>(p);
            c->self->ball_animation(*c->target, c->wobbles, c->caught);
        }
        void set_hp(fighter& f, int hp);
        void flash(fighter& f);
        void faint(fighter& f);
        void end_of_turn();
        void win();
        battle_report finish(battle_outcome outcome);
        void hide_sprites(bool hidden);
        void focus_on(fighter& f);
    };

    void battle::layout()
    {
        // Foes across the far platform (x 128-232, a pack 136-232 beside its HP boxes); every other one a little lower, so a pack reads as a group.
        for(int i = 0; i < _foe_count; ++i)
        {
            fighter& f = _foes[i];
            f.base_x = _foe_count > 1 ? 136 + (2 * i + 1) * 96 / (2 * _foe_count) : 128 + 52;
            f.base_y = 30 + (_foe_count > 1 ? (i % 2) * 8 : 0);
            bool big = _foe_count == 1;
            f.hud_big = big;
            if(big)
            {
                f.hud_tx = 0; f.hud_ty = 1; f.hud_tw = 14; f.hud_th = 4;
            }
            else
            {
                f.hud_tx = (i % 2) * 8; f.hud_ty = (i / 2) * 2; f.hud_tw = 8; f.hud_th = 2;
            }
        }
        // Your HP boxes: one big box, or compact ones two to a row, standing on the message box.
        int rows = (_own_count + 1) / 2;
        for(int i = 0; i < _own_count; ++i)
        {
            fighter& f = _own[i];
            bool big = _own_count == 1;
            f.hud_big = big;
            f.base_x = 56;
            f.base_y = 80;
            if(big)
            {
                f.hud_tx = 15; f.hud_ty = 8; f.hud_tw = 15; f.hud_th = 6;
            }
            else
            {
                f.hud_tx = 16 + (i % 2) * 7; f.hud_ty = 14 - rows * 2 + (i / 2) * 2; f.hud_tw = 7; f.hud_th = 2;
            }
        }
    }

    // Only the one of yours that's acting is on screen (the web game's .bs-field.player .bmon.focus).
    void battle::show_focus(int index)
    {
        if(index < 0 || index >= _own_count)
        {
            return;
        }
        _focus = index;
        fighter& f = _own[index];
        if(_own_sprite_index != index || ! _own_sprite)
        {
            _own_sprite_index = index;
            _own_sprite = f.m->data().back.create_sprite(sx(56), sy(80));
            _own_sprite->set_bg_priority(2);
            _own_sprite->set_z_order(-80);
        }
        _own_sprite->set_visible(! f.out());
    }

    void battle::focus_on(fighter& f)
    {
        if(f.own)
        {
            show_focus(int(&f - _own));
        }
    }

    void battle::create_foe_sprite(fighter& f)
    {
        const species& s = f.m->data();
        f.sprite = s.front.create_sprite(sx(f.base_x), sy(f.base_y));
        f.sprite->set_bg_priority(2);
        int scale = foe_scale_x100[bn::min(_foe_count, 6) - 1];
        f.scale = bn::fixed(scale) / 100;
        if(scale != 100)
        {
            f.sprite->set_scale(f.scale);
        }
        f.sprite->set_z_order(-f.base_y);
        f.sprite->set_visible(! f.out());
    }

    void battle::draw_hud(fighter& f)
    {
        ui& u = gui();
        f.hud_text.clear();
        if(f.out() && ! f.own)
        {
            f.bar.clear();
            u.win().clear(f.hud_tx, f.hud_ty, f.hud_tw, f.hud_th);
            return;
        }
        u.win().box(window_style::HUD, f.hud_tx, f.hud_ty, f.hud_tw, f.hud_th);
        int x = f.hud_tx * 8, y = f.hud_ty * 8;
        bn::string<8> lv("Lv");
        lv.append(bn::to_string<4>(f.m->level));
        if(f.hud_big)
        {
            u.print_fit(x + 8, y + 3, f.m->name(), f.hud_tw * 8 - 20 - u.width(lv), text_color::HUD, f.hud_text);
            u.print(x + f.hud_tw * 8 - 8 - u.width(lv), y + 3, lv, text_color::HUD, f.hud_text);
            if(f.m->st != status::NONE)
            {
                u.print(x + 8, y + 19, status_tag(f.m->st), text_color::RED, f.hud_text, true);
            }
            draw_hp_bar(f.bar, x + f.hud_tw * 8 - 8 - 48, y + 20, 6, f.shown_hp, f.m->max_hp);
            if(f.own)
            {
                bn::string<16> hp(bn::to_string<4>(f.shown_hp));
                hp.append("/");
                hp.append(bn::to_string<4>(f.m->max_hp));
                u.print(x + f.hud_tw * 8 - 8 - u.width(hp), y + 27, hp, text_color::HUD, f.hud_text);
            }
        }
        else
        {
            // Compact (the web game's .dense HP boxes): name and level (or status) on one line, the bar under them.
            int room = f.hud_tw * 8 - 11;
            bn::string<16> name(f.m->name());
            bn::string<8> right = lv;
            bool st = f.m->st != status::NONE && ! f.m->fainted();
            if(st)
            {
                right = status_tag(f.m->st);
            }
            // Name and level (or status) side by side, the bar under them: the small font with "Lv43", then with
            // just "43", then the condensed font the same two ways.
            bn::string<8> number = st ? right : bn::string<8>(bn::to_string<4>(f.m->level));
            bool placed = false;
            for(int font = 0; font < 2 && ! placed; ++font)
            {
                for(int r = 0; r < 2 && ! placed; ++r)
                {
                    const bn::string<8>& rt = r ? number : right;
                    int rw = u.width(rt, true);
                    int nw = font ? u.narrow_width(name) : u.width(name, true);
                    if(nw <= room - rw - 2)
                    {
                        u.print_fit(x + 4, y + 2, name, font ? nw : room - rw - 2, text_color::HUD, f.hud_text, true);
                        u.print(x + f.hud_tw * 8 - 4 - rw, y + 2, rt, st ? text_color::RED : text_color::HUD, f.hud_text, true);
                        draw_hp_bar(f.bar, x + 4, y + 10, (f.hud_tw * 8 - 8) / 8, f.shown_hp, f.m->max_hp);
                        placed = true;
                    }
                }
            }
            if(! placed)
            {
                // A long name takes the whole top line (condensed); the level number or status goes beside a
                // shorter bar.
                int rw = u.narrow_width(number);
                u.print_fit(x + 4, y + 2, name, bn::min(u.narrow_width(name), f.hud_tw * 8 - 7), text_color::HUD,
                            f.hud_text, true);
                int segments = (f.hud_tw * 8 - 10 - rw) / 8;
                draw_hp_bar(f.bar, x + 4, y + 10, segments, f.shown_hp, f.m->max_hp);
                u.print_fit(x + f.hud_tw * 8 - 3 - rw, y + 8, number, rw, st ? text_color::RED : text_color::HUD, f.hud_text,
                            true);
            }
        }
    }

    void battle::bob()
    {
        ++_bob;
        if(_own_sprite && ! _animating)
        {
            int dy = _active >= 0 && _active == _focus ? ((_bob / 15) % 2) : 0;
            _own_sprite->set_y(sy(80 + dy));
        }
    }

    void battle::hide_sprites(bool hidden)
    {
        // Full-screen menus need the sprite palettes, so the battlers step aside meanwhile.
        if(hidden)
        {
            _own_sprite.reset();
            _own_sprite_index = -1;
        }
        for(fighter* side : { _own, _foes })
        {
            int n = side == _own ? _own_count : _foe_count;
            for(int i = 0; i < n; ++i)
            {
                fighter& f = side[i];
                if(hidden)
                {
                    f.sprite.reset();
                    f.bar.clear();
                    f.hud_text.clear();
                }
                else
                {
                    if(! f.own)
                    {
                        create_foe_sprite(f);
                    }
                    draw_hud(f);
                }
            }
        }
        if(hidden)
        {
            gui().win().clear_all();
        }
        else
        {
            show_focus(_focus);
            gui().set_battle_style(true);
        }
    }

    // ----- Commands -----
    struct move_info_ctx
    {
        const mon* m;
        bn::vector<bn::sprite_ptr, 16>* sprites;
    };

    // moveInfo(): TYPE, POWER, ACCURACY and the category.
    void show_move_info(void* p, int index)
    {
        auto* c = static_cast<move_info_ctx*>(p);
        ui& u = gui();
        c->sprites->clear();
        if(index >= c->m->move_count)
        {
            return;
        }
        const move& mv = move_data(c->m->moves[index]);
        // Each row: the label on the left, its value on the right (the web's .row spans).
        // Label on the left, value on the right; a pair too wide for the box drops to the condensed font.
        auto row = [&](int y, const bn::string_view& label, const bn::string_view& value)
        {
            constexpr int left = 166, right = 234;
            int vw = value.empty() ? 0 : u.fit_width(value, (right - left) / 2, true);
            int lw = u.print_fit(left, y, label, right - left - vw - 3, text_color::INK, *c->sprites, true);
            (void) lw;
            if(! value.empty())
            {
                u.print_fit(right - vw, y, value, vw, text_color::INK, *c->sprites, true);
            }
        };
        bn::string<24> type("TYPE/");
        type.append(type_name(mv.type));
        row(116, type, "");
        row(126, "POWER", mv.power ? bn::string_view(bn::to_string<4>(mv.power)) : bn::string_view("-"));
        row(136, "ACCURACY", bn::to_string<4>(mv.accuracy));
        row(146, mv.category == move_category::PHYSICAL ? "Physical" : mv.category == move_category::STATUS ? "Status" : "Special", "");
    }

    int battle::pick_move(int own_index)
    {
        ui& u = gui();
        const mon& m = *_own[own_index].m;
        // Two to a row like Emerald; a long name drops to a smaller font (the menu's fit_columns).
        bn::string_view names[4];
        for(int i = 0; i < m.move_count; ++i)
        {
            names[i] = move_data(m.moves[i]).name;
        }
        // Emerald: the moves in a 2x2 grid on the left, the move's details on the right.
        bn::vector<bn::sprite_ptr, 16> info;
        u.win().box(window_style::WINDOW, 20, 14, 10, 6);
        move_info_ctx ctx{ &m, &info };
        menu_spec s;
        s.options = names;
        s.count = m.move_count;
        s.tx = 0; s.ty = 14; s.tw = 20; s.th = 6;
        s.columns = 2;
        s.column_width = 72;
        s.start = bn::min(_last_move[own_index], m.move_count - 1);
        s.on_move = show_move_info;
        s.ctx = &ctx;
        int pick = u.menu(s);
        info.clear();
        u.win().clear(20, 14, 10, 6);
        u.set_battle_style(true);
        if(pick >= 0)
        {
            _last_move[own_index] = pick;
        }
        return pick;
    }

    int battle::pick_target(const char* what)
    {
        ui& u = gui();
        int idx[max_foes];
        bn::string<32> names[max_foes];
        bn::string_view views[max_foes];
        int n = 0;
        for(int i = 0; i < _foe_count; ++i)
        {
            if(! _foes[i].out())
            {
                idx[n] = i;
                names[n] = _foes[i].m->name();
                views[n] = names[n];
                ++n;
            }
        }
        if(n == 1)
        {
            return idx[0];
        }
        u.show_text(what, 80);
        menu_spec s;
        s.options = views;
        s.count = n;
        s.tx = 12; s.ty = 14; s.tw = 18; s.th = 6;
        s.columns = 2;
        s.column_width = 66;
        int pick = u.menu(s);
        u.clear_text();
        return pick < 0 ? -1 : idx[pick];
    }

    // The battle's BAG (renderCmd 'bag'): POKé BALLS against wild Pokémon, and your medicine. Returns an item
    // id, or -1.
    int battle::pick_bag_item()
    {
        ui& u = gui();
        game_state& g = state();
        bool wild = ! _s.opponent && ! _s.free;
        int ids[items_count + 1];
        bn::string<32> labels[items_count + 1];
        bn::string_view views[items_count + 1];
        int n = 0;
        if(wild)
        {
            ids[n] = int(item_id::POKEBALL);
            labels[n] = "POKé BALL x";
            labels[n].append(bn::to_string<4>(g.item_count(item_id::POKEBALL)));
            views[n] = labels[n];
            ++n;
        }
        if(! _s.free)
        {
            for(int i = 0; i < items_count; ++i)
            {
                if(is_medicine(i) && g.items[i])
                {
                    ids[n] = i;
                    labels[n] = game_data::items[i].name;
                    labels[n].append(" x");
                    labels[n].append(bn::to_string<4>(g.items[i]));
                    views[n] = labels[n];
                    ++n;
                }
            }
        }
        views[n] = "CANCEL";
        const char* tip = wild ? (g.item_count(item_id::POKEBALL) ? "Weaken it first for a better catch rate!" :
                                  "Out of POKé BALLS! Restock at a POKéMON CENTER.") :
                          "There's nothing in the BAG you can use here.";
        u.show_text(tip, 112);
        while(true)
        {
            menu_spec s;
            s.options = views;
            s.count = n + 1;
            s.rows = 2;
            s.tx = 15; s.ty = 14; s.tw = 15; s.th = 6;
            int k = u.menu(s);
            if(k < 0 || k == n)
            {
                u.clear_text();
                return -1;
            }
            if(ids[k] == int(item_id::POKEBALL) && ! g.item_count(item_id::POKEBALL))
            {
                continue;       // greyed out
            }
            u.clear_text();
            return ids[k];
        }
    }

    // POKéMON (renderCmd 'party'): everyone's HP, then back.
    void battle::party_view()
    {
        hide_sprites(true);
        party_screen(party_mode::BATTLE);
        hide_sprites(false);
        ui::fade_in(8);
    }

    // Each Pokémon that can fight gets a command; B goes back to the previous one. Returns false if the
    // player ran.
    bool battle::command(choice* choices, bool& ran)
    {
        ui& u = gui();
        int queue[max_own];
        int qn = 0;
        for(int i = 0; i < _own_count; ++i)
        {
            if(! _own[i].out())
            {
                queue[qn++] = i;
            }
        }
        int pos = 0;
        while(pos < qn)
        {
            int who = queue[pos];
            _active = who;
            show_focus(who);
            const mon& m = *_own[who].m;
            bn::string<48> prompt("What will ");
            for(const char* c = m.name(); *c; ++c)
            {
                prompt.push_back(*c >= 'a' && *c <= 'z' ? char(*c - 32) : *c);
            }
            prompt.append(" do?");
            if(qn > 1)
            {
                prompt.append(" ");
                prompt.append(bn::to_string<4>(pos + 1));
                prompt.append("/");
                prompt.append(bn::to_string<4>(qn));
            }
            u.show_text(prompt, 104);
            constexpr bn::string_view commands[] = { "FIGHT", "BAG", "POKéMON", "RUN" };
            menu_spec s;
            s.options = commands;
            s.count = 4;
            s.tx = 15; s.ty = 14; s.tw = 15; s.th = 6;
            s.columns = 2;
            s.column_width = 58;
            s.start = _last_command;
            s.cancel = pos > 0;
            int c = u.menu(s);
            u.clear_text();
            if(c < 0)
            {
                --pos;
                continue;
            }
            _last_command = c;
            if(c == 0)
            {
                int mv = pick_move(who);
                if(mv < 0)
                {
                    continue;
                }
                bn::string<48> what("Use ");
                what.append(move_data(m.moves[mv]).name);
                what.append(" on which one?");
                int target = pick_target(what.c_str());
                if(target < 0)
                {
                    continue;
                }
                choices[who] = { choice_kind::MOVE, mv, target, item_id::POTION };
                ++pos;
            }
            else if(c == 1)
            {
                int it = pick_bag_item();
                if(it < 0)
                {
                    continue;
                }
                item_id id = item_id(it);
                if(id == item_id::POKEBALL)
                {
                    int target = pick_target("Throw the POKé BALL at which one?");
                    if(target < 0)
                    {
                        continue;
                    }
                    choices[who] = { choice_kind::BALL, 0, target, id };
                }
                else
                {
                    choices[who] = { choice_kind::ITEM, 0, 0, id };
                }
                ++pos;
            }
            else if(c == 2)
            {
                party_view();
            }
            else
            {
                if(_s.free)
                {
                    u.show_text("Forfeit this battle?");
                    bool yes = u.yes_no(false);
                    u.clear_text();
                    if(yes)
                    {
                        ran = true;
                        _active = -1;
                        return false;
                    }
                    continue;
                }
                if(_s.opponent)
                {
                    u.say("No! There's no running from a Trainer battle!");
                    continue;
                }
                audio::play(audio::sfx::SELECT);
                u.say("Got away safely!");
                ran = true;
                _active = -1;
                return false;
            }
        }
        _active = -1;
        return true;
    }

    // ----- The turn (submitTurn) -----
    void battle::turn(choice* choices)
    {
        bn::random& r = rng();
        bn::vector<action, max_own + max_foes> actions;
        for(int i = 0; i < _own_count; ++i)
        {
            if(! _own[i].out())
            {
                actions.push_back({ &_own[i], &_foes[choices[i].target], choices[i], true });
            }
        }
        // Foes pick a random move and a random target among yours.
        for(int i = 0; i < _foe_count; ++i)
        {
            fighter& f = _foes[i];
            if(f.out() || ! alive(true))
            {
                continue;
            }
            int targets[max_own], tn = 0;
            for(int k = 0; k < _own_count; ++k)
            {
                if(! _own[k].out())
                {
                    targets[tn++] = k;
                }
            }
            choice c;
            c.kind = choice_kind::MOVE;
            c.move = r.get_int(f.m->move_count);
            actions.push_back({ &f, &_own[targets[r.get_int(tn)]], c, false });
        }
        // The BAG (medicine and POKé BALLS) first, then by speed (paralysis halves it); ties keep their order.
        auto speed = [](const action& a){ return a.user->m->st == status::PARALYSIS ? a.user->m->spe / 2 : int(a.user->m->spe); };
        for(int i = 1; i < actions.size(); ++i)
        {
            for(int j = i; j > 0; --j)
            {
                const action& a = actions[j - 1];
                const action& b = actions[j];
                bool a_item = a.c.kind != choice_kind::MOVE, b_item = b.c.kind != choice_kind::MOVE;
                bool swap = (b_item && ! a_item) || (a_item == b_item && speed(b) > speed(a));
                if(! swap)
                {
                    break;
                }
                action t = actions[j - 1];
                actions[j - 1] = actions[j];
                actions[j] = t;
            }
        }
        for(action& act : actions)
        {
            if(act.user->out() || ! alive(true) || ! alive(false))
            {
                continue;
            }
            if(act.c.kind == choice_kind::ITEM)
            {
                // The medicine goes to the Pokémon that was told to use it.
                focus_on(*act.user);
                bn::string<80> message;
                bn::string<128> text(state().name);
                text.append(" used a ");
                text.append(game_data::items[int(act.c.item)].name);
                text.append("! ");
                if(use_item_on(act.c.item, *act.user->m, message))
                {
                    text.append(message);
                    set_hp(*act.user, act.user->m->hp);
                }
                else
                {
                    text = "It won't have any effect.";
                }
                gui().say(text);
                continue;
            }
            // Its target already fainted (tester #19): an attack moves on to another foe.
            if(act.target->out())
            {
                if(act.c.kind == choice_kind::BALL)
                {
                    continue;
                }
                fighter* side = act.own ? _foes : _own;
                int n = act.own ? _foe_count : _own_count;
                act.target = nullptr;
                for(int i = 0; i < n && ! act.target; ++i)
                {
                    if(! side[i].out())
                    {
                        act.target = &side[i];
                    }
                }
                if(! act.target)
                {
                    continue;
                }
            }
            // focusOn(act.user) || focusOn(act.target)
            if(act.own)
            {
                focus_on(*act.user);
            }
            else
            {
                focus_on(*act.target);
            }
            if(act.c.kind == choice_kind::BALL)
            {
                throw_ball(*act.target);
                continue;
            }
            act.user->m->flags |= mon_flag::FOUGHT;
            use_move(*act.user, act.user->m->moves[act.c.move], *act.target);
        }
        end_of_turn();
    }

    // End of turn: burn and poison, then Leftovers and the Sitrus Berry; yours first.
    void battle::end_of_turn()
    {
        for(fighter* side : { _own, _foes })
        {
            int n = side == _own ? _own_count : _foe_count;
            for(int i = 0; i < n; ++i)
            {
                fighter& f = side[i];
                if(f.out() || ! alive(true) || ! alive(false))
                {
                    continue;
                }
                mon& m = *f.m;
                if(m.st == status::BURN || m.st == status::POISON)
                {
                    focus_on(f);
                    int d = m.max_hp / 12;
                    set_hp(f, m.hp - d);
                    bn::string<64> text(label(f));
                    text.append(m.st == status::BURN ? " is hurt by its burn!" : " is hurt by poison!");
                    gui().say(text);
                    faint(f);
                }
                if(m.item == held_item::LEFTOVERS && ! m.fainted())
                {
                    int h = m.max_hp * 6 / 100;
                    if(h > 0 && m.hp < m.max_hp)
                    {
                        set_hp(f, bn::min(int(m.max_hp), m.hp + h));
                        bn::string<64> text(label(f));
                        text.append(" restored HP with LEFTOVERS.");
                        gui().say(text);
                    }
                }
                if(m.item == held_item::SITRUS_BERRY && ! (m.flags & mon_flag::BERRY_USED) && ! m.fainted() && m.hp * 2 <= m.max_hp)
                {
                    int h = m.max_hp / 4;
                    m.flags |= mon_flag::BERRY_USED;
                    set_hp(f, bn::min(int(m.max_hp), m.hp + h));
                    bn::string<64> text(label(f));
                    text.append(" restored HP with its SITRUS BERRY!");
                    gui().say(text);
                }
            }
        }
    }

    bool battle::can_act(fighter& f)
    {
        bn::random& r = rng();
        mon& m = *f.m;
        bn::string<64> text(label(f));
        if(m.st == status::PARALYSIS && r.get_int(4) == 0)
        {
            text.append(" is paralyzed! It can't move!");
            gui().say(text);
            return false;
        }
        if(m.st == status::SLEEP)
        {
            if(m.sleep_turns)
            {
                --m.sleep_turns;
            }
            if(m.sleep_turns == 0)
            {
                m.st = status::NONE;
                text.append(" woke up!");
                gui().say(text);
                draw_hud(f);
            }
            else
            {
                text.append(" is fast asleep.");
                gui().say(text);
                return false;
            }
        }
        if(m.st == status::FREEZE)
        {
            if(r.get_int(5) == 0)
            {
                m.st = status::NONE;
                text.append(" thawed out!");
                gui().say(text);
                draw_hud(f);
            }
            else
            {
                text.append(" is frozen solid!");
                gui().say(text);
                return false;
            }
        }
        return true;
    }

    void battle::apply_status(fighter& target, status s, int chance)
    {
        if(target.m->st != status::NONE || target.out() || s == status::NONE || rng().get_int(100) >= chance)
        {
            return;
        }
        target.m->st = s;
        if(s == status::SLEEP)
        {
            target.m->sleep_turns = uint8_t(1 + rng().get_int(3));
        }
        bn::string<80> text(label(target));
        text.append(status_word(s));
        gui().say(text);
        draw_hud(target);
    }

    fx_body battle::body_of(fighter& f)
    {
        fx_body b;
        if(f.own)
        {
            b.sprite = _own_sprite && _own_sprite_index == &f - _own ? &*_own_sprite : nullptr;
            b.x = 56;
            b.y = 84;
            b.w = 48;
            b.h = 48;
        }
        else
        {
            b.sprite = f.sprite ? &*f.sprite : nullptr;
            b.x = f.base_x;
            b.y = f.base_y + 4;
            b.w = (f.scale * 48).integer();
            b.h = b.w;
        }
        return b;
    }

    void battle::used(const used_ctx& c)
    {
        _animating = true;
        play_move_fx(c.move_index, body_of(*c.user), body_of(*c.target));
        _animating = false;
        if(c.damage > 0)
        {
            audio::play(audio::sfx::HIT);
            flash(*c.target);
        }
        if(c.damage >= 0)
        {
            set_hp(*c.target, c.target->m->hp - c.damage);
        }
    }

    void battle::use_move(fighter& user, int move_index, fighter& target)
    {
        if(! can_act(user))
        {
            return;
        }
        const move& mv = move_data(move_index);
        bn::string<64> text(label(user));
        text.append(" used ");
        text.append(mv.name);
        text.append("!");
        if(rng().get_int(100) >= mv.accuracy)
        {
            gui().say(text);
            text = label(user);
            text.append("'s attack missed!");
            gui().say(text);
            return;
        }
        if(mv.category == move_category::STATUS)
        {
            used_ctx c{ this, &user, &target, move_index, -1 };
            gui().say(text, used_hook, &c);
            status had = target.m->st;
            apply_status(target, mv.inflicts, 100);
            if(target.m->st == had)
            {
                gui().say("But it failed!");
            }
            return;
        }
        // Disguise takes the first hit (and the web game shows only that).
        if(target.m->abil().kind == ability_kind::DISGUISE && ! (target.m->flags & mon_flag::DISGUISE_USED))
        {
            target.m->flags |= mon_flag::DISGUISE_USED;
            text = label(target);
            text.append("'s Disguise absorbed the hit - no damage!");
            gui().say(text);
            return;
        }
        damage_result r = calc_damage(*user.m, mv, *target.m, rng());
        int dmg = r.damage;
        mon& t = *target.m;
        bool sash = false;
        if(t.item == held_item::FOCUS_SASH && ! (t.flags & mon_flag::SASH_USED) && t.hp == t.max_hp && dmg >= t.hp)
        {
            dmg = t.hp - 1;
            t.flags |= mon_flag::SASH_USED;
            sash = true;
        }
        used_ctx c{ this, &user, &target, move_index, dmg };
        gui().say(text, used_hook, &c);
        if(sash)
        {
            text = label(target);
            text.append(" hung on using its FOCUS SASH!");
            gui().say(text);
        }
        if(r.effectiveness_x4 == 0)
        {
            text = "It doesn't affect ";
            text.append(label(target));
            text.append("...");
            gui().say(text);
        }
        else if(r.effectiveness_x4 > 4)
        {
            gui().say("It's super effective!");
        }
        else if(r.effectiveness_x4 < 4)
        {
            gui().say("It's not very effective...");
        }
        if(mv.secondary != status::NONE)
        {
            apply_status(target, mv.secondary, mv.secondary_chance);
        }
        faint(target);
    }

    void battle::throw_ball(fighter& target)
    {
        game_state& g = state();
        ui& u = gui();
        if(! g.item_count(item_id::POKEBALL))
        {
            u.say("No POKé BALLS left!");
            return;
        }
        g.items[int(item_id::POKEBALL)] = uint8_t(g.items[int(item_id::POKEBALL)] - 1);
        bn::string<64> text(g.name);
        text.append(" used POKé BALL!");
        mon& m = *target.m;
        int wobbles;
        bool caught;
        int max_hp = bn::max(1, int(m.max_hp));
        if(_s.legendary)
        {
            // The guardian: the web game's odds (0.95 - 0.7 x the HP left, x1.5 with a status, x0.35), each of
            // three shakes passing with the cube root of that.
            int chance = bn::clamp(950 - 700 * m.hp / max_hp, 100, 950);
            if(m.st != status::NONE)
            {
                chance = chance * 15 / 10;
            }
            chance = chance * 35 / 100;
            int per_shake = 0;
            while(per_shake < 1000 && (per_shake + 1) * (per_shake + 1) / 1000 * (per_shake + 1) / 1000 <= chance)
            {
                ++per_shake;
            }
            wobbles = 0;
            while(wobbles < 3 && rng().get_int(1000) < per_shake)
            {
                ++wobbles;
            }
            caught = wobbles == 3;
        }
        else
        {
            // Emerald's catch formula (pokeemerald CalcCatchOdds): the species' catch rate scaled by missing HP,
            // x2 asleep or frozen, x1.5 with another status; past 254 it's caught outright, otherwise four
            // checks against 1048560 / sqrt(sqrt(16711680 / odds)), and the ball wobbles once per check passed.
            int odds = m.data().capture_rate * (3 * max_hp - 2 * m.hp) / (3 * max_hp);
            if(m.st == status::SLEEP || m.st == status::FREEZE)
            {
                odds *= 2;
            }
            else if(m.st != status::NONE)
            {
                odds = odds * 15 / 10;
            }
            int checks = 0;
            if(odds > 254)
            {
                checks = 4;
            }
            else
            {
                int b = 1048560 / bn::max(1, isqrt(isqrt(16711680 / bn::max(1, odds))));
                while(checks < 4 && rng().get_int(65536) < b)
                {
                    ++checks;
                }
            }
            caught = checks == 4;
            wobbles = bn::min(checks, 3);
        }
        // The throw plays while "used POKé BALL!" is up (ballFx).
        ball_ctx bc{ this, &target, wobbles, caught };
        u.say(text, ball_hook, &bc);
        if(! caught)
        {
            text = "Oh no! The wild ";
            text.append(m.name());
            text.append(" broke free!");
            u.say(text);
            return;
        }
        target.caught = true;
        audio::play(audio::sfx::CAUGHT);
        draw_hud(target);
        text = "Gotcha! ";
        text.append(m.name());
        text.append(" was caught!");
        u.say(text);
        if(g.mark_owned(m.species_index))
        {
            text = m.species_name();
            text.append("'s data was added to the POKéDEX.");
            u.say(text);
            if(! _report.dex_new.full())
            {
                _report.dex_new.push_back(m.species_index);
            }
        }
        mon new_member = m;
        new_member.flags = 0;
        new_member.item = held_item::NONE;
        if(_s.legendary)
        {
            // Its battle HP was scaled up.
            int frac_hp = new_member.hp, frac_max = new_member.max_hp;
            new_member.max_hp = 0;
            new_member.recalc_stats();
            new_member.hp = uint16_t(bn::max(1, frac_hp * new_member.max_hp / bn::max(1, frac_max)));
        }
        int slot;
        if(! g.add_mon(new_member, slot))
        {
            text = "There's no room for ";
            text.append(m.name());
            text.append(", so it was released.");
            u.say(text);
            return;
        }
        if(slot >= 0)
        {
            text = m.name();
            text.append(" was sent to the Box (party full).");
            u.say(text);
        }
        if(! _report.caught_party.full())
        {
            _report.caught_party.push_back(int16_t(slot >= 0 ? 100 + slot : g.party_count - 1));
        }
    }

    // The throw (ballFx): the ball arcs over from your side, the Pokémon is drawn into it, the ball drops and
    // shakes once per check passed; caught, it clicks shut; not, it bursts open and the Pokémon is back.
    void battle::ball_animation(fighter& target, int wobbles, bool caught)
    {
        audio::play(audio::sfx::THROW);
        bn::sprite_ptr ball = bn::sprite_items::ball.create_sprite(sx(40), sy(90));
        ball.set_bg_priority(1);
        int tx = target.base_x, ty = target.base_y + 4;
        constexpr int flight = 33;
        for(int f = 1; f <= flight; ++f)
        {
            int x = 40 + (tx - 40) * f / flight;
            int y = 90 + (ty - 90) * f / flight - (f * (flight - f) * 48) / (flight * flight);
            ball.set_position(sx(x), sy(y));
            ball.set_rotation_angle((f * 22) % 360);
            frame();
        }
        audio::play(audio::sfx::BALL);
        ball.set_rotation_angle(0);
        for(int f = 12; f > 0; --f)
        {
            target.sprite->set_scale(target.scale * f / 12 + bn::fixed(0.01));
            frame();
        }
        target.sprite->set_visible(false);
        for(int f = 0; f < 16; ++f)
        {
            ball.set_y(ball.y() + (f < 10 ? 1 : f < 13 ? -1 : 1));
            frame();
        }
        for(int w = 0; w < wobbles; ++w)
        {
            wait(21);
            audio::play(audio::sfx::SHAKE);
            for(int f = 0; f < 27; ++f)
            {
                int a = f < 7 ? -f * 4 : f < 20 ? -28 + (f - 7) * 4 : 24 - (f - 20) * 4;
                ball.set_rotation_angle(a < 0 ? 360 + a : a);
                frame();
            }
            ball.set_rotation_angle(0);
        }
        wait(wobbles == 3 ? 15 : 27);
        if(caught)
        {
            return;
        }
        audio::play(audio::sfx::POP);
        ball.set_visible(false);
        target.sprite->set_visible(true);
        for(int f = 1; f <= 15; ++f)
        {
            target.sprite->set_scale(target.scale * f / 15);
            frame();
        }
    }

    // Animates the HP bar to the new value.
    void battle::set_hp(fighter& f, int hp)
    {
        hp = bn::clamp(hp, 0, int(f.m->max_hp));
        f.m->hp = uint16_t(hp);
        int step = bn::max(1, bn::abs(hp - f.shown_hp) / 24);
        while(f.shown_hp != hp)
        {
            f.shown_hp += f.shown_hp < hp ? bn::min(step, hp - f.shown_hp) : -bn::min(step, f.shown_hp - hp);
            draw_hud(f);
            frame();
        }
        draw_hud(f);
        if(f.own && _own_sprite && _own_sprite_index == &f - _own)
        {
            _own_sprite->set_visible(! f.out());
        }
    }

    // The hit blinks the Pokémon twice (hitBlink .5s steps(2) 2).
    void battle::flash(fighter& f)
    {
        bn::sprite_ptr* s = f.own ? (_own_sprite ? &*_own_sprite : nullptr) : (f.sprite ? &*f.sprite : nullptr);
        if(! s)
        {
            return;
        }
        for(int i = 0; i < 4; ++i)
        {
            s->set_visible(false);
            wait(8);
            s->set_visible(true);
            wait(7);
        }
    }

    void battle::faint(fighter& f)
    {
        if(! f.m->fainted())
        {
            return;
        }
        bn::sprite_ptr* s = f.own ? (_own_sprite && _own_sprite_index == &f - _own ? &*_own_sprite : nullptr) :
                                    (f.sprite ? &*f.sprite : nullptr);
        if(! f.own && (! s || ! s->visible()))
        {
            return;     // already gone
        }
        if(s && s->visible())
        {
            // It sinks and fades (.bmon.fainted: down 40%, transparent).
            int y0 = s->y().integer();
            for(int i = 0; i < 16; ++i)
            {
                s->set_y(y0 + i);
                s->set_visible(i < 8 || i % 4 < 2);
                frame();
            }
            s->set_visible(false);
            s->set_y(y0);
        }
        f.m->st = status::NONE;
        draw_hud(f);
        bn::string<48> text(label(f));
        text.append(" fainted!");
        gui().say(text);
    }

    void battle::win()
    {
        game_state& g = state();
        ui& u = gui();
        const trainer* t = _s.opponent;
        // checkEnd(): 18 EXP per wild Pokémon, 35 from a trainer, 60 from a Gym Leader; prize money is the top
        // level x 20 (route trainers, gym juniors, the Elite Four), x 60 (rivals, the Champion) or x 100
        // (Gym Leaders).
        bool leader = t && t->role == trainer_role::LEADER;
        bool route = t && (t->role == trainer_role::ROUTE || t->role == trainer_role::JUNIOR || t->role == trainer_role::ELITE);
        int xp = ! t ? 18 * _foe_count : leader ? 60 : 35;
        bn::string<128> text;
        if(t)
        {
            text = "You defeated ";
            if(leader)
            {
                text.append("Gym Leader ");
                text.append(world_data::maps[t->area].leader_name);
            }
            else if(route)
            {
                text.append(t->title);
            }
            else
            {
                text.append(world_data::maps[t->area].leader_name);
            }
            text.append("! (+");
            text.append(bn::to_string<4>(xp));
            text.append(" EXP. Points)");
            u.say(text);
            g.beaten.set(t->id);
            if(leader)
            {
                bn::string_view town(world_data::maps[t->area].place_name);
                if(town == "TIDALKEEP CITY" && ! g.item_count(item_id::HM03))
                {
                    g.story |= story::SURF_GIFT;
                }
                if(town == "RIMEFALL TOWN" && ! g.item_count(item_id::HM08))
                {
                    g.story |= story::DIVE_GIFT;
                }
            }
            if(t->vanish)
            {
                g.walk_off = int16_t(t->id);      // the rival says goodbye back on the overworld (afterStory)
            }
            int top = 0;
            for(int i = 0; i < _foe_count; ++i)
            {
                top = bn::max(top, int(_foes[i].m->level));
            }
            int prize = top * (leader ? 100 : route ? 20 : 60);
            g.money += uint32_t(prize);
            text = g.name;
            text.append(" got $");
            text.append(bn::to_string<8>(prize));
            text.append(" for winning!");
            u.say(text);
        }
        else
        {
            text = "The wild POKéMON retreated. (+";
            text.append(bn::to_string<4>(xp));
            text.append(" EXP. Points)");
            u.say(text);
        }
        // EXP SHARE off: only those who fought.
        for(int i = 0; i < g.party_count; ++i)
        {
            mon& m = g.party[i];
            if(! g.opt.exp_share && ! (m.flags & mon_flag::FOUGHT))
            {
                continue;
            }
            int before = m.species_index;
            m.grant_xp(xp, u);
            if(m.species_index != before)
            {
                g.mark_owned(m.species_index);
            }
        }
    }

    battle_report battle::finish(battle_outcome outcome)
    {
        gui().set_frame_hook(nullptr, nullptr);
        ui::fade_out(16);
        gui().set_battle_style(false);
        gui().win().clear_all();
        _report.outcome = outcome;
        return _report;
    }

    battle_report battle::run()
    {
        ui& u = gui();
        game_state& g = state();
        _foe_count = _s.foe_count;
        for(int i = 0; i < _foe_count; ++i)
        {
            _foes[i].m = &_s.foes[i];
            _foes[i].own = false;
            _foes[i].shown_hp = _s.foes[i].hp;
            if(! _s.free)
            {
                g.mark_seen(_s.foes[i].species_index);
            }
        }
        _own_count = _s.own_count;
        for(int i = 0; i < _own_count; ++i)
        {
            _own[i].m = &_s.own[i];
            _own[i].own = true;
            _own[i].shown_hp = _s.own[i].hp;
            _s.own[i].flags = uint8_t(_s.own[i].flags & ~mon_flag::FOUGHT);    // startBattleUI: nobody has fought yet
        }
        layout();
        for(int i = 0; i < _foe_count; ++i)
        {
            create_foe_sprite(_foes[i]);
            draw_hud(_foes[i]);
        }
        audio::play_music(battle_music(_s));
        u.set_battle_style(true);
        u.set_frame_hook(bob_hook, this);
        ui::fade_in(16);

        bn::string<128> text;
        const trainer* t = _s.opponent;
        if(_s.free)
        {
            text = "Battle start: ";
            text.append(bn::to_string<4>(_own_count));
            text.append(" vs ");
            text.append(bn::to_string<4>(_foe_count));
            text.append("!");
        }
        else if(t && (t->role == trainer_role::LEADER || t->role == trainer_role::RIVAL || t->role == trainer_role::CHAMPION))
        {
            // "Gym Leader Rell challenges you with 3 Pokémon! (Lv.9)"
            text = t->role == trainer_role::LEADER ? "Gym Leader " : "Rival ";
            text.append(world_data::maps[t->area].leader_name);
            text.append(" challenges you with ");
            text.append(bn::to_string<4>(_foe_count));
            text.append(" POKéMON! (Lv.");
            text.append(bn::to_string<4>(_s.foes[0].level));
            text.append(")");
        }
        else if(t)
        {
            text = t->title;
            text.append(" would like to battle!");
        }
        else
        {
            text = "Wild ";
            for(int i = 0; i < _foe_count; ++i)
            {
                if(i)
                {
                    text.append(i == _foe_count - 1 ? " and " : ", ");
                }
                text.append(_s.foes[i].name());
                text.append(" (Lv");
                text.append(bn::to_string<4>(_s.foes[i].level));
                text.append(")");
            }
            text.append(" appeared!");
        }
        u.say(text);
        // Your party: every HP box, and the first who can fight on screen.
        for(int i = 0; i < _own_count; ++i)
        {
            draw_hud(_own[i]);
        }
        int first = 0;
        for(int i = 0; i < _own_count; ++i)
        {
            if(! _own[i].out())
            {
                first = i;
                break;
            }
        }
        show_focus(first);
        text = "Go! ";
        int sent = 0;
        for(int i = 0; i < _own_count; ++i)
        {
            if(! _own[i].out())
            {
                if(sent < 3)
                {
                    if(sent)
                    {
                        text.append(", ");
                    }
                    text.append(_own[i].m->name());
                }
                ++sent;
            }
        }
        text.append(sent > 3 ? " and the rest!" : "!");
        u.say(text);

        while(true)
        {
            choice choices[max_own];
            bool ran = false;
            if(! command(choices, ran))
            {
                return finish(battle_outcome::RAN);
            }
            turn(choices);
            int a = alive(true), b = alive(false);
            if(_s.free && (! a || ! b))
            {
                audio::play_music("victory");
                u.say(a ? "Your side wins!" : b ? "Enemy side wins." : "Double knockout - draw!");
                return finish(a ? battle_outcome::WON : battle_outcome::WHITED_OUT);
            }
            if(! b)
            {
                bool caught_any = false;
                for(int i = 0; i < _foe_count; ++i)
                {
                    caught_any |= _foes[i].caught;
                }
                audio::play_music("victory");
                win();
                _report.trainer_beaten = t != nullptr;
                return finish(caught_any && ! t ? battle_outcome::CAUGHT : battle_outcome::WON);
            }
            if(! a)
            {
                u.say("Your party was defeated...");
                text = state().name;
                text.append(" whited out!");
                u.say(text);
                return finish(battle_outcome::WHITED_OUT);
            }
        }
    }
}

battle_report run_battle(battle_setup& setup)
{
    bn::bg_palettes::set_transparent_color(bn::color(26, 30, 24));
    battle_report report;
    {
        bn::unique_ptr<battle> b(new battle(setup));
        report = b->run();
    }
    return report;
}

}
