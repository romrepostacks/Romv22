// Move animations, after the web game's atkFx(): each move's MOVE_FX script (parsed at build time into
// pr_fx_data.h) plays step by step. A step throws pixel shapes about (sprites on keyframes), draws bars
// (slashes, beams, whips: rows of dots), or moves the Pokémon themselves. Times are the web's
// milliseconds turned into frames; sizes are the web's scaled to the GBA screen (u = 8 px).
#include "pr_move_fx.h"

#include <initializer_list>

#include "bn_bg_palettes.h"
#include "bn_math.h"
#include "bn_optional.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_sprite_palette_item.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_palettes.h"
#include "bn_vector.h"

#include "pr_fx_data.h"
#include "pr_game_data.h"
#include "pr_state.h"
#include "pr_ui.h"

namespace pr
{

namespace
{
    using namespace fx_data;

    constexpr int u = 8;                // the web's u (scene width / 28), on the GBA's screen
    constexpr int max_keys = 6;

    int ms(int milliseconds)
    {
        return (milliseconds * 3 + 25) / 50;
    }

    int rnd(int lo, int hi)            // lo..hi inclusive
    {
        return hi <= lo ? lo : lo + rng().get_int(hi - lo + 1);
    }

    // One keyframe: when (0..100 % of the duration), where (screen px), how opaque (0..100) and how big (%).
    struct key
    {
        int t, x, y, op, sc;
    };

    struct shape_particle
    {
        bn::optional<bn::sprite_ptr> sprite;
        int shape = 0;
        int palette = 0;
        int start = 0, dur = 1;
        bool flip = false;
        key keys[max_keys];
        int count = 0;
        bool done = false;
    };

    // A bar from (x, y), len long at angle a, drawn out to sx % of its length: keyframes of angle, length
    // and opacity.
    struct bar_key
    {
        int t, a, sx, op;
    };

    struct bar_particle
    {
        bn::vector<bn::sprite_ptr, 14> dots;
        int x = 0, y = 0, len = 0, rows = 1;
        int palette = 0;
        int start = 0, dur = 1;
        bar_key keys[max_keys];
        int count = 0;
        bool done = false;
    };

    // A Pokémon moving (rush, shake, hop) or changing colour (charged, glowing).
    struct body_key
    {
        int t, dx, dy, vis, fade;
    };

    struct body_tween
    {
        bn::sprite_ptr* sprite = nullptr;
        int bx = 0, by = 0;
        int start = 0, dur = 1;
        bn::color fade_color;
        body_key keys[max_keys];
        int count = 0;
        bool done = false;
    };

    struct engine
    {
        fx_body A, T;
        bool flip = false;              // the target is to the left
        int now = 0;
        bn::vector<shape_particle, 48> shapes;
        bn::vector<bar_particle, 6> bars;
        bn::vector<body_tween, 6> bodies;
        bn::vector<bn::sprite_palette_ptr, 6> palettes;
        bn::vector<bn::color, 6> palette_c1;
        bn::vector<bn::color, 6> palette_c2;
        bn::optional<bn::sprite_affine_mat_ptr> mats[6];
        int screen_fade_start = -1, screen_fade_dur = 1;
        bn::color screen_fade_color;
        int shake_start = -1, shake_dur = 1, shake_j = 0;
        bn::fixed old_bg_fade, old_sprite_fade;

        // ----- Palettes and scales -----
        int palette_for(bn::color c1, bn::color c2)
        {
            for(int i = 0; i < palettes.size(); ++i)
            {
                if(palette_c1[i] == c1 && palette_c2[i] == c2)
                {
                    return i;
                }
            }
            if(palettes.full())
            {
                return 0;
            }
            bn::color colors[16] = { bn::color(31, 0, 31), bn::color(4, 4, 4), bn::color(31, 31, 31), c1, c2 };
            for(int i = 5; i < 16; ++i)
            {
                colors[i] = bn::color(0, 0, 0);
            }
            bn::sprite_palette_item item(colors, bn::bpp_mode::BPP_4);
            palettes.push_back(bn::sprite_palette_ptr::create_new(item));
            palette_c1.push_back(c1);
            palette_c2.push_back(c2);
            return palettes.size() - 1;
        }

