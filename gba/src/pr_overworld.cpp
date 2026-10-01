// The overworld: the web game's areas and rooms drawn from generated metatiles and scrolled with the
// player, Emerald-style connections between areas, Emerald step timing (16 frames a step, 8 running),
// people who wander, route trainers who spot you, signs, item balls, ledges, tall grass, doors into the
// POKéMON CENTER, the MART and the houses, and the START menu.
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_common.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_optional.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_span.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_unique_ptr.h"
#include "bn_vector.h"

#include "bn_sprite_items_bang.h"
#include "bn_sprite_items_grass_front.h"
#include "bn_regular_bg_items_bag_bg.h"
#include "bn_sprite_items_cursor.h"

#include "pr_game_data.h"
#include "pr_people_sprites.h"
#include "pr_scenes.h"
#include "pr_screens.h"
#include "pr_state.h"
#include "pr_ui.h"
#include "pr_world_data.h"

namespace pr
{

namespace
{
    namespace wd = world_data;

    constexpr int walk_frames = 16;     // Emerald: a walking step is 16 frames, a running one 8
    constexpr int run_frames = 8;
    constexpr int turn_frames = 8;
    constexpr int jump_frames = 32;
    constexpr int encounter_percent = 8;
    constexpr int wander_interval = 42; // startWander(): every 700 ms, each wanderer steps one time in four
    constexpr int sight = 5;            // trainers see 5 tiles
    // The player's tile sits here on screen (the web game centres its view on the player).
    constexpr int view_x = 112;
    constexpr int view_y = 72;

    constexpr int dx_of(direction d)
    {
        return d == direction::LEFT ? -1 : d == direction::RIGHT ? 1 : 0;
    }
    constexpr int dy_of(direction d)
    {
        return d == direction::UP ? -1 : d == direction::DOWN ? 1 : 0;
    }
    constexpr direction opposite(direction d)
    {
        return d == direction::UP ? direction::DOWN : d == direction::DOWN ? direction::UP :
               d == direction::LEFT ? direction::RIGHT : direction::LEFT;
    }
    direction toward(int fx, int fy, int tx, int ty)
    {
        return tx > fx ? direction::RIGHT : tx < fx ? direction::LEFT : ty > fy ? direction::DOWN : direction::UP;
    }

    // Sprite sheet frames: down 0-2, up 3-5, left 6-8 (right is left mirrored); 0 stands, 1 and 2 step.
    void set_frame(bn::sprite_ptr& sprite, person_kind kind, direction d, int step)
    {
        int base = d == direction::DOWN ? 0 : d == direction::UP ? 3 : 6;
        sprite.set_tiles(person_sprites[int(kind)]->tiles_item(), base + step);
        sprite.set_horizontal_flip(d == direction::RIGHT);
    }

    alignas(int) BN_DATA_EWRAM_BSS bn::regular_bg_map_cell bg_cells[32 * 32];

    // What lies at a tile, looking through this area's connections when it's past the edge.
    struct place
    {
        int map = -1;           // -1: nothing (forest, the black around a room, or an area not in this build)
        int x = 0;
        int y = 0;
        int link = -1;          // the link used, if outside the current map
    };

    struct actor
    {
        const person* who = nullptr;
        const trainer* tr = nullptr;
        person_kind kind = person_kind::player;
        int x = 0, y = 0;
        int home_x = 0, home_y = 0;
        direction facing = direction::DOWN;
        bool wander = false;
        int move_frames = 0;        // > 0 while stepping
        direction move_dir = direction::DOWN;
        bool step_foot = false;
        bn::optional<bn::sprite_ptr> sprite;

        [[nodiscard]] bool active_trainer() const
        {
            return tr && ! state().beaten.test(tr->id);
        }
    };

    class overworld
    {

    public:
        overworld();
        bool run(encounter& battle);

    private:
        const map_def* _map = nullptr;
        int _map_index = -1;
        int _tileset = -1;
        bn::optional<bn::regular_bg_ptr> _bg;
        bn::optional<bn::regular_bg_map_ptr> _bg_map;
        int _px = 0, _py = 0, _cam_x = 0, _cam_y = 0;
        int _last_cx = 0x7fffffff, _last_cy = 0x7fffffff;
        bn::optional<bn::sprite_ptr> _player;
        bn::optional<bn::sprite_ptr> _grass_here;
        bn::optional<bn::sprite_ptr> _grass_from;
        int _grass_x = 0, _grass_y = 0, _from_x = -1000, _from_y = -1000;
        bn::vector<actor, 16> _actors;
        int _wander_timer = 0;
        int _step_parity = 0;
        bool _fresh_press = true;
        int _lift = 0;
        encounter* _battle = nullptr;
        bool _quit = false;
        bool _start_battle = false;

        // World
        [[nodiscard]] place find(int tx, int ty) const;
        [[nodiscard]] int metatile_at(int tx, int ty) const;
        [[nodiscard]] behaviour behaviour_at(int tx, int ty) const;
        [[nodiscard]] int actor_at(int tx, int ty) const;
        [[nodiscard]] bool blocked(int tx, int ty) const;
        void load_map(int index);
        void build_bg();
        void draw_cells(bool force);
        void refresh(bool force = false);
        void suspend();
        void resume();
        void load_actors();
        void place_actor(bn::sprite_ptr& sprite, int wx, int wy, int lift = 0);

        // Frames
        void tick();
        void tick_actors();
        void wander();

        // The player
        void step(direction want);
        void arrive();
        void interact();
        void talk_to(int index);
        bool check_sight();
        void trainer_approach(int index);
        void start_trainer_battle(int index);
        void wild_battle();

        // Places
        void enter_room(const door& d);
        void leave_room();
        void nurse(int index);
        void mom();
        void clerk();
        void mart();
        void obtain(item_id id, int count);

        // Story
        int add_temp_actor(person_kind kind, int x, int y, direction facing);
        void starter_event();
        int starter_bag();
        void starter_thanks();

        // START
        void start_menu();
        void save_menu();
        void card();
    };

    overworld::overworld()
    {
        load_map(state().map);
    }

