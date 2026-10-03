// The overworld scene's internals, shared by pr_overworld.cpp (the world, drawing and walking) and
// pr_field.cpp (talking, places, the story and the START menu).
#ifndef PR_OVERWORLD_IMPL_H
#define PR_OVERWORLD_IMPL_H

#include "bn_bg_palette_ptr.h"
#include "bn_optional.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "pr_scenes.h"
#include "pr_state.h"
#include "pr_world_data.h"

namespace pr
{

namespace wd = world_data;

constexpr int walk_frames = 16;     // Emerald: a walking step is 16 frames, a running one 8
constexpr int run_frames = 8;
constexpr int turn_frames = 8;
constexpr int jump_frames = 32;
constexpr int grass_percent = 8;    // owArrive: tall grass 8%, a cave floor 5%, the water 5%
constexpr int cave_percent = 5;
constexpr int water_percent = 5;
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
inline direction toward(int fx, int fy, int tx, int ty)
{
    return tx > fx ? direction::RIGHT : tx < fx ? direction::LEFT : ty > fy ? direction::DOWN : direction::UP;
}

// What lies at a tile, looking through this area's connections when it's past the edge.
struct place
{
    int map = -1;           // -1: nothing (forest, the black around a room, or nowhere)
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
    bool event = false;            // put there by a scene (the professor, a grunt)
    bool neighbour = false;        // standing in a connected area: drawn, but not here to talk to
    bool legend = false;           // the guardian of the Sunken Shrine (or the tower's summoned legendary)
    bool legend_shiny = false;
    species_id legend_species = species_id(0);
    int move_frames = 0;           // > 0 while stepping
    int step_length = walk_frames;
    direction move_dir = direction::DOWN;
    bool step_foot = false;
    bool running = false;
    int fade = 0;                  // > 0: fading out over this many frames; < 0: fading in
    bn::optional<bn::sprite_ptr> sprite;
    bn::vector<int8_t, 24> trail;  // the way a trainer came to you (directions), to walk back after

    [[nodiscard]] bool active_trainer() const
    {
        return tr && ! state().beaten.test(tr->id);
    }
};

// The trainer who walked up to you (walkBackNpc): they walk back along the way they came after the battle.
struct walk_back
{
    int map = -1;
    int trainer_index = -1;
    int x = 0, y = 0;              // where they ended up
    bn::vector<int8_t, 24> trail;
};
walk_back& pending_walk_back();

class overworld
{

public:
    overworld();
    bool run(encounter& battle, const battle_report* last);

    // ----- World (pr_overworld.cpp) -----
    [[nodiscard]] place find(int tx, int ty) const;
    [[nodiscard]] int metatile_at(int tx, int ty) const;
    [[nodiscard]] behaviour behaviour_at(int tx, int ty) const;
    [[nodiscard]] int actor_at(int tx, int ty) const;
    [[nodiscard]] bool walkable(int tx, int ty) const;          // WALKABLE (not counting people)
    [[nodiscard]] bool blocked(int tx, int ty) const;
    [[nodiscard]] bool water_at(int tx, int ty) const;
    [[nodiscard]] bool area_flag(uint8_t flag) const;
    [[nodiscard]] const area_info* area() const;                 // the area you're in (a room's: its town's)
    [[nodiscard]] int area_index() const;
    [[nodiscard]] bool deep() const;                              // under the sea, outside
    [[nodiscard]] bool dive_spot_here() const;
    [[nodiscard]] bool shaft_here() const;
    void load_map(int index, bool keep_bg = false);
    void build_bg();
    void draw_cells(bool force);
    void refresh(bool force = false);
    void suspend();
    void resume();
    void load_actors();
    void make_sprite(actor& a);
    void place_actor(bn::sprite_ptr& sprite, int wx, int wy, int lift = 0);
    void set_player_frame(int step, bool run = false);
    void update_weather();
    void update_tint();
    void animate_tiles();
    void sweep_ash(int tx, int ty);
    void show_place_name();

