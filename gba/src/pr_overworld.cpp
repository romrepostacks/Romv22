// The overworld: the web game's areas drawn from generated metatiles, scrolled tile-by-tile with the
// player, Emerald-style connections into neighbouring areas, Emerald step timing (16 frames a step,
// 8 running), people, signs, items, ledges and tall-grass encounters.
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_random.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_span.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "pr_game_data.h"
#include "pr_people_sprites.h"
#include "pr_scenes.h"
#include "pr_state.h"
#include "pr_ui.h"
#include "pr_world_data.h"

#include "bn_sprite_items_grass_front.h"

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
    // The player's tile sits here on screen (the web game centres a 15x10 view; this is the GBA's 15x10).
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

    // Sprite sheet frames: down 0-2, up 3-5, left 6-8 (right is left mirrored); 0 stands, 1 and 2 step.
    int frame_of(direction d, int step)
    {
        int base = d == direction::DOWN ? 0 : d == direction::UP ? 3 : 6;
        return base + step;
    }

    alignas(int) bn::regular_bg_map_cell bg_cells[32 * 32];

    // What lies at a tile, looking through this area's connections when it's past the edge.
    struct place
    {
        int area = -1;          // world_data::areas index; -1 = nothing walkable (forest / not in this build)
        int x = 0;
        int y = 0;
        int link = -1;          // link used, if outside the current area
    };

    class world
    {

    public:
        int area_index;

        explicit world(int area) :
            area_index(area)
        {
        }

        [[nodiscard]] const pr::area& current() const
        {
            return wd::areas[area_index];
        }

        [[nodiscard]] place find(int tx, int ty) const
        {
            const pr::area& a = current();
            place p;
            if(tx >= 0 && ty >= 0 && tx < a.w && ty < a.h)
            {
                p.area = area_index;
                p.x = tx;
                p.y = ty;
                return p;
            }
            for(int i = 0; i < a.links_count; ++i)
            {
                const pr::link& l = a.links[i];
                if(tx >= l.ox && ty >= l.oy && tx < l.ox + l.w && ty < l.oy + l.h)
                {
                    p.area = l.target;
                    p.x = tx - l.ox;
                    p.y = ty - l.oy;
                    p.link = i;
                    return p;
                }
            }
            return p;
        }

        [[nodiscard]] int metatile_at(int tx, int ty) const
        {
            place p = find(tx, ty);
            if(p.area < 0)
            {
                return wd::forest_metatile;
            }
            const pr::area& a = wd::areas[p.area];
            int i = p.y * a.w + p.x;
            if(behaviour(a.behaviours[i]) == behaviour::ITEM && state().item_picked(p.area, p.x, p.y))
            {
                return wd::grass_metatile;
            }
            return a.map[i];
        }

        [[nodiscard]] behaviour behaviour_at(int tx, int ty) const
        {
            place p = find(tx, ty);
            if(p.area < 0)
            {
                return behaviour::SOLID;
            }
            const pr::area& a = wd::areas[p.area];
            int i = p.y * a.w + p.x;
            behaviour b = behaviour(a.behaviours[i]);
            if(b == behaviour::ITEM && state().item_picked(p.area, p.x, p.y))
            {
                return behaviour::WALK;
            }
            return b;
        }

        // Rewrites the 31x21 cells the camera can see (the hardware map is 32x32 and wraps).
        void draw(int cam_x, int cam_y) const
        {
            int cx0 = cam_x >> 3;
            int cy0 = cam_y >> 3;
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
                        meta = wd::metatiles[metatile_at(tx, ty)];
                        last_tx = tx;
                    }
                    row[cx & 31] = meta[sub_y + (cx & 1)];
                }
            }
        }
    };

    struct actor_sprite
    {
        bn::sprite_ptr sprite;
        const bn::sprite_item* item;
    };

    void set_actor_frame(bn::sprite_ptr& sprite, const bn::sprite_item& item, direction d, int step)
    {
        sprite.set_tiles(item.tiles_item(), frame_of(d, step));
        sprite.set_horizontal_flip(d == direction::RIGHT);
    }

    // A 16x32 actor whose tile's top-left is at world pixel (wx, wy).
    void place_actor(bn::sprite_ptr& sprite, int wx, int wy, int cam_x, int cam_y, int lift = 0)
    {
        int screen_x = wx - cam_x + 8;
        int screen_y = wy - cam_y - 2 - lift;
        sprite.set_position(screen_x - 120, screen_y - 80);
        sprite.set_z_order(-(wy >> 4) - 2);
    }

    const pr::sign* sign_at(const pr::area& a, int tx, int ty)
    {
        for(int i = 0; i < a.signs_count; ++i)
        {
            if(a.signs[i].x == tx && a.signs[i].y == ty)
            {
                return &a.signs[i];
            }
        }
        return nullptr;
    }

    const pr::door* door_at(const pr::area& a, int tx, int ty)
    {
        for(int i = 0; i < a.doors_count; ++i)
        {
            if(a.doors[i].x == tx && a.doors[i].y == ty)
            {
                return &a.doors[i];
            }
        }
        return nullptr;
    }

    int wild_level(const pr::area& a, bn::random& random)
    {
        // wildLevel(): a little under the party's average level, capped by the area.
        const game_state& g = state();
        int sum = 0;
        for(int i = 0; i < g.party_count; ++i)
        {
            sum += g.party[i].level;
        }
        int avg = g.party_count ? (sum + g.party_count / 2) / g.party_count : 5;
        return bn::max(2, bn::min(int(a.level_cap), avg - 2 + random.get_int(3)));
    }

    void party_menu(ui& ui)
    {
        game_state& g = state();
        while(true)
        {
            bn::string<24> labels[max_party];
            bn::string<48> hints[max_party];
            bn::string_view options[max_party];
            bn::string_view hint_views[max_party];
            for(int i = 0; i < g.party_count; ++i)
            {
                const mon& m = g.party[i];
                labels[i] = m.name();
                hints[i] = "Lv";
                hints[i].append(bn::to_string<4>(m.level));
                hints[i].append("  HP ");
                hints[i].append(bn::to_string<4>(m.hp));
                hints[i].append("/");
                hints[i].append(bn::to_string<4>(m.max_hp));
                hints[i].append("  ");
                hints[i].append(type_name(m.data().type1));
                if(m.data().type2 >= 0)
                {
                    hints[i].append("/");
                    hints[i].append(type_name(m.data().type2));
                }
                options[i] = labels[i];
                hint_views[i] = hints[i];
            }
            int pick = ui.menu(options, g.party_count, true, 0, true, hint_views);
            if(pick < 0)
            {
                return;
            }
            constexpr bn::string_view actions[] = { "SUMMARY", "LEAD", "CANCEL" };
            int act = ui.menu(actions, 3, false);
            const mon& m = g.party[pick];
            if(act == 0)
            {
                bn::string<160> text(m.name());
                text.append(" Lv");
                text.append(bn::to_string<4>(m.level));
                text.append("  EXP ");
                text.append(bn::to_string<6>(m.xp));
                text.append("/");
                text.append(bn::to_string<6>(m.xp_next()));
                text.append("\nATK ");
                text.append(bn::to_string<4>(m.atk));
                text.append(" DEF ");
                text.append(bn::to_string<4>(m.def));
                text.append(" SPD ");
                text.append(bn::to_string<4>(m.spe));
                text.append("\nMOVES: ");
                for(int k = 0; k < m.move_count; ++k)
                {
                    if(k)
                    {
                        text.append(", ");
                    }
                    text.append(move_data(m.moves[k]).name);
                }
                ui.say(text);
            }
            else if(act == 1 && pick > 0)
            {
                mon lead = g.party[pick];
                for(int k = pick; k > 0; --k)
                {
                    g.party[k] = g.party[k - 1];
                }
                g.party[0] = lead;
                bn::string<48> text(lead.name());
                text.append(" will lead your party.");
                ui.say(text);
            }
        }
    }

    // START: returns false to quit to the title.
    bool start_menu(ui& ui)
    {
        while(true)
        {
            constexpr bn::string_view options[] = { "POKéMON", "BAG", "SAVE", "TITLE", "EXIT" };
            int pick = ui.menu(options, 5, true);
            if(pick == 0)
            {
                party_menu(ui);
            }
            else if(pick == 1)
            {
                bn::string<48> text("POKé BALLS x");
                text.append(bn::to_string<4>(state().poke_balls));
                ui.say(text);
            }
            else if(pick == 2)
            {
                ui.show_text("Would you like to save the game?");
                constexpr bn::string_view yes_no[] = { "YES", "NO" };
                int yes = ui.menu(yes_no, 2, false, 0);
                ui.clear_text();
                if(yes == 0)
                {
                    save_game();
                    ui.say("Saved the game.");
                }
            }
            else if(pick == 3)
            {
                ui.show_text("Return to the title screen? Unsaved progress will be lost.");
                constexpr bn::string_view yes_no[] = { "YES", "NO" };
                int yes = ui.menu(yes_no, 2, false, 1);
                ui.clear_text();
                if(yes == 0)
                {
                    return false;
                }
            }
            else
            {
                return true;
            }
        }
    }
}