    // ----- World -----
    place overworld::find(int tx, int ty) const
    {
        place p;
        if(tx >= 0 && ty >= 0 && tx < _map->w && ty < _map->h)
        {
            p.map = _map_index;
            p.x = tx;
            p.y = ty;
            return p;
        }
        for(int i = 0; i < _map->links_count; ++i)
        {
            const link& l = _map->links[i];
            if(tx >= l.ox && ty >= l.oy && tx < l.ox + l.w && ty < l.oy + l.h)
            {
                p.map = l.target;
                p.x = tx - l.ox;
                p.y = ty - l.oy;
                p.link = i;
                return p;
            }
        }
        return p;
    }

    bool picked(int map, int tx, int ty)
    {
        const map_def& m = wd::maps[map];
        for(int i = 0; i < m.items_count; ++i)
        {
            if(m.items[i].x == tx && m.items[i].y == ty)
            {
                return state().picked.test(m.items[i].id);
            }
        }
        return false;
    }

    int overworld::metatile_at(int tx, int ty) const
    {
        place p = find(tx, ty);
        const tileset& ts = wd::tilesets[_tileset];
        if(p.map < 0)
        {
            return ts.fill_metatile;
        }
        const map_def& m = wd::maps[p.map];
        int i = p.y * m.w + p.x;
        if(behaviour(m.behaviours[i]) == behaviour::ITEM && picked(p.map, p.x, p.y))
        {
            return ts.grass_metatile;
        }
        return m.map[i];
    }

    behaviour overworld::behaviour_at(int tx, int ty) const
    {
        place p = find(tx, ty);
        if(p.map < 0)
        {
            return behaviour::SOLID;
        }
        const map_def& m = wd::maps[p.map];
        behaviour b = behaviour(m.behaviours[p.y * m.w + p.x]);
        if(b == behaviour::ITEM && picked(p.map, p.x, p.y))
        {
            return behaviour::WALK;
        }
        return b;
    }

    int overworld::actor_at(int tx, int ty) const
    {
        for(int i = 0; i < _actors.size(); ++i)
        {
            const actor& a = _actors[i];
            // A stepping actor already holds the tile it's moving to.
            if(a.x == tx && a.y == ty)
            {
                return i;
            }
        }
        return -1;
    }

    bool overworld::blocked(int tx, int ty) const
    {
        behaviour b = behaviour_at(tx, ty);
        bool walkable = b == behaviour::WALK || b == behaviour::TALL_GRASS || b == behaviour::DOOR || b == behaviour::MAT;
        return ! walkable || actor_at(tx, ty) >= 0 || (tx == state().x && ty == state().y);
    }

    void overworld::build_bg()
    {
        // The tileset's tiles go at index 0 so the generated cells can be used as they are.
        _bg_map.reset();
        _bg.reset();
        const tileset& ts = wd::tilesets[_tileset];
        bn::bg_tiles::set_allow_offset(false);
        bn::regular_bg_tiles_item tiles_item(bn::span<const bn::tile>(ts.tiles, ts.tiles_count), bn::bpp_mode::BPP_4);
        bn::bg_palette_item palette_item(bn::span<const bn::color>(ts.colors, ts.colors_count), bn::bpp_mode::BPP_4);
        bn::regular_bg_map_item map_item(bg_cells[0], bn::size(32, 32));
        draw_cells(true);
        _bg = bn::regular_bg_item(tiles_item, palette_item, map_item).create_bg(0, 0);
        bn::bg_tiles::set_allow_offset(true);
        _bg->set_priority(2);
        _bg_map = _bg->map();
        bn::bg_palettes::set_transparent_color(_map->is_room() ? bn::color(0, 0, 0) : bn::color(4, 9, 3));
    }

    void overworld::load_map(int index)
    {
        game_state& g = state();
        _map_index = index;
        _map = &wd::maps[index];
        g.map = int8_t(index);
        _px = g.x * 16;
        _py = g.y * 16;
        _cam_x = _px - view_x;
        _cam_y = _py - view_y;
        if(_map->tileset != _tileset || ! _bg)
        {
            _tileset = _map->tileset;
            build_bg();
        }
        load_actors();
        _grass_x = g.x;
        _grass_y = g.y;
        _from_x = _from_y = -1000;
        refresh(true);
    }

    void overworld::load_actors()
    {
        _actors.clear();
        for(int i = 0; i < _map->people_count && ! _actors.full(); ++i)
        {
            const person& p = _map->people[i];
            actor a;
            a.who = &p;
            a.kind = p.kind;
            a.x = a.home_x = p.x;
            a.y = a.home_y = p.y;
            a.facing = p.facing;
            a.wander = p.wander;
            _actors.push_back(a);
        }
        for(int i = 0; i < _map->trainers_count && ! _actors.full(); ++i)
        {
            const trainer& t = _map->trainers[i];
            actor a;
            a.tr = &t;
            a.kind = t.kind;
            a.x = a.home_x = t.x;
            a.y = a.home_y = t.y;
            a.facing = t.facing;
            _actors.push_back(a);
        }
        for(actor& a : _actors)
        {
            a.sprite = person_sprites[int(a.kind)]->create_sprite(0, 0, 0);
            a.sprite->set_bg_priority(1);
            set_frame(*a.sprite, a.kind, a.facing, 0);
        }
    }

    void overworld::draw_cells(bool force)
    {
        int cx0 = _cam_x >> 3;
        int cy0 = _cam_y >> 3;
        if(! force && cx0 == _last_cx && cy0 == _last_cy)
        {
            return;
        }
        _last_cx = cx0;
        _last_cy = cy0;
        const tileset& ts = wd::tilesets[_tileset];
        for(int cy = cy0; cy < cy0 + 21; ++cy)
        {
            bn::regular_bg_map_cell* row = bg_cells + (cy & 31) * 32;
            int ty = cy >> 1;
            int sub_y = (cy & 1) * 2;
            int last_tx = 0x7fff;
            const uint16_t* meta = nullptr;
            for(int cx = cx0; cx < cx0 + 31; ++cx)
            {
                int tx = cx >> 1;
                if(tx != last_tx)
                {
                    meta = ts.metatiles[metatile_at(tx, ty)];
                    last_tx = tx;
                }
                row[cx & 31] = meta[sub_y + (cx & 1)];
            }
        }
        if(_bg_map)
        {
            _bg_map->reload_cells_ref();
        }
    }

    void overworld::place_actor(bn::sprite_ptr& sprite, int wx, int wy, int lift)
    {
        sprite.set_position(sx(wx - _cam_x + 8), sy(wy - _cam_y - 2 - lift));
        sprite.set_z_order(-((wy + 8) >> 4) - 2);
    }