        // Shared scale matrices: 50%, 75%, 125%, 150%, 200%, 250% (100% needs none).
        void set_scale(bn::sprite_ptr& s, int sc)
        {
            constexpr int steps[] = { 50, 75, 125, 150, 200, 250 };
            if(sc >= 88 && sc <= 112)
            {
                if(s.affine_mat())
                {
                    s.remove_affine_mat();
                }
                return;
            }
            int best = 0;
            for(int i = 1; i < 6; ++i)
            {
                if(bn::abs(steps[i] - sc) < bn::abs(steps[best] - sc))
                {
                    best = i;
                }
            }
            if(! mats[best])
            {
                mats[best] = bn::sprite_affine_mat_ptr::create();
                mats[best]->set_scale(bn::fixed(steps[best]) / 100);
            }
            if(! s.affine_mat() || s.affine_mat()->id() != mats[best]->id())
            {
                s.set_affine_mat(*mats[best]);
                s.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            }
        }

        // ----- Adding things -----
        // put(): one pixel shape, size px wide, on keyframes (positions relative to (x, y)).
        void put(const fx_step& st, int shape, int x, int y, int size, std::initializer_list<key> kf, int dur_ms,
                 int delay_ms = 0)
        {
            if(shapes.full() || shape < 0)
            {
                return;
            }
            shape_particle p;
            p.shape = shape;
            bool hands = shape == fx_shape::FIST || shape == fx_shape::PALM || shape == fx_shape::BOOT;
            bn::color c1 = st.c1, c2 = st.c2;
            if(hands && ! st.custom)
            {
                c1 = bn::color(31, 31, 31);
                c2 = bn::color(25, 25, 27);
            }
            p.palette = palette_for(c1, c2);
            p.start = now + ms(delay_ms);
            p.dur = bn::max(1, ms(dur_ms));
            p.flip = flip && (hands || shape == fx_shape::HORN || shape == fx_shape::NEEDLE || shape == fx_shape::BLADE);
            int base = size * 100 / bn::max(1, int(shape_w[shape]));
            for(const key& k : kf)
            {
                if(p.count < max_keys)
                {
                    p.keys[p.count++] = { k.t, x + k.x, y + k.y, k.op, base * k.sc / 100 };
                }
            }
            shapes.push_back(bn::move(p));
        }

        void bar(const fx_step& st, int x1, int y1, int x2, int y2, int rows, bn::color c1, bn::color c2,
                 std::initializer_list<bar_key> kf, int dur_ms, int delay_ms = 0)
        {
            if(bars.full())
            {
                return;
            }
            bar_particle b;
            b.x = x1;
            b.y = y1;
            int dx = x2 - x1, dy = y2 - y1;
            b.len = bn::sqrt(dx * dx + dy * dy);
            int angle = 0;
            if(dx || dy)
            {
                angle = bn::degrees_atan2(dy, dx).integer();
            }
            b.rows = rows;
            b.palette = palette_for(c1, c2);
            b.start = now + ms(delay_ms);
            b.dur = bn::max(1, ms(dur_ms));
            for(const bar_key& k : kf)
            {
                if(b.count < max_keys)
                {
                    b.keys[b.count++] = { k.t, angle + k.a, k.sx, k.op };
                }
            }
            (void) st;
            bars.push_back(bn::move(b));
        }

        void move_body(const fx_body& who, std::initializer_list<body_key> kf, int dur_ms, int delay_ms = 0,
                       bn::color fade_color = bn::color(31, 31, 31))
        {
            if(! who.sprite || bodies.full())
            {
                return;
            }
            body_tween b;
            b.sprite = who.sprite;
            b.bx = who.sprite->x().integer();
            b.by = who.sprite->y().integer();
            for(const body_tween& o : bodies)
            {
                if(o.sprite == who.sprite && ! o.done)
                {
                    b.bx = o.bx;
                    b.by = o.by;
                }
            }
            b.start = now + ms(delay_ms);
            b.dur = bn::max(1, ms(dur_ms));
            b.fade_color = fade_color;
            for(const body_key& k : kf)
            {
                if(b.count < max_keys)
                {
                    b.keys[b.count++] = k;
                }
            }
            bodies.push_back(b);
        }

        void sparks(const fx_step& st, int x, int y, int n = 5)
        {
            for(int i = 0; i < n; ++i)
            {
                int a = i * 360 / n;
                int r = u * 16 / 10;
                int ex = (bn::degrees_lut_cos(a) * r).integer(), ey = (bn::degrees_lut_sin(a) * r).integer();
                put(st, fx_shape::HIT, x, y, u, { { 0, 0, 0, 100, 60 }, { 100, ex, ey, 0, 100 } }, 260);
            }
        }

        // ----- Running -----
        template<typename K>
        static int segment(const K* keys, int count, int t, int& f)
        {
            // Which pair of keys t (0..100) falls between, and how far (f, 0..256).
            int k = 0;
            while(k + 1 < count - 1 && t > keys[k + 1].t)
            {
                ++k;
            }
            if(count < 2)
            {
                f = 0;
                return 0;
            }
            int span = bn::max(1, keys[k + 1].t - keys[k].t);
            f = bn::clamp((t - keys[k].t) * 256 / span, 0, 256);
            return k;
        }