bool overworld_scene(ui& ui, encounter& wild)
{
    bn::random& random = rng();
    game_state& g = state();
    world w(g.area);

    // Build the background from the shared overworld tiles; tiles are placed at index 0 so the
    // generated cells can be used as they are.
    bn::bg_tiles::set_allow_offset(false);
    bn::regular_bg_tiles_item tiles_item(bn::span<const bn::tile>(wd::tiles), bn::bpp_mode::BPP_4);
    bn::bg_palette_item palette_item(bn::span<const bn::color>(wd::colors), bn::bpp_mode::BPP_4);
    bn::regular_bg_map_item map_item(bg_cells[0], bn::size(32, 32));
    int px = g.x * 16;
    int py = g.y * 16;
    int cam_x = px - view_x;
    int cam_y = py - view_y;
    w.draw(cam_x, cam_y);
    bn::regular_bg_ptr bg = bn::regular_bg_item(tiles_item, palette_item, map_item).create_bg(0, 0);
    bn::bg_tiles::set_allow_offset(true);
    bg.set_priority(2);
    bn::regular_bg_map_ptr bg_map = bg.map();
    bn::bg_palettes::set_transparent_color(bn::color(4, 9, 3));

    const bn::sprite_item& player_item = *person_sprites[int(person_kind::player)];
    bn::sprite_ptr player = player_item.create_sprite(0, 0, frame_of(g.facing, 0));
    player.set_bg_priority(1);
    set_actor_frame(player, player_item, g.facing, 0);

    // Tall grass covers the legs of whoever stands in it: the bottom half of the tile, drawn in front
    // (the web game's .tg-front). One for the tile you're on, one for the tile you're stepping from.
    bn::sprite_ptr grass_here = bn::sprite_items::grass_front.create_sprite(0, 0);
    bn::sprite_ptr grass_from = bn::sprite_items::grass_front.create_sprite(0, 0);
    for(bn::sprite_ptr* s : { &grass_here, &grass_from })
    {
        s->set_bg_priority(1);
        s->set_z_order(-200);
        s->set_visible(false);
    }
    int grass_x = g.x, grass_y = g.y, from_x = -1000, from_y = -1000;

    // People in the current area (they stand still in this build; the web game's wander is next).
    struct person_state
    {
        const pr::person* data;
        direction facing;
    };
    bn::vector<person_state, 12> people;
    bn::vector<actor_sprite, 12> people_sprites;
    auto load_people = [&]()
    {
        people_sprites.clear();
        people.clear();
        const pr::area& a = w.current();
        for(int i = 0; i < a.people_count && ! people.full(); ++i)
        {
            const pr::person& p = a.people[i];
            const bn::sprite_item& item = *person_sprites[int(p.kind)];
            people.push_back({ &p, p.facing });
            people_sprites.push_back({ item.create_sprite(0, 0, frame_of(p.facing, 0)), &item });
            people_sprites.back().sprite.set_bg_priority(1);
            set_actor_frame(people_sprites.back().sprite, item, p.facing, 0);
        }
    };
    load_people();
    auto person_at = [&](int tx, int ty) -> int
    {
        for(int i = 0; i < people.size(); ++i)
        {
            if(people[i].data->x == tx && people[i].data->y == ty)
            {
                return i;
            }
        }
        return -1;
    };

    int last_cx = 0x7fffffff;
    int last_cy = 0x7fffffff;
    int last_area = -1;
    auto refresh = [&]()
    {
        int cam_cx = cam_x >> 3;
        int cam_cy = cam_y >> 3;
        if(cam_cx != last_cx || cam_cy != last_cy || last_area != w.area_index)
        {
            w.draw(cam_x, cam_y);
            bg_map.reload_cells_ref();
            last_cx = cam_cx;
            last_cy = cam_cy;
            last_area = w.area_index;
        }
        // 256x256 map: hardware scroll = 8 - x, 48 - y (Butano centres BGs on the screen).
        bg.set_position(8 - (cam_x & 255), 48 - (cam_y & 255));
        for(int i = 0; i < people.size(); ++i)
        {
            place_actor(people_sprites[i].sprite, people[i].data->x * 16, people[i].data->y * 16, cam_x, cam_y);
        }
        auto place_grass = [&](bn::sprite_ptr& s, int tx, int ty)
        {
            bool on = w.behaviour_at(tx, ty) == behaviour::TALL_GRASS;
            s.set_visible(on);
            if(on)
            {
                s.set_position(tx * 16 - cam_x + 8 - 120, ty * 16 + 8 - cam_y + 4 - 80);
            }
        };
        place_grass(grass_here, grass_x, grass_y);
        place_grass(grass_from, from_x, from_y);
    };
    auto force_redraw = [&]()
    {
        w.draw(cam_x, cam_y);
        bg_map.reload_cells_ref();
    };

    int step_parity = 0;
    int idle_frames = 0;
    bool fresh_press = true;        // a direction pressed from a standstill turns first (Emerald)
    bool result = false;
    bool done = false;

    place_actor(player, px, py, cam_x, cam_y);
    refresh();
    force_redraw();
    ui::fade_in(12);

    while(! done)
    {
        // ----- Input while standing -----
        direction want = g.facing;
        bool dir_held = true;
        if(bn::keypad::up_held()) want = direction::UP;
        else if(bn::keypad::down_held()) want = direction::DOWN;
        else if(bn::keypad::left_held()) want = direction::LEFT;
        else if(bn::keypad::right_held()) want = direction::RIGHT;
        else dir_held = false;

        if(bn::keypad::start_pressed())
        {
            if(! start_menu(ui))
            {
                done = true;
                result = false;
                break;
            }
            bn::core::update();
            continue;
        }

        if(bn::keypad::a_pressed())
        {
            int fx = g.x + dx_of(g.facing);
            int fy = g.y + dy_of(g.facing);
            int who = person_at(fx, fy);
            place spot = w.find(fx, fy);
            if(who >= 0)
            {
                people[who].facing = opposite(g.facing);
                set_actor_frame(people_sprites[who].sprite, *people_sprites[who].item, people[who].facing, 0);
                const pr::person& p = *people[who].data;
                for(int i = 0; i < p.lines_count; ++i)
                {
                    ui.say(p.lines[i]);
                }
            }
            else if(spot.area >= 0)
            {
                const pr::area& a = wd::areas[spot.area];
                behaviour b = w.behaviour_at(fx, fy);
                if(b == behaviour::SIGN)
                {
                    if(const pr::sign* s = sign_at(a, spot.x, spot.y))
                    {
                        ui.say(s->text);
                    }
                }
                else if(b == behaviour::ITEM)
                {
                    g.pick_item(spot.area, spot.x, spot.y);
                    g.poke_balls = uint8_t(bn::min(99, g.poke_balls + 1));
                    force_redraw();
                    ui.say("You found a POKé BALL!");
                }
                else if(b == behaviour::WATER)
                {
                    ui.say("The water is a deep, clear blue.");
                }
            }
            bn::core::update();
            continue;
        }

        if(! dir_held)
        {
            random.update();
            fresh_press = true;
            set_actor_frame(player, player_item, g.facing, 0);
            bn::core::update();
            ++g.play_frames;
            continue;
        }

        // ----- A step (or a turn on the spot) -----
        if(fresh_press && want != g.facing)
        {
            g.facing = want;
            for(int f = 0; f < turn_frames; ++f)
            {
                set_actor_frame(player, player_item, g.facing, f < turn_frames / 2 ? 1 + step_parity : 0);
                bn::core::update();
                ++g.play_frames;
            }
            step_parity ^= 1;
            fresh_press = false;
            continue;
        }
        g.facing = want;
        fresh_press = false;

        int nx = g.x + dx_of(want);
        int ny = g.y + dy_of(want);
        behaviour target = w.behaviour_at(nx, ny);
        place target_place = w.find(nx, ny);
        bool blocked_by_person = person_at(nx, ny) >= 0;

        // An area that isn't in this build yet: say so instead of walking in.
        if(target_place.link >= 0 && target_place.area < 0)
        {
            set_actor_frame(player, player_item, g.facing, 0);
            bn::string<96> text(w.current().links[target_place.link].name);
            text.append(" isn't in this test build yet.");
            ui.say(text);
            fresh_press = true;
            while(bn::keypad::up_held() || bn::keypad::down_held() || bn::keypad::left_held() || bn::keypad::right_held())
            {
                bn::core::update();
            }
            continue;
        }

        // Ledges: hop over to the tile beyond.
        bool jump = (target == behaviour::LEDGE_DOWN && want == direction::DOWN) ||
                    (target == behaviour::LEDGE_RIGHT && want == direction::RIGHT) ||
                    (target == behaviour::LEDGE_LEFT && want == direction::LEFT);
        if(jump)
        {
            behaviour beyond = w.behaviour_at(nx + dx_of(want), ny + dy_of(want));
            jump = (beyond == behaviour::WALK || beyond == behaviour::TALL_GRASS) &&
                   person_at(nx + dx_of(want), ny + dy_of(want)) < 0;
        }
        bool walkable = target == behaviour::WALK || target == behaviour::TALL_GRASS || target == behaviour::DOOR;

        if(! jump && (! walkable || blocked_by_person))
        {
            // Bump: walk on the spot.
            for(int f = 0; f < walk_frames; ++f)
            {
                set_actor_frame(player, player_item, g.facing, f < walk_frames / 2 ? 1 + step_parity : 0);
                bn::core::update();
                ++g.play_frames;
            }
            step_parity ^= 1;
            continue;
        }

        int tiles = jump ? 2 : 1;
        bool run = ! jump && bn::keypad::b_held();
        int frames = jump ? jump_frames : run ? run_frames : walk_frames;
        int start_px = px;
        int start_py = py;
        from_x = g.x;
        from_y = g.y;
        grass_x = g.x + dx_of(want) * tiles;
        grass_y = g.y + dy_of(want) * tiles;
        for(int f = 1; f <= frames; ++f)
        {
            px = start_px + dx_of(want) * 16 * tiles * f / frames;
            py = start_py + dy_of(want) * 16 * tiles * f / frames;
            cam_x = px - view_x;
            cam_y = py - view_y;
            int lift = jump ? (f * (frames - f) * 8) / (frames * frames / 4) : 0;
            set_actor_frame(player, player_item, g.facing, f <= frames / 2 ? 1 + step_parity : 0);
            place_actor(player, px, py, cam_x, cam_y, lift);
            refresh();
            bn::core::update();
            ++g.play_frames;
        }
        step_parity ^= 1;
        g.x = int8_t(g.x + dx_of(want) * tiles);
        g.y = int8_t(g.y + dy_of(want) * tiles);
        from_x = from_y = -1000;
        grass_x = g.x;
        grass_y = g.y;

        // Crossed into a connected area: its coordinates take over (the camera doesn't move).
        place here = w.find(g.x, g.y);
        if(here.link >= 0 && here.area >= 0)
        {
            const pr::link& l = w.current().links[here.link];
            g.area = int8_t(here.area);
            g.x = int8_t(here.x);
            g.y = int8_t(here.y);
            px -= l.ox * 16;
            py -= l.oy * 16;
            cam_x = px - view_x;
            cam_y = py - view_y;
            w.area_index = here.area;
            grass_x = g.x;
            grass_y = g.y;
            load_people();
            refresh();
        }

        // Arrived: doors, tall grass.
        behaviour arrived = w.behaviour_at(g.x, g.y);
        if(arrived == behaviour::DOOR)
        {
            const pr::door* d = door_at(w.current(), g.x, g.y);
            set_actor_frame(player, player_item, g.facing, 0);
            door_kind kind = d ? d->kind : door_kind::HOUSE;
            if(kind == door_kind::CENTER)
            {
                // POKéMON CENTER: rooms come later; for now the nurse heals you at the door.
                ui.say("Welcome to the POKéMON CENTER! We'll restore your POKéMON to full health.");
                g.heal_party();
                ui.say("Your POKéMON are fully healed. We hope to see you again!");
            }
            else if(kind == door_kind::MART)
            {
                ui.say("The POKé MART is closed for now. (Buildings come in a later build.)");
            }
            else
            {
                ui.say("The door is locked.");
            }
            // Step back out.
            g.y = int8_t(g.y + 1);
            g.facing = direction::DOWN;
            py += 16;
            cam_y = py - view_y;
            grass_x = g.x;
            grass_y = g.y;
            place_actor(player, px, py, cam_x, cam_y);
            set_actor_frame(player, player_item, g.facing, 0);
            refresh();
            fresh_press = true;
            while(bn::keypad::up_held())
            {
                bn::core::update();
            }
        }
        else if(arrived == behaviour::TALL_GRASS && random.get_int(100) < encounter_percent && g.first_able() >= 0)
        {
            const pr::area& a = w.current();
            if(a.pool_count)
            {
                wild.species = a.pool[random.get_int(a.pool_count)];
                wild.level = wild_level(a, random);
                set_actor_frame(player, player_item, g.facing, 0);
                result = true;
                done = true;
            }
        }
        idle_frames = 0;
    }

    (void) idle_frames;
    if(result)
    {
        // Battle intro: the screen flashes twice, then fades.
        for(int i = 0; i < 2; ++i)
        {
            bn::bg_palettes::set_fade(bn::color(31, 31, 31), 1);
            ui::wait(4);
            bn::bg_palettes::set_fade(bn::color(31, 31, 31), 0);
            ui::wait(4);
        }
    }
    ui::fade_out(result ? 20 : 12);
    return result;
}

}
