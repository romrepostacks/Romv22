// A wild battle, following the web game's rules (js/app.js submitTurn / damage / checkEnd): moves in
// speed order, type chart and STAB, accuracy, status (poison, burn, paralysis, sleep, freeze), Poké Balls
// with the same catch odds, running away, EXP for the whole party, level-ups, new moves and evolution.
// This build fights one-on-one with your lead; the web game's party-against-pack battles come next.
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_optional.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_battle_bg.h"
#include "bn_sprite_items_hpbar.h"

#include "pr_game_data.h"
#include "pr_scenes.h"
#include "pr_state.h"
#include "pr_ui.h"
#include "pr_world_data.h"

namespace pr
{

namespace
{
    constexpr int sx(int x)
    {
        return x - 120;
    }
    constexpr int sy(int y)
    {
        return y - 80;
    }

    // Layout (screen pixels), matching battle_bg from build_assets.py.
    constexpr int foe_x = 184, foe_y = 34;              // sprite centre
    constexpr int own_x = 58, own_y = 80;
    constexpr int foe_name_x = 10, foe_name_y = 9, foe_panel_right = 108, foe_bar_x = 36, foe_bar_y = 28;
    constexpr int own_name_x = 132, own_name_y = 70, own_panel_right = 232, own_bar_x = 164, own_bar_y = 88;

    struct panel
    {
        bn::vector<bn::sprite_ptr, 16> text;
        bn::vector<bn::sprite_ptr, 6> bar;
    };

    void draw_bar(panel& p, int x, int y, int hp, int max_hp)
    {
        // 48 px in six 8 px segments; green over half, yellow over a fifth, red below (hpClass).
        int fill = max_hp ? (hp * 48 + max_hp - 1) / max_hp : 0;
        if(hp > 0)
        {
            fill = bn::max(fill, 1);
        }
        int colour = hp * 2 > max_hp ? 0 : hp * 5 > max_hp ? 1 : 2;
        if(p.bar.empty())
        {
            for(int i = 0; i < 6; ++i)
            {
                p.bar.push_back(bn::sprite_items::hpbar.create_sprite(sx(x + 8 * i + 4), sy(y + 4), 0));
                p.bar.back().set_bg_priority(1);
            }
        }
        for(int i = 0; i < 6; ++i)
        {
            int seg = bn::clamp(fill - 8 * i, 0, 8);
            p.bar[i].set_tiles(bn::sprite_items::hpbar.tiles_item(), colour * 9 + seg);
        }
    }

    void draw_panel(ui& ui, panel& p, const mon& m, int shown_hp, bool own)
    {
        bn::sprite_text_generator& gen = ui.text_generator();
        p.text.clear();
        int name_x = own ? own_name_x : foe_name_x;
        int name_y = own ? own_name_y : foe_name_y;
        int right = own ? own_panel_right : foe_panel_right;
        gen.generate_top_left(name_x, name_y, m.name(), p.text);
        bn::string<8> lv("Lv");
        lv.append(bn::to_string<4>(m.level));
        gen.generate_top_left(right - gen.width(lv), name_y, lv, p.text);
        if(own)
        {
            bn::string<16> hp(bn::to_string<4>(shown_hp));
            hp.append("/");
            hp.append(bn::to_string<4>(m.max_hp));
            gen.generate_top_left(right - gen.width(hp), own_bar_y + 4, hp, p.text);
        }
        for(bn::sprite_ptr& s : p.text)
        {
            s.set_bg_priority(1);
        }
        draw_bar(p, own ? own_bar_x : foe_bar_x, own ? own_bar_y : foe_bar_y, shown_hp, m.max_hp);
    }

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
        battle(ui& ui, const encounter& wild) :
            _ui(ui),
            _foe(mon::make(wild.species, wild.level)),
            _bg(bn::regular_bg_items::battle_bg.create_bg(8, 48))
        {
            _bg.set_priority(3);
        }