    void overworld::refresh(bool force)
    {
        if(! _bg)
        {
            return;
        }
        draw_cells(force);
        // 256x256 map: hardware scroll = 8 - x, 48 - y (Butano centres BGs on the screen).
        _bg->set_position(8 - (_cam_x & 255), 48 - (_cam_y & 255));
        game_state& g = state();
        if(! _player)
        {
            _player = person_sprites[int(person_kind::player)]->create_sprite(0, 0, 0);
            _player->set_bg_priority(1);
            set_frame(*_player, person_kind::player, g.facing, 0);
            _grass_here = bn::sprite_items::grass_front.create_sprite(0, 0);
            _grass_from = bn::sprite_items::grass_front.create_sprite(0, 0);
            for(bn::sprite_ptr* s : { &*_grass_here, &*_grass_from })
            {
                s->set_bg_priority(1);
                s->set_z_order(-200);
            }
        }
        place_actor(*_player, _px, _py, _lift);
        for(actor& a : _actors)
        {
            int ox = 0, oy = 0;
            if(a.move_frames > 0)
            {
                ox = -dx_of(a.move_dir) * a.move_frames;
                oy = -dy_of(a.move_dir) * a.move_frames;
            }
            place_actor(*a.sprite, a.x * 16 + ox, a.y * 16 + oy);
        }
        auto place_grass = [&](bn::sprite_ptr& s, int tx, int ty)
        {
            bool on = behaviour_at(tx, ty) == behaviour::TALL_GRASS;
            s.set_visible(on);
            if(on)
            {
                s.set_position(sx(tx * 16 - _cam_x + 8), sy(ty * 16 + 8 - _cam_y + 4));
            }
        };
        place_grass(*_grass_here, _grass_x, _grass_y);
        place_grass(*_grass_from, _from_x, _from_y);
    }

    // Full-screen menus need the palettes and sprites the map uses, so the map steps aside meanwhile.
    void overworld::suspend()
    {
        ui::fade_out(8);
        _bg_map.reset();
        _bg.reset();
        _player.reset();
        _grass_here.reset();
        _grass_from.reset();
        for(actor& a : _actors)
        {
            a.sprite.reset();
        }
    }

    void overworld::resume()
    {
        build_bg();
        for(actor& a : _actors)
        {
            a.sprite = person_sprites[int(a.kind)]->create_sprite(0, 0, 0);
            a.sprite->set_bg_priority(1);
            set_frame(*a.sprite, a.kind, a.facing, 0);
        }
        refresh(true);
        ui::fade_in(8);
    }

    // ----- Frames -----
    void overworld::tick()
    {
        tick_actors();
        refresh();
        frame();
        ++state().play_frames;
    }

    void overworld::tick_actors()
    {
        for(actor& a : _actors)
        {
            if(a.move_frames > 0)
            {
                a.move_frames -= 1;
                set_frame(*a.sprite, a.kind, a.facing, a.move_frames > 8 ? (a.step_foot ? 1 : 2) : 0);
            }
        }
        if(++_wander_timer >= wander_interval)
        {
            _wander_timer = 0;
            wander();
        }
    }

    // startWander(): a wanderer takes one step, staying within 2 tiles of home, never onto you, a door or
    // another person.
    void overworld::wander()
    {
        bn::random& r = rng();
        for(actor& a : _actors)
        {
            if(! a.wander || a.move_frames || r.get_int(4) != 0)
            {
                continue;
            }
            direction d = direction(r.get_int(4));
            int nx = a.x + dx_of(d), ny = a.y + dy_of(d);
            a.facing = d;
            behaviour b = behaviour_at(nx, ny);
            bool free = (b == behaviour::WALK || b == behaviour::TALL_GRASS) && ! blocked(nx, ny) &&
                        bn::abs(nx - a.home_x) <= 2 && bn::abs(ny - a.home_y) <= 2 && find(nx, ny).link < 0;
            if(free)
            {
                a.x = nx;
                a.y = ny;
                a.move_dir = d;
                a.move_frames = walk_frames;
                a.step_foot = ! a.step_foot;
            }
            set_frame(*a.sprite, a.kind, a.facing, 0);
        }
    }

    // ----- The player -----
    void overworld::step(direction want)
    {
        game_state& g = state();
        // From a standstill, a new direction first turns you on the spot; keep holding and you walk.
        if(_fresh_press && want != g.facing)
        {
            g.facing = want;
            for(int f = 0; f < turn_frames; ++f)
            {
                set_frame(*_player, person_kind::player, g.facing, f < turn_frames / 2 ? 1 + _step_parity : 0);
                tick();
            }
            _step_parity ^= 1;
            _fresh_press = false;
            return;
        }
        g.facing = want;
        _fresh_press = false;
        int nx = g.x + dx_of(want);
        int ny = g.y + dy_of(want);
        place target_place = find(nx, ny);
        behaviour target = behaviour_at(nx, ny);
        int who = actor_at(nx, ny);

        // An area that isn't in this build yet.
        if(target_place.link >= 0 && target_place.map < 0)
        {
            set_frame(*_player, person_kind::player, g.facing, 0);
            bn::string<96> text(_map->links[target_place.link].name);
            text.append(" isn't in this test build yet.");
            gui().say(text);
            _fresh_press = true;
            while(bn::keypad::up_held() || bn::keypad::down_held() || bn::keypad::left_held() || bn::keypad::right_held())
            {
                tick();
            }
            return;
        }
        // Walking into an unbeaten trainer starts their battle.
        if(who >= 0 && _actors[who].active_trainer())
        {
            set_frame(*_player, person_kind::player, g.facing, 0);
            start_trainer_battle(who);
            return;
        }

        // Ledges: hop over to the tile beyond (32 frames for the two tiles).
        bool jump = (target == behaviour::LEDGE_DOWN && want == direction::DOWN) ||
                    (target == behaviour::LEDGE_RIGHT && want == direction::RIGHT) ||
                    (target == behaviour::LEDGE_LEFT && want == direction::LEFT);
        if(jump)
        {
            int bx = nx + dx_of(want), by = ny + dy_of(want);
            behaviour beyond = behaviour_at(bx, by);
            jump = (beyond == behaviour::WALK || beyond == behaviour::TALL_GRASS) && actor_at(bx, by) < 0;
        }
        bool walkable = target == behaviour::WALK || target == behaviour::TALL_GRASS || target == behaviour::DOOR ||
                        target == behaviour::MAT;
        if(! jump && (! walkable || who >= 0))
        {
            // Bump: walk on the spot.
            for(int f = 0; f < walk_frames; ++f)
            {
                set_frame(*_player, person_kind::player, g.facing, f < walk_frames / 2 ? 1 + _step_parity : 0);
                tick();
            }
            _step_parity ^= 1;
            return;
        }

        int tiles = jump ? 2 : 1;
        bool run = ! jump && bn::keypad::b_held() && ! _map->is_room();   // running shoes don't work indoors
        int frames = jump ? jump_frames : run ? run_frames : walk_frames;
        int start_px = _px, start_py = _py;
        _from_x = g.x;
        _from_y = g.y;
        _grass_x = g.x + dx_of(want) * tiles;
        _grass_y = g.y + dy_of(want) * tiles;
        // The tile is ours from the start of the step, so nobody wanders into it.
        int old_x = g.x, old_y = g.y;
        g.x = int8_t(g.x + dx_of(want) * tiles);
        g.y = int8_t(g.y + dy_of(want) * tiles);
        for(int f = 1; f <= frames; ++f)
        {
            _px = start_px + dx_of(want) * 16 * tiles * f / frames;
            _py = start_py + dy_of(want) * 16 * tiles * f / frames;
            _cam_x = _px - view_x;
            _cam_y = _py - view_y;
            _lift = jump ? (f * (frames - f) * 8) / (frames * frames / 4) : 0;
            set_frame(*_player, person_kind::player, g.facing, f <= frames / 2 ? 1 + _step_parity : 0);
            tick();
        }
        (void) old_x;
        (void) old_y;
        _lift = 0;
        _step_parity ^= 1;
        _from_x = _from_y = -1000;
        _grass_x = g.x;
        _grass_y = g.y;
        arrive();
    }