    // ----- Frames -----
    void tick();
    void tick_actors();
    void wander();
    void wait_frames(int frames);
    void walk_actor(int index, direction d, int tiles, bool run = false);

    // ----- The player -----
    void step(direction want);
    void arrive();
    void cross_area(const place& here);
    bool check_sight();
    void trainer_approach(int index);
    void start_trainer_battle(int index);
    void wild_battle(bool water);
    void fixed_battle(species_id s, int level, bool legendary = false);
    void tower_legend_battle(species_id s, int level);
    void hold_until_released();

    // ----- Field (pr_field.cpp) -----
    void interact();
    void talk_to(int index);
    void legend_talk(int index);
    void water_action(int tx, int ty);
    void start_surf(int tx, int ty);
    void go_fish();
    void dive_action();
    void surface_action();
    void enter_room(const door& d);
    void leave_room();
    void nurse(int index);
    void mom();
    void clerk();
    void mart();
    void obtain(item_id id, int count, bool key_item = false);
    void obtain_named(const char* what, int count, const char* pocket);
    void use_pc();
    void player_pc();

    int add_temp_actor(person_kind kind, int x, int y, direction facing);
    void remove_actor(int index);
    int scene_npc(person_kind kind, int n, int min);
    void starter_event();
    int starter_bag();
    void starter_thanks();
    void rival_leaves();
    void story_enter();
    bool play_scene();
    void after_story();
    void move_prompts();
    void nickname_prompts();
    void dex_registration();
    void trainer_walk_back();
    void credits();

    void start_menu();
    void cheat_menu();
    void fast_travel(int area);
    void save_menu();
    void say(const bn::string_view& text);
    // A story line (scenes, calls, the professor, rivals' goodbyes): SKIP STORY TEXT leaves it out.
    void story_say(const bn::string_view& text);
    void tower_guide();
    // The hidden editor's trigger: the tree beside DUSKMERE HOLLOW's MART, then A, B, A at the lone tree up and to its right.
    bool egg_watch();
    // The CHALLENGE TOWER (SPIRECREST TOWN): in at the door, up floor by floor, the SUMMONING STONE at the
    // summit, the legendary in its chamber, and out again.
    bool tower_door(const door& d);
    void tower_after_battle();
    void tower_offer_up(int floor);
    void tower_stone();
    bool tower_leave();
    void tower_end(bool keep_streak);
    void warp_to_room(int map_index);
    void graveyard();
    void adventure_begins();

    // ----- Data -----
    const map_def* _map = nullptr;
    int _map_index = -1;
    int _tileset = -1;
    bn::optional<bn::regular_bg_ptr> _bg;
    bn::optional<bn::regular_bg_map_ptr> _bg_map;
    bn::optional<bn::regular_bg_ptr> _weather;
    int _weather_kind = -1;
    int _weather_x = 0, _weather_y = 0, _weather_timer = 0;
    int _px = 0, _py = 0, _cam_x = 0, _cam_y = 0;
    int _last_cx = 0x7fffffff, _last_cy = 0x7fffffff;
    bn::optional<bn::sprite_ptr> _player;
    bn::optional<bn::sprite_ptr> _grass_here;
    bn::optional<bn::sprite_ptr> _grass_from;
    bn::optional<bn::sprite_ptr> _dust;
    int _dust_frames = 0, _dust_x = 0, _dust_y = 0;
    int _grass_x = 0, _grass_y = 0, _from_x = -1000, _from_y = -1000;
    bn::vector<actor, 24> _actors;
    int _wander_timer = 0;
    int _anim_timer = 0;
    bool _anim_b = false;
    int _bob_timer = 0;
    int _step_parity = 0;
    bool _fresh_press = true;
    bool _surf_fresh = false;
    int _lift = 0;
    int _player_step = 0;
    bool _player_run = false;
    encounter* _battle = nullptr;
    const battle_report* _last = nullptr;
    bool _quit = false;
    bool _start_battle = false;
    bool _suspended = false;
};

}

#endif