        battle_outcome run()
        {
            game_state& g = state();
            _own_index = g.first_able();
            _foe_sprite = _foe.data().front.create_sprite(sx(foe_x), sy(foe_y));
            _foe_sprite->set_bg_priority(2);
            _foe_hp = _foe.hp;
            draw_panel(_ui, _foe_panel, _foe, _foe_hp, false);
            _ui.set_keep_box(true);
            ui::fade_in(16);

            bn::string<64> text("A wild ");
            text.append(_foe.name());
            text.append(" appeared!");
            _ui.say(text);
            _send_out();

            while(true)
            {
                mon& own = g.party[_own_index];
                text = "What will ";
                text.append(own.name());
                text.append(" do?");
                _ui.show_text(text);
                constexpr bn::string_view commands[] = { "FIGHT", "BALL", "RUN" };
                int command = _ui.menu(commands, 3, false, _last_command, false);
                _ui.clear_text();
                _last_command = command;

                if(command == 2)
                {
                    _ui.say("Got away safely!");
                    return _finish(battle_outcome::RAN);
                }
                int own_move = -1;
                bool throw_ball = false;
                if(command == 0)
                {
                    own_move = _pick_move(own);
                    if(own_move < 0)
                    {
                        continue;
                    }
                }
                else
                {
                    if(! g.poke_balls)
                    {
                        _ui.say("You don't have any POKé BALLS!");
                        continue;
                    }
                    throw_ball = true;
                }

                // Order by speed (paralysis halves it); the web game lets yours go first on a tie.
                int foe_move = _foe.moves[_random.get_int(_foe.move_count)];
                int own_speed = own.st == status::PARALYSIS ? own.spe / 2 : own.spe;
                int foe_speed = _foe.st == status::PARALYSIS ? _foe.spe / 2 : _foe.spe;
                bool own_first = own_speed >= foe_speed;
                for(int turn = 0; turn < 2; ++turn)
                {
                    bool own_turn = (turn == 0) == own_first;
                    mon& user = own_turn ? own : _foe;
                    mon& target = own_turn ? _foe : own;
                    if(user.fainted() || target.fainted())
                    {
                        continue;
                    }
                    if(own_turn && throw_ball)
                    {
                        if(_throw_ball())
                        {
                            return _finish(battle_outcome::CAUGHT);
                        }
                        continue;
                    }
                    _use_move(user, own_turn ? own_move : foe_move, target, own_turn);
                }
                // End of turn: burn and poison.
                for(int side = 0; side < 2; ++side)
                {
                    mon& m = side == 0 ? own : _foe;
                    if(! m.fainted() && (m.st == status::BURN || m.st == status::POISON))
                    {
                        int hurt = m.max_hp / 12;
                        _set_hp(m, m.hp - hurt, side == 0);
                        text = side == 0 ? "" : "Wild ";
                        text.append(m.name());
                        text.append(m.st == status::BURN ? " is hurt by its burn!" : " is hurt by poison!");
                        _ui.say(text);
                        _check_faint(m, side == 0);
                    }
                }
                if(_foe.fainted())
                {
                    _win();
                    return _finish(battle_outcome::WON);
                }
                if(own.fainted())
                {
                    _own_index = g.first_able();
                    if(_own_index < 0)
                    {
                        _ui.say("You have no more POKéMON that can fight!");
                        _ui.say("You whited out!");
                        return _finish(battle_outcome::WHITED_OUT);
                    }
                    _send_out();
                }
            }
        }

    private:
        ui& _ui;
        bn::random& _random = rng();
        mon _foe;
        int _foe_hp = 0;
        int _own_hp = 0;
        int _own_index = 0;
        int _last_command = 0;
        int _last_move = 0;
        bn::regular_bg_ptr _bg;
        bn::optional<bn::sprite_ptr> _foe_sprite;
        bn::optional<bn::sprite_ptr> _own_sprite;
        panel _foe_panel;
        panel _own_panel;

        void _send_out()
        {
            mon& own = state().party[_own_index];
            bn::string<48> text("Go! ");
            text.append(own.name());
            text.append("!");
            _own_sprite = own.data().back.create_sprite(sx(own_x), sy(own_y));
            _own_sprite->set_bg_priority(2);
            _own_hp = own.hp;
            draw_panel(_ui, _own_panel, own, _own_hp, true);
            _ui.say(text);
        }

