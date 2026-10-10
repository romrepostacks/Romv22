// The overworld: the web game's areas and rooms drawn from generated metatiles and scrolled with the player,
// Emerald-style connections between areas (each area's tileset holds what can be seen of its neighbours),
// water and flowers that sway, weather and the time of day, Emerald step timing (16 frames a step, 8
// running), SURF and DIVE, ledges, tall grass, people who wander and trainers who spot you.
// Talking, places, the story and the START menu are in pr_field.cpp.
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_blending.h"
#include "bn_common.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_span.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_unique_ptr.h"

#include "bn_sprite_items_bang.h"
#include "bn_sprite_items_dive.h"
#include "bn_sprite_items_dust.h"
#include "bn_sprite_items_glint.h"
#include "bn_sprite_items_grass_front.h"
#include "bn_sprite_items_surf.h"
#include "bn_regular_bg_items_wx_ash_1.h"
#include "bn_regular_bg_items_wx_ash_2.h"
#include "bn_regular_bg_items_wx_ash_3.h"
#include "bn_regular_bg_items_wx_cave.h"
#include "bn_regular_bg_items_wx_deep_1.h"
#include "bn_regular_bg_items_wx_deep_2.h"
#include "bn_regular_bg_items_wx_deep_3.h"
#include "bn_regular_bg_items_wx_fog.h"
#include "bn_regular_bg_items_wx_ghost_1.h"
#include "bn_regular_bg_items_wx_ghost_2.h"
#include "bn_regular_bg_items_wx_ghost_3.h"
#include "bn_regular_bg_items_wx_rain_1.h"
#include "bn_regular_bg_items_wx_rain_2.h"
#include "bn_regular_bg_items_wx_rain_3.h"
#include "bn_regular_bg_items_wx_snow_1.h"
#include "bn_regular_bg_items_wx_snow_2.h"
#include "bn_regular_bg_items_wx_snow_3.h"

#include "pr_audio.h"
#include "pr_game_data.h"
#include "pr_icons.h"
#include "pr_overworld_impl.h"
#include "pr_people_sprites.h"
#include "pr_ui.h"

namespace pr
{

namespace
{
    alignas(int) BN_DATA_EWRAM_BSS bn::regular_bg_map_cell bg_cells[32 * 32];

    // Ash you've walked through stays swept for the rest of the session (map.ashClean): a bit per tile, for
    // up to two ash areas.
    constexpr int ash_areas = 2;
    BN_DATA_EWRAM_BSS uint8_t ash_bits[ash_areas][1024];
    int ash_owner[ash_areas] = { -1, -1 };

    uint8_t* ash_of(int area, bool create)
    {
        for(int i = 0; i < ash_areas; ++i)
        {
            if(ash_owner[i] == area)
            {
                return ash_bits[i];
            }
        }
        if(! create)
        {
            return nullptr;
        }
        for(int i = 0; i < ash_areas; ++i)
        {
            if(ash_owner[i] < 0)
            {
                ash_owner[i] = area;
                return ash_bits[i];
            }
        }
        return nullptr;
    }

    walk_back pending_walk;
    int shown_place = -1;       // owShownLoc: the plank shows each time you arrive somewhere new

    // Sprite sheet frames: down 0-2, up 3-5, left 6-8 (right is left mirrored); 0 stands, 1 and 2 step.
    void set_frame(bn::sprite_ptr& sprite, person_kind kind, direction d, int step)
    {
        int base = d == direction::DOWN ? 0 : d == direction::UP ? 3 : 6;
        sprite.set_tiles(person_sprites[int(kind)]->tiles_item(), base + step);
        sprite.set_horizontal_flip(d == direction::RIGHT);
    }

    // The time of day's tint (.tod-layer): a colour over everything at some strength.
    struct tint
    {
        int r, g, b;
        int alpha_256;
    };
}

walk_back& pending_walk_back()
{
    return pending_walk;
}

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

namespace
{
    bool picked_at(int map, int tx, int ty)
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