    void overworld::arrive()
    {
        game_state& g = state();
        // Crossed into a connected area: its coordinates take over (the camera doesn't move).
        place here = find(g.x, g.y);
        if(here.link >= 0 && here.map >= 0)
        {
            const link& l = _map->links[here.link];
            g.x = int8_t(here.x);
            g.y = int8_t(here.y);
            int px = _px - l.ox * 16, py = _py - l.oy * 16;
            _map_index = here.map;
            _map = &wd::maps[here.map];
            g.map = int8_t(here.map);
            _px = px;
            _py = py;
            _cam_x = _px - view_x;
            _cam_y = _py - view_y;
            _grass_x = g.x;
            _grass_y = g.y;
            load_actors();
            refresh(true);
            if(! (g.story & story_starter) && _map_index == 1)
            {
                starter_event();
                return;
            }
        }
        behaviour b = behaviour_at(g.x, g.y);
        if(b == behaviour::DOOR)
        {
            for(int i = 0; i < _map->doors_count; ++i)
            {
                if(_map->doors[i].x == g.x && _map->doors[i].y == g.y)
                {
                    enter_room(_map->doors[i]);
                    return;
                }
            }
        }
        if(b == behaviour::MAT && _map->is_room())
        {
            leave_room();
            return;
        }
        if(check_sight())
        {
            return;
        }
        if(b == behaviour::TALL_GRASS && rng().get_int(100) < encounter_percent && g.first_able() >= 0 && _map->pool_count)
        {
            wild_battle();
        }
    }

    // startWildBattle(): a pack of 1 up to half your (living) party, at most 4, picked from the area
    // without repeats, a little under your party's level.
    void overworld::wild_battle()
    {
        game_state& g = state();
        bn::random& r = rng();
        encounter& e = *_battle;
        e = encounter();
        int cap = bn::min(4, (g.able_count() + 1) / 2);
        e.count = 1 + r.get_int(bn::max(1, cap));
        species_id bag[16];
        int n = 0;
        for(int i = 0; i < _map->pool_count && n < 16; ++i)
        {
            bool dup = false;
            for(int k = 0; k < n; ++k)
            {
                dup |= bag[k] == _map->pool[i];
            }
            if(! dup)
            {
                bag[n++] = _map->pool[i];
            }
        }
        e.count = bn::min(e.count, n);
        for(int i = 0; i < e.count; ++i)
        {
            int k = r.get_int(n);
            e.species[i] = bag[k];
            bag[k] = bag[--n];
        }
        e.level = bn::max(2, bn::min(int(_map->level_cap), g.average_level() - 2 + r.get_int(3)));
        _start_battle = true;
    }

    // Trainers spot you when you walk into their line of sight (up to 5 tiles, nothing in between).
    bool overworld::check_sight()
    {
        game_state& g = state();
        for(int i = 0; i < _actors.size(); ++i)
        {
            actor& a = _actors[i];
            if(! a.active_trainer())
            {
                continue;
            }
            for(int s = 1; s <= sight; ++s)
            {
                int tx = a.x + dx_of(a.facing) * s, ty = a.y + dy_of(a.facing) * s;
                if(tx == g.x && ty == g.y)
                {
                    trainer_approach(i);
                    return true;
                }
                behaviour b = behaviour_at(tx, ty);
                if(b != behaviour::WALK && b != behaviour::TALL_GRASS && b != behaviour::WATER)
                {
                    break;
                }
            }
        }
        return false;
    }

    // Spotted (Emerald): "!" pops over the trainer for about a second, they walk up tile by tile until
    // they're next to you, you face each other, then the challenge.
    void overworld::trainer_approach(int index)
    {
        game_state& g = state();
        actor& a = _actors[index];
        set_frame(*_player, person_kind::player, g.facing, 0);
        bn::sprite_ptr bang = bn::sprite_items::bang.create_sprite(0, 0);
        bang.set_bg_priority(0);
        for(int f = 0; f < 60; ++f)
        {
            int rise = bn::min(f, 6);
            bang.set_position(sx(a.x * 16 - _cam_x + 8), sy(a.y * 16 - _cam_y - 26 - rise / 2));
            tick();
        }
        bang.set_visible(false);
        while(bn::abs(a.x - g.x) + bn::abs(a.y - g.y) > 1)
        {
            a.facing = toward(a.x, a.y, g.x, g.y);
            a.x += dx_of(a.facing);
            a.y += dy_of(a.facing);
            a.move_dir = a.facing;
            a.move_frames = walk_frames;
            a.step_foot = ! a.step_foot;
            while(a.move_frames > 0)
            {
                tick();
            }
        }
        start_trainer_battle(index);
    }