        int _pick_move(const mon& own)
        {
            bn::string_view names[4];
            bn::string<40> hints[4];
            bn::string_view hint_views[4];
            for(int i = 0; i < own.move_count; ++i)
            {
                const move& mv = move_data(own.moves[i]);
                names[i] = mv.name;
                hints[i] = type_name(mv.type);
                if(mv.category == move_category::STATUS)
                {
                    hints[i].append("  STATUS");
                }
                else
                {
                    hints[i].append("  POWER ");
                    hints[i].append(bn::to_string<4>(mv.power));
                }
                hints[i].append("  ACC ");
                hints[i].append(bn::to_string<4>(mv.accuracy));
                hint_views[i] = hints[i];
            }
            int pick = _ui.menu(names, own.move_count, true, bn::min(_last_move, own.move_count - 1), true, hint_views);
            if(pick >= 0)
            {
                _last_move = pick;
                return own.moves[pick];
            }
            return -1;
        }

        // Animates the HP bar down (or up) to the new value.
        void _set_hp(mon& m, int new_hp, bool own)
        {
            new_hp = bn::clamp(new_hp, 0, int(m.max_hp));
            int& shown = own ? _own_hp : _foe_hp;
            int target = new_hp;
            m.hp = uint16_t(new_hp);
            int step = bn::max(1, bn::abs(target - shown) / 24);
            while(shown != target)
            {
                shown += shown < target ? bn::min(step, target - shown) : -bn::min(step, shown - target);
                if(own)
                {
                    draw_panel(_ui, _own_panel, m, shown, true);
                }
                else
                {
                    draw_bar(_foe_panel, foe_bar_x, foe_bar_y, shown, m.max_hp);
                }
                bn::core::update();
            }
        }

        void _flash(bool own)
        {
            bn::optional<bn::sprite_ptr>& s = own ? _own_sprite : _foe_sprite;
            for(int i = 0; i < 4; ++i)
            {
                s->set_visible(false);
                ui::wait(4);
                s->set_visible(true);
                ui::wait(4);
            }
        }

        void _check_faint(mon& m, bool own)
        {
            if(! m.fainted())
            {
                return;
            }
            // Sink a little and blink out (a full slide off the platform would cross the HP panels).
            bn::optional<bn::sprite_ptr>& s = own ? _own_sprite : _foe_sprite;
            for(int i = 0; i < 16; ++i)
            {
                if(i % 2 == 0)
                {
                    s->set_y(s->y() + 1);
                }
                s->set_visible(i < 8 || i % 4 < 2);
                bn::core::update();
            }
            s->set_visible(false);
            bn::string<48> text(own ? "" : "Wild ");
            text.append(m.name());
            text.append(" fainted!");
            _ui.say(text);
        }

        bool _can_act(mon& m, bool own)
        {
            bn::string<64> text(own ? "" : "Wild ");
            text.append(m.name());
            if(m.st == status::PARALYSIS && _random.get_int(4) == 0)
            {
                text.append(" is paralyzed! It can't move!");
                _ui.say(text);
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
                    _ui.say(text);
                    return true;
                }
                text.append(" is fast asleep.");
                _ui.say(text);
                return false;
            }
            if(m.st == status::FREEZE)
            {
                if(_random.get_int(5) == 0)
                {
                    m.st = status::NONE;
                    text.append(" thawed out!");
                    _ui.say(text);
                    return true;
                }
                text.append(" is frozen solid!");
                _ui.say(text);
                return false;
            }
            return true;
        }

        void _apply_status(mon& target, status s, int chance, bool target_own)
        {
            if(target.st != status::NONE || target.fainted() || s == status::NONE)
            {
                return;
            }
            if(_random.get_int(100) >= chance)
            {
                return;
            }
            target.st = s;
            if(s == status::SLEEP)
            {
                target.sleep_turns = uint8_t(1 + _random.get_int(3));
            }
            bn::string<80> text(target_own ? "" : "Wild ");
            text.append(target.name());
            text.append(status_word(s));
            _ui.say(text);
        }