    // League gates (leagueGates): open once the Elite Four trainer below is beaten.
    bool gate_open(const map_def& m, int tx, int ty)
    {
        if(! m.room || ! m.room->gates_count)
        {
            return false;
        }
        for(int i = 0; i < m.room->gates_count; ++i)
        {
            const league_gate& g = m.room->gates[i];
            if(g.y == ty && tx >= g.x0 && tx <= g.x1)
            {
                for(int k = 0; k < m.trainers_count; ++k)
                {
                    if(m.trainers[k].elite == g.elite)
                    {
                        return state().beaten.test(m.trainers[k].id);
                    }
                }
            }
        }
        return false;
    }
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
    if(p.link < 0)
    {
        int i = p.y * m.w + p.x;
        behaviour b = behaviour(m.behaviours[i]);
        if(b == behaviour::ITEM)
        {
            for(int k = 0; k < m.items_count; ++k)
            {
                if(m.items[k].x == p.x && m.items[k].y == p.y && state().picked.test(m.items[k].id))
                {
                    return m.items[k].ground;
                }
            }
        }
        if(m.room && m.room->gates_count && gate_open(m, p.x, p.y))
        {
            return m.room->gate_metatile;
        }
        if(m.area && m.area->clean)
        {
            if(const uint8_t* bits = ash_of(p.map, false))
            {
                if((bits[i >> 3] >> (i & 7)) & 1)
                {
                    return m.area->clean[i];
                }
            }
        }
        return m.map[i];
    }
    const link& l = _map->links[p.link];
    if(p.x < l.sx || p.y < l.sy || p.x >= l.sx + l.sw || p.y >= l.sy + l.sh)
    {
        return ts.fill_metatile;
    }
    for(int k = 0; k < l.items_count; ++k)
    {
        if(l.items[k].x == p.x && l.items[k].y == p.y && picked_at(p.map, p.x, p.y))
        {
            return l.items[k].ground;
        }
    }
    return l.strip[(p.y - l.sy) * l.sw + (p.x - l.sx)];
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
    if(b == behaviour::ITEM && picked_at(p.map, p.x, p.y))
    {
        return behaviour::WALK;
    }
    if(b == behaviour::SOLID && m.room && gate_open(m, p.x, p.y))
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

bool overworld::walkable(int tx, int ty) const
{
    behaviour b = behaviour_at(tx, ty);
    return b == behaviour::WALK || b == behaviour::TALL_GRASS || b == behaviour::DOOR || b == behaviour::MAT ||
           (b >= behaviour::WIND_UP && b <= behaviour::WIND_RIGHT);
}

bool overworld::water_at(int tx, int ty) const
{
    return behaviour_at(tx, ty) == behaviour::WATER;
}

bool overworld::blocked(int tx, int ty) const
{
    return ! walkable(tx, ty) || actor_at(tx, ty) >= 0 || (tx == state().x && ty == state().y);
}

int overworld::area_index() const
{
    return _map->is_room() ? _map->exit_map : _map_index;
}

const area_info* overworld::area() const
{
    return wd::maps[area_index()].area;
}

bool overworld::area_flag(uint16_t flag) const
{
    const area_info* a = area();
    return a && (a->flags & flag);
}

bool overworld::deep() const
{
    return ! _map->is_room() && _map->area && _map->area->theme == area_theme::DEEP;
}

bool overworld::dive_spot_here() const
{
    const area_info* a = _map->area;
    if(_map->is_room() || ! a)
    {
        return false;
    }
    for(int i = 0; i < a->dive_count; ++i)
    {
        if(a->dive_spots[i * 2] == state().x && a->dive_spots[i * 2 + 1] == state().y)
        {
            return true;
        }
    }
    return false;
}

bool overworld::shaft_here() const
{
    const area_info* a = _map->area;
    if(_map->is_room() || ! a)
    {
        return false;
    }
    for(int i = 0; i < a->shaft_count; ++i)
    {
        if(a->shafts[i * 2] == state().x && a->shafts[i * 2 + 1] == state().y)
        {
            return true;
        }
    }
    return false;
}

void overworld::sweep_ash(int tx, int ty)
{
    // weatherStep(): walking through ashy grass knocks the ash off.
    const area_info* a = _map->area;
    if(! a || ! a->clean || _map->is_room() || tx < 0 || ty < 0 || tx >= _map->w || ty >= _map->h)
    {
        return;
    }
    int i = ty * _map->w + tx;
    if(a->clean[i] == _map->map[i])
    {
        return;
    }
    if(uint8_t* bits = ash_of(_map_index, true))
    {
        if(! ((bits[i >> 3] >> (i & 7)) & 1))
        {
            bits[i >> 3] = uint8_t(bits[i >> 3] | (1 << (i & 7)));
            _last_cx = 0x7fffffff;      // redraw
        }
    }
}

void overworld::build_bg()
{
    // The tileset's tiles go at index 0 so the generated cells can be used as they are.
    // The weather's palette goes too: left in place it can split the free banks so a 14-bank tileset
    // doesn't fit. update_weather() brings it back after the map.
    _weather.reset();
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
    _anim_b = false;
    bn::bg_palettes::set_transparent_color(_map->is_room() ? bn::color(0, 0, 0) : bn::color(4, 9, 3));
}

// Water and flowers change every half second (artWater / artFlower).
void overworld::animate_tiles()
{
    if(! _bg)
    {
        return;
    }
    const tileset& ts = wd::tilesets[_tileset];
    if(! ts.anim_count)
    {
        return;
    }
    if(++_anim_timer < 30)
    {
        return;
    }
    _anim_timer = 0;
    _anim_b = ! _anim_b;
    bn::regular_bg_tiles_ptr tiles = _bg->tiles();
    bn::optional<bn::span<bn::tile>> vram = tiles.vram();
    if(! vram)
    {
        return;
    }
    bn::span<bn::tile>& v = *vram;
    for(int i = 0; i < ts.anim_count; ++i)
    {
        int index = ts.anim_index[i];
        v[index] = _anim_b ? ts.anim_tiles[i] : ts.tiles[index];
    }
}

void overworld::load_map(int index, bool keep_bg)
{
    game_state& g = state();
    _map_index = index;
    _map = &wd::maps[index];
    g.map = int16_t(index);
    g.region = uint8_t(bn::max(int(g.region), map_region(index) - 1));
    if(! _map->is_room())
    {
        g.visited.set(index);
        roamers_move();
    }
    _px = g.x * 16;
    _py = g.y * 16;
    _cam_x = _px - view_x;
    _cam_y = _py - view_y;
    if(_map->tileset != _tileset || ! _bg || ! keep_bg)
    {
        _tileset = _map->tileset;
        build_bg();
    }
    load_actors();
    update_glints();
    _grass_x = g.x;
    _grass_y = g.y;
    _from_x = _from_y = -1000;
    refresh(true);
    update_weather();
    update_tint();
    show_place_name();
}

// The area's name on the plank (with the time of day, except under the sea), when you arrive somewhere new.
void overworld::show_place_name()
{
    if(_map->is_room() || shown_place == _map_index)
    {
        return;
    }
    shown_place = _map_index;
    bn::string<64> name(_map->name);
    bn::string<64> with_time(name);
    with_time.append(" - ");
    with_time.append(time_of_day_name(current_time_of_day()));
    // The time of day is left off under the sea, and when the name is too long for the plank with it.
    if(! (_map->area && _map->area->theme == area_theme::DEEP) && gui().width(with_time, true) <= 192)
    {
        name.append(" - ");
        name.append(time_of_day_name(current_time_of_day()));
    }
    gui().show_place(name);
}

void overworld::make_sprite(actor& a)
{
    if(a.legend)
    {
        const species& s = game_data::species_list[int(a.legend_species)];
        a.sprite = s.front.create_sprite(0, 0);
        apply_shiny(*a.sprite, int(a.legend_species), a.legend_shiny, mon_view::FRONT);
        a.sprite->set_bg_priority(1);
        return;
    }
    a.sprite = person_sprites[int(a.kind)]->create_sprite(0, 0, 0);
    a.sprite->set_bg_priority(1);
    set_frame(*a.sprite, a.kind, a.facing, 0);
}

void overworld::load_actors()
{
    game_state& g = state();
    _actors.clear();
    for(int i = 0; i < _map->people_count && ! _actors.full(); ++i)
    {
        const person& p = _map->people[i];
        if(p.role == person_role::TOWER && ! state().run.adventure)
        {
            continue;       // the CHALLENGE TOWER's guide only comes in ADVENTURE MODE
        }
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
        // A beaten rival has left (unless they're still about to say goodbye); scene trainers only come with
        // their scene.
        if((t.vanish && g.beaten.test(t.id) && g.walk_off != t.id) || t.scene)
        {
            continue;
        }
        actor a;
        a.tr = &t;
        a.kind = t.kind;
        a.x = a.home_x = t.x;
        a.y = a.home_y = t.y;
        a.facing = t.facing;
        // The trainer who walked up to you is still standing there (they walk back after: trainerWalkBack).
        if(pending_walk.map == _map_index && pending_walk.trainer_index == i)
        {
            a.x = pending_walk.x;
            a.y = pending_walk.y;
        }
        _actors.push_back(a);
    }
    // The guardian of the Sunken Shrine, until it's caught.
    int beast = _map->area ? legend_flag(_map->area->legend) : -1;
    if(_map->area && _map->area->legend_x >= 0 && ! legend_caught(_map->area->legend) && ! (beast >= 0 && beast < 3 && g.roam[beast]) &&
       ! _actors.full())
    {
        actor a;
        a.legend = true;
        a.legend_species = _map->area->legend;
        a.x = a.home_x = _map->area->legend_x;
        a.y = a.home_y = _map->area->legend_y;
        _actors.push_back(a);
    }
    // The CHALLENGE TOWER's summoned legendary, waiting in its chamber.
    if(_map->room && _map->room->kind == room_kind::CHAMBER && g.tower.active && g.tower.legend >= 0 && ! _actors.full())
    {
        actor a;
        a.legend = true;
        a.legend_species = species_id(g.tower.legend);
        a.legend_shiny = g.tower.legend_shiny;
        a.x = a.home_x = 7;
        a.y = a.home_y = 3;
        _actors.push_back(a);
    }
    // People in the connected areas, standing where they are (nbNpcs).
    if(! _map->is_room())
    {
        for(int li = 0; li < _map->links_count; ++li)
        {
            const link& l = _map->links[li];
            if(l.target < 0)
            {
                continue;
            }
            const map_def& m = wd::maps[l.target];
            auto in_view = [&](int x, int y){ return x >= l.sx && y >= l.sy && x < l.sx + l.sw && y < l.sy + l.sh; };
            for(int i = 0; i < m.people_count && ! _actors.full(); ++i)
            {
                const person& p = m.people[i];
                if(in_view(p.x, p.y))
                {
                    actor a;
                    a.kind = p.kind;
                    a.x = a.home_x = p.x + l.ox;
                    a.y = a.home_y = p.y + l.oy;
                    a.facing = p.facing;
                    a.neighbour = true;
                    _actors.push_back(a);
                }
            }
            for(int i = 0; i < m.trainers_count && ! _actors.full(); ++i)
            {
                const trainer& t = m.trainers[i];
                if((t.vanish && g.beaten.test(t.id)) || t.scene)
                {
                    continue;
                }
                if(in_view(t.x, t.y))
                {
                    actor a;
                    a.kind = t.kind;
                    a.x = a.home_x = t.x + l.ox;
                    a.y = a.home_y = t.y + l.oy;
                    a.facing = t.facing;
                    a.neighbour = true;
                    _actors.push_back(a);
                }
            }
        }
    }
    for(actor& a : _actors)
    {
        make_sprite(a);
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
    // Lower on screen is in front: one layer per screen row (Butano has a few dozen sort layers).
    sprite.set_z_order(-bn::clamp((wy - _cam_y + 8 + 32) >> 4, 0, 16) - 2);
}

// The player's look: walking, running, surfing or diving (drawPlayer).
void overworld::set_player_frame(int step, bool run)
{
    game_state& g = state();
    bool riding = (g.surfing && ! _map->is_room()) || deep();
    _player_step = step;
    _player_run = run;
    if(! _player)
    {
        return;
    }
    if(riding)
    {
        const bn::sprite_item& item = deep() ? bn::sprite_items::dive : bn::sprite_items::surf;
        int base = g.facing == direction::DOWN ? 0 : g.facing == direction::UP ? 2 : 4;
        int bob = (_bob_timer / 30) % 2;
        if(_player->shape_size() != item.shape_size())
        {
            _player = item.create_sprite(0, 0, base + bob);
            _player->set_bg_priority(1);
        }
        _player->set_tiles(item.tiles_item(), base + bob);
        _player->set_palette(item.palette_item());
        _player->set_horizontal_flip(g.facing == direction::RIGHT);
        return;
    }
    person_kind kind = run ? person_kind::player_run : person_kind::player;
    const bn::sprite_item& item = *person_sprites[int(kind)];
    if(_player->shape_size() != item.shape_size())
    {
        _player = item.create_sprite(0, 0, 0);
        _player->set_bg_priority(1);
    }
    set_frame(*_player, kind, g.facing, step);
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
        set_player_frame(_player_step, _player_run);
        _grass_here = bn::sprite_items::grass_front.create_sprite(0, 0);
        _grass_from = bn::sprite_items::grass_front.create_sprite(0, 0);
        for(bn::sprite_ptr* s : { &*_grass_here, &*_grass_from })
        {
            s->set_bg_priority(1);
            s->set_z_order(-30);
        }
        update_tint();
    }
    bool riding = (g.surfing && ! _map->is_room()) || deep();
    if(riding)
    {
        _player->set_position(sx(_px - _cam_x + 16), sy(_py - _cam_y + 1));
        _player->set_z_order(-bn::clamp((_py - _cam_y + 8 + 32) >> 4, 0, 16) - 2);
    }
    else
    {
        place_actor(*_player, _px, _py, _lift);
    }
    for(actor& a : _actors)
    {
        int ox = 0, oy = 0;
        if(a.move_frames > 0)
        {
            ox = -dx_of(a.move_dir) * 16 * a.move_frames / a.step_length;
            oy = -dy_of(a.move_dir) * 16 * a.move_frames / a.step_length;
        }
        if(a.legend)
        {
            // The guardian floats over its spot (.ow-legend: 64x64 art, centred, bobbing).
            int bob = (_bob_timer / 45) % 2 ? -2 : 0;
            a.sprite->set_position(sx(a.x * 16 - _cam_x + 8), sy(a.y * 16 - _cam_y - 12 + bob));
            a.sprite->set_z_order(-bn::clamp((a.y * 16 - _cam_y + 8 + 32) >> 4, 0, 16) - 2);
            continue;
        }
        place_actor(*a.sprite, a.x * 16 + ox, a.y * 16 + oy);
    }
    auto place_grass = [&](bn::sprite_ptr& s, int tx, int ty)
    {
        bool on = behaviour_at(tx, ty) == behaviour::TALL_GRASS && ! deep() && ! riding;
        s.set_visible(on);
        if(on)
        {
            s.set_position(sx(tx * 16 - _cam_x + 8), sy(ty * 16 + 8 - _cam_y + 4));
        }
    };
    place_grass(*_grass_here, _grass_x, _grass_y);
    place_grass(*_grass_from, _from_x, _from_y);
    if(_dust)
    {
        _dust->set_position(sx(_dust_x * 16 - _cam_x + 8), sy(_dust_y * 16 - _cam_y + 9 + 4));
    }
    // Hidden items glint for a moment every few seconds.
    int t = _bob_timer % 200;
    for(int i = 0; i < _glints.size(); ++i)
    {
        const hidden_item& h = wd::hidden_items[_glint_items[i]];
        int x = h.x * 16 - _cam_x + 8, y = h.y * 16 - _cam_y + 6;
        bool on = t < 24 && x > -8 && x < 248 && y > -8 && y < 168;
        _glints[i].set_visible(on);
        if(on)
        {
            _glints[i].set_position(sx(x + (i % 2 ? 3 : -2)), sy(y));
            _glints[i].set_tiles(bn::sprite_items::glint.tiles_item(), (t / 6) % 2);
        }
    }
}

// The glints of this area's hidden items still to find.
void overworld::update_glints()
{
    _glints.clear();
    if(_suspended || _map->is_room())
    {
        return;
    }
    for(int i = 0; i < wd::hidden_items_count && ! _glints.full(); ++i)
    {
        const hidden_item& h = wd::hidden_items[i];
        if(h.area == _map_index && ! state().picked.test(h.id))
        {
            bn::sprite_ptr s = bn::sprite_items::glint.create_sprite(0, 0);
            s.set_bg_priority(1);
            s.set_z_order(-40);
            s.set_visible(false);
            _glint_items[_glints.size()] = i;
            _glints.push_back(s);
        }
    }
}

// ----- Weather and the time of day -----
namespace
{
    enum weather_layer
    {
        WX_NONE = -1,
        WX_RAIN,
        WX_SNOW,
        WX_ASH,
        WX_DEEP,
        WX_FOG,
        WX_CAVE,
        WX_GHOST
    };