        static int lerp(int a, int b, int f)
        {
            return a + (b - a) * f / 256;
        }

        void update_shape(shape_particle& p)
        {
            int age = now - p.start;
            if(age < 0 || p.done)
            {
                return;
            }
            if(age >= p.dur)
            {
                p.sprite.reset();
                p.done = true;
                return;
            }
            int t = age * 100 / p.dur;
            int f;
            int k = segment(p.keys, p.count, t, f);
            const key& a = p.keys[k];
            const key& b = p.keys[bn::min(k + 1, p.count - 1)];
            int x = lerp(a.x, b.x, f), y = lerp(a.y, b.y, f), op = lerp(a.op, b.op, f), sc = lerp(a.sc, b.sc, f);
            if(! p.sprite)
            {
                p.sprite = bn::sprite_items::fx.create_sprite(sx(x), sy(y), p.shape);
                p.sprite->set_palette(palettes[p.palette]);
                p.sprite->set_bg_priority(1);
                p.sprite->set_z_order(-100);
                p.sprite->set_horizontal_flip(p.flip);
            }
            p.sprite->set_position(sx(x), sy(y));
            p.sprite->set_visible(op >= 35 && sc > 8);
            set_scale(*p.sprite, sc);
        }

        void update_bar(bar_particle& b)
        {
            int age = now - b.start;
            if(age < 0 || b.done)
            {
                return;
            }
            if(age >= b.dur)
            {
                b.dots.clear();
                b.done = true;
                return;
            }
            int t = age * 100 / b.dur;
            int f;
            int k = segment(b.keys, b.count, t, f);
            const bar_key& a = b.keys[k];
            const bar_key& c = b.keys[bn::min(k + 1, b.count - 1)];
            int angle = lerp(a.a, c.a, f), sxp = lerp(a.sx, c.sx, f), op = lerp(a.op, c.op, f);
            int n = bn::clamp(b.len / 5, 1, 14 / b.rows);
            int needed = n * b.rows;
            while(b.dots.size() < needed && ! b.dots.full())
            {
                bn::sprite_ptr d = bn::sprite_items::fx.create_sprite(0, 0, fx_shape::DOT);
                d.set_palette(palettes[b.palette]);
                d.set_bg_priority(1);
                d.set_z_order(-100);
                b.dots.push_back(bn::move(d));
            }
            int a360 = ((angle % 360) + 360) % 360;
            bn::fixed cs = bn::degrees_lut_cos(a360), sn = bn::degrees_lut_sin(a360);
            for(int r = 0; r < b.rows; ++r)
            {
                int off = (r - (b.rows - 1) / 2) * 4;
                for(int i = 0; i < n && r * n + i < b.dots.size(); ++i)
                {
                    int along = (b.len * (2 * i + 1)) / (2 * n);
                    bn::sprite_ptr& d = b.dots[r * n + i];
                    int px = b.x + (cs * along - sn * off).integer();
                    int py = b.y + (sn * along + cs * off).integer();
                    d.set_position(sx(px), sy(py));
                    d.set_visible(op >= 35 && along * 100 <= b.len * sxp);
                }
            }
        }

        void update_body(body_tween& b)
        {
            int age = now - b.start;
            if(age < 0 || b.done)
            {
                return;
            }
            if(age >= b.dur)
            {
                b.sprite->set_position(b.bx, b.by);
                b.sprite->set_visible(true);
                bn::sprite_palette_ptr pal = b.sprite->palette();
                pal.set_fade_intensity(0);
                b.done = true;
                return;
            }
            int t = age * 100 / b.dur;
            int f;
            int k = segment(b.keys, b.count, t, f);
            const body_key& a = b.keys[k];
            const body_key& c = b.keys[bn::min(k + 1, b.count - 1)];
            b.sprite->set_position(b.bx + lerp(a.dx, c.dx, f), b.by + lerp(a.dy, c.dy, f));
            b.sprite->set_visible(lerp(a.vis, c.vis, f) >= 50);
            int fade = lerp(a.fade, c.fade, f);
            bn::sprite_palette_ptr pal = b.sprite->palette();
            pal.set_fade(b.fade_color, bn::fixed(bn::clamp(fade, 0, 100)) / 100);
        }