    void overworld::start_trainer_battle(int index)
    {
        game_state& g = state();
        actor& a = _actors[index];
        g.facing = toward(g.x, g.y, a.x, a.y);
        a.facing = opposite(g.facing);
        set_frame(*_player, person_kind::player, g.facing, 0);
        set_frame(*a.sprite, a.kind, a.facing, 0);
        tick();
        if(g.first_able() < 0)
        {
            gui().say("Your whole party has fainted! Rest at the POKéMON CENTER.");
            return;
        }
        bn::string<160> text(a.tr->title);
        text.append(": \"");
        text.append(a.tr->intro);
        text.append("\"");
        gui().say(text);
        encounter& e = *_battle;
        e = encounter();
        e.trainer = true;
        e.map = _map_index;
        e.trainer_index = a.tr - _map->trainers;
        _start_battle = true;
    }

    void overworld::interact()
    {
        game_state& g = state();
        ui& u = gui();
        int fx = g.x + dx_of(g.facing);
        int fy = g.y + dy_of(g.facing);
        // Talk across a counter.
        if(behaviour_at(fx, fy) == behaviour::COUNTER)
        {
            fx += dx_of(g.facing);
            fy += dy_of(g.facing);
        }
        int who = actor_at(fx, fy);
        if(who >= 0)
        {
            talk_to(who);
            return;
        }
        place spot = find(g.x + dx_of(g.facing), g.y + dy_of(g.facing));
        if(spot.map < 0)
        {
            return;
        }
        const map_def& m = wd::maps[spot.map];
        behaviour b = behaviour_at(g.x + dx_of(g.facing), g.y + dy_of(g.facing));
        for(int i = 0; i < m.signs_count; ++i)
        {
            if(m.signs[i].x == spot.x && m.signs[i].y == spot.y)
            {
                for(int k = 0; k < m.signs[i].lines_count; ++k)
                {
                    u.say(m.signs[i].lines[k]);
                }
                return;
            }
        }
        if(b == behaviour::ITEM)
        {
            for(int i = 0; i < m.items_count; ++i)
            {
                const item_ball& it = m.items[i];
                if(it.x == spot.x && it.y == spot.y)
                {
                    g.picked.set(it.id);
                    refresh(true);
                    obtain(it.item, 1);
                    return;
                }
            }
        }
        if(b == behaviour::PC)
        {
            pc_screen();
            return;
        }
        if(b == behaviour::WATER)
        {
            u.say("The water is a deep, clear blue.");
            return;
        }
        for(int i = 0; i < m.things_count; ++i)
        {
            if(m.things[i].x == spot.x && m.things[i].y == spot.y)
            {
                for(int k = 0; k < m.things[i].lines_count; ++k)
                {
                    u.say(m.things[i].lines[k]);
                }
                return;
            }
        }
    }

    void overworld::talk_to(int index)
    {
        game_state& g = state();
        actor& a = _actors[index];
        // faceNpcToPlayer()
        a.facing = toward(a.x, a.y, g.x, g.y);
        set_frame(*a.sprite, a.kind, a.facing, 0);
        tick();
        if(a.tr)
        {
            if(a.active_trainer())
            {
                start_trainer_battle(index);
                return;
            }
            bn::string<160> text(a.tr->title);
            text.append(": \"");
            text.append(a.tr->after);
            text.append("\"");
            gui().say(text);
            return;
        }
        const person& p = *a.who;
        switch(p.role)
        {
        case person_role::NURSE:
            nurse(index);
            break;
        case person_role::MOM:
            mom();
            break;
        case person_role::CLERK:
            clerk();
            break;
        default:
            for(int i = 0; i < p.lines_count; ++i)
            {
                gui().say(p.lines[i]);
            }
            break;
        }
    }

    // ----- Places -----
    void overworld::enter_room(const door& d)
    {
        game_state& g = state();
        set_frame(*_player, person_kind::player, g.facing, 0);
        ui::fade_out(12);
        const map_def& room = wd::maps[d.room];
        g.x = room.spawn_x;
        g.y = room.spawn_y;
        g.facing = direction::UP;
        _player.reset();
        load_map(d.room);
        ui::fade_in(12);
        _fresh_press = true;
    }

    // Coming out: you appear in the doorway and step down onto the path.
    void overworld::leave_room()
    {
        game_state& g = state();
        ui::fade_out(12);
        int area = _map->exit_map;
        g.x = _map->exit_x;
        g.y = _map->exit_y;
        g.facing = direction::DOWN;
        _player.reset();
        load_map(area);
        ui::fade_in(12);
        int start_py = _py;
        for(int f = 1; f <= walk_frames; ++f)
        {
            _py = start_py + 16 * f / walk_frames;
            _cam_y = _py - view_y;
            set_frame(*_player, person_kind::player, g.facing, f <= walk_frames / 2 ? 1 + _step_parity : 0);
            tick();
        }
        _step_parity ^= 1;
        g.y = int8_t(g.y + 1);
        _grass_y = g.y;
        _fresh_press = true;
    }

    // The POKéMON CENTER nurse (Emerald's script, as the web game's nurseTalk/nurseHeal).
    void overworld::nurse(int index)
    {
        ui& u = gui();
        actor& n = _actors[index];
        u.say("Hello, and welcome to the POKéMON CENTER.");
        u.say("We restore your tired POKéMON to full health.");
        u.show_text("Would you like to rest your POKéMON?");
        bool yes = u.yes_no();
        u.clear_text();
        if(! yes)
        {
            u.say("We hope to see you again!");
            return;
        }
        u.show_text("Okay, I'll take your POKéMON for a few seconds.");
        wait(30);
        set_frame(*n.sprite, n.kind, direction::LEFT, 0);
        // A ball for each party member, then the machine's jingle (pokeemerald timings, shortened).
        wait(4 + 25 * (state().party_count - 1) + 32);
        bn::bg_palettes::set_fade(bn::color(31, 31, 31), bn::fixed(0.25));
        wait(20);
        bn::bg_palettes::set_fade(bn::color(31, 31, 31), 0);
        wait(40);
        set_frame(*n.sprite, n.kind, direction::DOWN, 0);
        state().heal_party();
        u.clear_text();
        u.say("Thank you for waiting.");
        u.say("We've restored your POKéMON to full health.");
        u.say("We hope to see you again!");
    }