    const bn::regular_bg_item& weather_item(int kind, int amount)
    {
        switch(kind)
        {
        case WX_RAIN: return amount == 1 ? bn::regular_bg_items::wx_rain_1 : amount == 2 ? bn::regular_bg_items::wx_rain_2 : bn::regular_bg_items::wx_rain_3;
        case WX_SNOW: return amount == 1 ? bn::regular_bg_items::wx_snow_1 : amount == 2 ? bn::regular_bg_items::wx_snow_2 : bn::regular_bg_items::wx_snow_3;
        case WX_ASH: return amount == 1 ? bn::regular_bg_items::wx_ash_1 : amount == 2 ? bn::regular_bg_items::wx_ash_2 : bn::regular_bg_items::wx_ash_3;
        case WX_DEEP: return amount == 1 ? bn::regular_bg_items::wx_deep_1 : amount == 2 ? bn::regular_bg_items::wx_deep_2 : bn::regular_bg_items::wx_deep_3;
        case WX_FOG: return bn::regular_bg_items::wx_fog;
        case WX_CAVE: return bn::regular_bg_items::wx_cave;
        default: return amount <= 1 ? bn::regular_bg_items::wx_ghost_1 : amount == 2 ? bn::regular_bg_items::wx_ghost_2 : bn::regular_bg_items::wx_ghost_3;
        }
    }