        void step_frame()
        {
            for(shape_particle& p : shapes)
            {
                update_shape(p);
            }
            for(bar_particle& b : bars)
            {
                update_bar(b);
            }
            for(body_tween& b : bodies)
            {
                update_body(b);
            }
            if(screen_fade_start >= 0)
            {
                int age = now - screen_fade_start;
                if(age >= screen_fade_dur)
                {
                    bn::bg_palettes::set_fade(screen_fade_color, old_bg_fade);
                    bn::sprite_palettes::set_fade(screen_fade_color, old_sprite_fade);
                    screen_fade_start = -1;
                }
                else if(age >= 0)
                {
                    int half = screen_fade_dur / 2;
                    int v = age < half ? age * 75 / bn::max(1, half) : (screen_fade_dur - age) * 75 / bn::max(1, screen_fade_dur - half);
                    bn::bg_palettes::set_fade(screen_fade_color, bn::fixed(v) / 100);
                    bn::sprite_palettes::set_fade(screen_fade_color, bn::fixed(v) / 100);
                }
            }
            frame();
            ++now;
            // Drop what's finished, so the lists don't fill up.
            for(int i = 0; i < shapes.size();)
            {
                if(shapes[i].done)
                {
                    shapes.erase(shapes.begin() + i);
                }
                else
                {
                    ++i;
                }
            }
            for(int i = 0; i < bars.size();)
            {
                if(bars[i].done)
                {
                    bars.erase(bars.begin() + i);
                }
                else
                {
                    ++i;
                }
            }
            for(int i = 0; i < bodies.size();)
            {
                if(bodies[i].done)
                {
                    bodies.erase(bodies.begin() + i);
                }
                else
                {
                    ++i;
                }
            }
        }

        void run(int frames)
        {
            for(int i = 0; i < frames; ++i)
            {
                step_frame();
            }
        }

        void wait_ms(int milliseconds)
        {
            run(ms(milliseconds));
        }

        void finish()
        {
            int guard = 0;
            while((! shapes.empty() || ! bars.empty() || ! bodies.empty() || screen_fade_start >= 0) && guard++ < 240)
            {
                step_frame();
            }
            shapes.clear();
            bars.clear();
            for(body_tween& b : bodies)
            {
                b.sprite->set_position(b.bx, b.by);
                b.sprite->set_visible(true);
                bn::sprite_palette_ptr pal = b.sprite->palette();
                pal.set_fade_intensity(0);
            }
            bodies.clear();
        }

        // ----- The kinds (the web's K) -----
        void flash(const fx_step& st)
        {
            bn::color c = st.c2 != bn::color(31, 31, 31) ? st.c2 : bn::color(31, 31, 31);
            screen_fade_color = c;
            screen_fade_start = now;
            screen_fade_dur = ms(320);
            old_bg_fade = bn::bg_palettes::fade_intensity();
            old_sprite_fade = bn::sprite_palettes::fade_intensity();
            wait_ms(320);
        }