    void overworld::mom()
    {
        ui& u = gui();
        if(! state().party_count)
        {
            bn::string<128> text("MOM: ");
            text.append(state().name);
            text.append("! The professor went out toward ROUTE 1. Go on, catch up with him!");
            u.say(text);
            return;
        }
        bn::string<128> text("MOM: Welcome home, ");
        text.append(state().name);
        text.append("! You and your POKéMON look worn out. Sit down and rest a while.");
        u.say(text);
        state().heal_party();
        u.say("MOM: There, all better! Come home whenever you need a rest. Take care out there!");
    }

    void overworld::obtain(item_id id, int count)
    {
        ui& u = gui();
        game_state& g = state();
        const item_info& it = game_data::items[int(id)];
        g.add_item(id, count);
        bn::string<64> text("Obtained ");
        if(count > 1)
        {
            text.append(bn::to_string<4>(count));
            text.append(" ");
            text.append(it.name);
            text.append("S!");
        }
        else
        {
            text.append("the ");
            text.append(it.name);
            text.append("!");
        }
        u.say(text);
        bn::string<96> put(g.name);
        put.append(" put away the ");
        put.append(it.name);
        put.append(it.pocket == 1 ? " in the POKé BALLS POCKET." : " in the ITEMS POCKET.");
        u.say(put);
    }

    void overworld::clerk()
    {
        game_state& g = state();
        if(! g.mart_gift)
        {
            g.mart_gift = true;
            gui().say("Welcome to the POKé MART!");
            gui().say("You're new in town? Here, take these on the house!");
            obtain(item_id::POKEBALL, 5);
            return;
        }
        mart();
    }

    // martOpen(): BUY / SELL / SEE YA!, with your money in a window top left.
    void overworld::mart()
    {
        game_state& g = state();
        ui& u = gui();
        bn::vector<bn::sprite_ptr, 8> money;
        auto show_money = [&]()
        {
            money.clear();
            u.win().box(window_style::WINDOW, 0, 0, 12, 4);
            u.print(10, 9, "MONEY", text_color::INK, money);
            bn::string<16> m("$");
            m.append(bn::to_string<8>(g.money));
            u.print(86 - u.width(m), 9, m, text_color::INK, money);
        };
        show_money();
        while(true)
        {
            u.show_text("Welcome! How may I serve you?");
            constexpr bn::string_view top[] = { "BUY", "SELL", "SEE YA!" };
            int pick = u.list(top, 3);
            u.clear_text();
            if(pick == 0)
            {
                constexpr int stock_count = int(sizeof(game_data::mart_stock) / sizeof(game_data::mart_stock[0]));
                bn::string<32> labels[stock_count + 1];
                bn::string_view views[stock_count + 1];
                for(int i = 0; i < stock_count; ++i)
                {
                    const item_info& it = game_data::items[int(game_data::mart_stock[i])];
                    labels[i] = it.name;
                    labels[i].append("  $");
                    labels[i].append(bn::to_string<6>(it.price));
                    views[i] = labels[i];
                }
                views[stock_count] = "CANCEL";
                int last = 0;
                while(true)
                {
                    u.show_text("What would you like?");
                    int k = u.list(views, stock_count + 1, last);
                    u.clear_text();
                    if(k < 0 || k == stock_count)
                    {
                        break;
                    }
                    last = k;
                    item_id id = game_data::mart_stock[k];
                    const item_info& it = game_data::items[int(id)];
                    int max = bn::min(99, int(g.money / it.price));
                    if(! max)
                    {
                        u.say("You don't have enough money.");
                        continue;
                    }
                    int qs[3] = { 1, 5, 10 };
                    bn::string<24> ql[4];
                    bn::string_view qv[4];
                    int qn = 0;
                    for(int q : qs)
                    {
                        if(q <= max)
                        {
                            ql[qn] = "x";
                            ql[qn].append(bn::to_string<4>(q));
                            ql[qn].append("  $");
                            ql[qn].append(bn::to_string<8>(q * it.price));
                            qv[qn] = ql[qn];
                            ++qn;
                        }
                    }
                    qv[qn] = "CANCEL";
                    bn::string<96> text(it.name);
                    text.append("? Certainly. How many would you like?");
                    u.show_text(text);
                    int qi = u.list(qv, qn + 1);
                    u.clear_text();
                    if(qi < 0 || qi == qn)
                    {
                        continue;
                    }
                    int q = qs[qi];
                    text = it.name;
                    text.append(", and you want ");
                    text.append(bn::to_string<4>(q));
                    text.append("? That will be $");
                    text.append(bn::to_string<8>(q * it.price));
                    text.append(". OK?");
                    u.show_text(text);
                    bool yes = u.yes_no();
                    u.clear_text();
                    if(! yes)
                    {
                        continue;
                    }
                    g.money -= uint32_t(q * it.price);
                    g.add_item(id, q);
                    show_money();
                    u.say("Here you go! Thank you very much.");
                }
            }
            else if(pick == 1)
            {
                // SELL: anything you carry, at half price.
                while(true)
                {
                    int ids[items_count];
                    bn::string<32> labels[items_count + 1];
                    bn::string_view views[items_count + 1];
                    int n = 0;
                    for(int i = 0; i < items_count; ++i)
                    {
                        if(g.items[i] && game_data::items[i].price)
                        {
                            ids[n] = i;
                            labels[n] = game_data::items[i].name;
                            labels[n].append(" x");
                            labels[n].append(bn::to_string<4>(g.items[i]));
                            views[n] = labels[n];
                            ++n;
                        }
                    }
                    if(! n)
                    {
                        u.say("You don't have anything to sell.");
                        break;
                    }
                    views[n] = "CANCEL";
                    u.show_text("What would you like to sell?");
                    int k = u.list(views, bn::min(n + 1, 6));
                    u.clear_text();
                    if(k < 0 || k >= n)
                    {
                        break;
                    }
                    const item_info& it = game_data::items[ids[k]];
                    int have = g.items[ids[k]];
                    int each = it.price / 2;
                    int qs[3] = { 1, 5, have };
                    bn::string<24> ql[4];
                    bn::string_view qv[4];
                    int qn = 0, qvals[3];
                    for(int q : qs)
                    {
                        bool dup = false;
                        for(int j = 0; j < qn; ++j)
                        {
                            dup |= qvals[j] == q;
                        }
                        if(q <= have && ! dup)
                        {
                            qvals[qn] = q;
                            ql[qn] = "x";
                            ql[qn].append(bn::to_string<4>(q));
                            ql[qn].append("  $");
                            ql[qn].append(bn::to_string<8>(q * each));
                            qv[qn] = ql[qn];
                            ++qn;
                        }
                    }
                    qv[qn] = "CANCEL";
                    u.show_text("How many would you like to sell?");
                    int qi = u.list(qv, qn + 1);
                    u.clear_text();
                    if(qi < 0 || qi == qn)
                    {
                        continue;
                    }
                    int q = qvals[qi];
                    bn::string<64> text("I can pay $");
                    text.append(bn::to_string<8>(q * each));
                    text.append(". Would that be OK?");
                    u.show_text(text);
                    bool yes = u.yes_no();
                    u.clear_text();
                    if(! yes)
                    {
                        continue;
                    }
                    g.items[ids[k]] = uint8_t(have - q);
                    g.money += uint32_t(q * each);
                    show_money();
                    text = "Turned over the ";
                    text.append(it.name);
                    text.append(" and received $");
                    text.append(bn::to_string<8>(q * each));
                    text.append(".");
                    u.say(text);
                }
            }
            else
            {
                break;
            }
        }
        money.clear();
        u.win().clear(0, 0, 12, 4);
        u.say("Please come again!");
    }