        void _use_move(mon& user, int move_index, mon& target, bool own)
        {
            if(! _can_act(user, own))
            {
                return;
            }
            const move& mv = move_data(move_index);
            bn::string<64> text(own ? "" : "Wild ");
            text.append(user.name());
            text.append(" used ");
            text.append(mv.name);
            text.append("!");
            _ui.say(text);
            if(_random.get_int(100) >= mv.accuracy)
            {
                text = own ? "" : "Wild ";
                text.append(user.name());
                text.append("'s attack missed!");
                _ui.say(text);
                return;
            }
            if(mv.category == move_category::STATUS)
            {
                status had = target.st;
                _apply_status(target, mv.inflicts, 100, ! own);
                if(target.st == had)
                {
                    _ui.say("But it failed!");
                }
                return;
            }
            damage_result r = calc_damage(user, mv, target, _random);
            _flash(! own);
            _set_hp(target, target.hp - r.damage, ! own);
            if(r.effectiveness_x4 == 0)
            {
                text = "It doesn't affect ";
                text.append(own ? "Wild " : "");
                text.append(target.name());
                text.append("...");
                _ui.say(text);
            }
            else if(r.effectiveness_x4 > 4)
            {
                _ui.say("It's super effective!");
            }
            else if(r.effectiveness_x4 < 4)
            {
                _ui.say("It's not very effective...");
            }
            if(mv.secondary != status::NONE)
            {
                _apply_status(target, mv.secondary, mv.secondary_chance, ! own);
            }
            _check_faint(target, ! own);
        }

        bool _throw_ball()
        {
            game_state& g = state();
            --g.poke_balls;
            _ui.say("You threw a POKé BALL!");
            // Same odds as the web game: 0.95 - 0.7 x HP fraction, clamped to 0.1-0.95, x1.5 with a
            // status; three shake checks at the cube root.
            int chance = 950 - (700 * _foe.hp) / bn::max(1, int(_foe.max_hp));
            chance = bn::clamp(chance, 100, 950);
            if(_foe.st != status::NONE)
            {
                chance = bn::min(1000, chance * 3 / 2);
            }
            int per_shake = cbrt_scaled(chance);
            int shakes = 0;
            while(shakes < 3 && _random.get_int(1000) < per_shake)
            {
                ++shakes;
            }
            _foe_sprite->set_visible(false);
            for(int i = 0; i < bn::max(1, shakes); ++i)
            {
                _ui.show_text(i == 0 ? "..." : i == 1 ? "... ..." : "... ... ...");
                ui::wait(30);
            }
            _ui.clear_text();
            if(shakes == 3)
            {
                bn::string<64> text("Gotcha! ");
                text.append(_foe.name());
                text.append(" was caught!");
                _ui.say(text);
                if(g.add_to_party(_foe))
                {
                    text = _foe.name();
                    text.append(" joined your party!");
                }
                else
                {
                    // No PC boxes in this build yet.
                    text = "Your party is full, so ";
                    text.append(_foe.name());
                    text.append(" was released.");
                }
                _ui.say(text);
                return true;
            }
            _foe_sprite->set_visible(true);
            bn::string<64> text("Oh no! The wild ");
            text.append(_foe.name());
            text.append(" broke free!");
            _ui.say(text);
            return false;
        }

        void _win()
        {
            game_state& g = state();
            constexpr int xp = 18;       // checkEnd(): 18 per wild Pokémon, to the whole party
            bn::string<64> text("Your party gained ");
            text.append(bn::to_string<4>(xp));
            text.append(" EXP. Points!");
            _ui.say(text);
            for(int i = 0; i < g.party_count; ++i)
            {
                g.party[i].grant_xp(xp, _ui);
            }
        }

        battle_outcome _finish(battle_outcome outcome)
        {
            ui::fade_out(16);
            _ui.set_keep_box(false);
            return outcome;
        }
    };
}

battle_outcome battle_scene(ui& ui, const encounter& wild)
{
    bn::bg_palettes::set_transparent_color(bn::color(26, 30, 24));
    battle b(ui, wild);
    battle_outcome outcome = b.run();
    if(outcome == battle_outcome::WHITED_OUT)
    {
        // sendToCenter(): back home, outside the POKéMON CENTER, healed.
        game_state& g = state();
        g.heal_party();
        g.area = 0;
        const pr::area& home = world_data::areas[0];
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