        void play(const fx_step& st)
        {
            int dxAT = T.x - A.x, dyAT = T.y - A.y;
            int sh = st.shape;
            switch(st.kind)
            {

            case fx_kind::RUSH:
            {
                int dx = dxAT * (st.big ? 62 : 50) / 100;
                int dy = dyAT * (st.big ? 62 : 50) / 100 - (st.above ? T.h * 4 / 10 : 0);
                if(sh >= 0)
                {
                    for(int i = 0; i < st.n; ++i)
                    {
                        int fx = A.x + dx * (i + 1) / (st.n + 1), fy = A.y + dy * (i + 1) / (st.n + 1);
                        put(st, sh, fx, fy, u * 12 / 10, { { 0, 0, 0, 0, 100 }, { 30, 0, 0, 100, 100 }, { 100, 0, 0, 0, 140 } },
                            380, i * 25);
                    }
                }
                int d = st.big ? 520 : 420;
                move_body(A, { { 0, 0, 0, 100, 0 }, { 55, dx, dy, 100, 0 }, { 100, 0, 0, 100, 0 } }, d);
                wait_ms(d);
                sparks(st, T.x, T.y, st.big ? 8 : 5);
                wait_ms(120);
                break;
            }

            case fx_kind::JAWS:
            {
                int w = T.w * (st.big ? 95 : 75) / 100, off = T.h * 45 / 100;
                fx_step cl = st;
                if(! st.custom)
                {
                    cl.c1 = bn::color(31, 31, 31);
                    cl.c2 = bn::color(25, 25, 27);
                }
                for(int i = 0; i < st.n; ++i)
                {
                    put(cl, fx_shape::JAWTOP, T.x, T.y - w * 16 / 100, w,
                        { { 0, 0, -off, 0, 100 }, { 30, 0, -off, 100, 100 }, { 65, 0, 0, 100, 100 }, { 100, 0, 0, 0, 100 } }, 480);
                    put(cl, fx_shape::JAWBOT, T.x, T.y + w * 16 / 100, w,
                        { { 0, 0, off, 0, 100 }, { 30, 0, off, 100, 100 }, { 65, 0, 0, 100, 100 }, { 100, 0, 0, 0, 100 } }, 480);
                    wait_ms(480);
                    move_body(T, { { 0, 0, 0, 100, 0 }, { 33, u / 3, 0, 100, 0 }, { 66, -u / 3, 0, 100, 0 }, { 100, 0, 0, 100, 0 } }, 200);
                }
                sparks(st, T.x, T.y);
                break;
            }

            case fx_kind::SLASH:
            {
                int L = T.w * (st.big ? 90 : 70) / 100;
                bn::color col = st.c1 == bn::color(31, 31, 31) ? bn::color(31, 31, 31) : st.c1;
                for(int i = 0; i < st.n; ++i)
                {
                    int o = st.cross ? 0 : (2 * i - (st.n - 1)) * u * 11 / 20;
                    bool back = st.cross && (i % 2);
                    bar(st, T.x - L / 2 + o + (back ? L : 0), T.y - L / 2, T.x + L / 2 + o - (back ? L : 0), T.y + L / 2, 1,
                        col, bn::color(31, 31, 31), { { 0, 0, 0, 100 }, { 45, 0, 100, 100 }, { 100, 0, 100, 0 } }, 320, i * 110);
                }
                wait_ms(320 + st.n * 110);
                sparks(st, T.x, T.y, 4);
                break;
            }

            case fx_kind::IMPACT:
            {
                int size = T.w * (st.big ? 60 : 45) / 100;
                int shape = sh >= 0 ? sh : fx_shape::HIT;
                for(int i = 0; i < st.n; ++i)
                {
                    int ox = st.n > 1 ? (i % 2 ? 1 : -1) * T.w * 14 / 100 : 0;
                    int oy = st.n > 1 ? (2 * i - (st.n - 1)) * T.h * 7 / 100 : 0;
                    if(st.above)
                    {
                        put(st, shape, T.x + ox, T.y + oy, size, { { 0, 0, -T.h * 9 / 10, 0, 100 }, { 60, 0, 0, 100, 100 }, { 100, 0, 0, 0, 90 } }, 300);
                    }
                    else
                    {
                        put(st, shape, T.x + ox, T.y + oy, size, { { 0, 0, 0, 0, 220 }, { 60, 0, 0, 100, 100 }, { 100, 0, 0, 0, 90 } }, 300);
                    }
                    wait_ms(300);
                    sparks(st, T.x + ox, T.y + oy, 4);
                }
                break;
            }

            case fx_kind::PROJECTILE:
            {
                int size = u * (st.big ? 24 : 13) / 10;
                int shape = sh >= 0 ? sh : fx_shape::ORB;
                for(int i = 0; i < st.n; ++i)
                {
                    int jx = st.n > 1 ? rnd(-T.w / 4, T.w / 4) : 0, jy = st.n > 1 ? rnd(-T.h / 5, T.h / 5) : 0;
                    int ex = dxAT + jx, ey = dyAT + jy;
                    put(st, shape, A.x, A.y, size, { { 0, 0, 0, 100, 100 }, { 50, ex / 2, ey / 2 - (st.arc ? T.h * 6 / 10 : 0), 100, 100 },
                                                     { 100, ex, ey, 100, 100 } }, 420, i * 110);
                }
                wait_ms(420 + (st.n - 1) * 110);
                sparks(st, T.x, T.y, st.big ? 8 : 5);
                break;
            }

            case fx_kind::STREAM:
            {
                int size = u * (st.big ? 18 : 13) / 10;
                int shape = sh >= 0 ? sh : fx_shape::DROP;
                for(int i = 0; i < st.n; ++i)
                {
                    int ex = dxAT + rnd(-T.w * 15 / 100, T.w * 15 / 100), ey = dyAT + rnd(-T.h * 15 / 100, T.h * 15 / 100);
                    int wob = rnd(-u, u);
                    put(st, shape, A.x, A.y, size, { { 0, 0, 0, 100, 40 }, { 50, ex / 2 + wob, ey / 2 - wob, 100, 90 }, { 100, ex, ey, 15, 140 } },
                        360, i * 38);
                }
                wait_ms(360 + st.n * 38);
                break;
            }

            case fx_kind::BEAM:
            {
                bar(st, A.x, A.y, T.x, T.y, st.big ? 3 : 2, st.c1, st.c2,
                    { { 0, 0, 0, 100 }, { 30, 0, 100, 100 }, { 75, 0, 100, 100 }, { 100, 0, 100, 0 } }, 700);
                wait_ms(260);
                sparks(st, T.x, T.y, 8);
                wait_ms(440);
                break;
            }

            case fx_kind::GATHER:
            {
                int shape = sh >= 0 ? sh : fx_shape::HIT;
                for(int i = 0; i < st.n; ++i)
                {
                    int a = i * 360 / st.n, r = A.w * 7 / 10;
                    int cx = (bn::degrees_lut_cos(a) * r).integer(), cy = (bn::degrees_lut_sin(a) * r).integer();
                    put(st, shape, A.x + cx, A.y + cy, u, { { 0, 0, 0, 0, 100 }, { 30, 0, 0, 100, 100 }, { 100, -cx, -cy, 100, 40 } }, 420, i * 30);
                }
                wait_ms(420 + st.n * 30);
                break;
            }

            case fx_kind::RAIN:
            {
                int size = u * (st.big ? 19 : 14) / 10;
                int shape = sh >= 0 ? sh : fx_shape::ROCK;
                for(int i = 0; i < st.n; ++i)
                {
                    int x = T.x + (st.n > 1 ? rnd(-T.w / 2, T.w / 2) : 0), h = T.h * 11 / 10;
                    put(st, shape, x, T.y - h, size, { { 0, 0, 0, 100, 100 }, { 80, 0, h, 100, 100 }, { 100, 0, h, 0, 100 } }, 420, i * 80);
                }
                wait_ms(420 + st.n * 80);
                sparks(st, T.x, T.y);
                break;
            }

            case fx_kind::RISE:
            {
                int size = u * (st.big ? 19 : 14) / 10;
                int shape = sh >= 0 ? sh : fx_shape::FLAME;
                for(int i = 0; i < st.n; ++i)
                {
                    put(st, shape, T.x + rnd(-T.w * 45 / 100, T.w * 45 / 100), T.y + T.h * 4 / 10, size,
                        { { 0, 0, 0, 100, 60 }, { 100, 0, -T.h * 9 / 10, 0, 120 } }, 520, i * 55);
                }
                wait_ms(520 + st.n * 55);
                break;
            }

            case fx_kind::BURST:
            {
                int R = T.w * (st.big ? 60 : 45) / 100;
                int shape = sh >= 0 ? sh : fx_shape::HIT;
                for(int i = 0; i < st.n; ++i)
                {
                    int a = i * 360 / st.n;
                    int ex = (bn::degrees_lut_cos(a) * R).integer(), ey = (bn::degrees_lut_sin(a) * R).integer();
                    put(st, shape, T.x, T.y, u * (st.big ? 16 : 12) / 10, { { 0, 0, 0, 100, 50 }, { 100, ex, ey, 0, 120 } }, 460, i * 18);
                }
                wait_ms(460 + st.n * 18);
                break;
            }

            case fx_kind::VORTEX:
            {
                int R = T.w * (st.big ? 60 : 45) / 100;
                int shape = sh >= 0 ? sh : fx_shape::BLADE;
                for(int i = 0; i < st.n; ++i)
                {
                    int a0 = i * 360 / st.n;
                    key k[6];
                    for(int j = 0; j < 6; ++j)
                    {
                        int f = j * 20;     // %
                        int a = ((a0 + f * 516 / 100) % 360 + 360) % 360;       // 9 radians over the spin
                        int r = R * (100 - f * 4 / 10) / 100;
                        k[j] = { f, (bn::degrees_lut_cos(a) * r).integer(),
                                 (bn::degrees_lut_sin(a) * r / 2).integer() - f * T.h / 200 + T.h / 4, f < 80 ? 100 : 0, 100 };
                    }
                    put(st, shape, T.x, T.y, u * 12 / 10, { k[0], k[1], k[2], k[3], k[4], k[5] }, 800, i * 20);
                }
                wait_ms(800 + st.n * 20);
                break;
            }

            case fx_kind::RINGS:
            {
                // The ring grows on its way from the user to the target.
                for(int i = 0; i < st.n; ++i)
                {
                    put(st, fx_shape::RING, A.x, A.y, u * 2, { { 0, 0, 0, 100, 40 }, { 100, dxAT, dyAT, 0, 160 } }, 600, i * 140);
                }
                wait_ms(600 + (st.n - 1) * 140);
                break;
            }

            case fx_kind::RINGIN:
            {
                for(int i = 0; i < st.n; ++i)
                {
                    put(st, fx_shape::RING, T.x, T.y, T.w * (st.big ? 16 : 12) / 10,
                        { { 0, 0, 0, 0, 100 }, { 40, 0, 0, 100, 70 }, { 100, 0, 0, 0, 10 } }, 520, i * 160);
                }
                move_body(T, { { 0, 0, 0, 100, 0 }, { 50, 0, 0, 100, 50 }, { 100, 0, 0, 100, 0 } }, 520 + st.n * 160, 0, st.c1);
                wait_ms(520 + (st.n - 1) * 160);
                break;
            }

            case fx_kind::NOTES:
            {
                int shape = sh >= 0 ? sh : fx_shape::NOTE;
                for(int i = 0; i < st.n; ++i)
                {
                    int ex = dxAT + rnd(-T.w * 3 / 10, T.w * 3 / 10), ey = dyAT + rnd(-T.h * 3 / 10, T.h * 3 / 10);
                    key k[5];
                    for(int j = 0; j < 5; ++j)
                    {
                        int f = j * 25;
                        int wave = (bn::degrees_lut_sin((f * 540 / 100) % 360) * u).integer();
                        k[j] = { f, ex * f / 100, ey * f / 100 + wave, f < 90 ? 100 : 0, 100 };
                    }
                    put(st, shape, A.x, A.y - A.h / 5, u * 13 / 10, { k[0], k[1], k[2], k[3], k[4] }, 700, i * 110);
                }
                wait_ms(700 + (st.n - 1) * 110);
                break;
            }

            case fx_kind::BIND:
            {
                int L = T.w * 9 / 10;
                for(int i = 0; i < 3; ++i)
                {
                    int y = T.y + (i - 1) * T.h * 22 / 100;
                    bar(st, T.x - L / 2, y, T.x + L / 2, y, 1, st.c1, st.c2,
                        { { 0, 0, 0, 100 }, { 30, 0, 100, 100 }, { 55, 0, 85, 100 }, { 75, 0, 100, 100 }, { 100, 0, 85, 0 } }, 700, i * 60);
                }
                move_body(T, { { 0, 0, 0, 100, 0 }, { 50, 0, 1, 100, 0 }, { 100, 0, 0, 100, 0 } }, 700);
                wait_ms(820);
                break;
            }

            case fx_kind::WHIP:
            {
                int L = T.w * (st.big ? 110 : 90) / 100;
                for(int i = 0; i < st.n; ++i)
                {
                    int s = i % 2 ? -1 : 1;
                    int px = T.x - s * L * 55 / 100, py = T.y - T.h / 2;
                    bar(st, px, py, px + s * L, py, 1, st.c1, st.c2,
                        { { 0, -50 * s, 100, 100 }, { 60, 40 * s, 100, 100 }, { 100, 45 * s, 100, 0 } }, 360, i * 200);
                }
                wait_ms(260 + (st.n - 1) * 200);
                sparks(st, T.x, T.y, 4);
                wait_ms(120);
                break;
            }

            case fx_kind::BOLT:
            {
                int size = T.w * (st.big ? 70 : 45) / 100;
                for(int i = 0; i < st.n; ++i)
                {
                    put(st, fx_shape::BOLT, T.x + (st.n > 1 ? rnd(-T.w * 3 / 10, T.w * 3 / 10) : 0), T.y - T.h / 4, size,
                        { { 0, 0, -T.h * 6 / 10, 0, 100 }, { 35, 0, 0, 100, 100 }, { 55, 0, 0, 20, 100 }, { 75, 0, 0, 100, 100 },
                          { 100, 0, 0, 0, 100 } }, 380, i * 120);
                }
                move_body(T, { { 0, 0, 0, 100, 0 }, { 50, 0, 0, 100, 70 }, { 100, 0, 0, 100, 0 } }, 300, 120, bn::color(31, 28, 8));
                wait_ms(380 + (st.n - 1) * 120);
                break;
            }

            case fx_kind::QUAKE:
            {
                int j = u * (st.big ? 6 : 4) / 10;
                move_body(A, { { 0, -j, 0, 100, 0 }, { 25, j, j / 2, 100, 0 }, { 50, -j, 0, 100, 0 }, { 75, j, j / 2, 100, 0 }, { 100, 0, 0, 100, 0 } }, 520);
                move_body(T, { { 0, j, 0, 100, 0 }, { 25, -j, j / 2, 100, 0 }, { 50, j, 0, 100, 0 }, { 75, -j, j / 2, 100, 0 }, { 100, 0, 0, 100, 0 } }, 520);
                wait_ms(520);
                break;
            }

            case fx_kind::DRAIN:
            {
                int shape = sh >= 0 ? sh : fx_shape::ORB;
                for(int i = 0; i < st.n; ++i)
                {
                    int ex = -dxAT, ey = -dyAT, b = rnd(-T.h / 2, T.h / 2);
                    put(st, shape, T.x, T.y, u, { { 0, 0, 0, 100, 60 }, { 50, ex / 2, ey / 2 + b, 100, 100 }, { 100, ex, ey, 0, 50 } }, 520, i * 60);
                }
                wait_ms(520 + st.n * 60);
                move_body(A, { { 0, 0, 0, 100, 0 }, { 50, 0, 0, 100, 60 }, { 100, 0, 0, 100, 0 } }, 300);
                wait_ms(300);
                break;
            }

            case fx_kind::POWDER:
            {
                int shape = sh >= 0 ? sh : fx_shape::BLOB;
                for(int i = 0; i < st.n; ++i)
                {
                    int x = T.x + rnd(-T.w / 2, T.w / 2), y = T.y - T.h * 6 / 10 + rnd(0, T.h * 3 / 10), sw = rnd(-u, u);
                    put(st, shape, x, y, u * 7 / 10, { { 0, 0, 0, 0, 100 }, { 40, sw, T.h * 3 / 10, 100, 100 }, { 100, -sw, T.h * 75 / 100, 0, 100 } },
                        700, i * 30);
                }
                wait_ms(700 + st.n * 30);
                break;
            }

            case fx_kind::THRASH:
            {
                int j = u * 6 / 10;
                move_body(A, { { 0, -j, 0, 100, 0 }, { 20, j, 0, 100, 0 }, { 40, -j, 0, 100, 0 }, { 60, j, 0, 100, 0 }, { 80, -j, 0, 100, 0 },
                               { 100, 0, 0, 100, 0 } }, 500);
                for(int i = 0; i < 3; ++i)
                {
                    wait_ms(150);
                    sparks(st, T.x + rnd(-T.w * 3 / 10, T.w * 3 / 10), T.y + rnd(-T.h * 3 / 10, T.h * 3 / 10), 4);
                }
                wait_ms(50);
                break;
            }

            case fx_kind::FLIP:
                move_body(T, { { 0, 0, 0, 100, 0 }, { 50, 0, -T.h * 3 / 10, 100, 0 }, { 100, 0, 0, 100, 0 } }, 500);
                wait_ms(500);
                break;

            case fx_kind::VANISH:
            {
                int dy = sh == fx_shape::UP ? -A.h * 3 / 2 : sh == fx_shape::DOWN ? A.h * 8 / 10 : 0;
                move_body(A, { { 0, 0, 0, 100, 0 }, { 100, 0, dy, 0, 0 }, { 100, 0, dy, 0, 0 } }, 320);
                wait_ms(320);
                move_body(A, { { 0, 0, dy, 0, 0 }, { 100, 0, dy, 0, 0 } }, 250);
                wait_ms(250);
                move_body(A, { { 0, 0, 0, 0, 0 }, { 50, 0, 0, 100, 0 }, { 100, 0, 0, 100, 0 } }, 300);
                break;
            }

            case fx_kind::SELF:
            {
                for(int i = 0; i < 12; ++i)
                {
                    int a = i * 30, R = A.w * 7 / 10;
                    int ex = (bn::degrees_lut_cos(a) * R).integer(), ey = (bn::degrees_lut_sin(a) * R).integer();
                    put(st, fx_shape::HIT, A.x, A.y, u * 18 / 10, { { 0, 0, 0, 100, 50 }, { 100, ex, ey, 0, 160 } }, 520, i * 15);
                }
                flash(st);
                wait_ms(240);
                break;
            }

            case fx_kind::FLASH:
                flash(st);
                break;

            default:
                break;
            }
        }
    };
}

void play_move_fx(int move_index, const fx_body& from, const fx_body& to)
{
    int first = fx_data::move_first[move_index], count = fx_data::move_count[move_index];
    if(! count)
    {
        return;
    }
    engine e;
    e.A = from;
    e.T = to;
    e.flip = to.x < from.x;
    const fx_data::fx_step& s0 = fx_data::steps[first];
    // A small lunge before a physical hit.
    const move& mv = game_data::moves[move_index];
    if(mv.category == move_category::PHYSICAL &&
       (s0.kind == fx_data::fx_kind::JAWS || s0.kind == fx_data::fx_kind::SLASH || s0.kind == fx_data::fx_kind::IMPACT ||
        s0.kind == fx_data::fx_kind::WHIP || s0.kind == fx_data::fx_kind::BIND))
    {
        e.move_body(from, { { 0, 0, 0, 100, 0 }, { 50, (to.x - from.x) * 12 / 100, (to.y - from.y) * 12 / 100, 100, 0 },
                            { 100, 0, 0, 100, 0 } }, 200);
        e.wait_ms(200);
    }
    for(int i = 0; i < count; ++i)
    {
        e.play(fx_data::steps[first + i]);
    }
    e.finish();
}

}