    // ----- Story: the professor and your first partner (starterEvent / starterBag / starterThanks) -----
    int overworld::add_temp_actor(person_kind kind, int x, int y, direction facing)
    {
        actor a;
        a.kind = kind;
        a.x = a.home_x = x;
        a.y = a.home_y = y;
        a.facing = facing;
        a.sprite = person_sprites[int(kind)]->create_sprite(0, 0, 0);
        a.sprite->set_bg_priority(1);
        set_frame(*a.sprite, kind, facing, 0);
        _actors.push_back(a);
        refresh();
        return _actors.size() - 1;
    }

    // On ROUTE 1 without a POKéMON: the professor runs up, chased by a wild ZIGZAGOON.
    void overworld::starter_event()
    {
        game_state& g = state();
        ui& u = gui();
        set_frame(*_player, person_kind::player, g.facing, 0);
        u.say("H-help me!");
        // He comes running in from the east to stand two tiles from you.
        int tx = g.x + 2, ty = g.y;
        if(blocked(tx, ty))
        {
            tx = g.x;
            ty = g.y - 2;
        }
        int prof = add_temp_actor(person_kind::prof, tx + 4, ty, direction::LEFT);
        actor& p = _actors[prof];
        for(int i = 0; i < 4; ++i)
        {
            p.x -= 1;
            p.move_dir = direction::LEFT;
            p.move_frames = walk_frames;
            p.step_foot = ! p.step_foot;
            while(_actors[prof].move_frames > 0)
            {
                tick();
            }
        }
        if(p.x != tx || p.y != ty)
        {
            p.x = tx;
            p.y = ty;
        }
        g.facing = toward(g.x, g.y, p.x, p.y);
        set_frame(*_player, person_kind::player, g.facing, 0);
        tick();
        bn::string<96> text(game_data::prof_name);
        text.append(": Hello! You over there! Please! Help me!");
        u.say(text);
        u.say("A wild ZIGZAGOON is after me! In my BAG! There's a POKé BALL in there!");
        suspend();
        int pick = starter_bag();
        resume();
        mon partner = mon::make(game_data::starters[pick], 5);
        bool to_box = false;
        g.add_mon(partner, to_box);
        g.owned.set(partner.species_index);
        g.seen.set(partner.species_index);
        g.story = uint8_t(g.story | story_starter | story_thanks);
        text = g.name;
        text.append(" chose ");
        text.append(partner.name());
        text.append("!");
        u.say(text);
        // ...and straight into battle with the ZIGZAGOON (Lv2).
        encounter& e = *_battle;
        e = encounter();
        e.species[0] = species_id::ZIGZAGOON;
        e.count = 1;
        e.level = 2;
        _start_battle = true;
    }

    // Emerald's bag screen: three POKé BALLS; Left/Right to choose, A to look.
    int overworld::starter_bag()
    {
        ui& u = gui();
        int index = 1;
        int result = 0;
        {
            bn::regular_bg_ptr bg = bn::regular_bg_items::bag_bg.create_bg(8, 48);
            bg.set_priority(3);
            bn::vector<bn::sprite_ptr, 3> mons;
            for(int i = 0; i < 3; ++i)
            {
                const species& s = game_data::species_list[int(game_data::starters[i])];
                mons.push_back(s.front.create_sprite(sx(48 + i * 72), sy(56)));
                mons.back().set_bg_priority(2);
            }
            bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(0, 0);
            cursor.set_bg_priority(0);
            ui::fade_in(8);
            while(true)
            {
                const species& s = game_data::species_list[int(game_data::starters[index])];
                bn::string<80> text("The ");
                text.append(type_name(s.type1));
                text.append(" POKéMON ");
                text.append(s.name);
                text.append(".");
                u.show_text(text);
                cursor.set_position(sx(48 + index * 72 - 34), sy(56));
                frame();
                if(bn::keypad::left_pressed())
                {
                    index = (index + 2) % 3;
                }
                else if(bn::keypad::right_pressed())
                {
                    index = (index + 1) % 3;
                }
                else if(bn::keypad::a_pressed())
                {
                    text = "Do you choose this POKéMON? The ";
                    text.append(type_name(s.type1));
                    text.append(" POKéMON ");
                    text.append(s.name);
                    text.append("!");
                    u.show_text(text);
                    if(u.yes_no())
                    {
                        result = index;
                        break;
                    }
                }
            }
            u.clear_text();
            ui::fade_out(8);
        }
        return result;
    }