    // How strongly each layer shows (the GBA blends a whole layer at one strength).
    int weather_alpha_256(int kind)
    {
        switch(kind)
        {
        case WX_RAIN: return 150;
        case WX_SNOW: return 230;
        case WX_ASH: return 210;
        case WX_DEEP: return 150;
        case WX_FOG: return 110;
        case WX_CAVE: return 140;
        default: return 240;
        }
    }
}

void overworld::update_weather()
{
    game_state& g = state();
    int kind = WX_NONE;
    int amount = 1;
    const area_info* a = _map->area;
    if(_map->is_room())
    {
        // The Ghost gym: dark but for a circle around you that grows with every junior beaten.
        if(_map->room && _map->room->kind == room_kind::GYM && _map->room->theme == gym_theme::GHOST)
        {
            kind = WX_GHOST;
            int beaten = 0;
            for(int i = 0; i < _map->trainers_count; ++i)
            {
                beaten += _map->trainers[i].role == trainer_role::JUNIOR && g.beaten.test(_map->trainers[i].id);
            }
            amount = 1 + beaten;
        }
    }
    else if(a)
    {
        int off = g.opt.weather == level4::OFF;
        amount = g.opt.weather == level4::HIGH ? 3 : g.opt.weather == level4::MID ? 2 : 1;
        switch(a->weather)
        {
        case area_weather::RAIN: kind = off ? WX_NONE : WX_RAIN; break;
        case area_weather::SNOW: kind = off ? WX_NONE : WX_SNOW; break;
        case area_weather::ASH: kind = off ? WX_NONE : WX_ASH; break;
        case area_weather::DEEP: kind = off ? WX_NONE : WX_DEEP; break;
        case area_weather::FOG: kind = WX_FOG; break;
        case area_weather::CAVE: kind = WX_CAVE; break;
        default: break;
        }
    }
    int key = kind < 0 ? -1 : kind * 4 + amount;
    if(key == _weather_kind && (kind < 0 || _weather))
    {
        return;
    }
    _weather_kind = key;
    _weather.reset();
    if(kind < 0 || _suspended)
    {
        bn::blending::set_transparency_alpha(1);
        return;
    }
    _weather = weather_item(kind, amount).create_bg(0, 0);
    _weather->set_priority(0);
    _weather->set_z_order(10);           // under the windows
    _weather->set_blending_enabled(true);
    bn::blending::set_transparency_alpha(bn::fixed(weather_alpha_256(kind)) / 256);
}

// The colour laid over the field (the stylesheet's .tod-layer and the weather's haze), as one fade of the
// map's and the people's palettes toward a colour.
void overworld::update_tint()
{
    game_state& g = state();
    // Layers over the field, bottom first: the time of day, then the haze of the weather.
    struct layer
    {
        int r, g, b, a;     // a: /256
    };
    layer layers[2];
    int n = 0;
    const area_info* a = _map->area;
    bool indoors = _map->is_room();
    bool no_tod = indoors || (a && (a->weather == area_weather::DEEP || a->weather == area_weather::CAVE));
    if(! no_tod)
    {
        switch(current_time_of_day())
        {
        case time_of_day::MORNING: layers[n++] = { 0xff, 0xd8, 0xa0, 0x14 }; break;
        case time_of_day::EVENING: layers[n++] = { 0xe0, 0x70, 0x30, 0x30 }; break;
        case time_of_day::NIGHT: layers[n++] = { 0x0c, 0x18, 0x4c, 0x70 }; break;
        default: break;
        }
    }
    if(! indoors && a)
    {
        bool off = g.opt.weather == level4::OFF;
        switch(a->weather)
        {
        case area_weather::RAIN: if(! off) layers[n++] = { 0x20, 0x30, 0x50, 0x18 }; break;
        case area_weather::ASH: if(! off) layers[n++] = { 0x40, 0x40, 0x40, 0x22 }; break;
        case area_weather::FOG: layers[n++] = { 0xe8, 0xe8, 0xf0, 0x40 }; break;
        case area_weather::WIND: if(! off) layers[n++] = { 0xd8, 0xec, 0xff, 0x28 }; break;   // 4.0.0
        case area_weather::DEEP: layers[n++] = { 0x1a, 0x48, 0x80, 0x50 }; break;
        default: break;
        }
    }
    // Two overlays make one: strength A = 1 - (1 - a1)(1 - a2), colour = (c1 a1 (1 - a2) + c2 a2) / A.
    int A = 0, R = 0, G = 0, B = 0;
    for(int i = 0; i < n; ++i)
    {
        const layer& l = layers[i];
        R = (R * (256 - l.a) + l.r * l.a * 256 / 256) ;
        G = (G * (256 - l.a) + l.g * l.a);
        B = (B * (256 - l.a) + l.b * l.a);
        R /= 256;
        G /= 256;
        B /= 256;
        A = 256 - (256 - A) * (256 - l.a) / 256;
    }
    bn::color c(0, 0, 0);
    bn::fixed strength = 0;
    if(A > 0)
    {
        // R, G, B hold colour x strength / 256.
        int r = bn::min(255, R * 256 / A), gg = bn::min(255, G * 256 / A), b = bn::min(255, B * 256 / A);
        c = bn::color(r >> 3, gg >> 3, b >> 3);
        strength = bn::fixed(A) / 256;
    }
    if(_bg)
    {
        bn::bg_palette_ptr pal = _bg->palette();
        pal.set_fade(c, strength);
    }
    auto tint_sprite = [&](bn::optional<bn::sprite_ptr>& s)
    {
        if(s)
        {
            bn::sprite_palette_ptr pal = s->palette();
            pal.set_fade(c, strength);
        }
    };
    tint_sprite(_player);
    tint_sprite(_grass_here);
    tint_sprite(_grass_from);
    for(actor& act : _actors)
    {
        tint_sprite(act.sprite);
    }
}

// Full-screen menus need the palettes and sprites the map uses, so the map steps aside meanwhile.
void overworld::suspend()
{
    ui::fade_out(8);
    _suspended = true;
    _weather.reset();
    _weather_kind = -2;
    bn::blending::set_transparency_alpha(1);
    _bg_map.reset();
    _bg.reset();
    _player.reset();
    _grass_here.reset();
    _grass_from.reset();
    _dust.reset();
    _glints.clear();
    for(actor& a : _actors)
    {
        a.sprite.reset();
    }
}

void overworld::resume()
{
    _suspended = false;
    build_bg();
    for(actor& a : _actors)
    {
        make_sprite(a);
    }
    update_glints();
    refresh(true);
    update_weather();
    update_tint();
    ui::fade_in(8);
}

// ----- Frames -----
void overworld::tick()
{
    // musicWanted() on the field: under the sea, on the water, in a cave, in a town (or indoors), on a route.
    {
        const area_info* a = area();
        const char* track = "route";
        if(deep())
        {
            track = "deep";
        }
        else if(state().surfing && ! _map->is_room())
        {
            track = "sea";
        }
        else if(a && a->theme == area_theme::CAVE && ! _map->is_room())
        {
            track = "cave";
        }
        else if(_map->is_room() || (a && (a->kind == area_kind::TOWN || a->kind == area_kind::GYM)))
        {
            track = "town";
        }
        audio::play_music(track);
    }
    tick_actors();
    animate_tiles();
    ++_bob_timer;
    if((_bob_timer & 31) == 0)
    {
        // Riding the waves: the bob (surfBob, a pixel every half second); and the light changes with the hour.
        if((state().surfing && ! _map->is_room()) || deep())
        {
            set_player_frame(_player_step, _player_run);
        }
    }
    if(_bob_timer % 600 == 0)
    {
        update_tint();
    }
    if(_dust)
    {
        if(++_dust_frames >= 24)
        {
            _dust.reset();
        }
        else
        {
            _dust->set_tiles(bn::sprite_items::dust.tiles_item(), bn::min(2, _dust_frames / 8));
        }
    }
    // The weather drifts across the screen (rain falls slanted, snow and ash sway down, bubbles rise, fog drifts).
    if(_weather)
    {
        ++_weather_timer;
        int k = _weather_kind / 4;
        int sway = ((_weather_timer / 4) % 24);
        sway = sway < 12 ? sway - 6 : 18 - sway;
        switch(k)
        {
        case WX_RAIN: _weather_x -= 1; _weather_y -= 6; break;
        case WX_SNOW: if(_weather_timer % 3 == 0) { _weather_y -= 1; } break;
        case WX_ASH: if(_weather_timer % 2 == 0) { _weather_y -= 1; } break;
        case WX_DEEP: if(_weather_timer % 2 == 0) { _weather_y += 1; } break;
        case WX_FOG: if(_weather_timer % 4 == 0) { _weather_x -= 1; } break;
        default: break;
        }
        int x = _weather_x + ((k == WX_SNOW || k == WX_ASH || k == WX_DEEP) ? sway / 2 : 0);
        _weather->set_position(x & 255, _weather_y & 255);
    }
    refresh();
    frame();
}

void overworld::wait_frames(int frames)
{
    for(int i = 0; i < frames; ++i)
    {
        tick();
    }
}

void overworld::tick_actors()
{
    for(actor& a : _actors)
    {
        if(a.move_frames > 0)
        {
            a.move_frames -= 1;
            if(! a.legend)
            {
                person_kind k = a.kind;
                set_frame(*a.sprite, k, a.facing, a.move_frames > a.step_length / 2 ? (a.step_foot ? 1 : 2) : 0);
            }
        }
        if(a.fade > 0)
        {
            --a.fade;
            a.sprite->set_visible(a.fade > 12 || a.fade % 4 < 2);
        }
        else if(a.fade < 0)
        {
            ++a.fade;
            a.sprite->set_visible(a.fade < -12 || a.fade % 4 > -2);
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
        if(! a.wander || a.neighbour || a.move_frames || r.get_int(4) != 0)
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
            a.step_length = walk_frames;
            a.move_frames = walk_frames;
            a.step_foot = ! a.step_foot;
        }
        set_frame(*a.sprite, a.kind, a.facing, 0);
    }
}

// npcMove(): an actor walks (or runs) a few tiles in one direction.
void overworld::walk_actor(int index, direction d, int tiles, bool run)
{
    for(int i = 0; i < tiles; ++i)
    {
        actor& a = _actors[index];
        a.facing = d;
        a.x += dx_of(d);
        a.y += dy_of(d);
        a.move_dir = d;
        a.step_length = run ? run_frames : walk_frames;
        a.move_frames = a.step_length;
        a.step_foot = ! a.step_foot;
        while(_actors[index].move_frames > 0)
        {
            tick();
        }
    }
    set_frame(*_actors[index].sprite, _actors[index].kind, d, 0);
}

void overworld::hold_until_released()
{
    _fresh_press = true;
    while(bn::keypad::up_held() || bn::keypad::down_held() || bn::keypad::left_held() || bn::keypad::right_held())
    {
        tick();
    }
}

// ----- The player -----
void overworld::step(direction want)
{
    game_state& g = state();
    bool riding = (g.surfing && ! _map->is_room()) || deep();
    // From a standstill, a new direction first turns you on the spot; keep holding and you walk.
    if(_fresh_press && want != g.facing)
    {
        g.facing = want;
        for(int f = 0; f < turn_frames; ++f)
        {
            set_player_frame(riding ? 0 : f < turn_frames / 2 ? 1 + _step_parity : 0);
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

    // Walking into an unbeaten trainer starts their battle.
    if(who >= 0 && _actors[who].active_trainer())
    {
        set_player_frame(0);
        trainer_approach(who);
        return;
    }
    if(target_place.link >= 0)
    {
        const link& l = _map->links[target_place.link];
        // No POKéMON yet: only ROUTE 1 (the professor went that way).
        if(! g.has(story::STARTER) && target_place.map != 1)
        {
            set_player_frame(0);
            say("It's dangerous to go out without POKéMON!");
            bn::string<64> text(game_data::prof_name);
            text.append(" went toward ROUTE 1...");
            say(text);
            hold_until_released();
            return;
        }
        // Victory Road: all eight badges (of that region). The SKYREACH's updrafts (4.0.0): three, then six.
        if(l.badges && g.region_badges(map_region(target_place.map)) < l.badges)
        {
            set_player_frame(0);
            bn::string<96> text(l.badges < 8 ? "A strong updraft howls up the cliff. Riding it takes " : "Only trainers with all ");
            text.append(bn::to_string<4>(l.badges));
            text.append(l.badges < 8 ? " SKYREACH badges." : " badges may pass beyond this point.");
            say(text);
            hold_until_released();
            return;
        }
        // Calderra's CRATER RIM: a branch opens from this side once you've walked it from PORT CALDER, and the
        // ASHEN TOWER once all three beasts are free.
        if(l.need == link_need::VISITED && target_place.map >= 0 && ! g.visited.test(target_place.map))
        {
            set_player_frame(0);
            say("Loose rock and steep cliffs. There's no way down from this side.");
            hold_until_released();
            return;
        }
        if(l.need == link_need::SHRINES && ! shrines_cleared(map_region(_map_index)))
        {
            set_player_frame(0);
            say(map_region(_map_index) == 4
                    ? "A howling wind seals the SKY PILLAR's door. It stays shut until DEOXYS and RAYQUAZA are settled."
                    : map_region(_map_index) == 3
                    ? "The ground shakes and the sea roars. MT. KEEL stays shut until TEAM QUAKE and TEAM NEPTUNE are stopped."
                    : "A wall of heat and storm bars the way to the tower. The three beasts must be freed first.");
            hold_until_released();
            return;
        }
        // SPIRECREST TOWN (the CHALLENGE TOWER) opens to CHAMPIONS.
        if(target_place.map >= 0 && wd::maps[target_place.map].area &&
           (wd::maps[target_place.map].area->flags & area_flag::TOWER_TOWN) && ! g.has(story::CHAMPION))
        {
            set_player_frame(0);
            say("A gate blocks the road. Its sign reads: \"SPIRECREST TOWN - CHALLENGE TOWER. CHAMPIONS ONLY.\"");
            hold_until_released();
            return;
        }
        // The SAFARI ZONE: $5000 at the gate, each time in.
        if(target_place.map >= 0 && wd::maps[target_place.map].area && (wd::maps[target_place.map].area->flags & area_flag::SAFARI) &&
           ! (_map->area && (_map->area->flags & area_flag::SAFARI)))
        {
            set_player_frame(0);
            if(! safari_gate())
            {
                hold_until_released();
                return;
            }
        }
        // The way on is shut until this place's rival or Gym Leader is beaten (linkAreas gate).
        if(l.gate && _map->leader_id >= 0 && ! g.beaten.test(_map->leader_id))
        {
            set_player_frame(0);
            if(_map->gate == gate_kind::RIVAL)
            {
                // Slipped past the rival's line of sight? They call you back rather than leave you hunting for them.
                bn::string<128> text;
                for(const char* c = _map->leader_name; *c; ++c)
                {
                    text.push_back(*c >= 'a' && *c <= 'z' ? char(*c - 32) : *c);
                }
                text.append(": \"Hey! Not so fast! You're not getting past without a battle!\"");
                say(text);
                for(int i = 0; i < _actors.size(); ++i)
                {
                    if(_actors[i].tr && _actors[i].tr->id == _map->leader_id)
                    {
                        start_trainer_battle(i);
                        return;
                    }
                }
            }
            else
            {
                bn::string<96> text("You should challenge ");
                text.append(_map->gate == gate_kind::GYM ? "Gym Leader " : "your rival ");
                text.append(_map->leader_name);
                text.append(" before moving on.");
                say(text);
            }
            hold_until_released();
            return;
        }
        if(target_place.map < 0)
        {
            // Nowhere to go (the edge of the world).
            target = behaviour::SOLID;
        }
    }

    int tiles = 1;
    int frames = walk_frames;
    bool jump = false;
    bool run = false;
    bool hop_off = false;
    if(riding && ! deep())
    {
        // Surfing: glide over water; step toward land and you hop off onto it.
        if(who < 0 && target == behaviour::WATER)
        {
            frames = run_frames;
        }
        else if(who < 0 && walkable(nx, ny))
        {
            hop_off = true;
            frames = jump_frames;
            jump = true;
        }
        else
        {
            audio::play(audio::sfx::BUMP);
            for(int f = 0; f < walk_frames; ++f)
            {
                set_player_frame(0);
                tick();
            }
            return;
        }
    }
    else
    {
        // Ledges: hop over to the tile beyond (32 frames for the two tiles).
        jump = (target == behaviour::LEDGE_DOWN && want == direction::DOWN) ||
               (target == behaviour::LEDGE_RIGHT && want == direction::RIGHT) ||
               (target == behaviour::LEDGE_LEFT && want == direction::LEFT);
        if(jump)
        {
            int bx = nx + dx_of(want), by = ny + dy_of(want);
            jump = walkable(bx, by) && actor_at(bx, by) < 0;
        }
        bool can_walk = walkable(nx, ny);
        if(! jump && (! can_walk || who >= 0))
        {
            // Bump: walk on the spot.
            audio::play(audio::sfx::BUMP);
            for(int f = 0; f < walk_frames; ++f)
            {
                set_player_frame(riding ? 0 : f < walk_frames / 2 ? 1 + _step_parity : 0);
                tick();
            }
            _step_parity ^= 1;
            return;
        }
        if(jump)
        {
            tiles = 2;
            frames = jump_frames;
            audio::play(audio::sfx::JUMP);
        }
        else
        {
            run = bn::keypad::b_held() && ! _map->is_room() && ! deep();   // running shoes don't work indoors
            frames = run ? run_frames : walk_frames;
        }
    }
    if(hop_off)
    {
        audio::play(audio::sfx::JUMP);
        g.surfing = false;
    }

    int start_px = _px, start_py = _py;
    _from_x = g.x;
    _from_y = g.y;
    _grass_x = g.x + dx_of(want) * tiles;
    _grass_y = g.y + dy_of(want) * tiles;
    if(! jump && ! riding)
    {
        sweep_ash(g.x + dx_of(want), g.y + dy_of(want));
    }
    // The tile is ours from the start of the step, so nobody wanders into it.
    g.x = int16_t(g.x + dx_of(want) * tiles);
    g.y = int16_t(g.y + dy_of(want) * tiles);
    for(int f = 1; f <= frames; ++f)
    {
        _px = start_px + dx_of(want) * 16 * tiles * f / frames;
        _py = start_py + dy_of(want) * 16 * tiles * f / frames;
        _cam_x = _px - view_x;
        _cam_y = _py - view_y;
        if(jump)
        {
            // Up to 12 px above the shadow, two walk cycles' worth of legs in the air.
            _lift = (f * (frames - f) * 12) / (frames * frames / 4);
            int q = f * 4 / frames;
            set_player_frame(hop_off || q % 2 ? 0 : (q ? 2 - _step_parity : 1 + _step_parity));
        }
        else
        {
            int stride = run ? 5 * frames / 8 : frames / 2;   // running holds the stride a little longer
            set_player_frame((riding && ! hop_off) ? 0 : f <= stride ? 1 + _step_parity : 0, run);
        }
        tick();
    }
    _lift = 0;
    _step_parity ^= 1;
    _from_x = _from_y = -1000;
    _grass_x = g.x;
    _grass_y = g.y;
    if(jump && ! hop_off)
    {
        // A puff of dust where you land.
        _dust = bn::sprite_items::dust.create_sprite(0, 0, 0);
        _dust->set_bg_priority(1);
        _dust->set_z_order(-31);
        _dust_frames = 0;
        _dust_x = g.x;
        _dust_y = g.y;
    }
    set_player_frame(0, run);
    arrive();
}

// Stepped over the edge into a connected area: it becomes the current map, with no fade.
void overworld::cross_area(const place& here)
{
    game_state& g = state();
    const link& l = _map->links[here.link];
    g.x = int16_t(here.x);
    g.y = int16_t(here.y);
    int px = _px - l.ox * 16, py = _py - l.oy * 16;
    int dust_x = _dust_x - l.ox, dust_y = _dust_y - l.oy;
    _map_index = here.map;
    _map = &wd::maps[here.map];
    g.map = int16_t(here.map);
    g.visited.set(here.map);
    roamers_move();
    _px = px;
    _py = py;
    _cam_x = _px - view_x;
    _cam_y = _py - view_y;
    _grass_x = g.x;
    _grass_y = g.y;
    _dust_x = dust_x;
    _dust_y = dust_y;
    if(_map->tileset != _tileset)
    {
        // Every area has its own tileset (with this one's edge in it): swap it in with the same view.
        _tileset = _map->tileset;
        build_bg();
    }
    load_actors();
    refresh(true);
    update_weather();
    update_tint();
    show_place_name();
}

void overworld::arrive()
{
    game_state& g = state();
    place here = find(g.x, g.y);
    if(here.link >= 0 && here.map >= 0)
    {
        cross_area(here);
        if(! g.has(story::STARTER) && _map_index == 1)
        {
            starter_event();
            return;
        }
        story_enter();
        if(_start_battle)
        {
            return;
        }
    }
    behaviour b = behaviour_at(g.x, g.y);
    if(b >= behaviour::WIND_UP && b <= behaviour::WIND_RIGHT && ! g.surfing)
    {
        // 4.0.0: a wind current carries you on, a step at a time, until you're off it (each run ends on open ground).
        constexpr direction blow[] = { direction::UP, direction::DOWN, direction::LEFT, direction::RIGHT };
        direction d = blow[int(b) - int(behaviour::WIND_UP)];
        int nx = g.x + dx_of(d), ny = g.y + dy_of(d);
        if(walkable(nx, ny) && actor_at(nx, ny) < 0 && find(nx, ny).link < 0)
        {
            _fresh_press = false;
            step(d);
            return;
        }
    }
    if(b == behaviour::DOOR)
    {
        for(int i = 0; i < _map->doors_count; ++i)
        {
            if(_map->doors[i].x == g.x && _map->doors[i].y == g.y)
            {
                if(_map->doors[i].kind == door_kind::TOWER && ! tower_door(_map->doors[i]))
                {
                    return;
                }
                enter_room(_map->doors[i]);
                return;
            }
        }
    }
    if(b == behaviour::MAT && _map->is_room())
    {
        if(tower_leave())
        {
            leave_room();
        }
        return;
    }
    // REPEL, the DAY CARE and EGGS count the step; REPEL keeps wild Pokémon away (GBA 1.8).
    bool spoke = step_counters();
    bool can_fight = g.first_able() >= 0 && ! g.has(story::CHEAT_NO_WILD) && ! g.extra.repel_steps && ! spoke;
    if(b == behaviour::TALL_GRASS && rng().get_int(100) < grass_percent && can_fight)
    {
        wild_battle(false);
        return;
    }
    if(b == behaviour::WALK && _map->area && _map->area->theme == area_theme::CAVE && rng().get_int(100) < cave_percent && can_fight)
    {
        wild_battle(false);
        return;
    }
    if(b == behaviour::WATER && g.surfing && ! _surf_fresh && rng().get_int(100) < water_percent && can_fight)
    {
        wild_battle(true);
        return;
    }
    _surf_fresh = false;
    check_sight();
}

namespace
{
    // timePicks(): n species from the pool, without repeats, those of the hour's types three times as likely.
    int time_picks(const species_id* pool, int pool_count, species_id* out, int n)
    {
        species_id bag[24];
        int weight[24];
        int count = 0;
        const int8_t* boost = game_data::time_types[int(current_time_of_day())];
        for(int i = 0; i < pool_count && count < 24; ++i)
        {
            bool dup = false;
            for(int k = 0; k < count; ++k)
            {
                dup |= bag[k] == pool[i];
            }
            if(dup)
            {
                continue;
            }
            const species& s = game_data::species_list[int(pool[i])];
            bool boosted = false;
            for(int k = 0; boost[k] >= 0; ++k)
            {
                boosted |= s.type1 == boost[k] || s.type2 == boost[k];
            }
            bag[count] = pool[i];
            weight[count] = boosted ? 3 : 1;
            ++count;
        }
        int picked = 0;
        while(picked < n && count)
        {
            int total = 0;
            for(int i = 0; i < count; ++i)
            {
                total += weight[i];
            }
            int r = rng().get_int(total), i = 0;
            while(r >= weight[i])
            {
                r -= weight[i];
                ++i;
            }
            out[picked++] = bag[i];
            bag[i] = bag[count - 1];
            weight[i] = weight[count - 1];
            --count;
        }
        return picked;
    }
}

// startWildBattle(): a pack of 1 up to half your (living) party, at most 4, from the area's pool (or the
// water's), a little under your party's level. At night, outdoors, a visitor may join the pool.
void overworld::wild_battle(bool water)
{
    game_state& g = state();
    bn::random& r = rng();
    encounter& e = *_battle;
    e = encounter();
    e.kind = encounter_kind::WILD;
    e.water = water;
    int cap = bn::min(4, (g.able_count() + 1) / 2);
    int n = 1 + r.get_int(bn::max(1, cap));
    species_id pool[24];
    int pool_count = 0;
    const species_id* src = water ? _map->water : _map->pool;
    int src_count = water ? _map->area->water_count : _map->pool_count;
    for(int i = 0; i < src_count && pool_count < 23; ++i)
    {
        pool[pool_count++] = src[i];
    }
    bool outdoors = ! water && _map->area && _map->area->theme != area_theme::DEEP && _map->area->theme != area_theme::CAVE;
    if(outdoors && current_time_of_day() == time_of_day::NIGHT && r.get_int(4) == 0)
    {
        constexpr int visitors = int(sizeof(game_data::night_visitors) / sizeof(game_data::night_visitors[0]));
        pool[pool_count++] = game_data::night_visitors[r.get_int(visitors)];
    }
    e.count = time_picks(pool, pool_count, e.species, n);
    if(! water && _map->area && (_map->area->flags & area_flag::SAFARI))
    {
        // The SAFARI ZONE: every species as likely as any other, legendaries and all.
        e.count = 0;
        while(e.count < n)
        {
            auto s = species_id(r.get_int(species_count));
            bool dupe = false;
            for(int k = 0; k < e.count; ++k)
            {
                dupe |= e.species[k] == s;
            }
            if(! dupe)
            {
                e.species[e.count++] = s;
            }
        }
    }
    int lv_cap = map_level_cap(_map_index);
    e.level = bn::max(map_region(_map_index) >= 2 ? lv_cap - 6 : 2, bn::min(lv_cap, g.average_level() - 2 + r.get_int(3)));
    // A roaming beast on this route: one grass encounter in three is it instead.
    for(int f = 0; f < 3 && ! water; ++f)
    {
        if(g.roam[f] == _map_index + 1 && ! g.flags.test(f) && r.get_int(3) == 0)
        {
            e.count = 1;
            e.species[0] = roamer_species(f);
            e.level = bn::min(200, lv_cap + 5);
            e.legendary = true;
            e.roamer = f;
            break;
        }
    }
    _start_battle = true;
}

void overworld::fixed_battle(species_id s, int level, bool legendary)
{
    encounter& e = *_battle;
    e = encounter();
    e.kind = encounter_kind::FIXED;
    e.species[0] = s;
    e.count = 1;
    e.level = level;
    e.legendary = legendary;
    _start_battle = true;
}

void overworld::tower_legend_battle(species_id s, int level)
{
    fixed_battle(s, level, true);
    _battle->tower_legend = true;
}

// Trainers spot you when you walk into their line of sight (up to 5 tiles, nothing in between; they see
// across water).
bool overworld::check_sight()
{
    game_state& g = state();
    for(int i = 0; i < _actors.size(); ++i)
    {
        actor& a = _actors[i];
        if(! a.active_trainer() || a.neighbour)
        {
            continue;
        }
        for(int s = 1; s <= sight; ++s)
        {
            int tx = a.x + dx_of(a.facing) * s, ty = a.y + dy_of(a.facing) * s;
            behaviour b = behaviour_at(tx, ty);
            bool see_through = b == behaviour::WALK || b == behaviour::TALL_GRASS || b == behaviour::WATER ||
                               b == behaviour::DOOR || b == behaviour::MAT;
            if(! see_through)
            {
                break;
            }
            if(tx == g.x && ty == g.y)
            {
                trainer_approach(i);
                return true;
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
    set_player_frame(0);
    audio::play(audio::sfx::SPOT);
    bn::sprite_ptr bang = bn::sprite_items::bang.create_sprite(0, 0);
    bang.set_bg_priority(0);
    for(int f = 0; f < 60; ++f)
    {
        actor& a = _actors[index];
        int rise = bn::min(f, 6);
        bang.set_position(sx(a.x * 16 - _cam_x + 8), sy(a.y * 16 - _cam_y - 26 - rise / 2));
        tick();
    }
    bang.set_visible(false);
    _actors[index].trail.clear();
    while(bn::abs(_actors[index].x - g.x) + bn::abs(_actors[index].y - g.y) > 1)
    {
        actor& a = _actors[index];
        a.facing = toward(a.x, a.y, g.x, g.y);
        if(! a.trail.full())
        {
            a.trail.push_back(int8_t(a.facing));
        }
        a.x += dx_of(a.facing);
        a.y += dy_of(a.facing);
        a.move_dir = a.facing;
        a.step_length = walk_frames;
        a.move_frames = walk_frames;
        a.step_foot = ! a.step_foot;
        while(_actors[index].move_frames > 0)
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
    set_player_frame(0);
    set_frame(*a.sprite, a.kind, a.facing, 0);
    tick();
    if(g.first_able() < 0)
    {
        say("Your whole party has fainted! Rest at the POKéMON CENTER.");
        return;
    }
    for(int i = 0; i < a.tr->intro_count; ++i)
    {
        say(a.tr->intro[i]);
    }
    // They walk back the way they came once it's over (trainerWalkBack).
    walk_back& w = pending_walk;
    w.trail.clear();
    w.map = -1;
    if(! a.trail.empty())
    {
        w.map = _map_index;
        w.trainer_index = a.tr - _map->trainers;
        w.x = a.x;
        w.y = a.y;
        for(int8_t d : a.trail)
        {
            w.trail.push_back(d);
        }
    }
    encounter& e = *_battle;
    e = encounter();
    e.kind = encounter_kind::TRAINER;
    e.map = _map_index;
    e.trainer_index = a.tr - _map->trainers;
    _start_battle = true;
}

void overworld::say(const bn::string_view& text)
{
    gui().say(text);
}

void overworld::story_say(const bn::string_view& text)
{
    if(! state().run.skip_story)
    {
        gui().say(text);
    }
}

// ----- Main loop -----
bool overworld::run(encounter& battle, const battle_report* last)
{
    _battle = &battle;
    _last = last;
    game_state& g = state();
    if(g.surfing && (_map->is_room() || behaviour_at(g.x, g.y) != behaviour::WATER))
    {
        g.surfing = false;
    }
    refresh(true);
    update_weather();
    update_tint();
    ui::fade_in(12);
    after_story();
    if(! _start_battle && ! _quit)
    {
        story_enter();
    }
    while(! _quit && ! _start_battle)
    {
        // A + B + START + SELECT: back to the title (a soft reset).
        if(bn::keypad::a_held() && bn::keypad::b_held() && bn::keypad::start_held() && bn::keypad::select_held())
        {
            _quit = true;
            break;
        }
        direction want = g.facing;
        bool dir_held = true;
        if(bn::keypad::up_held()) want = direction::UP;
        else if(bn::keypad::down_held()) want = direction::DOWN;
        else if(bn::keypad::left_held()) want = direction::LEFT;
        else if(bn::keypad::right_held()) want = direction::RIGHT;
        else dir_held = false;

        if(bn::keypad::start_pressed())
        {
            if(cheat_code_entered())
            {
                audio::play(audio::sfx::SPOT);
                cheat_menu();
                tick();
                continue;
            }
            audio::play(audio::sfx::OPEN);
            start_menu();
            tick();
            continue;
        }
        if(egg_watch())
        {
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
            set_player_frame(0);
            tick();
            continue;
        }
        step(want);
    }
    if(_start_battle)
    {
        // Encounter (Emerald): the field flashes white twice, then fades.
        for(int i = 0; i < 2; ++i)
        {
            bn::bg_palettes::set_fade(bn::color(31, 31, 31), 1);
            wait(4);
            bn::bg_palettes::set_fade(bn::color(31, 31, 31), 0);
            wait(4);
        }
        ui::fade_out(20);
        _weather.reset();
        bn::blending::set_transparency_alpha(1);
        return true;
    }
    ui::fade_out(12);
    _weather.reset();
    bn::blending::set_transparency_alpha(1);
    return false;
}

bool overworld_scene(encounter& battle, const battle_report* last)
{
    // The scene lives on the heap (EWRAM), not the small IWRAM stack.
    bn::unique_ptr<overworld> scene(new overworld());
    return scene->run(battle, last);
}

}
