// Battles, on the web game's rules (js/app.js startWildBattle / startTrainerBattle / renderCmd / submitTurn /
// checkEnd): your whole party against a wild pack or a trainer's team. Every Pokémon of yours that can
// fight gets a command (FIGHT and a target, BAG, a look at the party, or RUN), then everyone acts in speed
// order (items first): type chart and STAB, accuracy, status, Poké Balls with the same catch odds, EXP for
// the whole party, level-ups, new moves, evolution and prize money.
// Layout: Emerald's (foe top right, yours bottom left, the dark message box with the white command box);
// with more than one Pokémon a side, sprites shrink and the HP boxes go compact, as the web game's do.
#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_unique_ptr.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_battle_bg.h"

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

    // Sprite sizes by count, from the web game's spriteSize() (player 170..80 px, foe 120..70 px).
    constexpr int own_scale_x100[] = { 100, 82, 71, 59, 53, 47 };
    constexpr int foe_scale_x100[] = { 100, 87, 77, 70, 63, 58 };

    struct fighter
    {
        mon* m = nullptr;
        bool own = false;
        bool caught = false;
        int shown_hp = 0;
        int base_x = 0, base_y = 0;     // sprite centre (screen)
        bn::optional<bn::sprite_ptr> sprite;
        bn::vector<bn::sprite_ptr, 10> hud_text;
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
        int order;
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

    // Integer cube root of x (0..1000), scaled by 1000: cbrt(x/1000) * 1000.
    int cbrt_scaled(int x)
    {
        int lo = 0, hi = 1000;
        while(lo < hi)
        {
            int mid = (lo + hi + 1) / 2;
            if(int64_t(mid) * mid * mid <= int64_t(x) * 1000000)
            {
                lo = mid;
            }
            else
            {
                hi = mid - 1;
            }
        }
        return lo;
    }

    class battle
    {

    public:
        explicit battle(const encounter& e) :
            _e(e),
            _bg(bn::regular_bg_items::battle_bg.create_bg(8, 48))
        {
            _bg.set_priority(3);
        }

        battle_outcome run();

    private:
        const encounter& _e;
        bn::regular_bg_ptr _bg;
        mon _foe_mons[max_foes];
        fighter _own[max_party];
        fighter _foes[max_foes];
        int _own_count = 0;
        int _foe_count = 0;
        const trainer* _trainer = nullptr;
        int _active = -1;           // own fighter being commanded (it bobs)
        int _bob = 0;
        int _last_command = 0;
        int _last_move[max_party] = {};

        [[nodiscard]] bn::string<24> label(const fighter& f) const
        {
            bn::string<24> s(f.own ? "" : _trainer ? "Foe " : "Wild ");
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

        void setup();
        void layout_sprites();
        void create_sprite(fighter& f);
        void draw_hud(fighter& f);
        void bob();
        static void bob_hook(void* self)
        {
            static_cast<battle*>(self)->bob();
        }

        bool command(choice* choices, bool& ran);
        int pick_target(const char* what);
        int pick_move(int own_index);
        void turn(choice* choices);
        void use_move(fighter& user, int move_index, fighter& target);
        bool can_act(fighter& f);
        void apply_status(fighter& target, status s, int chance);
        void throw_ball(fighter& user, fighter& target);
        void set_hp(fighter& f, int hp);
        void flash(fighter& f);
        void faint(fighter& f);
        void win();
        battle_outcome finish(battle_outcome outcome);
        void hide_sprites(bool hidden);
    };

    // ----- Setup -----
    void battle::setup()
    {
        game_state& g = state();
        bn::random& r = rng();
        if(_e.trainer)
        {
            // A route trainer: their team's first `party size` Pokémon, a level under your party's average.
            const map_def& m = world_data::maps[_e.map];
            _trainer = &m.trainers[_e.trainer_index];
            int lv = bn::max(3, bn::min(int(m.level_cap), g.average_level() - 1));
            _foe_count = bn::min(int(_trainer->team_count), bn::max(1, int(g.party_count)));
            _foe_count = bn::min(_foe_count, max_foes);
            for(int i = 0; i < _foe_count; ++i)
            {
                _foe_mons[i] = mon::make(_trainer->team[i], lv);
            }
        }
        else
        {
            _foe_count = _e.count;
            for(int i = 0; i < _foe_count; ++i)
            {
                _foe_mons[i] = mon::make(_e.species[i], _e.level);
            }
        }
        (void) r;
        for(int i = 0; i < _foe_count; ++i)
        {
            _foes[i].m = &_foe_mons[i];
            _foes[i].own = false;
            _foes[i].shown_hp = _foe_mons[i].hp;
            g.seen.set(_foe_mons[i].species_index);
        }
        _own_count = g.party_count;
        for(int i = 0; i < _own_count; ++i)
        {
            _own[i].m = &g.party[i];
            _own[i].own = true;
            _own[i].shown_hp = g.party[i].hp;
        }
        layout_sprites();
    }

    void battle::layout_sprites()
    {
        // Foes across the far platform (x 128-232), yours across the near one (x 0-112); every other one a
        // little lower, so a pack reads as a group.
        for(int i = 0; i < _foe_count; ++i)
        {
            fighter& f = _foes[i];
            f.base_x = 128 + (2 * i + 1) * 104 / (2 * _foe_count);
            f.base_y = 30 + (_foe_count > 1 ? (i % 2) * 8 : 0);
        }
        for(int i = 0; i < _own_count; ++i)
        {
            fighter& f = _own[i];
            int scale = own_scale_x100[_own_count - 1];
            f.base_x = 4 + (2 * i + 1) * 108 / (2 * _own_count);
            f.base_y = 112 - 32 * scale / 100 - (_own_count > 1 && i % 2 ? 0 : 4);
        }
        // HUD: Emerald's two big boxes for one-on-one, compact boxes otherwise.
        bool big = _own_count == 1 && _foe_count == 1;
        for(int i = 0; i < _foe_count; ++i)
        {
            fighter& f = _foes[i];
            f.hud_big = big;
            if(big)
            {
                f.hud_tx = 0; f.hud_ty = 1; f.hud_tw = 14; f.hud_th = 4;
            }
            else
            {
                f.hud_tx = (i % 2) * 7; f.hud_ty = (i / 2) * 2; f.hud_tw = 7; f.hud_th = 2;
            }
        }
        for(int i = 0; i < _own_count; ++i)
        {
            fighter& f = _own[i];
            f.hud_big = big;
            if(big)
            {
                f.hud_tx = 15; f.hud_ty = 8; f.hud_tw = 15; f.hud_th = 6;
            }
            else
            {
                f.hud_tx = 16 + (i % 2) * 7; f.hud_ty = 8 + (i / 2) * 2; f.hud_tw = 7; f.hud_th = 2;
            }
        }
    }

    void battle::create_sprite(fighter& f)
    {
        const species& s = f.m->data();
        f.sprite = (f.own ? s.back : s.front).create_sprite(sx(f.base_x), sy(f.base_y));
        f.sprite->set_bg_priority(2);
        int scale = f.own ? own_scale_x100[_own_count - 1] : foe_scale_x100[_foe_count - 1];
        if(scale != 100)
        {
            f.sprite->set_scale(bn::fixed(scale) / 100);
        }
        f.sprite->set_z_order(-f.base_y);
        f.sprite->set_visible(! f.out());
    }

    void battle::draw_hud(fighter& f)
    {
        ui& u = gui();
        f.hud_text.clear();
        u.win().box(window_style::HUD, f.hud_tx, f.hud_ty, f.hud_tw, f.hud_th);
        int x = f.hud_tx * 8, y = f.hud_ty * 8;
        bn::string<8> lv("Lv");
        lv.append(bn::to_string<4>(f.m->level));
        if(f.hud_big)
        {
            u.print(x + 8, y + 3, f.m->name(), text_color::HUD, f.hud_text);
            u.print(x + f.hud_tw * 8 - 8 - u.width(lv), y + 3, lv, text_color::HUD, f.hud_text);
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
            // Compact (the web game's .dense HP boxes): name and level on one line, the bar under them.
            int room = f.hud_tw * 8 - 11;
            bn::string<16> name(f.m->name());
            int lv_w = u.width(lv, true);
            if(u.width(name, true) + lv_w > room)
            {
                lv = bn::to_string<4>(f.m->level);     // just the number when it's tight
                lv_w = u.width(lv, true);
            }
            while(name.size() > 3 && u.width(name, true) + lv_w > room)
            {
                name.pop_back();
            }
            u.print(x + 4, y + 2, name, text_color::HUD, f.hud_text, true);
            u.print(x + f.hud_tw * 8 - 4 - lv_w, y + 2, lv, text_color::HUD, f.hud_text, true);
            draw_hp_bar(f.bar, x + 4, y + 10, 6, f.shown_hp, f.m->max_hp);
        }
        if(f.out())
        {
            f.bar.clear();
            f.hud_text.clear();
            u.win().clear(f.hud_tx, f.hud_ty, f.hud_tw, f.hud_th);
        }
    }

    void battle::bob()
    {
        ++_bob;
        for(int i = 0; i < _own_count; ++i)
        {
            fighter& f = _own[i];
            if(f.sprite)
            {
                int dy = i == _active ? ((_bob / 10) % 2) : 0;
                f.sprite->set_y(sy(f.base_y + dy));
            }
        }
    }

    void battle::hide_sprites(bool hidden)
    {
        // Full-screen menus need the sprite palettes, so the battlers step aside meanwhile.
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
                    create_sprite(f);
                    if(! f.out())
                    {
                        draw_hud(f);
                    }
                }
            }
        }
        if(hidden)
        {
            gui().win().clear_all();
        }
        else
        {
            gui().set_battle_style(true);
        }
    }

    // ----- Commands -----
    struct move_info_ctx
    {
        battle* b;
        const mon* m;
        bn::vector<bn::sprite_ptr, 8>* sprites;
    };

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
        bn::string<16> pwr;
        if(mv.category == move_category::STATUS)
        {
            pwr = "STATUS";
        }
        else
        {
            pwr = "PWR ";
            pwr.append(bn::to_string<4>(mv.power));
        }
        u.print(170, 119, pwr, text_color::INK, *c->sprites);
        u.print(170, 135, type_name(mv.type), text_color::INK, *c->sprites);
    }

    int battle::pick_move(int own_index)
    {
        ui& u = gui();
        const mon& m = *_own[own_index].m;
        bn::string_view names[4];
        for(int i = 0; i < m.move_count; ++i)
        {
            names[i] = move_data(m.moves[i]).name;
        }
        // Emerald: the moves in a 2x2 grid on the left, TYPE and power on the right.
        bn::vector<bn::sprite_ptr, 8> info;
        u.win().box(window_style::WINDOW, 20, 14, 10, 6);
        move_info_ctx ctx{ this, &m, &info };
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
        bn::string<24> names[max_foes];
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

    // Each Pokémon that can fight gets a command; B goes back to the previous one. Returns false if the
    // player ran.
    bool battle::command(choice* choices, bool& ran)
    {
        ui& u = gui();
        game_state& g = state();
        int queue[max_party];
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
            const mon& m = *_own[who].m;
            bn::string<48> prompt("What will ");
            prompt.append(m.name());
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
                int target = pick_target("Attack which one?");
                if(target < 0)
                {
                    continue;
                }
                choices[who] = { choice_kind::MOVE, mv, target, item_id::POTION };
                ++pos;
            }
            else if(c == 1)
            {
                hide_sprites(true);
                int it = bag_screen(bag_mode::BATTLE);
                hide_sprites(false);
                ui::fade_in(8);
                if(it < 0)
                {
                    continue;
                }
                item_id id = item_id(it);
                if(id == item_id::POKEBALL)
                {
                    if(_trainer)
                    {
                        u.say("The TRAINER blocked the BALL! Don't be a thief!");
                        continue;
                    }
                    int target = pick_target("Throw at which one?");
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
                hide_sprites(true);
                party_screen(party_mode::BATTLE);
                hide_sprites(false);
                ui::fade_in(8);
            }
            else
            {
                if(_trainer)
                {
                    u.say("No! There's no running from a TRAINER battle!");
                    continue;
                }
                u.say("Got away safely!");
                ran = true;
                _active = -1;
                return false;
            }
        }
        _active = -1;
        (void) g;
        return true;
    }

    // ----- The turn -----
    void battle::turn(choice* choices)
    {
        bn::random& r = rng();
        bn::vector<action, max_party + max_foes> actions;
        int order = 0;
        for(int i = 0; i < _own_count; ++i)
        {
            if(! _own[i].out())
            {
                actions.push_back({ &_own[i], &_foes[choices[i].target], choices[i], true, order++ });
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
            int targets[max_party], tn = 0;
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
            actions.push_back({ &f, &_own[targets[r.get_int(tn)]], c, false, order++ });
        }
        // Items first, then by speed (paralysis halves it); ties keep their order.
        auto speed = [](const action& a){ return a.user->m->st == status::PARALYSIS ? a.user->m->spe / 2 : int(a.user->m->spe); };
        for(int i = 1; i < actions.size(); ++i)
        {
            for(int j = i; j > 0; --j)
            {
                const action& a = actions[j - 1];
                const action& b = actions[j];
                bool a_item = a.c.kind == choice_kind::ITEM, b_item = b.c.kind == choice_kind::ITEM;
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
                game_state& g = state();
                int index = act.user->m - g.party.data();
                bn::string<80> message;
                bn::string<128> text("You used a ");
                text.append(game_data::items[int(act.c.item)].name);
                text.append("! ");
                if(use_item(act.c.item, index, message))
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
            // Its target already fainted: an attack moves on to another foe (tester #19).
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
            if(act.c.kind == choice_kind::BALL)
            {
                throw_ball(*act.user, *act.target);
                continue;
            }
            use_move(*act.user, act.user->m->moves[act.c.move], *act.target);
        }
        // End of turn: burn and poison, yours first.
        for(fighter* side : { _own, _foes })
        {
            int n = side == _own ? _own_count : _foe_count;
            for(int i = 0; i < n; ++i)
            {
                fighter& f = side[i];
                if(! f.out() && (f.m->st == status::BURN || f.m->st == status::POISON))
                {
                    set_hp(f, f.m->hp - f.m->max_hp / 12);
                    bn::string<64> text(label(f));
                    text.append(f.m->st == status::BURN ? " is hurt by its burn!" : " is hurt by poison!");
                    gui().say(text);
                    faint(f);
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
                return true;
            }
            text.append(" is fast asleep.");
            gui().say(text);
            return false;
        }
        if(m.st == status::FREEZE)
        {
            if(r.get_int(5) == 0)
            {
                m.st = status::NONE;
                text.append(" thawed out!");
                gui().say(text);
                return true;
            }
            text.append(" is frozen solid!");
            gui().say(text);
            return false;
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
        gui().say(text);
        if(rng().get_int(100) >= mv.accuracy)
        {
            text = label(user);
            text.append("'s attack missed!");
            gui().say(text);
            return;
        }
        if(mv.category == move_category::STATUS)
        {
            status had = target.m->st;
            apply_status(target, mv.inflicts, 100);
            if(target.m->st == had)
            {
                gui().say("But it failed!");
            }
            return;
        }
        damage_result r = calc_damage(*user.m, mv, *target.m, rng());
        flash(target);
        set_hp(target, target.m->hp - r.damage);
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

    void battle::throw_ball(fighter& user, fighter& target)
    {
        game_state& g = state();
        ui& u = gui();
        (void) user;
        if(! g.item_count(item_id::POKEBALL))
        {
            u.say("No POKé BALLS left!");
            return;
        }
        g.items[int(item_id::POKEBALL)] = uint8_t(g.items[int(item_id::POKEBALL)] - 1);
        bn::string<64> text(g.name);
        text.append(" used POKé BALL!");
        u.say(text);
        // The web game's odds: 0.95 - 0.7 x HP fraction, clamped to 0.1-0.95, x1.5 with a status; three
        // shake checks at the cube root.
        mon& m = *target.m;
        int chance = bn::clamp(950 - (700 * m.hp) / bn::max(1, int(m.max_hp)), 100, 950);
        if(m.st != status::NONE)
        {
            chance = bn::min(1000, chance * 3 / 2);
        }
        int per_shake = cbrt_scaled(chance);
        int shakes = 0;
        while(shakes < 3 && rng().get_int(1000) < per_shake)
        {
            ++shakes;
        }
        target.sprite->set_visible(false);
        for(int i = 0; i < bn::max(1, shakes); ++i)
        {
            u.show_text(i == 0 ? "..." : i == 1 ? "... ..." : "... ... ...");
            wait(30);
        }
        u.clear_text();
        if(shakes < 3)
        {
            target.sprite->set_visible(true);
            text = "Oh no! The wild ";
            text.append(m.name());
            text.append(" broke free!");
            u.say(text);
            return;
        }
        target.caught = true;
        draw_hud(target);
        text = "Gotcha! ";
        text.append(m.name());
        text.append(" was caught!");
        u.say(text);
        if(! g.owned.test(m.species_index))
        {
            g.owned.set(m.species_index);
            text = m.name();
            text.append("'s data was added to the POKéDEX.");
            u.say(text);
        }
        mon caught = m;
        caught.st = status::NONE;
        bool to_box = false;
        if(! g.add_mon(caught, to_box))
        {
            text = "There's no room for ";
            text.append(m.name());
            text.append(", so it was released.");
            u.say(text);
        }
        else if(to_box)
        {
            text = m.name();
            text.append(" was sent to the BOX (party full).");
            u.say(text);
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
    }

    void battle::flash(fighter& f)
    {
        for(int i = 0; i < 4; ++i)
        {
            f.sprite->set_visible(false);
            wait(4);
            f.sprite->set_visible(true);
            wait(4);
        }
    }

    void battle::faint(fighter& f)
    {
        if(! f.m->fainted() || ! f.sprite->visible())
        {
            return;
        }
        // Sink a little and blink out.
        for(int i = 0; i < 16; ++i)
        {
            if(i % 2 == 0)
            {
                f.sprite->set_y(f.sprite->y() + 1);
            }
            f.sprite->set_visible(i < 8 || i % 4 < 2);
            frame();
        }
        f.sprite->set_visible(false);
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
        // checkEnd(): 18 EXP per wild Pokémon (35 from a route trainer) to the whole party.
        int xp = _trainer ? 35 : 18 * _foe_count;
        bn::string<96> text;
        if(_trainer)
        {
            text = "You defeated ";
            text.append(_trainer->title);
            text.append("!");
            u.say(text);
            bn::string<128> after(_trainer->title);
            after.append(": \"");
            after.append(_trainer->after);
            after.append("\"");
            u.say(after);
            int top = 0;
            for(int i = 0; i < _foe_count; ++i)
            {
                top = bn::max(top, int(_foe_mons[i].level));
            }
            int prize = top * 20;
            g.money += uint32_t(prize);
            g.beaten.set(_trainer->id);
            text = g.name;
            text.append(" got $");
            text.append(bn::to_string<8>(prize));
            text.append(" for winning!");
            u.say(text);
        }
        text = "Your party gained ";
        text.append(bn::to_string<4>(xp));
        text.append(" EXP. Points!");
        u.say(text);
        for(int i = 0; i < g.party_count; ++i)
        {
            int before = g.party[i].species_index;
            g.party[i].grant_xp(xp, u);
            if(g.party[i].species_index != before)
            {
                g.owned.set(g.party[i].species_index);
                g.seen.set(g.party[i].species_index);
            }
        }
    }

    battle_outcome battle::finish(battle_outcome outcome)
    {
        gui().set_frame_hook(nullptr, nullptr);
        ui::fade_out(16);
        gui().set_battle_style(false);
        gui().win().clear_all();
        return outcome;
    }

    battle_outcome battle::run()
    {
        ui& u = gui();
        setup();
        for(int i = 0; i < _foe_count; ++i)
        {
            create_sprite(_foes[i]);
            draw_hud(_foes[i]);
        }
        u.set_battle_style(true);
        u.set_frame_hook(bob_hook, this);
        ui::fade_in(16);

        bn::string<96> text;
        if(_trainer)
        {
            text = _trainer->title;
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
                text.append(_foe_mons[i].name());
            }
            text.append(" appeared!");
        }
        u.say(text);
        // Your party comes out together.
        text = "Go! ";
        int sent = 0;
        for(int i = 0; i < _own_count; ++i)
        {
            create_sprite(_own[i]);
            if(! _own[i].out())
            {
                draw_hud(_own[i]);
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
            choice choices[max_party];
            bool ran = false;
            if(! command(choices, ran))
            {
                return finish(battle_outcome::RAN);
            }
            turn(choices);
            if(! alive(false))
            {
                bool caught_any = false;
                for(int i = 0; i < _foe_count; ++i)
                {
                    caught_any |= _foes[i].caught;
                }
                win();
                return finish(caught_any && ! _trainer ? battle_outcome::CAUGHT : battle_outcome::WON);
            }
            if(! alive(true))
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

battle_outcome battle_scene(const encounter& e)
{
    bn::bg_palettes::set_transparent_color(bn::color(26, 30, 24));
    battle_outcome outcome;
    {
        bn::unique_ptr<battle> b(new battle(e));
        outcome = b->run();
    }
    if(outcome == battle_outcome::WHITED_OUT)
    {
        // sendToCenter(): outside the POKéMON CENTER at home, healed.
        game_state& g = state();
        g.heal_party();
        const map_def& home = world_data::maps[0];
        g.map = 0;
        g.x = home.spawn_x;
        g.y = home.spawn_y;
        for(int i = 0; i < home.doors_count; ++i)
        {
            if(home.doors[i].kind == door_kind::CENTER)
            {
                g.x = home.doors[i].x;
                g.y = int8_t(home.doors[i].y + 1);
            }
        }
        g.facing = direction::DOWN;
    }
    return outcome;
}

}