    // After the first battle, the professor thanks you and heads off.
    void overworld::starter_thanks()
    {
        game_state& g = state();
        ui& u = gui();
        g.story = uint8_t(g.story & ~story_thanks);
        int tx = g.x + 2, ty = g.y;
        if(blocked(tx, ty))
        {
            tx = g.x - 2;
        }
        int prof = add_temp_actor(person_kind::prof, tx, ty, toward(tx, ty, g.x, g.y));
        g.facing = toward(g.x, g.y, tx, ty);
        set_frame(*_player, person_kind::player, g.facing, 0);
        tick();
        bn::string<128> text(game_data::prof_name);
        text.append(": Whew... I went into the tall grass to look at wild POKéMON, and it jumped me!");
        u.say(text);
        u.say("You saved me. Thanks a lot!");
        text = "That ";
        text.append(g.party_count ? g.party[0].name() : "POKéMON");
        text.append(" seems to like you. Please, keep it!");
        u.say(text);
        u.say("Travel with it and fill up your POKéDEX. I'll be watching your progress!");
        // He walks off and is gone.
        actor& p = _actors[prof];
        for(int i = 0; i < 6; ++i)
        {
            direction d = tx > g.x ? direction::RIGHT : direction::LEFT;
            p.facing = d;
            p.move_dir = d;
            p.x += dx_of(d);
            p.move_frames = walk_frames;
            p.step_foot = ! p.step_foot;
            while(_actors[prof].move_frames > 0)
            {
                tick();
            }
        }
        _actors.erase(_actors.begin() + prof);
        refresh();
    }

    // ----- START -----
    void overworld::start_menu()
    {
        static int last = 0;
        game_state& g = state();
        ui& u = gui();
        while(! _quit)
        {
            bn::string_view options[] = { "POKéMON", "BAG", g.name, "SAVE", "TITLE", "EXIT" };
            // Emerald: a window in the top right, sized to its longest entry.
            int widest = 0;
            for(const bn::string_view& o : options)
            {
                widest = bn::max(widest, u.width(o));
            }
            menu_spec s;
            s.options = options;
            s.count = 6;
            s.tw = (widest + 16 + 12 + 7) / 8;
            s.tx = 30 - s.tw;
            s.ty = 0;
            s.th = s.count * 2 + 2;
            s.start = last;
            int pick = u.menu(s);
            if(pick < 0 || pick == 5)
            {
                return;
            }
            last = pick;
            if(pick == 0)
            {
                suspend();
                party_screen(party_mode::FIELD);
                resume();
            }
            else if(pick == 1)
            {
                suspend();
                bag_screen(bag_mode::FIELD);
                resume();
            }
            else if(pick == 2)
            {
                card();
            }
            else if(pick == 3)
            {
                save_menu();
                return;
            }
            else if(pick == 4)
            {
                u.show_text("Return to the title screen? Unsaved progress will be lost.");
                bool yes = u.yes_no(false);
                u.clear_text();
                if(yes)
                {
                    _quit = true;
                }
            }
        }
    }

    bn::string<16> play_time(uint32_t frames)
    {
        int minutes = int(frames / 3600);
        bn::string<16> t(bn::to_string<6>(minutes / 60));
        t.append(":");
        if(minutes % 60 < 10)
        {
            t.append("0");
        }
        t.append(bn::to_string<4>(minutes % 60));
        return t;
    }

    // TRAINER CARD, in short.
    void overworld::card()
    {
        game_state& g = state();
        bn::string<128> text("NAME: ");
        text.append(g.name);
        text.append("   MONEY: $");
        text.append(bn::to_string<8>(g.money));
        text.append("\nPOKéDEX: ");
        text.append(bn::to_string<4>(g.owned.count()));
        text.append("   TIME: ");
        text.append(play_time(g.play_frames));
        gui().say(text);
    }

    // Emerald's save: an info window top left (place, PLAYER, BADGES, POKéDEX, TIME), YES/NO, "SAVING...",
    // then "{PLAYER} saved the game."
    void overworld::save_menu()
    {
        game_state& g = state();
        ui& u = gui();
        bn::vector<bn::sprite_ptr, 24> info;
        u.win().box(window_style::WINDOW, 0, 0, 16, 12);
        const map_def& here = _map->is_room() ? wd::maps[_map->exit_map] : *_map;
        u.print(10, 8, here.name, text_color::INK, info);
        auto row = [&](int i, const char* label, const bn::string_view& value)
        {
            u.print(10, 26 + i * 16, label, text_color::INK, info);
            u.print(118 - u.width(value), 26 + i * 16, value, text_color::INK, info);
        };
        row(0, "PLAYER", g.name);
        row(1, "BADGES", "0");
        row(2, "POKéDEX", bn::to_string<4>(g.owned.count()));
        row(3, "TIME", play_time(g.play_frames));
        u.show_text("Would you like to save the game?");
        bool yes = u.yes_no();
        if(yes)
        {
            u.say_timed("SAVING... DON'T TURN OFF THE POWER.", 40);
            save_game();
            bn::string<48> text(g.name);
            text.append(" saved the game.");
            u.say_timed(text, 60);
        }
        u.clear_text();
        info.clear();
        u.win().clear(0, 0, 16, 12);
    }

    // ----- Main loop -----
    bool overworld::run(encounter& battle)
    {
        _battle = &battle;
        game_state& g = state();
        refresh(true);
        ui::fade_in(12);
        if((g.story & story_thanks) && _map_index == 1)
        {
            starter_thanks();
        }
        while(! _quit && ! _start_battle)
        {
            direction want = g.facing;
            bool dir_held = true;
            if(bn::keypad::up_held()) want = direction::UP;
            else if(bn::keypad::down_held()) want = direction::DOWN;
            else if(bn::keypad::left_held()) want = direction::LEFT;
            else if(bn::keypad::right_held()) want = direction::RIGHT;
            else dir_held = false;

            if(bn::keypad::start_pressed())
            {
                start_menu();
                tick();
                continue;
            }
            if(bn::keypad::a_pressed())
            {
                interact();
                tick();
                continue;
            }
            if(! dir_held)
            {
                rng().update();
                _fresh_press = true;
                set_frame(*_player, person_kind::player, g.facing, 0);
                tick();
                continue;
            }
            step(want);
        }
        if(_start_battle)
        {
            // Battle intro: the screen flashes twice, then fades.
            for(int i = 0; i < 2; ++i)
            {
                bn::bg_palettes::set_fade(bn::color(31, 31, 31), 1);
                wait(4);
                bn::bg_palettes::set_fade(bn::color(31, 31, 31), 0);
                wait(4);
            }
            ui::fade_out(20);
            return true;
        }
        ui::fade_out(12);
        return false;
    }
}

bool overworld_scene(encounter& battle)
{
    // The scene lives on the heap (EWRAM), not the small IWRAM stack.
    bn::unique_ptr<overworld> scene(new overworld());
    return scene->run(battle);
}

}
