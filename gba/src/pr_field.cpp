// The overworld's field actions, after the web game: talking (talkTo), signs and things (owInteract), item
// balls, SURF, fishing and DIVE, doors, the POKéMON CENTER nurse, Mom, the MART, the PC, the story (the
// professor, the rival, Team Tempest's scenes, the guardian, the gifts, the calls), what happens after a
// battle (afterStory), the credits and the START menu.
#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_palette_ptr.h"

#include "bn_regular_bg_items_bag_bg.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_ball.h"

#include "pr_audio.h"
#include "pr_game_data.h"
#include "pr_overworld_impl.h"
#include "pr_people_sprites.h"
#include "pr_screens.h"
#include "pr_ui.h"

namespace pr
{

namespace
{
    bool just_loaded = false;
    bool new_game_started = false;

    const char* pocket_name(int pocket)
    {
        constexpr const char* names[] = { "ITEMS", "POKé BALLS", "TMs & HMs", "BERRIES", "KEY ITEMS" };
        return names[pocket];
    }

    // The party member that does the work in the field: a Water type that can fight, else anyone who can.
    const mon& field_mon()
    {
        game_state& g = state();
        for(int i = 0; i < g.party_count; ++i)
        {
            if(! g.party[i].fainted() && g.party[i].has_type(2))
            {
                return g.party[i];
            }
        }
        int k = g.first_able();
        return g.party[k >= 0 ? k : 0];
    }

    void upper(bn::istring& out, const char* text)
    {
        for(const char* c = text; *c; ++c)
        {
            out.push_back(*c >= 'a' && *c <= 'z' ? char(*c - 32) : *c);
        }
    }

    // The trainer id of an area's rival / leader, and of a named gym (canSurf: Tidalkeep, canDive: Rimefall).
    bool area_cleared(const char* name)
    {
        for(int i : wd::area_maps)
        {
            const map_def& m = wd::maps[i];
            if(bn::string_view(m.place_name) == bn::string_view(name))
            {
                return m.leader_id >= 0 && state().beaten.test(m.leader_id);
            }
        }
        return false;
    }

    bool can_surf()
    {
        return state().item_count(item_id::HM03) || area_cleared("TIDALKEEP CITY");
    }

    bool can_dive()
    {
        return state().item_count(item_id::HM08) || area_cleared("RIMEFALL TOWN");
    }

    // layerSpot(): a spot between a sea area and the one beneath it (they're different sizes), then the
    // nearest listed tile.
    void layer_spot(const map_def& from, const map_def& to, const uint8_t* points, int count, int x, int y, int& ox, int& oy)
    {
        int px = (2 * x * to.w + from.w) / (2 * from.w), py = (2 * y * to.h + from.h) / (2 * from.h);
        ox = to.spawn_x;
        oy = to.spawn_y;
        int best = 1 << 30;
        for(int i = 0; i < count; ++i)
        {
            int d = bn::abs(points[i * 2] - px) + bn::abs(points[i * 2 + 1] - py);
            if(d < best)
            {
                best = d;
                ox = points[i * 2];
                oy = points[i * 2 + 1];
            }
        }
    }
}

void set_just_loaded()
{
    just_loaded = true;
}

void set_new_game_started()
{
    new_game_started = true;
}

namespace
{
    // The CHALLENGE TOWER's rooms.
    int tower_room(room_kind kind, int floor = -1, gym_theme theme = gym_theme::NONE)
    {
        for(int i = 0; i < wd::maps_count; ++i)
        {
            const room_info* r = wd::maps[i].room;
            if(r && r->kind == kind && (floor < 0 || r->floor == floor) && (kind != room_kind::CHAMBER || r->theme == theme))
            {
                return i;
            }
        }
        return -1;
    }

    // Where a legendary waits: the chamber themed for its (first) type.
    gym_theme chamber_theme(int species)
    {
        bn::string_view t(game_data::type_names[game_data::species_list[species].type1]);
        if(t == "FIRE") return gym_theme::FIRE;
        if(t == "WATER") return gym_theme::WATER;
        if(t == "ELECTRIC") return gym_theme::ELECTRIC;
        if(t == "GRASS" || t == "BUG") return gym_theme::GRASS;
        if(t == "ICE") return gym_theme::ICE;
        if(t == "GHOST" || t == "PSYCHIC" || t == "DARK" || t == "POISON" || t == "FAIRY") return gym_theme::GHOST;
        if(t == "DRAGON" || t == "FLYING") return gym_theme::DRAGON;
        if(t == "GROUND" || t == "ROCK" || t == "STEEL" || t == "FIGHTING") return gym_theme::GROUND;
        return gym_theme::LEAGUE;
    }

    // Every tower floor's trainer stands ready again.
    void reset_tower_trainers()
    {
        game_state& g = state();
        for(int i = 0; i < wd::maps_count; ++i)
        {
            const map_def& m = wd::maps[i];
            for(int k = 0; k < m.trainers_count; ++k)
            {
                if(m.trainers[k].role == trainer_role::TOWER)
                {
                    g.beaten.reset(m.trainers[k].id);
                }
            }
        }
    }

    int tower_level()
    {
        game_state& g = state();
        int clears = bn::max(0, g.run.tower_clears - 1);
        return bn::min(champion_cap(), bn::max(50 + 3 * clears, g.average_level() + clears) + 5);
    }
}

// ----- The hidden editor -----
// In any POKéMON CENTER, stand in the bottom-left corner (1, 7), under the plant, and press LEFT, LEFT (at the
// wall), then A, B, A facing it: the editor opens. Every CENTER shares the one layout, and that wall has nothing
// to say, so nothing else answers those keys there.
namespace
{
    int egg_progress = 0;       // how much of LEFT, LEFT, A, B, A has been pressed in the corner

    constexpr int egg_spot_x = 1, egg_spot_y = 7;
}

bool overworld::egg_watch()
{
    game_state& g = state();
    bool in_center = _map->room && _map->room->kind == room_kind::CENTER;
    if(! in_center || g.x != egg_spot_x || g.y != egg_spot_y)
    {
        egg_progress = 0;
        return false;
    }
    enum key { NONE, LEFT, A, B, OTHER };
    key k = NONE;
    if(bn::keypad::left_pressed())
    {
        k = LEFT;
    }
    else if(bn::keypad::a_pressed())
    {
        k = A;
    }
    else if(bn::keypad::b_pressed())
    {
        k = B;
    }
    else if(bn::keypad::up_pressed() || bn::keypad::down_pressed() || bn::keypad::right_pressed() ||
            bn::keypad::start_pressed() || bn::keypad::select_pressed() || bn::keypad::l_pressed() ||
            bn::keypad::r_pressed())
    {
        k = OTHER;
    }
    if(k == NONE)
    {
        return false;
    }
    constexpr key want[] = { LEFT, LEFT, A, B, A };
    if(k == want[egg_progress] && (k == LEFT || g.facing == direction::LEFT))
    {
        ++egg_progress;
    }
    else
    {
        egg_progress = k == LEFT ? 1 : 0;   // a LEFT always starts it over
    }
    if(egg_progress < 5)
    {
        return false;
    }
    egg_progress = 0;
    audio::play(audio::sfx::SPOT);
    suspend();
    secret_editor_screen();
    resume();
    return true;
}

// ----- Talking, signs and things (owInteract) -----
void overworld::interact()
{
    game_state& g = state();
    ui& u = gui();
    bool outdoors = ! _map->is_room();
    if(outdoors && g.surfing && dive_spot_here())
    {
        dive_action();
        return;
    }
    if(outdoors && shaft_here())
    {
        surface_action();
        return;
    }
    int dx = dx_of(g.facing), dy = dy_of(g.facing);
    int fx = g.x + dx, fy = g.y + dy;
    // Talk across a counter.
    if(behaviour_at(fx, fy) == behaviour::COUNTER)
    {
        fx += dx;
        fy += dy;
    }
    int who = actor_at(fx, fy);
    if(who >= 0 && ! _actors[who].neighbour)
    {
        talk_to(who);
        return;
    }
    // Signs and tablets on this map.
    for(int i = 0; i < _map->signs_count; ++i)
    {
        if(_map->signs[i].x == fx && _map->signs[i].y == fy)
        {
            for(int k = 0; k < _map->signs[i].lines_count; ++k)
            {
                say(_map->signs[i].lines[k]);
            }
            return;
        }
    }
    int tx = g.x + dx, ty = g.y + dy;
    if(tx < 0 || ty < 0 || tx >= _map->w || ty >= _map->h)
    {
        return;
    }
    if(find_hidden(tx, ty))
    {
        return;
    }
    behaviour b = behaviour_at(tx, ty);
    if(b == behaviour::PC)
    {
        use_pc();
        return;
    }
    if(b == behaviour::ITEM)
    {
        for(int i = 0; i < _map->items_count; ++i)
        {
            const item_ball& it = _map->items[i];
            if(it.x == tx && it.y == ty)
            {
                g.picked.set(it.id);
                refresh(true);
                obtain(it.item, 1);
                save_game();
                return;
            }
        }
    }
    if(b == behaviour::WATER && outdoors)
    {
        water_action(tx, ty);
        return;
    }
    if(b == behaviour::STATUE && ! (_map->room && _map->room->kind == room_kind::GYM))
    {
        say("A statue of a great trainer of old.");
        return;
    }
    if(b == behaviour::STATUE)
    {
        // "CINDERGATE TOWN POKéMON GYM / Leader: Rell / Winning trainers: ..."
        const map_def& town = wd::maps[_map->exit_map];
        bn::string<64> text(town.name);
        text.append(" POKéMON GYM");
        say(text);
        text = "Leader: ";
        text.append(town.leader_name);
        say(text);
        text = "Winning trainers: ";
        text.append(town.leader_id >= 0 && g.beaten.test(town.leader_id) ? bn::string_view(g.name) : bn::string_view("..."));
        say(text);
        return;
    }
    for(int i = 0; i < _map->things_count; ++i)
    {
        if(_map->things[i].x == tx && _map->things[i].y == ty && _map->room && _map->room->kind == room_kind::SUMMIT)
        {
            tower_stone();
            return;
        }
        if(_map->things[i].x == tx && _map->things[i].y == ty)
        {
            for(int k = 0; k < _map->things[i].lines_count; ++k)
            {
                u.say(_map->things[i].lines[k]);
            }
            return;
        }
    }
}

void overworld::talk_to(int index)
{
    game_state& g = state();
    actor& a = _actors[index];
    if(a.legend)
    {
        legend_talk(index);
        return;
    }
    // faceNpcToPlayer()
    a.facing = toward(a.x, a.y, g.x, g.y);
    a.sprite->set_tiles(person_sprites[int(a.kind)]->tiles_item(), a.facing == direction::DOWN ? 0 : a.facing == direction::UP ? 3 : 6);
    a.sprite->set_horizontal_flip(a.facing == direction::RIGHT);
    tick();
    // A fisherman who isn't battling hands out the OLD ROD; later on (3 and 6 badges, GBA 1.8) the GOOD ROD
    // and the SUPER ROD.
    if(a.kind == person_kind::fisher && ! a.tr && ! g.item_count(item_id::OLDROD))
    {
        g.add_item(item_id::OLDROD, 1);
        say("Hey there! Ever tried fishing?");
        say("Nothing beats the thrill of a bite. Here, you have this OLD ROD!");
        obtain_named("OLD ROD", 1, "KEY ITEMS");
        say("Face any water and press A to cast your line. Good luck!");
        save_game();
        return;
    }
    if(a.kind == person_kind::fisher && ! a.tr && g.badges() >= 3 && ! g.item_count(item_id::GOODROD))
    {
        g.add_item(item_id::GOODROD, 1);
        say("Still fishing with that OLD ROD? You've earned better!");
        obtain_named("GOOD ROD", 1, "KEY ITEMS");
        say("Bigger POKéMON bite on a GOOD ROD. Give it a try!");
        save_game();
        return;
    }
    if(a.kind == person_kind::fisher && ! a.tr && g.badges() >= 6 && ! g.item_count(item_id::SUPERROD))
    {
        g.add_item(item_id::SUPERROD, 1);
        say("You've got the look of a true angler. Take my SUPER ROD!");
        obtain_named("SUPER ROD", 1, "KEY ITEMS");
        say("Even rare POKéMON bite on that one. Good luck!");
        save_game();
        return;
    }
    if(a.tr)
    {
        if(a.active_trainer())
        {
            trainer_approach(index);
            return;
        }
        for(int i = 0; i < a.tr->after_count; ++i)
        {
            say(a.tr->after[i]);
        }
        if(offer_rematch(index))
        {
            return;
        }
        if(a.tr->role == trainer_role::TOWER)
        {
            tower_offer_up(a.tr->elite);
        }
        return;
    }
    if(! a.who)
    {
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
    case person_role::TOWER:
        tower_guide();
        break;
    case person_role::TRADER:
        trader();
        break;
    case person_role::DAYCARE:
        daycare();
        break;
    case person_role::TUTOR:
        move_tutor();
        break;
    case person_role::CAPTAIN:
        captain();
        break;
    case person_role::PILOT:
        pilot();
        break;
    case person_role::SHARD_NORMAL:
    case person_role::SHARD_ATTACK:
    case person_role::SHARD_DEFENSE:
    case person_role::SHARD_SPEED:
        for(int i = 0; i < p.lines_count; ++i)
        {
            say(p.lines[i]);
        }
        meteor_shard(int(p.role) - int(person_role::SHARD_NORMAL));
        break;
    case person_role::GUIDE_ON:
    case person_role::GUIDE_BACK:
        region_guide(p.role == person_role::GUIDE_ON);
        break;
    case person_role::MEW_SIGHT:
        mew_sighting(a.who - _map->people);
        break;
    case person_role::TWIN:
        twin_rift();
        break;
    case person_role::SEED:
    case person_role::TREE:
        ancient_seed(p.role == person_role::TREE);
        break;
    case person_role::FORM:
        for(int i = 0; i < p.lines_count; ++i)
        {
            say(p.lines[i]);
        }
        form_altar();
        break;
    case person_role::FERRY:
        ferry();
        break;
    case person_role::PROF:
        calderra_prof();
        break;
    default:
        for(int i = 0; i < p.lines_count; ++i)
        {
            say(p.lines[i]);
        }
        break;
    }
}

// The guardian of the Sunken Shrine: Vesper stands in the way until she's beaten.
void overworld::legend_talk(int index)
{
    game_state& g = state();
    if(_map->leader_id >= 0 && ! g.beaten.test(_map->leader_id))
    {
        bn::string<128> text;
        upper(text, _map->leader_name);
        text.append(": \"Get away from the guardian! You'll have to go through me!\"");
        say(text);
        for(int i = 0; i < _actors.size(); ++i)
        {
            if(_actors[i].tr && _actors[i].tr->id == _map->leader_id)
            {
                start_trainer_battle(i);
                return;
            }
        }
        return;
    }
    species_id legend = _actors[index].legend_species;
    if(legend == species_id::MEW && map_region(_map_index) == 5 && mew_sightings_seen() < flag::mew_sightings)
    {
        // 5.0.0: GENOVA's MEW hides under the old truck until you've seen it in all ten places.
        say("An old truck sits at the end of the pier. Something pink peeks out from underneath... and hides again.");
        bn::string<96> text("MEW sightings: ");
        text.append(bn::to_string<4>(mew_sightings_seen()));
        text.append(" of ");
        text.append(bn::to_string<4>(flag::mew_sightings));
        text.append(". It doesn't trust you yet.");
        say(text);
        return;
    }
    if(_map->room && _map->room->kind == room_kind::CHAMBER)
    {
        bn::string<64> call;
        upper(call, game_data::species_list[int(legend)].name);
        call.append(" looks down at you, waiting...");
        say(call);
        tower_legend_battle(legend, tower_level());
        return;
    }
    if(map_region(_map_index) == 1 && ! g.has(story::MASTER_GIFT) && ! g.has(story::LEGEND_CAUGHT))
    {
        // (A save from before 1.6, already past WREN at the door: the MASTER BALL waits at the guardian's feet.)
        g.story |= story::MASTER_GIFT;
        say("Something glints at the guardian's feet...");
        obtain(item_id::MASTERBALL, 1);
    }
    int beast = legend_flag(legend);
    if(beast >= 0 && beast < 3)
    {
        // Calderra's beasts don't stay to fight: they run, and roam their branch's routes until caught.
        bn::string<96> run_text;
        upper(run_text, game_data::species_list[int(legend)].name);
        run_text.append(" looked at you for a long moment...");
        say(run_text);
        run_text.clear();
        upper(run_text, game_data::species_list[int(legend)].name);
        run_text.append(" bolted away into the wilds!");
        say(run_text);
        start_roaming(beast);
        save_game();
        load_actors();
        refresh(true);
        say("It must be roaming the routes nearby. It won't stay in one place for long...");
        return;
    }
    bn::string<64> text("The great ");
    text.append(game_data::species_list[int(legend)].name);
    text.append(" is stirring...");
    say(text);
    say("Gyaaaoooh!");
    g.story |= story::LEGEND_FIGHT;
    // Calderra's: a little above the level cap there (HO-OH at the very top).
    int level = map_region(_map_index) == 1 ? 50 : legend == species_id::HO_OH ? 200 :
                bn::min(region_cap(map_region(_map_index)), map_level_cap(_map_index) + 5);
    fixed_battle(legend, level, true);
}

// ----- Water: SURF and the OLD ROD (waterAction) -----
void overworld::water_action(int tx, int ty)
{
    game_state& g = state();
    ui& u = gui();
    bool surf = can_surf() && ! g.surfing;
    int rod = g.item_count(item_id::SUPERROD) ? 2 : g.item_count(item_id::GOODROD) ? 1 : 0;
    bool fish = g.item_count(item_id::OLDROD) || rod;
    constexpr const char* rod_names[] = { "OLD ROD", "GOOD ROD", "SUPER ROD" };
    if(! surf && ! fish)
    {
        say("The water is dyed a deep blue...");
        return;
    }
    int pick = -1;      // 0 SURF, 1 FISH
    if(surf != fish)
    {
        bn::string<64> ask("Would you like to fish with the ");
        ask.append(rod_names[rod]);
        ask.append("?");
        u.show_text(surf ? bn::string_view("The water is dyed a deep blue... Would you like to SURF?") : bn::string_view(ask));
        bool yes = u.yes_no();
        u.clear_text();
        pick = yes ? (surf ? 0 : 1) : -1;
    }
    else
    {
        u.show_text("The water is dyed a deep blue... What would you like to do?");
        constexpr bn::string_view options[] = { "SURF", "FISH", "CANCEL" };
        int k = u.list(options, 3);
        u.clear_text();
        pick = k == 0 ? 0 : k == 1 ? 1 : -1;
    }
    if(pick == 0)
    {
        start_surf(tx, ty);
    }
    else if(pick == 1)
    {
        go_fish(rod);
    }
}

void overworld::start_surf(int tx, int ty)
{
    game_state& g = state();
    bn::string<48> text;
    upper(text, field_mon().name());
    text.append(" used SURF!");
    say(text);
    audio::play(audio::sfx::JUMP);
    int start_px = _px, start_py = _py;
    int dx = tx - g.x, dy = ty - g.y;
    g.x = int16_t(tx);
    g.y = int16_t(ty);
    for(int f = 1; f <= jump_frames; ++f)
    {
        _px = start_px + dx * 16 * f / jump_frames;
        _py = start_py + dy * 16 * f / jump_frames;
        _cam_x = _px - view_x;
        _cam_y = _py - view_y;
        _lift = (f * (jump_frames - f) * 12) / (jump_frames * jump_frames / 4);
        tick();
    }
    _lift = 0;
    g.surfing = true;
    _surf_fresh = true;
    _grass_x = g.x;
    _grass_y = g.y;
    set_player_frame(0);
    refresh();
    arrive();
}

// Emerald's fishing: cast, a wait, then "Oh! A bite!" (a single wild Pokémon) or "Not even a nibble...".
// The OLD ROD (0) catches the area's fish a little under your level; the GOOD ROD (1) bites more often, at
// your level, from the surf's pool too; the SUPER ROD (2) nearly always bites, a little higher, and now and
// then hooks a rare Water type from anywhere (GBA 1.8).
void overworld::go_fish(int rod)
{
    game_state& g = state();
    bn::random& r = rng();
    constexpr const char* rod_names[] = { "OLD ROD", "GOOD ROD", "SUPER ROD" };
    audio::play(audio::sfx::BALL);
    bn::string<48> text(g.name);
    text.append(" used the ");
    text.append(rod_names[rod]);
    text.append("!");
    say(text);
    say(". . . . . .");
    constexpr int miss[] = { 30, 15, 5 };
    if(r.get_int(100) < miss[rod])
    {
        say("Not even a nibble...");
        return;
    }
    audio::play(audio::sfx::SPOT);
    say("Oh! A bite!");
    int fish = _map->area->fish_count, water = rod ? _map->area->water_count : 0;
    int k = r.get_int(fish + water);
    species_id s = k < fish ? _map->fish[k] : _map->water[k - fish];
    if(rod == 2 && r.get_int(10) == 0)
    {
        int water_type = type_index("WATER");
        for(int tries = 0; tries < 200; ++tries)
        {
            int sp = r.get_int(species_count);
            bool legend = false;
            for(int i = 0; i < game_data::legendaries_count; ++i)
            {
                legend |= game_data::legendaries[i] == sp;
            }
            const species& d = game_data::species_list[sp];
            if(! legend && (d.type1 == water_type || d.type2 == water_type))
            {
                s = species_id(sp);
                break;
            }
        }
    }
    // wildLevel() - 2, at least 3 (the better rods: wildLevel(), then + 2).
    int cap = map_level_cap(g.map);
    int wild = bn::max(map_region(g.map) >= 2 ? cap - 6 : 2, bn::min(cap, g.average_level() - 2 + r.get_int(3)));
    constexpr int bonus[] = { -2, 0, 2 };
    fixed_battle(s, bn::clamp(wild + bonus[rod], 3, bn::max(100, cap)));
}

// ----- DIVE -----
void overworld::dive_action()
{
    game_state& g = state();
    ui& u = gui();
    if(! can_dive())
    {
        say("The sea is deep here.");
        say("A POKéMON may be able to go underwater.");
        return;
    }
    u.show_text("The sea is deep here. Would you like to use DIVE?");
    bool yes = u.yes_no();
    u.clear_text();
    if(! yes)
    {
        return;
    }
    bn::string<48> text;
    upper(text, field_mon().name());
    text.append(" used DIVE!");
    say(text);
    ui::fade_out(18);
    int down = _map->area->dive;
    const map_def& dm = wd::maps[down];
    int x, y;
    layer_spot(*_map, dm, dm.area->shafts, dm.area->shaft_count, g.x, g.y, x, y);
    g.x = int16_t(x);
    g.y = int16_t(y);
    g.surfing = false;
    g.facing = direction::DOWN;
    _player.reset();
    load_map(down);
    ui::fade_in(18);
    save_game();
    hold_until_released();
}

void overworld::surface_action()
{
    game_state& g = state();
    ui& u = gui();
    u.show_text("Light is filtering down from above. Would you like to use DIVE?");
    bool yes = u.yes_no();
    u.clear_text();
    if(! yes)
    {
        return;
    }
    ui::fade_out(18);
    int up = _map->area->surface;
    const map_def& um = wd::maps[up];
    int x, y;
    layer_spot(*_map, um, um.area->dive_spots, um.area->dive_count, g.x, g.y, x, y);
    g.x = int16_t(x);
    g.y = int16_t(y);
    g.surfing = true;
    g.facing = direction::DOWN;
    _player.reset();
    load_map(up);
    ui::fade_in(18);
    save_game();
    hold_until_released();
}

// ----- Places -----
void overworld::enter_room(const door& d)
{
    game_state& g = state();
    set_player_frame(0);
    audio::play(audio::sfx::DOOR);
    wait_frames(12);
    _player->set_visible(false);
    ui::fade_out(12);
    const map_def& room = wd::maps[d.room];
    g.x = room.spawn_x;
    g.y = room.spawn_y;
    g.facing = direction::UP;
    _player.reset();
    load_map(d.room);
    ui::fade_in(12);
    _fresh_press = true;
    save_game();
}

// Coming out: you appear in the open doorway and step down onto the path, then the door shuts.
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
    audio::play(audio::sfx::DOOR);
    int start_py = _py;
    for(int f = 1; f <= walk_frames; ++f)
    {
        _py = start_py + 16 * f / walk_frames;
        _cam_y = _py - view_y;
        set_player_frame(f <= walk_frames / 2 ? 1 + _step_parity : 0);
        tick();
    }
    _step_parity ^= 1;
    g.y = int16_t(g.y + 1);
    _grass_y = g.y;
    _fresh_press = true;
    save_game();
}

// The POKéMON CENTER nurse (Emerald's script, as the web game's nurseTalk / nurseHeal).
void overworld::nurse(int index)
{
    ui& u = gui();
    game_state& g = state();
    say("Hello, and welcome to\nthe POKéMON CENTER.");
    say("We restore your tired\nPOKéMON to full health.");
    u.show_text("Would you like to rest your POKéMON?");
    bool yes = u.yes_no();
    u.clear_text();
    if(! yes)
    {
        say("We hope to see you again!");
        return;
    }
    audio::set_hush(true);
    u.show_text("Okay, I'll take your\nPOKéMON for a few seconds.");
    actor& n = _actors[index];
    n.sprite->set_tiles(person_sprites[int(n.kind)]->tiles_item(), 6);
    n.sprite->set_horizontal_flip(false);
    // A ball every 25 frames, 32 frames' pause, then the jingle with the machine glowing (pokeemerald).
    for(int i = 0; i < g.party_count; ++i)
    {
        audio::play(audio::sfx::BALL);
        wait_frames(25);
    }
    wait_frames(32);
    audio::play(audio::sfx::HEAL);
    for(int f = 0; f < 150; ++f)
    {
        if(_bg)
        {
            bn::bg_palette_ptr pal = _bg->palette();
            pal.set_fade(bn::color(31, 31, 31), (f / 10) % 2 ? bn::fixed(0.2) : bn::fixed(0));
        }
        tick();
    }
    update_tint();
    wait_frames(14);
    n.sprite->set_tiles(person_sprites[int(n.kind)]->tiles_item(), 0);
    g.heal_party();
    g.last_heal = int16_t(area_index());
    save_game();
    audio::set_hush(false);
    u.clear_text();
    say("Thank you for waiting.");
    say("We've restored your\nPOKéMON to full health.");
    say("We hope to see you again!");
}

// Mom: a rest at home heals everyone (no Pokémon yet: just a word of encouragement).
void overworld::mom()
{
    game_state& g = state();
    if(! g.party_count)
    {
        bn::string<128> text("MOM: ");
        text.append(g.name);
        text.append("! The professor went out toward ROUTE 1. Go on, catch up with him!");
        say(text);
        return;
    }
    bn::string<128> text("MOM: Welcome home, ");
    text.append(g.name);
    text.append("! You and your POKéMON look worn out. Sit down and rest a while.");
    say(text);
    g.heal_party();
    g.last_heal = int16_t(area_index());
    save_game();
    audio::play(audio::sfx::HEAL);
    wait_frames(54);
    say("MOM: There, all better! Come home whenever you need a rest. Take care out there!");
}

// Emerald's item fanfare: "Obtained ..." while the jingle plays, then where it went.
void overworld::obtain_named(const char* what, int count, const char* pocket)
{
    game_state& g = state();
    ui& u = gui();
    audio::play(audio::sfx::OBTAIN);
    bn::string<64> text("Obtained ");
    if(count > 1)
    {
        text.append(bn::to_string<4>(count));
        text.append(" ");
    }
    else
    {
        text.append("the ");
    }
    text.append(what);
    text.append("!");
    u.say_timed(text, 60);
    bn::string<96> put(g.name);
    put.append(" put away the ");
    put.append(what);
    put.append("\nin the ");
    put.append(pocket);
    put.append(" POCKET.");
    say(put);
}

void overworld::obtain(item_id id, int count, bool)
{
    game_state& g = state();
    const item_info& it = game_data::items[int(id)];
    g.add_item(id, count);
    obtain_named(it.name, count, pocket_name(it.pocket));
}

void overworld::clerk()
{
    game_state& g = state();
    // New in town: free POKé BALLS (once per town in a row: restockedLoc).
    if(g.restocked != area_index())
    {
        g.restocked = int16_t(area_index());
        g.add_item(item_id::POKEBALL, 5);
        say("Welcome to the Poké Mart!");
        say("You're new in town? Here, take these on the house!");
        obtain_named("POKé BALLS", 5, "POKé BALLS");
        save_game();
        return;
    }
    mart();
}

// martOpen(): BUY / SELL / SEE YA!, with your money in a window top left. More stock with more badges.
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
    int badges = g.badges();
    item_id stock[20];
    int stock_count = 0;
    for(item_id id : { item_id::POKEBALL, item_id::POTION, item_id::ANTIDOTE, item_id::PARLYZHEAL, item_id::AWAKENING,
                       item_id::REPEL, item_id::ESCAPEROPE })
    {
        stock[stock_count++] = id;
    }
    if(badges >= 2)
    {
        stock[stock_count++] = item_id::GREATBALL;
        stock[stock_count++] = item_id::SUPERPOTION;
        stock[stock_count++] = item_id::BURNHEAL;
        stock[stock_count++] = item_id::SUPERREPEL;
    }
    if(badges >= 5)
    {
        stock[stock_count++] = item_id::ULTRABALL;
        stock[stock_count++] = item_id::HYPERPOTION;
        stock[stock_count++] = item_id::REVIVE;
        stock[stock_count++] = item_id::MAXREPEL;
    }
    if(badges >= 6)
    {
        stock[stock_count++] = item_id::MAXPOTION;
    }
    if(badges >= 8)
    {
        stock[stock_count++] = item_id::FULLRESTORE;
    }
    // GBA 1.8: the TMs (more with more badges; each bought once, as it's never used up) and the evolution items
    // (from the 3rd badge).
    item_id tm_stock[game_data::tms_count];
    int tm_count = 0;
    item_id evo_stock[14];
    int evo_count = 0;
    if(badges >= 3)
    {
        for(item_id id : { item_id::FIRESTONE, item_id::WATERSTONE, item_id::THUNDERSTONE, item_id::LEAFSTONE,
                           item_id::MOONSTONE, item_id::SUNSTONE, item_id::SHINYSTONE, item_id::DUSKSTONE,
                           item_id::DAWNSTONE, item_id::ICESTONE, item_id::OVALSTONE, item_id::RAZORCLAW,
                           item_id::RAZORFANG, item_id::LINKINGCORD })
        {
            evo_stock[evo_count++] = id;
        }
    }
    // GBA 1.9: held items (from the 2nd badge; LIFE ORB and CHOICE SCARF from the 4th).
    item_id held_stock[5];
    int held_count = 0;
    if(badges >= 2)
    {
        held_stock[held_count++] = item_id::SITRUSBERRY;
        held_stock[held_count++] = item_id::LEFTOVERS;
        held_stock[held_count++] = item_id::FOCUSSASH;
    }
    if(badges >= 4)
    {
        held_stock[held_count++] = item_id::LIFEORB;
        held_stock[held_count++] = item_id::CHOICESCARF;
    }
    // BUY from a list: how many (x1, x5, x10), then OK? A TM comes alone.
    auto buy = [&](const item_id* list, int count, bool tms)
    {
        int last = 0;
        while(true)
        {
            item_id shown[game_data::tms_count + 1];
            bn::string<32> labels[game_data::tms_count + 1];
            bn::string_view views[game_data::tms_count + 1];
            int n = 0;
            for(int i = 0; i < count; ++i)
            {
                if(tms && g.item_count(list[i]))
                {
                    continue;       // already yours
                }
                const item_info& it = game_data::items[int(list[i])];
                shown[n] = list[i];
                labels[n] = it.name;
                labels[n].append("  $");
                labels[n].append(bn::to_string<6>(it.price));
                views[n] = labels[n];
                ++n;
            }
            if(! n)
            {
                u.say_timed("You already have every TM I sell!", 72);
                return;
            }
            views[n] = "CANCEL";
            u.show_text("What would you like?");
            // (A long list is wide enough to cover the money window: it comes back after.)
            money.clear();
            u.win().clear(0, 0, 12, 4);
            int k = u.list(views, n + 1, bn::min(last, n));
            u.clear_text();
            show_money();
            if(k < 0 || k == n)
            {
                return;
            }
            last = k;
            item_id id = shown[k];
            const item_info& it = game_data::items[int(id)];
            int max = bn::min(tms ? 1 : 99, int(g.money / it.price));
            if(! max)
            {
                u.say_timed("You don't have enough money.", 72);
                continue;
            }
            int qs[3] = { 1, 5, 10 };
            int q = 1;
            bn::string<96> text(it.name);
            if(! tms)
            {
                bn::string<24> ql[4];
                bn::string_view qv[4];
                int qn = 0;
                for(int x : qs)
                {
                    if(x <= max)
                    {
                        ql[qn] = "x";
                        ql[qn].append(bn::to_string<4>(x));
                        ql[qn].append("  $");
                        ql[qn].append(bn::to_string<8>(x * it.price));
                        qv[qn] = ql[qn];
                        ++qn;
                    }
                }
                qv[qn] = "CANCEL";
                text.append("? Certainly. How many would you like?");
                u.show_text(text);
                int qi = u.list(qv, qn + 1);
                u.clear_text();
                if(qi < 0 || qi == qn)
                {
                    continue;
                }
                q = qs[qi];
            }
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
            save_game();
            show_money();
            audio::play(audio::sfx::BALL);
            u.say_timed("Here you go! Thank you very much.", 54);
        }
    };
    while(true)
    {
        tm_count = 0;
        for(int i = 0; i < game_data::tms_count; ++i)
        {
            if(game_data::tms[i].badges <= badges)
            {
                tm_stock[tm_count++] = game_data::tms[i].item;
            }
        }
        u.show_text("Welcome! How may I serve you?");
        bn::string_view top[6];
        int top_ids[6];
        int top_count = 0;
        top_ids[top_count] = 0;
        top[top_count++] = "BUY";
        top_ids[top_count] = 2;
        top[top_count++] = "TMs";
        if(evo_count)
        {
            top_ids[top_count] = 3;
            top[top_count++] = "EVOLUTION";
        }
        if(held_count)
        {
            top_ids[top_count] = 5;
            top[top_count++] = "HELD ITEMS";
        }
        top_ids[top_count] = 1;
        top[top_count++] = "SELL";
        top_ids[top_count] = 4;
        top[top_count++] = "SEE YA!";
        int chosen = u.list(top, top_count);
        int pick = chosen < 0 ? 4 : top_ids[chosen];
        u.clear_text();
        if(pick == 0)
        {
            buy(stock, stock_count, false);
        }
        else if(pick == 2)
        {
            buy(tm_stock, tm_count, true);
        }
        else if(pick == 3)
        {
            buy(evo_stock, evo_count, false);
        }
        else if(pick == 5)
        {
            buy(held_stock, held_count, false);
        }
        else if(pick == 1)
        {
            // SELL: anything you carry, at half price (key items and HMs can't be sold).
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
                    u.say_timed("You don't have anything to sell.", 72);
                    break;
                }
                views[n] = "CANCEL";
                u.show_text("What would you like to sell?");
                money.clear();
                int k = u.list(views, n + 1);
                show_money();
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
                save_game();
                show_money();
                text = "Turned over the ";
                text.append(it.name);
                text.append(" and received $");
                text.append(bn::to_string<8>(q * each));
                text.append(".");
                u.say_timed(text, 66);
            }
        }
        else
        {
            break;
        }
    }
    money.clear();
    u.win().clear(0, 0, 12, 4);
    say("Please come again!");
}

// ----- The PC (usePC): SOMEONE'S PC (the boxes), your own (item storage), LOG OFF -----
// NUZLOCKE's graveyard: the fallen (the latest 30), three to a line.
void overworld::graveyard()
{
    game_state& g = state();
    bn::string<64> text("POKéMON lost: ");
    text.append(bn::to_string<6>(g.run.deaths));
    say(text);
    int shown = 0;
    text.clear();
    for(int k = 0; k < graveyard_size; ++k)
    {
        // Oldest first: the ring starts at grave_next.
        const mon& m = g.run.graveyard[(g.run.grave_next + k) % graveyard_size];
        if(m.empty())
        {
            continue;
        }
        if(! text.empty())
        {
            text.append(", ");
        }
        text.append(m.name());
        text.append(" Lv");
        text.append(bn::to_string<4>(m.level));
        if(++shown % 3 == 0)
        {
            say(text);
            text.clear();
        }
    }
    if(! text.empty())
    {
        say(text);
    }
    say("May they rest in peace.");
}

void overworld::use_pc()
{
    game_state& g = state();
    ui& u = gui();
    audio::play(audio::sfx::PC_ON);
    audio::set_hush(true);
    wait_frames(30);
    bn::string<48> text(g.name);
    text.append(" booted up the PC.");
    say(text);
    while(true)
    {
        u.show_text("Which PC should be accessed?");
        bn::string<24> mine(g.name);
        mine.append("'s PC");
        // NUZLOCKE: the graveyard, once someone has fallen.
        bool grave = g.run.nuzlocke() && g.run.deaths;
        bn::string_view options[] = { "SOMEONE'S PC", mine, grave ? "GRAVEYARD" : "LOG OFF", "LOG OFF" };
        int k = u.list_top_left(options, grave ? 4 : 3);
        u.clear_text();
        if(grave && k == 2)
        {
            graveyard();
            continue;
        }
        if(k == 1)
        {
            audio::play(audio::sfx::PC_LOGIN);
            text = "Accessed ";
            text.append(g.name);
            text.append("'s PC.");
            say(text);
            player_pc();
            continue;
        }
        if(k != 0)
        {
            break;
        }
        audio::play(audio::sfx::PC_LOGIN);
        say("Accessed SOMEONE'S PC.");
        say("POKéMON Storage System opened.");
        int start = 0;
        while(true)
        {
            constexpr bn::string_view pc_options[] = { "WITHDRAW POKéMON", "DEPOSIT POKéMON", "MOVE POKéMON", "SEE YA!" };
            constexpr const char* pc_desc[] = { "Move POKéMON stored in BOXES to your party.", "Store POKéMON in your party in BOXES.",
                                               "Organize the POKéMON in BOXES and in your party.", "Return to the previous menu." };
            struct desc_ctx
            {
                const char* const* lines;
            } ctx{ pc_desc };
            auto on_move = [](void* p, int i)
            {
                gui().show_text(static_cast<desc_ctx*>(p)->lines[i]);
            };
            int pick = u.list_top_left(pc_options, 4, start, true, on_move, &ctx);
            u.clear_text();
            if(pick < 0 || pick == 3)
            {
                break;
            }
            start = pick;
            // Guards, as in Emerald: the menu stays up with the reason.
            const char* stop = nullptr;
            if(pick == 0 && g.party_count >= g.party_cap())
            {
                stop = "Your party is full!";
            }
            else if(pick == 1 && g.party_count <= 1)
            {
                stop = "There is just one POKéMON with you.";
            }
            else if(pick == 0 && ! g.box_used())
            {
                stop = "There are no POKéMON in the BOXES.";
            }
            if(stop)
            {
                u.say_timed(stop, 72);
                continue;
            }
            suspend();
            pc_box_screen(pick == 0 ? pc_mode::WITHDRAW : pick == 1 ? pc_mode::DEPOSIT : pc_mode::MOVE);
            save_game();
            resume();
        }
    }
    audio::play(audio::sfx::PC_OFF);
    audio::set_hush(false);
}

// {PLAYER}'s PC (Emerald): ITEM STORAGE, WITHDRAW ITEM / DEPOSIT ITEM.
void overworld::player_pc()
{
    game_state& g = state();
    ui& u = gui();
    while(true)
    {
        constexpr bn::string_view options[] = { "WITHDRAW ITEM", "DEPOSIT ITEM", "CANCEL" };
        constexpr const char* desc[] = { "Take out items from the PC.", "Store items in the PC.", "Go back to the previous menu." };
        auto on_move = [](void* p, int i)
        {
            gui().show_text(static_cast<const char* const*>(p)[i]);
        };
        int k = u.list_top_left(options, 3, 0, true, on_move, const_cast<const char**>(desc));
        u.clear_text();
        if(k < 0 || k == 2)
        {
            return;
        }
        bool withdraw = k == 0;
        while(true)
        {
            auto& from = withdraw ? g.pc_items : g.items;
            auto& to = withdraw ? g.items : g.pc_items;
            int ids[items_count];
            bn::string<32> labels[items_count + 1];
            bn::string_view views[items_count + 1];
            int n = 0;
            for(int i = 0; i < items_count; ++i)
            {
                if(from[i])
                {
                    ids[n] = i;
                    labels[n] = game_data::items[i].name;
                    labels[n].append(" x");
                    labels[n].append(bn::to_string<4>(from[i]));
                    views[n] = labels[n];
                    ++n;
                }
            }
            if(! n)
            {
                u.say_timed(withdraw ? "There are no items." : "You don't have any items.", 66);
                break;
            }
            views[n] = "CANCEL";
            u.show_text(withdraw ? "Withdraw which item?" : "Deposit which item?");
            int pick = u.list_top_left(views, n + 1);
            u.clear_text();
            if(pick < 0 || pick == n)
            {
                break;
            }
            from[ids[pick]] = uint8_t(from[ids[pick]] - 1);
            to[ids[pick]] = uint8_t(to[ids[pick]] + 1);
            save_game();
            bn::string<48> text(withdraw ? "Withdrew " : "Deposited ");
            text.append(game_data::items[ids[pick]].name);
            text.append(".");
            u.say_timed(text, 42);
        }
    }
}

// ----- Story -----
int overworld::add_temp_actor(person_kind kind, int x, int y, direction facing)
{
    actor a;
    a.kind = kind;
    a.x = a.home_x = x;
    a.y = a.home_y = y;
    a.facing = facing;
    a.event = true;
    _actors.push_back(a);
    make_sprite(_actors.back());
    update_tint();
    refresh();
    return _actors.size() - 1;
}

void overworld::remove_actor(int index)
{
    _actors.erase(_actors.begin() + index);
    refresh();
}

namespace
{
    // npcOpenRun(): how many open tiles there are from (x, y) in a direction, up to n.
    int open_run(const overworld& o, int x, int y, direction d, int n)
    {
        int k = 0;
        while(k < n)
        {
            int nx = x + dx_of(d) * (k + 1), ny = y + dy_of(d) * (k + 1);
            if(! o.walkable(nx, ny) || (nx == state().x && ny == state().y))
            {
                break;
            }
            ++k;
        }
        return k;
    }
}

// sceneNpc(): someone placed up to n open tiles ahead of you (at least min), facing you; -1 if there's no room.
int overworld::scene_npc(person_kind kind, int n, int min)
{
    game_state& g = state();
    int room = open_run(*this, g.x, g.y, g.facing, n);
    if(room < min)
    {
        return -1;
    }
    return add_temp_actor(kind, g.x + dx_of(g.facing) * room, g.y + dy_of(g.facing) * room, opposite(g.facing));
}

// On ROUTE 1 without a POKéMON: the professor runs up, chased by a wild ZIGZAGOON.
void overworld::starter_event()
{
    game_state& g = state();
    set_player_frame(0);
    // A spot a few tiles east (two open tiles side by side), then as far back up the open ground as possible.
    int sxp = g.x + 2, syp = g.y;
    constexpr int offsets[][2] = { {3, 0}, {3, -1}, {3, 1}, {4, 0}, {2, 0} };
    for(const auto& o : offsets)
    {
        int x = g.x + o[0], y = g.y + o[1];
        if(walkable(x, y) && walkable(x + 1, y))
        {
            sxp = x;
            syp = y;
            break;
        }
    }
    int back = open_run(*this, sxp, syp, direction::RIGHT, 8);
    story_say("H-help me!");
    int prof = add_temp_actor(person_kind::prof, sxp + back, syp, direction::LEFT);
    // The ZIGZAGOON on his heels.
    const species& zig = game_data::species_list[int(species_id::ZIGZAGOON)];
    bn::sprite_ptr chaser = zig.front.create_sprite(0, 0);
    chaser.set_bg_priority(1);
    chaser.set_scale(bn::fixed(0.5));
    auto place_chaser = [&]()
    {
        const actor& p = _actors[prof];
        int ox = p.move_frames > 0 ? 16 * p.move_frames / p.step_length : 0;
        chaser.set_position(sx(p.x * 16 + ox + 16 + 8 - _cam_x), sy(p.y * 16 - 4 - _cam_y));
    };
    _actors[prof].fade = back ? -24 : 0;
    for(int i = 0; i < back; ++i)
    {
        actor& p = _actors[prof];
        p.x -= 1;
        p.facing = direction::LEFT;
        p.move_dir = direction::LEFT;
        p.step_length = run_frames;
        p.move_frames = run_frames;
        p.step_foot = ! p.step_foot;
        while(_actors[prof].move_frames > 0)
        {
            place_chaser();
            tick();
        }
    }
    place_chaser();
    g.facing = toward(g.x, g.y, _actors[prof].x, _actors[prof].y);
    set_player_frame(0);
    tick();
    bn::string<96> text(game_data::prof_name);
    text.append(": Hello! You over there! Please! Help me!");
    story_say(text);
    story_say("A wild ZIGZAGOON is after me! In my BAG! There's a POKé BALL in there!");
    chaser.set_visible(false);
    suspend();
    int pick = starter_bag(game_data::starter_trios[g.starter_trio], 3);
    resume();
    species_id chosen = game_data::starter_trios[g.starter_trio][pick];
    mon partner = mon::make(chosen, 5);
    if(roll_shiny())
    {
        partner.traits |= mon_trait::SHINY;
    }
    int slot;
    g.add_mon(partner, slot);
    g.mark_owned(partner.species_index);
    g.story |= story::STARTER | story::THANKS;
    text = g.name;
    text.append(" chose ");
    text.append(partner.name());
    text.append("!");
    story_say(text);
    save_game();
    // ...and straight into battle with the ZIGZAGOON (Lv2).
    fixed_battle(species_id::ZIGZAGOON, 2);
    _battle->scripted = true;
}

// Emerald's bag screen: three POKé BALLS; Left/Right to choose, A to look.
int overworld::starter_bag(const species_id* list, int count)
{
    ui& u = gui();
    int index = count / 2 - (count > 3);
    int result = 0;
    {
        bn::regular_bg_ptr bg = bn::regular_bg_items::bag_bg.create_bg(8, 48);
        bg.set_priority(3);
        // Three balls 40 px apart, or Calderra's six closer together.
        const int step = count > 3 ? 32 : 40, x0 = 120 - step * (count - 1) / 2;
        bn::vector<bn::sprite_ptr, 6> balls;
        for(int i = 0; i < count; ++i)
        {
            balls.push_back(bn::sprite_items::ball.create_sprite(sx(x0 + i * step), sy(64)));
            balls.back().set_bg_priority(2);
        }
        bn::optional<bn::sprite_ptr> shown;
        bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(0, 0);
        cursor.set_bg_priority(0);
        cursor.set_rotation_angle(270);
        ui::fade_in(8);
        while(true)
        {
            cursor.set_position(sx(x0 + index * step), sy(48));
            frame();
            if(bn::keypad::left_pressed())
            {
                index = (index + count - 1) % count;
                audio::play(audio::sfx::SELECT);
            }
            else if(bn::keypad::right_pressed())
            {
                index = (index + 1) % count;
                audio::play(audio::sfx::SELECT);
            }
            else if(bn::keypad::a_pressed())
            {
                audio::play(audio::sfx::SELECT);
                const species& s = game_data::species_list[int(list[index])];
                shown = s.front.create_sprite(sx(120), sy(112 - 32));
                shown->set_bg_priority(1);
                bn::string<80> text("Do you choose this POKéMON? The ");
                text.append(type_name(s.type1));
                text.append(" POKéMON ");
                text.append(s.name);
                text.append("!");
                u.show_text(text);
                bool yes = u.yes_no();
                u.clear_text();
                if(yes)
                {
                    result = index;
                    break;
                }
                shown.reset();
            }
        }
        ui::fade_out(8);
    }
    return result;
}

// PORT CALDER's professor (2.0.0): a Gen 8 or 9 starter for the journey through Calderra, grown up to the
// region's level; then a word about the beasts.
void overworld::calderra_prof()
{
    game_state& g = state();
    if(g.flags.test(flag::CALDERRA_STARTER))
    {
        say("PROF. EMBER: \"Three beasts, three branches, one crater. Choose any road you like!\"");
        say("PROF. EMBER: \"Your levels here follow your CALDERRA badges, not the road. Every road grows with you.\"");
        return;
    }
    say("PROF. EMBER: \"So you're the CHAMPION from VELLORIN! Welcome to CALDERRA.\"");
    say("PROF. EMBER: \"I study the legend of the ASHEN TOWER, and the three beasts who rose from its ashes.\"");
    say("PROF. EMBER: \"A new region deserves a new partner. Pick one from my BAG!\"");
    static constexpr species_id starters[] = { species_id::GROOKEY, species_id::SCORBUNNY, species_id::SOBBLE,
                                               species_id::SPRIGATITO, species_id::FUECOCO, species_id::QUAXLY };
    suspend();
    int pick = starter_bag(starters, 6);
    resume();
    mon partner = mon::make(starters[pick], calderra_level(g.region_badges(2)));
    if(roll_shiny())
    {
        partner.traits |= mon_trait::SHINY;
    }
    bn::string<64> text(g.name);
    text.append(" chose ");
    text.append(partner.name());
    text.append("!");
    say(text);
    while(partner.data().evolves_to >= 0 && partner.level >= partner.data().evolve_level)
    {
        partner.evolve_into(partner.data().evolves_to, gui());
    }
    int slot;
    if(! g.add_mon(partner, slot))
    {
        say("...But there's no room for it, even in the BOXES! Make some space and come back.");
        return;
    }
    g.mark_owned(partner.species_index);
    g.flags.set(flag::CALDERRA_STARTER);
    if(slot >= 0)
    {
        say("It was sent to the BOX.");
    }
    save_game();
    say("PROF. EMBER: \"Now, the beasts. TEAM ECLIPSE is after them. Please, get to the shrines first!\"");
}

// After the first battle, the professor thanks you and heads off.
void overworld::starter_thanks()
{
    game_state& g = state();
    g.story &= ~story::THANKS;
    int tx = g.x + 2, ty = g.y;
    if(blocked(tx, ty))
    {
        tx = g.x - 2;
    }
    int prof = add_temp_actor(person_kind::prof, tx, ty, toward(tx, ty, g.x, g.y));
    g.facing = toward(g.x, g.y, tx, ty);
    set_player_frame(0);
    tick();
    bn::string<128> text(game_data::prof_name);
    text.append(": Whew... I went into the tall grass to look at wild POKéMON, and it jumped me!");
    story_say(text);
    story_say("You saved me. Thanks a lot!");
    text = "That ";
    upper(text, g.party_count ? g.party[0].species_name() : "POKéMON");
    text.append(" seems to like you. Please, keep it!");
    story_say(text);
    story_say("Travel with it and fill up your POKéDEX. I'll be watching your progress!");
    // He runs off and fades out.
    direction d = tx >= g.x ? direction::RIGHT : direction::LEFT;
    int n = open_run(*this, tx, ty, d, 6);
    _actors[prof].fade = 24;
    walk_actor(prof, d, n, true);
    while(_actors[prof].fade > 0)
    {
        tick();
    }
    remove_actor(prof);
    save_game();
}

// A beaten rival says their piece and walks off (afterStory: walkOff).
void overworld::rival_leaves()
{
    game_state& g = state();
    int index = -1;
    for(int i = 0; i < _actors.size(); ++i)
    {
        if(_actors[i].tr && _actors[i].tr->id == g.walk_off)
        {
            index = i;
        }
    }
    g.walk_off = -1;
    if(index < 0)
    {
        return;
    }
    for(int i = 0; i < _actors[index].tr->after_count; ++i)
    {
        story_say(_actors[index].tr->after[i]);
    }
    actor& r = _actors[index];
    direction d = r.x > g.x ? direction::RIGHT : r.x < g.x ? direction::LEFT : r.y > g.y ? direction::DOWN : direction::UP;
    int n = open_run(*this, r.x, r.y, d, 3);
    _actors[index].fade = 24 + n * walk_frames;
    walk_actor(index, d, n);
    while(_actors[index].fade > 0)
    {
        tick();
    }
    remove_actor(index);
    save_game();
}

// storyEnter(): on arriving in an area, its scene plays once; the professor calls after 1, 2 and 4 badges.
void overworld::story_enter()
{
    game_state& g = state();
    if(_map->is_room() || ! g.has(story::STARTER))
    {
        return;
    }
    if(play_scene())
    {
        save_game();
        return;
    }
    constexpr uint32_t flags[] = { story::CALL1, story::CALL2, story::CALL3 };
    for(int k = 0; k < 3; ++k)
    {
        if(g.badges() >= game_data::prof_call_badges[k] && ! g.has(flags[k]))
        {
            g.story |= flags[k];
            save_game();
            audio::play(audio::sfx::OPEN);
            bn::string<80> text("Beep beep beep! Incoming call from ");
            text.append(game_data::prof_name);
            text.append("...");
            story_say(text);
            for(int i = 0; i < game_data::prof_call_lines[k]; ++i)
            {
                story_say(game_data::prof_calls[k][i]);
            }
            return;
        }
    }
}

// SCENES: Marrow Pass (a grunt runs by), Portmere (Vesper and a grunt), the hideout, the shrine.
bool overworld::play_scene()
{
    game_state& g = state();
    const area_info* a = _map->area;
    if(! a || ! a->scene[0])
    {
        return false;
    }
    bn::string_view scene(a->scene);
    if(scene == "tempestRun" && ! g.has(story::SCENE_TEMPEST_RUN))
    {
        g.story |= story::SCENE_TEMPEST_RUN;
        int grunt = scene_npc(person_kind::grunt, 5, 3);
        if(grunt < 0)
        {
            story_say("Somewhere up the pass, someone shouts: \"Move it! TEAM TEMPEST has business on the coast!\"");
            return true;
        }
        // It runs at you (fading in), shouts, and runs back the way it came (fading out).
        actor& gr = _actors[grunt];
        direction d = gr.facing;
        int dist = bn::abs(gr.x - g.x) + bn::abs(gr.y - g.y) - 1;
        gr.fade = -24;
        walk_actor(grunt, d, dist, true);
        story_say("TEMPEST GRUNT: Hey! Outta the way, kid!");
        story_say("TEMPEST GRUNT: TEAM TEMPEST has business on the coast. Don't you dare follow me!");
        direction back = opposite(d);
        int n = open_run(*this, _actors[grunt].x, _actors[grunt].y, back, 6);
        _actors[grunt].fade = 24 + n * run_frames;
        walk_actor(grunt, back, n, true);
        while(_actors[grunt].fade > 0)
        {
            tick();
        }
        remove_actor(grunt);
        return true;
    }
    if(scene == "portmere" && ! g.has(story::SCENE_PORTMERE))
    {
        int admin = scene_npc(person_kind::admin, 5, 3);
        if(admin < 0)
        {
            return false;       // no room here: it plays on a later visit
        }
        int dist = bn::abs(_actors[admin].x - g.x) + bn::abs(_actors[admin].y - g.y) - 2;
        int grunt = scene_npc(person_kind::grunt, dist, 1);
        if(grunt < 0)
        {
            remove_actor(admin);
            return false;
        }
        g.story |= story::SCENE_PORTMERE;
        for(int i = 0; i < _map->trainers_count; ++i)
        {
            if(_map->trainers[i].scene)
            {
                _actors[grunt].tr = &_map->trainers[i];
            }
        }
        // The Admin is talking to the grunt, back turned to you.
        _actors[admin].facing = opposite(_actors[admin].facing);
        _actors[admin].sprite->set_tiles(person_sprites[int(person_kind::admin)]->tiles_item(), _actors[admin].facing == direction::DOWN ? 0 :
                                         _actors[admin].facing == direction::UP ? 3 : 6);
        _actors[admin].sprite->set_horizontal_flip(_actors[admin].facing == direction::RIGHT);
        tick();
        story_say("ADMIN VESPER: The TIDEWARDENS' shrine lies somewhere beneath these very waves.");
        story_say("ADMIN VESPER: Once we can reach it, the guardian of the sea and sky will answer to TEAM TEMPEST!");
        story_say("TEMPEST GRUNT: Storms for the skies! Tempest rises!");
        _actors[admin].facing = toward(_actors[admin].x, _actors[admin].y, g.x, g.y);
        _actors[admin].sprite->set_tiles(person_sprites[int(person_kind::admin)]->tiles_item(), _actors[admin].facing == direction::DOWN ? 0 :
                                         _actors[admin].facing == direction::UP ? 3 : 6);
        _actors[admin].sprite->set_horizontal_flip(_actors[admin].facing == direction::RIGHT);
        tick();
        story_say("ADMIN VESPER: ...A child? You've been trailing my grunts since MARROW PASS, haven't you?");
        story_say("ADMIN VESPER: How tiresome. Grunt, make sure this one stays on dry land.");
        direction d = g.facing;
        int n = open_run(*this, _actors[admin].x, _actors[admin].y, d, 6);
        _actors[admin].fade = 24 + n * walk_frames;
        walk_actor(admin, d, n);
        while(_actors[admin].fade > 0)
        {
            tick();
        }
        remove_actor(admin);
        for(int i = 0; i < _actors.size(); ++i)
        {
            if(_actors[i].tr && _actors[i].tr->scene && _actors[i].active_trainer())
            {
                trainer_approach(i);
                break;
            }
        }
        return true;
    }
    if(scene == "hideout" && ! g.has(story::SCENE_HIDEOUT))
    {
        g.story |= story::SCENE_HIDEOUT;
        story_say("Voices echo from deeper in the cave...");
        story_say("ADMIN VESPER: \"The TIDEWARDEN songs are nearly decoded. Soon the guardian wakes, and the storms answer to TEAM TEMPEST!\"");
        story_say("ADMIN VESPER: \"WREN. Our little shadow has followed us here. Prove you're one of us.\"");
        return true;
    }
    if(scene == "shrine" && ! g.has(story::SCENE_SHRINE))
    {
        g.story |= story::SCENE_SHRINE;
        story_say("WREN: \"Wait up! I followed TEMPEST's divers all the way down here.\"");
        story_say("WREN: \"Here, let me patch up your team first.\"");
        g.heal_party();
        audio::play(audio::sfx::HEAL);
        story_say("Your POKéMON were fully healed!");
        // GBA only: WREN's MASTER BALL, for the guardian.
        story_say("WREN: \"And take this. My mom gave it to me for something special... I think this is it.\"");
        g.story |= story::MASTER_GIFT;
        obtain(item_id::MASTERBALL, 1);
        story_say("WREN: \"A MASTER BALL never misses. Save it for the guardian!\"");
        story_say("WREN: \"I'll hold off the grunts behind us. Go stop VESPER!\"");
        return true;
    }
    return false;
}

// "X wants to learn Y. However, X already knows four moves. Should a move be forgotten...?" (movePromptNext)
void overworld::move_prompts()
{
    ui& u = gui();
    pending_move p;
    while(take_pending_move(p))
    {
        mon& m = *p.m;
        bool known = false;
        for(int k = 0; k < m.move_count; ++k)
        {
            known |= m.move(k) == p.move;
        }
        if(known || m.empty())
        {
            continue;
        }
        bn::string<24> who;
        upper(who, m.name());
        const char* mvn = move_data(p.move).name;
        bn::string<96> text(who);
        text.append(" wants to learn the move ");
        text.append(mvn);
        text.append(".");
        say(text);
        text = "However, ";
        text.append(who);
        text.append(" already knows four moves.");
        say(text);
        text = "Should a move be forgotten to make space for ";
        text.append(mvn);
        text.append("?");
        u.show_text(text);
        bool yes = u.yes_no();
        u.clear_text();
        int forget = -1;
        if(yes)
        {
            u.show_text("Which move should be forgotten?");
            bn::string_view views[5];
            for(int k = 0; k < m.move_count; ++k)
            {
                views[k] = move_data(m.move(k)).name;
            }
            views[m.move_count] = "DON'T LEARN";
            forget = u.list(views, m.move_count + 1);
            u.clear_text();
            if(forget >= m.move_count)
            {
                forget = -1;
            }
        }
        if(forget < 0)
        {
            text = who;
            text.append(" did not learn ");
            text.append(mvn);
            text.append(".");
            say(text);
            continue;
        }
        const char* old = move_data(m.move(forget)).name;
        m.set_move(forget, p.move);
        say("1, 2, and... ... Poof!");
        text = who;
        text.append(" forgot ");
        text.append(old);
        text.append(".");
        say(text);
        text = "And... ";
        text.append(who);
        text.append(" learned ");
        text.append(mvn);
        text.append("!");
        say(text);
    }
}

// After a first catch: the new POKéDEX entry; then "Give a nickname to the caught X?" for each (nickNext).
void overworld::dex_registration()
{
    if(! _last)
    {
        return;
    }
    for(uint16_t s : _last->dex_new)
    {
        suspend();
        dex_screen(s);
        resume();
    }
}

void overworld::nickname_prompts()
{
    if(! _last)
    {
        return;
    }
    game_state& g = state();
    ui& u = gui();
    for(int16_t where : _last->caught_party)
    {
        mon& m = where >= 100 ? g.box[where - 100] : g.party[where];
        if(m.empty())
        {
            continue;
        }
        bool must = g.run.nuzlocke();     // NUZLOCKE: every catch is nicknamed
        bn::string<64> text;
        if(must)
        {
            text = "NUZLOCKE: give the caught ";
            upper(text, m.species_name());
            text.append(" a nickname.");
            say(text);
        }
        else
        {
            text = "Give a nickname to the caught ";
            upper(text, m.species_name());
            text.append("?");
            u.show_text(text);
            bool yes = u.yes_no();
            u.clear_text();
            if(! yes)
            {
                continue;
            }
        }
        suspend();
        bn::string<32> title;
        upper(title, m.species_name());
        title.append("'s nickname?");
        char nick[nick_length + 1];
        while(true)
        {
            bool ok = u.keyboard(title, nick, nick_length, must ? "" : m.species_name()) && nick[0];
            if(ok)
            {
                for(int i = 0; i <= nick_length; ++i)
                {
                    m.nick[i] = nick[i];
                }
            }
            if(ok || ! must)
            {
                break;
            }
        }
        resume();
        save_game();
    }
}

// A beaten trainer walks back along the way they came, so they never block a path (trainerWalkBack).
void overworld::trainer_walk_back()
{
    walk_back& w = pending_walk_back();
    if(w.map != _map_index || w.trainer_index < 0)
    {
        w.map = -1;
        return;
    }
    int index = -1;
    for(int i = 0; i < _actors.size(); ++i)
    {
        if(_actors[i].tr == &_map->trainers[w.trainer_index])
        {
            index = i;
        }
    }
    w.map = -1;
    if(index < 0 || _actors[index].active_trainer())
    {
        if(index >= 0)
        {
            _actors[index].x = _actors[index].home_x;
            _actors[index].y = _actors[index].home_y;
        }
        return;
    }
    for(int k = w.trail.size() - 1; k >= 0; --k)
    {
        walk_actor(index, opposite(direction(w.trail[k])), 1);
    }
    actor& a = _actors[index];
    a.facing = a.tr->facing;
    a.sprite->set_tiles(person_sprites[int(a.kind)]->tiles_item(), a.facing == direction::DOWN ? 0 : a.facing == direction::UP ? 3 : 6);
    a.sprite->set_horizontal_flip(a.facing == direction::RIGHT);
}

// afterStory(): what comes after a battle, in the web game's order.
void overworld::after_story()
{
    game_state& g = state();
    if(new_game_started)
    {
        // introFinish(): where the professor went, and which way ROUTE 1 is (routeOneWay).
        new_game_started = false;
        just_loaded = false;
        bn::string<160> text(game_data::prof_name);
        text.append(" went out toward ROUTE 1 to study wild POKéMON.");
        story_say(text);
        const char* way = "Follow the road to the right!";
        const char* compass = "east";
        for(int i = 0; i < _map->links_count; ++i)
        {
            if(_map->links[i].target == 1)
            {
                const link& l = _map->links[i];
                if(l.ox < 0) { compass = "west"; way = "Follow the road to the left!"; }
                else if(l.oy < 0 && l.ox < _map->w) { compass = "north"; way = "Follow the road up!"; }
                else if(l.oy >= _map->h) { compass = "south"; way = "Follow the road down!"; }
            }
        }
        text = "Maybe you should go and find him! ROUTE 1 is ";
        text.append(compass);
        text.append(" of town. ");
        text.append(way);
        story_say(text);
        return;
    }
    if(just_loaded)
    {
        // CONTINUE (startAdventure): the starter event, else the area's story.
        just_loaded = false;
        pending_walk_back().map = -1;
        if(! g.has(story::STARTER) && _map_index == 1)
        {
            starter_event();
        }
        else
        {
            story_enter();
        }
        return;
    }
    if(! _last)
    {
        return;
    }
    move_prompts();
    trainer_walk_back();
    // The League beaten for the first time: the Hall of Fame and the credits.
    for(int i : wd::area_maps)
    {
        const map_def& m = wd::maps[i];
        if(m.area->region == 1 && (m.area->flags & area_flag::CHAMPION) && m.leader_id >= 0 && g.beaten.test(m.leader_id) && ! g.has(story::CHAMPION))
        {
            credits();
            return;
        }
    }
    // A later region's League beaten for the first time (Calderra's 2.0.0, ...): its Hall of Fame and the CHAMPION's
    // farewell.
    struct farewell { const char* champion; const char* line1; const char* line2; };
    static constexpr farewell farewells[] = {
        { "SOLENNE", "Calderra has a new CHAMPION. The beasts knew it before any of us did.",
          "The crater is yours to roam. And the GYM LEADERS are already asking for a rematch!" },
        { "MAREA", "The tide turns, and so does the crown. The SUNDERED ISLES have a new CHAMPION.",
          "Groudon sleeps, Kyogre sleeps, and the islands stay islands. Sail where you like, CHAMPION!" },
        { "ALTAIR", "The wind changes, and so does the crown. The SKYREACH has a new CHAMPION.",
          "Rayquaza guards the sky again, and the thing from space found a friend. Fly where you like!" },
        { "CASSIA", "Every experiment in this city was meant to make something perfect. GENOVA got a CHAMPION instead.",
          "MEWTWO is free, and nobody will build another. Go wherever you like, CHAMPION!" },
        { "AEON", "Past and present agree on one thing now: AETERNA has a new CHAMPION.",
          "Time flows and space holds. The valley will remember you in both eras." },
        { "ELEGY", "Even in the HOLLOW LANDS, some things come back. Hope, for one. You're our CHAMPION.",
          "GIRATINA watches the tear, and the grey is lifting. Walk where you like, on either side." },
        { "ZEPHYRA", "Winter, storm and summer, all turning again. TEMPESTA crowns a new CHAMPION!",
          "The birds fly free. Come back every season: the festival will save you a seat." },
        { "KAI", "Every region, every League, and now the Hall of Origin. You're the CHAMPION of all nine lands.",
          "ARCEUS made the lands, and you crossed every one. Thanks for the race, CHAMPION." },
    };
    for(int i : wd::area_maps)
    {
        const map_def& m = wd::maps[i];
        int rg = m.area->region, f = champion_flag(rg);
        if(rg >= 2 && (m.area->flags & area_flag::CHAMPION) && m.leader_id >= 0 && g.beaten.test(m.leader_id) && ! g.flags.test(f))
        {
            g.flags.set(f);
            save_game();
            suspend();
            audio::play_music("credits");
            hall_of_fame_screen(region_name(rg));
            resume();
            const farewell& fw = farewells[rg - 2];
            for(const char* line : { fw.line1, fw.line2 })
            {
                bn::string<160> text(fw.champion);
                text.append(": \"");
                text.append(line);
                text.append("\"");
                story_say(text);
            }
        }
    }
    if(g.has(story::DIVE_GIFT))
    {
        g.story &= ~story::DIVE_GIFT;
        g.add_item(item_id::HM08, 1);
        save_game();
        story_say("HALE: \"The ice keeps old secrets, and so does the sea. Take this.\"");
        obtain_named("HM08 DIVE", 1, "TMs & HMs");
        story_say("HALE: \"Look for dark, deep water while you SURF, and press A there to DIVE.\"");
        story_say("HALE: \"They say the TIDEWARDENS' shrine lies somewhere beneath the GLIMMER SEA.\"");
    }
    if(g.has(story::LEGEND_FIGHT))
    {
        g.story &= ~story::LEGEND_FIGHT;
        bool got = false;
        species_id legend = _map->area && _map->area->legend_x >= 0 ? _map->area->legend : species_id::LUGIA;
        for(int i = 0; i < g.party_count; ++i)
        {
            got |= g.party[i].species_index == uint16_t(legend);
        }
        for(const mon& m : g.box)
        {
            got |= ! m.empty() && m.species_index == uint16_t(legend);
        }
        if(legend_flag(legend) >= 0)
        {
            // Calderra's beasts and HO-OH.
            if(got)
            {
                g.flags.set(legend_flag(legend));
            }
            save_game();
            bn::string<64> text;
            upper(text, game_data::species_list[int(legend)].name);
            if(got)
            {
                load_actors();
                refresh(true);
                text.append(" joined your team!");
                say(text);
            }
            else
            {
                // What it does when it gets away, region by region (Calderra's HO-OH first).
                constexpr const char* fled[][2] = {
                    { " fled into the crater's wilds...", "Maybe it will return to its shrine." },
                    { " sank back into its slumber...", "It still sleeps here. Come back and try again." },
                    { " rose back into the sky...", "It will come back here. Try again." },
                    { " vanished in a flash of psychic light...", "It hasn't left. Come back and try again." },
                    { " slipped through a rift in time...", "The rift is still open. Come back and try again." },
                    { " sank into the shadows...", "It still watches from the other side. Try again." },
                    { " flew back to its peak...", "It will roost here again. Try again." },
                    { " withdrew into the light...", "The HALL OF ORIGIN waits. Try again." } };
                const auto& f = fled[bn::max(0, bn::min(7, map_region(_map_index) - 2))];
                text.append(f[0]);
                say(text);
                say(f[1]);
            }
        }
        else
        {
            bool first = ! g.has(story::STORM_ENDED);
            if(got)
            {
                g.story |= story::LEGEND_CAUGHT;
            }
            g.story |= story::STORM_ENDED;
            save_game();
            if(got)
            {
                load_actors();
                update_tint();
                refresh(true);
                say("LUGIA, guardian of the sea and sky, joined your team!");
            }
            else
            {
                say("LUGIA sank back into the depths of the shrine...");
                say("Maybe it will rise again if you return.");
            }
            if(first)
            {
                story_say("Far above, the storm clouds over VELLORIN begin to break apart.");
                story_say("WREN: \"You did it... The storms are clearing!\"");
                story_say("WREN: \"VESPER's gone, and TEAM TEMPEST with her. See you at the top!\"");
            }
        }
    }
    if(g.has(story::SURF_GIFT))
    {
        g.story &= ~story::SURF_GIFT;
        g.add_item(item_id::HM03, 1);
        save_game();
        story_say("SABLE: \"The sea chose well today. Take this as well.\"");
        obtain_named("HM03 SURF", 1, "TMs & HMs");
        story_say("SABLE: \"With SURF, a POKéMON can carry you across the water.\"");
        story_say("SABLE: \"Face the water and press A. The sea is wider than you think!\"");
    }
    if(g.walk_off >= 0)
    {
        rival_leaves();
    }
    if(g.has(story::THANKS) && _map_index == 1)
    {
        starter_thanks();
    }
    dex_registration();
    nickname_prompts();
    save_game();
    tower_after_battle();
}

// The Hall of Fame, the credits, then home (playCredits).
void overworld::credits()
{
    game_state& g = state();
    g.story |= story::CHAMPION;
    save_game();
    suspend();
    credits_screen();
    // Home: outside your house, everyone healed.
    const map_def& home = wd::maps[0];
    g.x = home.spawn_x;
    g.y = home.spawn_y;
    for(int i = 0; i < home.doors_count; ++i)
    {
        const map_def& room = wd::maps[home.doors[i].room];
        if(room.room && room.room->home)
        {
            g.x = home.doors[i].x;
            g.y = int16_t(home.doors[i].y + 1);
        }
    }
    g.surfing = false;
    g.facing = direction::DOWN;
    g.heal_party();
    save_game();
    _suspended = false;
    _player.reset();
    load_map(0);
    ui::fade_in(16);
    story_say("Back home in DUSKMERE HOLLOW...");
    bn::string<96> text("MOM: \"");
    text.append(g.name);
    text.append("! The CHAMPION! I'm so proud of you!\"");
    story_say(text);
    adventure_begins();
}

// The first clear (Phase 7): the cartridge's unlock (NEW ADVENTURE MODE on the title) and, outside a
// NUZLOCKE run, this save carries on as ADVENTURE MODE with the CHALLENGE TOWER at the POKéMON LEAGUE.
void overworld::adventure_begins()
{
    game_state& g = state();
    set_device_cleared();
    if(g.run.nuzlocke())
    {
        say("Your NUZLOCKE run is complete. Congratulations, CHAMPION!");
        bn::string<64> text("POKéMON lost along the way: ");
        text.append(bn::to_string<6>(g.run.deaths));
        say(text);
        say("NEW ADVENTURE MODE is now on the title screen.");
        save_game();
        return;
    }
    g.run.adventure = true;
    save_game();
    say("Your adventure continues in ADVENTURE MODE!");
    say("The road south of DUSKMERE HOLLOW is open: SPIRECREST TOWN and its CHALLENGE TOWER await you!");
    say("NEW ADVENTURE MODE is now on the title screen, too.");
}

// The CHALLENGE TOWER's guide, by its door: what the tower is, your rank, best streak and the legendaries
// still in the stone.
void overworld::tower_guide()
{
    game_state& g = state();
    int left = 0;
    for(int i = 0; i < game_data::legendaries_count; ++i)
    {
        left += ! g.owned.test(game_data::legendaries[i]);
    }
    say("Welcome to SPIRECREST TOWN, CHAMPION. Behind me stands the CHALLENGE TOWER.");
    say("Five floors, five themes, one trainer on each. No healing inside but what you carry.");
    say("Reach the summit and touch the SUMMONING STONE. A legendary POKéMON will answer!");
    bn::string<96> text("RANK ");
    text.append(bn::to_string<4>(g.run.tower_clears + 1));
    text.append("   BEST STREAK ");
    text.append(bn::to_string<4>(g.run.tower_best));
    text.append("   LEGENDARIES LEFT ");
    text.append(bn::to_string<4>(left));
    say(text);
    if(g.run.nuzlocke())
    {
        say("...But the tower doesn't take NUZLOCKE challengers, I'm afraid.");
    }
}


// The ferry (2.0.0): PORTMERE HARBOUR to PORT CALDER for CHAMPIONS (ADVENTURE MODE), and back. It lands you
// at the other port's SAILOR.
void overworld::ferry()
{
    game_state& g = state();
    ui& u = gui();
    bool home = map_region(_map_index) == 1;
    if(home && ! g.run.adventure)
    {
        say("SAILOR: \"The ferry to CALDERRA only takes CHAMPIONS. Beat the POKéMON LEAGUE and come back!\"");
        return;
    }
    u.show_text(home ? "SAILOR: \"All aboard for CALDERRA, land of the great crater! Set sail?\""
                     : "SAILOR: \"Heading home? We sail for PORTMERE HARBOUR in VELLORIN. Set sail?\"");
    bool yes = u.yes_no();
    u.clear_text();
    if(! yes)
    {
        return;
    }
    for(int i : wd::area_maps)
    {
        const map_def& m = wd::maps[i];
        if(m.area->region == (home ? 2 : 1))
        {
            for(int k = 0; k < m.people_count; ++k)
            {
                if(m.people[k].role == person_role::FERRY)
                {
                    fast_travel(i);
                    return;
                }
            }
        }
    }
}

// The ship captain (3.0.0): PORT CALDER to PORT KEEL in the SUNDERED ISLES for CALDERRA's CHAMPION, and back.
// It lands you at the other port's CAPTAIN.
void overworld::captain()
{
    game_state& g = state();
    ui& u = gui();
    bool home = map_region(_map_index) == 2;
    if(home && ! g.flags.test(flag::CALDERRA_CHAMPION))
    {
        say("CAPTAIN: \"The SUNDERED ISLES are no place for greenhorns. Beat the CALDERRA LEAGUE, then we'll talk.\"");
        return;
    }
    u.show_text(home ? "CAPTAIN: \"Yarr, CHAMPION! Fancy a voyage to the SUNDERED ISLES, where the land broke apart? Set sail?\""
                     : "CAPTAIN: \"Back to PORT CALDER, then? Set sail?\"");
    bool yes = u.yes_no();
    u.clear_text();
    if(! yes)
    {
        return;
    }
    for(int i : wd::area_maps)
    {
        const map_def& m = wd::maps[i];
        if(m.area->region == (home ? 3 : 2))
        {
            for(int k = 0; k < m.people_count; ++k)
            {
                if(m.people[k].role == person_role::CAPTAIN)
                {
                    fast_travel(i);
                    return;
                }
            }
        }
    }
}

// The airship's PILOT (4.0.0): PORT KEEL to WINDWARD in the SKYREACH for the SUNDERED ISLES' CHAMPION, and back.
// It lands you at the other end's PILOT.
void overworld::pilot()
{
    game_state& g = state();
    ui& u = gui();
    bool home = map_region(_map_index) == 3;
    if(home && ! g.flags.test(flag::SUNDERED_CHAMPION))
    {
        say("PILOT: \"The SKYREACH is a long way up, and the wind there is no joke. Beat the SUNDERED LEAGUE first.\"");
        return;
    }
    u.show_text(home ? "PILOT: \"Hey, CHAMPION! My airship flies up to the SKYREACH, land of mesas and sky. Take off?\""
                     : "PILOT: \"Heading back down to PORT KEEL? Take off?\"");
    bool yes = u.yes_no();
    u.clear_text();
    if(! yes)
    {
        return;
    }
    for(int i : wd::area_maps)
    {
        const map_def& m = wd::maps[i];
        if(m.area->region == (home ? 4 : 3))
        {
            for(int k = 0; k < m.people_count; ++k)
            {
                if(m.people[k].role == person_role::PILOT)
                {
                    fast_travel(i);
                    return;
                }
            }
        }
    }
}

// A meteorite shard (4.0.0): DEOXYS in your party takes this shard's form (0 Normal, 1 Attack, 2 Defense, 3 Speed).
void overworld::meteor_shard(int form)
{
    game_state& g = state();
    constexpr species_id forms[] = { species_id::DEOXYS, species_id::DEOXYS_ATTACK, species_id::DEOXYS_DEFENSE,
                                     species_id::DEOXYS_SPEED };
    bool any = false;
    for(int i = 0; i < g.party_count; ++i)
    {
        mon& m = g.party[i];
        bool deoxys = false;
        for(species_id f : forms)
        {
            deoxys |= m.species_index == uint16_t(f);
        }
        if(! deoxys || m.species_index == uint16_t(forms[form]))
        {
            continue;
        }
        if(! any)
        {
            say("The meteorite shard glows. Something in your party answers it...");
        }
        any = true;
        m.set_species(int(forms[form]));
        g.mark_owned(int(forms[form]));
        bn::string<64> text(m.name());
        text.append(" changed its form!");
        say(text);
    }
    if(any)
    {
        save_game();
    }
}

// 5.0.0 on: the guide who takes the last region's CHAMPION on to the next region (GUIDE_ON), and back (GUIDE_BACK).
// It lands you by the guide at the other end.
void overworld::region_guide(bool on)
{
    game_state& g = state();
    ui& u = gui();
    struct guide { const char* who; const char* on_ask; const char* back_ask; const char* wait; };
    static constexpr guide guides[] = {
        { "SHUTTLE PILOT", "The SYNTHESIS CORP. shuttle flies down to GENOVA, the city of labs. Board?",
          "Back up to WINDWARD in the SKYREACH? Board?", "GENOVA only lets CHAMPIONS through its gates. Beat the SKYREACH LEAGUE first." },
        { "CONDUCTOR", "The old line runs out past the city walls to AETERNA, a valley where time runs strange. All aboard?",
          "Back to GENOVA CENTRAL? All aboard?", "The valley line only runs for GENOVA's CHAMPION. Beat the GENOVA LEAGUE first." },
        { "FERRYMAN", "Across the grey lake lie the HOLLOW LANDS. Not many come back the same. Shall we cross?",
          "Back across to AETERNA? Shall we cross?", "The lake won't carry you yet. Beat the AETERNA LEAGUE first." },
        { "BALLOONIST", "My balloon rides the winds to TEMPESTA, the land of seasons. Up we go?",
          "Back down to the HOLLOW LANDS? Up we go?", "The winds to TEMPESTA are too wild for you yet. Beat the HOLLOW LEAGUE first." },
        { "MOUNTAIN GUIDE", "The mountain all nine lands surround... the HALL OF ORIGIN is at its peak. Shall we climb?",
          "Back down to TEMPESTA? Shall we go?", "Only the CHAMPION of TEMPESTA may climb. Beat the TEMPESTA LEAGUE first." } };
    int here = map_region(_map_index), dest = on ? here + 1 : here - 1;
    const guide& gd = guides[bn::max(0, bn::min(4, (on ? dest : here) - 5))];
    bn::string<160> text(gd.who);
    text.append(": \"");
    if(on && ! g.flags.test(champion_flag(here)))
    {
        text.append(gd.wait);
        text.append("\"");
        say(text);
        return;
    }
    text.append(on ? gd.on_ask : gd.back_ask);
    text.append("\"");
    u.show_text(text);
    bool yes = u.yes_no();
    u.clear_text();
    if(! yes)
    {
        return;
    }
    for(int i : wd::area_maps)
    {
        const map_def& m = wd::maps[i];
        if(m.area->region == dest)
        {
            for(int k = 0; k < m.people_count; ++k)
            {
                if(m.people[k].role == (on ? person_role::GUIDE_BACK : person_role::GUIDE_ON))
                {
                    fast_travel(i);
                    return;
                }
            }
        }
    }
}

// 6.0.0: a rift: through to the same spot in the area's twin (mirrored on the HOLLOW LANDS' reverse side).
void overworld::twin_rift()
{
    const area_info* a = _map->area;
    if(! a || a->twin < 0)
    {
        return;
    }
    say(a->flags & area_flag::MIRROR ? "Your reflection in the rift reaches out a hand... You take it." :
        "The air ripples like water. You step through...");
    game_state& g = state();
    fast_travel(a->twin, a->flags & area_flag::MIRROR ? wd::maps[a->twin].w - 1 - g.x : g.x, g.y);
}

// 6.0.0: AETERNA's seed: picked up and planted on the bank in the past, a tree in the present with the MIND
// PLATE in its roots.
void overworld::ancient_seed(bool tree)
{
    game_state& g = state();
    if(! tree)
    {
        say("A seed from the ancient tree glints on the riverbank.");
        say("You press it deep into the soft earth. Something tells you it will grow for a long, long time.");
        g.flags.set(flag::ANCIENT_SEED);
    }
    else if(! g.flags.test(flag::TREE_GIFT))
    {
        say("A huge, ancient tree stands on the bank. It grew from the seed you planted, long ago!");
        say("Something glints between its roots...");
        g.add_item(item_id::MINDPLATE, 1);
        say("You found the MIND PLATE!");
        g.flags.set(flag::TREE_GIFT);
    }
    else
    {
        say("The tree you planted, long ago. Its leaves rustle as if it remembers you.");
        return;
    }
    save_game();
    load_actors();
    refresh(true);
}

// 6.0.0 on: an altar that changes a legend's form: DIALGA's and PALKIA's Origin forms, GIRATINA's.
void overworld::form_altar()
{
    game_state& g = state();
    constexpr species_id pairs[][2] = { { species_id::DIALGA, species_id::DIALGA_ORIGIN },
                                        { species_id::PALKIA, species_id::PALKIA_ORIGIN },
                                        { species_id::GIRATINA, species_id::GIRATINA_ORIGIN } };
    bool any = false;
    for(int i = 0; i < g.party_count; ++i)
    {
        mon& m = g.party[i];
        for(const auto& pr : pairs)
        {
            int to = m.species_index == uint16_t(pr[0]) ? int(pr[1]) : m.species_index == uint16_t(pr[1]) ? int(pr[0]) : -1;
            if(to < 0)
            {
                continue;
            }
            if(! any)
            {
                say("The altar flares. Something in your party answers it...");
            }
            any = true;
            m.set_species(to);
            g.mark_owned(to);
            bn::string<64> text(m.name());
            text.append(" changed its form!");
            say(text);
            break;
        }
    }
    if(any)
    {
        save_game();
    }
}

// 5.0.0: a pink blur in one of GENOVA's ten places: MEW, there and gone. Seeing all ten wins its trust.
void overworld::mew_sighting(int person)
{
    game_state& g = state();
    int k = mew_sighting_index(_map_index, person);
    say("A pink blur darts past you... and it's gone!");
    if(k < 0 || g.flags.test(flag::MEW_SIGHTING + k))
    {
        return;
    }
    g.flags.set(flag::MEW_SIGHTING + k);
    save_game();
    load_actors();
    refresh(true);
    int seen = mew_sightings_seen();
    bn::string<96> text("MEW sightings: ");
    text.append(bn::to_string<4>(seen));
    text.append(" of ");
    text.append(bn::to_string<4>(flag::mew_sightings));
    text.append(".");
    say(text);
    if(seen == flag::mew_sightings)
    {
        say("Somewhere in GENOVA, something pink is waiting for you. Maybe by the water...");
    }
}

void overworld::warp_to_room(int map_index)
{
    game_state& g = state();
    audio::play(audio::sfx::DOOR);
    ui::fade_out(12);
    const map_def& room = wd::maps[map_index];
    g.x = room.spawn_x;
    g.y = room.spawn_y;
    g.facing = direction::UP;
    _player.reset();
    load_map(map_index);
    ui::fade_in(12);
    _fresh_press = true;
    save_game();
}

// The tower's door: in ADVENTURE MODE, healed, for a fresh challenge.
bool overworld::tower_door(const door&)
{
    game_state& g = state();
    ui& u = gui();
    set_player_frame(0);
    if(g.run.nuzlocke() || ! g.run.adventure)
    {
        say(g.run.nuzlocke() ? "The CHALLENGE TOWER's doors stay shut to NUZLOCKE challengers."
                             : "The CHALLENGE TOWER's doors are shut.");
        hold_until_released();
        return false;
    }
    if(! g.able_count())
    {
        say("Your POKéMON can't battle! Rest them at the POKéMON CENTER first.");
        hold_until_released();
        return false;
    }
    u.show_text("Enter the CHALLENGE TOWER? Your POKéMON will be healed.");
    bool yes = u.yes_no();
    u.clear_text();
    if(! yes)
    {
        hold_until_released();
        return false;
    }
    g.heal_party();
    audio::play(audio::sfx::HEAL);
    reset_tower_trainers();
    g.tower = tower_state();
    g.tower.active = true;
    return true;
}

// The challenge ends (left early, or done): the floors reset, and a challenge left early ends the streak.
void overworld::tower_end(bool keep_streak)
{
    game_state& g = state();
    g.tower = tower_state();
    reset_tower_trainers();
    if(! keep_streak)
    {
        g.run.tower_streak = 0;
    }
    save_game();
}

// The mat on a tower floor: leaving mid-challenge ends it. False keeps you inside.
bool overworld::tower_leave()
{
    game_state& g = state();
    const room_info* r = _map->room;
    bool tower = r && (r->kind == room_kind::TOWER || r->kind == room_kind::SUMMIT || r->kind == room_kind::CHAMBER);
    if(! tower || ! g.tower.active)
    {
        return true;
    }
    ui& u = gui();
    set_player_frame(0);
    u.show_text("Leave the CHALLENGE TOWER? Your challenge will end.");
    bool yes = u.yes_no(false);
    u.clear_text();
    if(yes)
    {
        tower_end(false);
        return true;
    }
    // Back off the mat.
    g.y = int16_t(g.y - 1);
    g.facing = direction::UP;
    _player.reset();
    load_map(_map_index);
    hold_until_released();
    return false;
}

// "Up you go": the next floor, or the summit after the TOWER MASTER.
void overworld::tower_offer_up(int floor)
{
    game_state& g = state();
    ui& u = gui();
    if(! g.tower.active)
    {
        return;
    }
    bool summit = floor >= 4;
    bn::string<64> text(summit ? "Climb to the SUMMIT?" : "Go up to FLOOR ");
    if(! summit)
    {
        text.append(bn::to_string<4>(floor + 2));
        text.append("?");
    }
    u.show_text(text);
    bool yes = u.yes_no();
    u.clear_text();
    if(! yes)
    {
        return;
    }
    int next = summit ? tower_room(room_kind::SUMMIT) : tower_room(room_kind::TOWER, floor + 1);
    if(next >= 0)
    {
        warp_to_room(next);
        bn::string<48> where;
        if(summit)
        {
            where = "TOWER SUMMIT";
        }
        else
        {
            where = "CHALLENGE TOWER - FLOOR ";
            where.append(bn::to_string<4>(floor + 2));
        }
        gui().show_place(where);
    }
}

// After a battle in the tower: a floor cleared (the TOWER MASTER's raises your rank), or the legendary met.
void overworld::tower_after_battle()
{
    game_state& g = state();
    const room_info* r = _map->room;
    if(! _last || ! r || ! g.tower.active)
    {
        return;
    }
    if(r->kind == room_kind::TOWER && _last->trainer_beaten)
    {
        int floor = r->floor;
        if(floor >= 4)
        {
            g.run.tower_clears = uint8_t(bn::min(250, g.run.tower_clears + 1));
            g.run.tower_streak = uint8_t(bn::min(250, g.run.tower_streak + 1));
            g.run.tower_best = bn::max(g.run.tower_best, g.run.tower_streak);
            save_game();
            audio::play(audio::sfx::OBTAIN);
            say("You've conquered the CHALLENGE TOWER!");
            bn::string<48> text("RANK ");
            text.append(bn::to_string<4>(g.run.tower_clears + 1));
            text.append(" reached. The SUMMIT awaits.");
            say(text);
        }
        else
        {
            bn::string<48> text("FLOOR ");
            text.append(bn::to_string<4>(floor + 1));
            text.append(" cleared!");
            say(text);
        }
        tower_offer_up(floor);
        return;
    }
    if(r->kind == room_kind::CHAMBER && g.tower.legend >= 0)
    {
        bn::string<64> text(game_data::species_list[g.tower.legend].name);
        if(_last->outcome == battle_outcome::CAUGHT)
        {
            text.append(" is yours! It leaves the stone for good.");
        }
        else
        {
            text.append(" faded back into the SUMMONING STONE...");
        }
        say(text);
        say("The challenge is complete. Well done!");
        tower_end(true);
        leave_room();
    }
}

// The SUMMONING STONE: a legendary still in the stone answers, and waits in the chamber of its type.
void overworld::tower_stone()
{
    game_state& g = state();
    if(! g.tower.active)
    {
        say("A SUMMONING STONE. It's cold and silent.");
        return;
    }
    if(g.tower.legend < 0)
    {
        int left[game_data::legendaries_count];
        int n = 0;
        for(int i = 0; i < game_data::legendaries_count; ++i)
        {
            if(! g.owned.test(game_data::legendaries[i]))
            {
                left[n++] = game_data::legendaries[i];
            }
        }
        if(! n)
        {
            say("The SUMMONING STONE is silent. Every legendary POKéMON has already been caught!");
            tower_end(true);
            leave_room();
            return;
        }
        g.tower.legend = int16_t(left[rng().get_int(n)]);
        g.tower.legend_shiny = roll_shiny();
        save_game();
    }
    say("You place your hand on the SUMMONING STONE...");
    audio::play(audio::sfx::SPOT);
    bn::string<64> text("It blazes with light! ");
    upper(text, game_data::species_list[g.tower.legend].name);
    text.append(" answers the call!");
    say(text);
    int chamber = tower_room(room_kind::CHAMBER, -1, chamber_theme(g.tower.legend));
    if(chamber < 0)
    {
        chamber = tower_room(room_kind::CHAMBER, -1, gym_theme::LEAGUE);
    }
    warp_to_room(chamber);
    gui().show_place("SUMMONING CHAMBER");
}

// ----- START -----
void overworld::start_menu()
{
    static int last = 0;
    game_state& g = state();
    ui& u = gui();
    while(! _quit)
    {
        // POKéDEX shows up once you've seen something.
        bn::string_view options[8];
        int ids[8];
        int n = 0;
        if(g.seen.count())
        {
            ids[n] = 0;
            options[n++] = "POKéDEX";
        }
        if(g.party_count)
        {
            ids[n] = 1;
            options[n++] = "POKéMON";
        }
        ids[n] = 2;
        options[n++] = "BAG";
        ids[n] = 3;
        options[n++] = "POKéNAV";
        ids[n] = 4;
        options[n++] = g.name;
        ids[n] = 5;
        options[n++] = "SAVE";
        ids[n] = 6;
        options[n++] = "OPTION";
        ids[n] = 7;
        options[n++] = "EXIT";
        // Emerald: a window in the top right, sized to its longest entry.
        int widest = 0;
        for(int i = 0; i < n; ++i)
        {
            widest = bn::max(widest, u.width(options[i]));
        }
        menu_spec s;
        s.options = options;
        s.count = n;
        s.tw = (widest + 16 + 12 + 7) / 8;
        s.tx = 30 - s.tw;
        s.ty = 0;
        s.th = s.count * 2 + 2;
        s.start = bn::min(last, n - 1);
        int pick = u.menu(s);
        if(pick < 0 || ids[pick] == 7)
        {
            return;
        }
        last = pick;
        switch(ids[pick])
        {
        case 0:
            suspend();
            dex_screen();
            resume();
            break;
        case 1:
            suspend();
            party_screen(party_mode::FIELD);
            save_game();
            resume();
            break;
        case 2:
        {
            suspend();
            int used = bag_screen(bag_mode::FIELD);
            save_game();
            resume();
            move_prompts();     // a RARE CANDY's (or a TM's, or an evolution's) new moves
            if(used == int(item_id::ESCAPEROPE) && escape_rope())
            {
                return;
            }
            break;
        }
        case 3:
        {
            // Fast travel from the map, except from inside the CHALLENGE TOWER or the POKéMON LEAGUE.
            bool travel = ! g.tower.active && ! (_map->room && _map->room->kind == room_kind::LEAGUE);
            suspend();
            int dest = region_map_screen(travel);
            if(dest >= 0 && (wd::maps[dest].area->flags & area_flag::SAFARI) && ! (_map->area && (_map->area->flags & area_flag::SAFARI)))
            {
                // The SAFARI ZONE's fee, travelling in too.
                resume();
                if(! safari_gate())
                {
                    break;
                }
                suspend();
            }
            if(dest >= 0)
            {
                fast_travel(dest);
                return;
            }
            resume();
            break;
        }
        case 4:
            suspend();
            card_screen();
            resume();
            break;
        case 5:
            save_menu();
            return;
        case 6:
            suspend();
            option_screen();
            save_game();
            resume();
            break;
        default:
            break;
        }
    }
}

// Fast travel (the POKéNAV map): to the door of the place's POKéMON CENTER, else where the place is entered
// (its spawn), on the nearest open ground. Called while suspended.
void overworld::fast_travel(int area, int at_x, int at_y)
{
    game_state& g = state();
    const map_def& m = wd::maps[area];
    int x = m.spawn_x, y = m.spawn_y;
    if(at_x >= 0)
    {
        x = at_x;   // 6.0.0: a rift: the same spot on the other side
        y = at_y;
    }
    for(int i = 0; i < m.doors_count && at_x < 0; ++i)
    {
        if(m.doors[i].kind == door_kind::CENTER)
        {
            x = m.doors[i].x;
            y = m.doors[i].y + 1;
        }
    }
    auto open = [&](int tx, int ty)
    {
        if(tx < 0 || ty < 0 || tx >= m.w || ty >= m.h || behaviour(m.behaviours[ty * m.w + tx]) != behaviour::WALK)
        {
            return false;
        }
        for(int i = 0; i < m.people_count; ++i)
        {
            if(m.people[i].x == tx && m.people[i].y == ty)
            {
                return false;
            }
        }
        for(int i = 0; i < m.trainers_count; ++i)
        {
            if(m.trainers[i].x == tx && m.trainers[i].y == ty)
            {
                return false;
            }
        }
        return true;
    };
    // Rings out from there until there's a free path tile.
    bool found = open(x, y);
    for(int r = 1; r < 40 && ! found; ++r)
    {
        for(int dy = -r; dy <= r && ! found; ++dy)
        {
            for(int dx = -r; dx <= r && ! found; ++dx)
            {
                if((bn::abs(dx) == r || bn::abs(dy) == r) && open(x + dx, y + dy))
                {
                    x += dx;
                    y += dy;
                    found = true;
                }
            }
        }
    }
    g.x = int16_t(x);
    g.y = int16_t(y);
    g.facing = direction::DOWN;
    g.surfing = false;
    _suspended = false;
    _player.reset();
    load_map(area);
    ui::fade_in(12);
    _fresh_press = true;
    save_game();
}

// ----- Hidden items, the TRADER and the SAFARI ZONE (GBA only) -----
// A hidden item: facing its spot, A finds it.
bool overworld::find_hidden(int tx, int ty)
{
    game_state& g = state();
    if(_map->is_room())
    {
        return false;
    }
    for(int i = 0; i < wd::hidden_items_count; ++i)
    {
        const hidden_item& h = wd::hidden_items[i];
        if(h.area == _map_index && h.x == tx && h.y == ty && ! g.picked.test(h.id))
        {
            g.picked.set(h.id);
            update_glints();
            say("There's something here!");
            obtain(h.item, h.count);
            save_game();
            return true;
        }
    }
    return false;
}

// The SAFARI ZONE's gate: $5000 to go in. Returns whether you paid.
bool overworld::safari_gate()
{
    game_state& g = state();
    ui& u = gui();
    say("Welcome to the SAFARI ZONE! Any POKéMON at all could be waiting in its grass.");
    if(g.money < 5000)
    {
        say("The entry fee is $5000. Come back when you have it!");
        return false;
    }
    u.show_text("The entry fee is $5000. Would you like to go in?");
    bool yes = u.yes_no();
    u.clear_text();
    if(! yes)
    {
        say("Come back anytime!");
        return false;
    }
    g.money -= 5000;
    audio::play(audio::sfx::SELECT);
    say("Thank you! Good luck out there!");
    return true;
}

// TRADEWIND VILLAGE's TRADER: one of yours for a random Pokémon of about the same level (not a legendary).
void overworld::trader()
{
    game_state& g = state();
    ui& u = gui();
    say("TRADER: \"I trade POKéMON with TRAINERS from all over. Give me one of yours, and I'll send you one of mine "
        "about as strong. Who knows what you'll get!\"");
    if(g.run.nuzlocke())
    {
        say("TRADER: \"Hm? A NUZLOCKE challenger... Your rules say no trading. Good luck!\"");
        return;
    }
    while(true)
    {
        u.show_text("Trade a POKéMON?");
        bool yes = u.yes_no();
        u.clear_text();
        if(! yes)
        {
            say("TRADER: \"Come back anytime!\"");
            return;
        }
        suspend();
        int pick = party_screen(party_mode::CHOOSE, "Trade which POKéMON?");
        resume();
        if(pick < 0)
        {
            continue;
        }
        mon& old = g.party[pick];
        if(g.able_count() <= 1 && ! old.fainted())
        {
            say("TRADER: \"That's your only POKéMON that can battle! Keep it.\"");
            continue;
        }
        bn::random& r = rng();
        int species = 0;
        for(int tries = 0; tries < 50; ++tries)
        {
            species = r.get_int(species_count);
            bool legend = false;
            for(int k = 0; k < game_data::legendaries_count; ++k)
            {
                legend |= game_data::legendaries[k] == species;
            }
            if(! legend)
            {
                break;
            }
        }
        int level = bn::clamp(old.level - 2 + r.get_int(5), 1, 100);
        mon got = mon::make(species_id(species), level, old.item);
        if(roll_shiny())
        {
            got.traits |= mon_trait::SHINY;
        }
        bn::string<96> text(old.name());
        text.append(" was sent to the TRADER.");
        say(text);
        audio::play(audio::sfx::OBTAIN);
        text = "In return, ";
        text.append(got.name());
        text.append(" (Lv");
        text.append(bn::to_string<4>(got.level));
        text.append(got.shiny() ? ", SHINY!) arrived!" : ") arrived!");
        say(text);
        g.mark_seen(species);
        g.mark_owned(species);
        old = got;
        save_game();
        say("TRADER: \"Take good care of it! Want to trade again?\"");
    }
}

// The cheat menu (LEFT, RIGHT, LEFT, RIGHT, B, A, START on the field).
void overworld::cheat_menu()
{
    game_state& g = state();
    ui& u = gui();
    int last = 0;
    while(! _quit)
    {
        bool wild = ! g.has(story::CHEAT_NO_WILD), perfect = g.has(story::CHEAT_PERFECT_CATCH);
        bn::string<32> wild_label("WILD ENCOUNTERS: ");
        wild_label.append(wild ? "ON" : "OFF");
        bn::string<32> catch_label("PERFECT CAPTURE: ");
        catch_label.append(perfect ? "ON" : "OFF");
        bn::string_view options[] = { wild_label, "HEAL PARTY POKéMON", "ADD 20 POKé BALLS", "ADD MASTER BALL", "ADD $1000",
                                      catch_label, "EXIT" };
        constexpr int n = 7;
        int widest = 0;
        for(const bn::string_view& o : options)
        {
            widest = bn::max(widest, u.width(o));
        }
        menu_spec s;
        s.options = options;
        s.count = n;
        s.tw = bn::min(30, (widest + 16 + 12 + 7) / 8);
        s.tx = 30 - s.tw;
        s.ty = 0;
        s.th = n * 2 + 2;
        s.start = last;
        int pick = u.menu(s);
        if(pick < 0 || pick == n - 1)
        {
            break;
        }
        last = pick;
        switch(pick)
        {
        case 0:
            g.story ^= story::CHEAT_NO_WILD;
            audio::play(audio::sfx::SELECT);
            break;
        case 1:
            g.heal_party();
            audio::play(audio::sfx::HEAL);
            say("Your POKéMON were fully healed!");
            break;
        case 2:
            g.add_item(item_id::POKEBALL, 20);
            audio::play(audio::sfx::SELECT);
            say("20 POKé BALLS were put in the BAG.");
            break;
        case 3:
            g.add_item(item_id::MASTERBALL, 1);
            audio::play(audio::sfx::SELECT);
            say("A MASTER BALL was put in the BAG.");
            break;
        case 4:
            g.money = bn::min(uint32_t(999999), uint32_t(g.money + 1000));
            audio::play(audio::sfx::SELECT);
            say("$1000 was added to your money.");
            break;
        case 5:
            g.story ^= story::CHEAT_PERFECT_CATCH;
            audio::play(audio::sfx::SELECT);
            break;
        default:
            break;
        }
    }
    save_game();
    hold_until_released();
}

// Emerald's save: an info window top left (place, PLAYER, BADGES, POKéDEX), YES/NO, "SAVING...", then
// "{PLAYER} saved the game." and back to the field by itself.
void overworld::save_menu()
{
    game_state& g = state();
    ui& u = gui();
    bn::vector<bn::sprite_ptr, 24> info;
    u.win().box(window_style::WINDOW, 0, 0, 16, 10);
    const map_def& here = wd::maps[area_index()];
    u.print_fit(10, 8, here.name, 112, text_color::BLUE, info);
    auto row = [&](int i, const char* label, const bn::string_view& value)
    {
        u.print(10, 26 + i * 16, label, text_color::INK, info);
        u.print(118 - u.width(value), 26 + i * 16, value, text_color::INK, info);
    };
    // POKéDEX: the different species you have, party and boxes.
    bitset<dex_size> have;
    for(int i = 0; i < g.party_count; ++i)
    {
        have.set(g.party[i].species_index);
    }
    for(const mon& m : g.box)
    {
        if(! m.empty())
        {
            have.set(m.species_index);
        }
    }
    row(0, "PLAYER", g.name);
    row(1, "BADGES", bn::to_string<4>(g.badges()));
    row(2, "POKéDEX", bn::to_string<4>(have.count()));
    u.show_text("Would you like to save the game?");
    bool yes = u.yes_no();
    if(yes)
    {
        u.say_timed("SAVING...\nDON'T TURN OFF THE POWER.", 30);
        save_game();
        audio::play(audio::sfx::SAVE);
        bn::string<48> text(g.name);
        text.append(" saved the game.");
        u.say_timed(text, 60);
    }
    u.clear_text();
    info.clear();
    u.win().clear(0, 0, 16, 10);
    if(! yes)
    {
        start_menu();
    }
}


// ----- GBA 1.8: the ESCAPE ROPE, the MOVE TUTOR, the DAY CARE and EGGS, REPEL and rematches -----
namespace
{
    constexpr int frames_per_day = 60 * 1800;   // a day of play: 30 minutes (current_time_of_day)

    bool in_group(int groups, int group)
    {
        return (groups & 15) == group || (groups >> 4) == group;
    }

    bool is_ditto(const mon& m)
    {
        return in_group(game_data::egg_groups[m.species_index], 13);
    }

    // Whether two Pokémon left at the DAY CARE can have an EGG: a shared egg group, or one of them DITTO
    // (but not both); never one that can't breed (legendaries, babies). This game has no genders.
    bool breeds(const mon& a, const mon& b)
    {
        int ga = game_data::egg_groups[a.species_index], gb = game_data::egg_groups[b.species_index];
        if(in_group(ga, 15) || in_group(gb, 15))
        {
            return false;
        }
        if(is_ditto(a) || is_ditto(b))
        {
            return is_ditto(a) != is_ditto(b);
        }
        return in_group(gb, ga & 15) || in_group(gb, ga >> 4);
    }

    // The level a Pokémon left at the DAY CARE has reached: 1 EXP a step, up to the level cap.
    int daycare_level(const mon& m, uint32_t steps, uint32_t& xp)
    {
        int level = m.level;
        int cap = level_cap_now();
        xp = m.xp + steps;
        while(level < cap && xp >= uint32_t(level * 8))
        {
            xp -= uint32_t(level * 8);
            ++level;
        }
        if(level >= cap)
        {
            xp = 0;
        }
        return level;
    }

    // Struggle goes once it knows something real.
    void drop_struggle(mon& m)
    {
        for(int k = 0; k < m.move_count; ++k)
        {
            if(m.move(k) == 0)
            {
                m.remove_move(k);
                --k;
            }
        }
    }
}

// The ESCAPE ROPE (from the bag): out of a cave or the sea floor, to the door of the last POKéMON CENTER.
bool overworld::escape_rope()
{
    game_state& g = state();
    bool deep = _map->area && ! _map->is_room() &&
                (_map->area->theme == area_theme::CAVE || _map->area->theme == area_theme::DEEP);
    if(! deep || g.tower.active || ! g.item_count(item_id::ESCAPEROPE))
    {
        say("This can't be used here.");
        return false;
    }
    g.items[int(item_id::ESCAPEROPE)] = uint8_t(g.items[int(item_id::ESCAPEROPE)] - 1);
    bn::string<48> text(g.name);
    text.append(" used the ESCAPE ROPE!");
    say(text);
    int area = g.last_heal;
    if(area < 0 || ! wd::maps[area].area || ! (wd::maps[area].area->flags & area_flag::CENTER))
    {
        area = 0;
    }
    suspend();
    fast_travel(area);
    return true;
}

// The MOVE TUTOR (every POKéMON CENTER): a move from the Pokémon's level-up list, up to its level.
// Calderra's MOVE TUTOR (2.0.0): four of the Gen 8 and 9 moves each day, for money, to a POKéMON of the
// move's type (NORMAL moves to anyone).
void overworld::tutor_new_move()
{
    game_state& g = state();
    ui& u = gui();
    constexpr int fresh = game_data::moves_count - game_data::moves_v1;
    int today = int(g.play_frames / frames_per_day);
    int moves[4], prices[4];
    bn::string<32> labels[5];
    bn::string_view views[5];
    for(int k = 0; k < 4; ++k)
    {
        moves[k] = game_data::moves_v1 + (today * 4 + k) % fresh;
        const move& mv = move_data(moves[k]);
        prices[k] = mv.category == move_category::STATUS || mv.power <= 70 ? 3000 : mv.power <= 95 ? 5000 : 8000;
        labels[k] = mv.name;
        labels[k].append(" $");
        labels[k].append(bn::to_string<6>(prices[k]));
        views[k] = labels[k];
    }
    views[4] = "CANCEL";
    u.show_text("Today's moves:");
    int k = u.list(views, 5);
    u.clear_text();
    if(k < 0 || k >= 4)
    {
        return;
    }
    if(g.money < uint32_t(prices[k]))
    {
        say("MOVE TUTOR: \"You'll need more money than that, I'm afraid.\"");
        return;
    }
    suspend();
    int pick = party_screen(party_mode::CHOOSE, "Teach which POKéMON?");
    resume();
    if(pick < 0)
    {
        return;
    }
    mon& m = g.party[pick];
    const move& mv = move_data(moves[k]);
    bn::string<96> text(m.name());
    bool known = false;
    for(int i = 0; i < m.move_count; ++i)
    {
        known |= m.move(i) == moves[k];
    }
    if(known)
    {
        text.append(" already knows it.");
        say(text);
        return;
    }
    if(mv.type != 0 && ! m.has_type(mv.type))
    {
        text.append(" can't learn a move of that type.");
        say(text);
        return;
    }
    g.money -= uint32_t(prices[k]);
    drop_struggle(m);
    if(m.move_count < 4)
    {
        m.set_move(m.move_count++, uint16_t(moves[k]));
        audio::play(audio::sfx::OBTAIN);
        text.append(" learned ");
        text.append(mv.name);
        text.append("!");
        say(text);
    }
    else
    {
        add_pending_move(&m, uint16_t(moves[k]));
        move_prompts();
    }
    save_game();
}

void overworld::move_tutor()
{
    game_state& g = state();
    ui& u = gui();
    say("MOVE TUTOR: \"I can help a POKéMON remember a move it learned before, or one it should have learned by now.\"");
    bool calderra = map_region(_map_index) >= 2;
    if(calderra)
    {
        say("MOVE TUTOR: \"Out here I also teach new moves, a different four each day. For a fee, of course!\"");
    }
    while(true)
    {
        if(calderra)
        {
            constexpr bn::string_view options[] = { "REMEMBER A MOVE", "LEARN A NEW MOVE", "CANCEL" };
            u.show_text("What would you like?");
            int c = u.list(options, 3);
            u.clear_text();
            if(c == 1)
            {
                tutor_new_move();
                continue;
            }
            if(c != 0)
            {
                say("MOVE TUTOR: \"Come back anytime!\"");
                return;
            }
        }
        else
        {
            u.show_text("Remember a move?");
            bool yes = u.yes_no();
            u.clear_text();
            if(! yes)
            {
                say("MOVE TUTOR: \"Come back anytime!\"");
                return;
            }
        }
        suspend();
        int pick = party_screen(party_mode::CHOOSE, "Which POKéMON?");
        resume();
        if(pick < 0)
        {
            continue;
        }
        mon& m = g.party[pick];
        const species& sp = m.data();
        uint16_t moves[64];
        int n = 0;
        for(int i = 0; i < sp.learnset_count && n < 64; ++i)
        {
            const learn_entry& e = sp.learnset[i];
            bool skip = e.level > m.level || e.move == 0;
            for(int k = 0; k < m.move_count && ! skip; ++k)
            {
                skip = m.move(k) == e.move;
            }
            for(int k = 0; k < n && ! skip; ++k)
            {
                skip = moves[k] == e.move;
            }
            if(! skip)
            {
                moves[n++] = e.move;
            }
        }
        bn::string<96> text(m.name());
        if(! n)
        {
            text.append(" has no moves to remember.");
            say(text);
            continue;
        }
        bn::string_view views[65];
        for(int k = 0; k < n; ++k)
        {
            views[k] = move_data(moves[k]).name;
        }
        views[n] = "CANCEL";
        u.show_text("Which move should it remember?");
        int k = u.list(views, n + 1);
        u.clear_text();
        if(k < 0 || k >= n)
        {
            continue;
        }
        drop_struggle(m);
        if(m.move_count < 4)
        {
            m.set_move(m.move_count++, moves[k]);
            audio::play(audio::sfx::OBTAIN);
            text.append(" learned ");
            text.append(move_data(moves[k]).name);
            text.append("!");
            say(text);
        }
        else
        {
            add_pending_move(&m, moves[k]);
            move_prompts();
        }
        save_game();
    }
}

// The DAY CARE (CINDERGATE TOWN): up to two Pokémon gain a level's EXP as you walk, and two that can breed now
// and then leave an EGG. Taking one back costs $100, plus $100 a level it grew.
void overworld::daycare()
{
    game_state& g = state();
    ui& u = gui();
    extras_state& x = g.extra;
    if(x.egg_waiting)
    {
        say("DAY CARE: \"Ah, there you are! We were raising your POKéMON, and my, were we surprised!\"");
        say("DAY CARE: \"Your POKéMON had an EGG! We don't know how it got there. Do you want it?\"");
        u.show_text("Take the EGG?");
        bool yes = u.yes_no();
        u.clear_text();
        if(yes && x.egg_count >= egg_slots)
        {
            say("DAY CARE: \"You're carrying too many EGGs already. Come back once one has hatched.\"");
        }
        else if(yes)
        {
            const mon& parent = is_ditto(x.daycare[0]) ? x.daycare[1] : x.daycare[0];
            egg& e = x.eggs[x.egg_count++];
            e.species = game_data::babies[parent.species_index];
            e.steps = uint16_t(egg_hatch_steps);
            e.shiny = roll_shiny();
            x.egg_waiting = false;
            audio::play(audio::sfx::OBTAIN);
            bn::string<48> text(g.name);
            text.append(" received the EGG!");
            say(text);
            say("DAY CARE: \"Keep it with you as you walk, and it'll hatch before long.\"");
            save_game();
        }
        else
        {
            x.egg_waiting = false;
            say("DAY CARE: \"Well then, I'll keep it. Thank you!\"");
        }
    }
    say("DAY CARE: \"I'm the DAY CARE lady. Leave a POKéMON or two with me, and they'll grow as you walk.\"");
    while(true)
    {
        int in_care = 0;
        for(int i = 0; i < 2; ++i)
        {
            const mon& m = x.daycare[i];
            if(m.empty())
            {
                continue;
            }
            ++in_care;
            uint32_t xp;
            int grown = daycare_level(m, x.daycare_steps[i], xp) - m.level;
            bn::string<96> text("Your ");
            text.append(m.name());
            if(grown)
            {
                text.append(" has grown by ");
                text.append(bn::to_string<4>(grown));
                text.append(grown == 1 ? " level." : " levels.");
            }
            else
            {
                text.append(" is doing just fine.");
            }
            say(text);
        }
        if(in_care == 2)
        {
            say(breeds(x.daycare[0], x.daycare[1]) ? "DAY CARE: \"Those two get along very well!\""
                                                   : "DAY CARE: \"Those two prefer to play with other POKéMON.\"");
        }
        if(x.egg_count)
        {
            bn::string<48> text("You're carrying ");
            text.append(bn::to_string<4>(x.egg_count));
            text.append(x.egg_count == 1 ? " EGG." : " EGGs.");
            say(text);
        }
        bn::string_view options[3];
        int ids[3];
        int n = 0;
        if(in_care < 2)
        {
            ids[n] = 0;
            options[n++] = "LEAVE ONE";
        }
        if(in_care)
        {
            ids[n] = 1;
            options[n++] = "TAKE ONE BACK";
        }
        ids[n] = 2;
        options[n++] = "CANCEL";
        u.show_text("What would you like to do?");
        int k = u.list(options, n);
        u.clear_text();
        int pick = k < 0 ? 2 : ids[k];
        if(pick == 2)
        {
            say("DAY CARE: \"Come again!\"");
            return;
        }
        if(pick == 0)
        {
            suspend();
            int who = party_screen(party_mode::CHOOSE, "Leave which POKéMON?");
            resume();
            if(who < 0)
            {
                continue;
            }
            mon& m = g.party[who];
            if(g.party_count <= 1 || (g.able_count() <= 1 && ! m.fainted()))
            {
                say("DAY CARE: \"That's your only POKéMON that can battle! Keep it with you.\"");
                continue;
            }
            int slot = x.daycare[0].empty() ? 0 : 1;
            x.daycare[slot] = m;
            x.daycare[slot].heal();
            x.daycare_steps[slot] = 0;
            x.egg_steps = 0;
            for(int i = who; i < g.party_count - 1; ++i)
            {
                g.party[i] = g.party[i + 1];
            }
            g.party[g.party_count - 1] = mon();
            --g.party_count;
            bn::string<64> text("Left ");
            text.append(x.daycare[slot].name());
            text.append(" at the DAY CARE.");
            say(text);
            save_game();
            continue;
        }
        // Take one back.
        bn::string<40> labels[2];
        bn::string_view views[3];
        int slots[2];
        int m_count = 0;
        for(int i = 0; i < 2; ++i)
        {
            const mon& m = x.daycare[i];
            if(m.empty())
            {
                continue;
            }
            uint32_t xp;
            int level = daycare_level(m, x.daycare_steps[i], xp);
            labels[m_count] = m.name();
            labels[m_count].append(" Lv");
            labels[m_count].append(bn::to_string<4>(level));
            labels[m_count].append("  $");
            labels[m_count].append(bn::to_string<6>(100 + 100 * (level - m.level)));
            views[m_count] = labels[m_count];
            slots[m_count++] = i;
        }
        views[m_count] = "CANCEL";
        u.show_text("Take back which one?");
        int t = u.list(views, m_count + 1);
        u.clear_text();
        if(t < 0 || t >= m_count)
        {
            continue;
        }
        int slot = slots[t];
        if(g.party_count >= g.party_cap())
        {
            say("DAY CARE: \"Your party is full. Make some room first.\"");
            continue;
        }
        uint32_t xp;
        int level = daycare_level(x.daycare[slot], x.daycare_steps[slot], xp);
        int cost = 100 + 100 * (level - x.daycare[slot].level);
        if(g.money < uint32_t(cost))
        {
            say("DAY CARE: \"You don't have enough money.\"");
            continue;
        }
        g.money -= uint32_t(cost);
        mon& m = g.party[g.party_count++];
        m = x.daycare[slot];
        x.daycare[slot] = mon();
        x.daycare_steps[slot] = 0;
        x.egg_steps = 0;
        bn::string<64> text("Got back ");
        text.append(m.name());
        text.append(" from the DAY CARE.");
        audio::play(audio::sfx::OBTAIN);
        say(text);
        while(m.level < level)
        {
            ++m.level;
            m.recalc_stats(m.level - 1);
            m.learn_moves_at(m.level, u);
        }
        m.xp = uint16_t(xp);
        m.heal();
        move_prompts();
        save_game();
    }
}

// Each step: REPEL counts down, the DAY CARE's Pokémon gain EXP (and may leave an EGG), and the EGGS you carry
// get closer to hatching. Returns true if something was said.
bool overworld::step_counters()
{
    game_state& g = state();
    extras_state& x = g.extra;
    bool spoke = false;
    if(x.repel_steps && ! --x.repel_steps)
    {
        say("REPEL's effect wore off...");
        spoke = true;
    }
    for(int i = 0; i < 2; ++i)
    {
        if(! x.daycare[i].empty() && x.daycare_steps[i] < 1000000)
        {
            ++x.daycare_steps[i];
        }
    }
    if(! x.daycare[0].empty() && ! x.daycare[1].empty() && ! x.egg_waiting && ++x.egg_steps >= egg_lay_steps)
    {
        x.egg_steps = 0;
        bool same = x.daycare[0].species_index == x.daycare[1].species_index;
        if(breeds(x.daycare[0], x.daycare[1]) && rng().get_int(100) < (same ? 70 : 50))
        {
            x.egg_waiting = true;
        }
    }
    for(int i = 0; i < x.egg_count; ++i)
    {
        if(x.eggs[i].steps > 1)
        {
            --x.eggs[i].steps;
        }
        else if(! spoke)
        {
            spoke = hatch(i);
        }
    }
    return spoke;
}

// "Oh?" ... an EGG hatches into a level 1 Pokémon: into the party, or the PC if it's full.
bool overworld::hatch(int slot)
{
    game_state& g = state();
    extras_state& x = g.extra;
    egg e = x.eggs[slot];
    mon m = mon::make(species_id(e.species), 1);
    if(e.shiny)
    {
        m.traits |= mon_trait::SHINY;
    }
    int box_slot;
    if(! g.add_mon(m, box_slot))
    {
        return false;   // nowhere to put it: it waits (one step from hatching)
    }
    for(int i = slot; i < x.egg_count - 1; ++i)
    {
        x.eggs[i] = x.eggs[i + 1];
    }
    x.eggs[x.egg_count - 1] = egg();
    --x.egg_count;
    set_player_frame(0);
    say("Oh?");
    say(". . . . . .");
    audio::play(audio::sfx::OBTAIN);
    bn::string<64> text(game_data::species_list[e.species].name);
    text.append(e.shiny ? " hatched from the EGG! It's SHINY!" : " hatched from the EGG!");
    say(text);
    if(box_slot >= 0)
    {
        say("It was sent to the PC.");
    }
    g.mark_seen(e.species);
    g.mark_owned(e.species);
    save_game();
    return true;
}

// A route trainer you've beaten will battle again once a day (30 minutes of play), stronger than before.
bool overworld::offer_rematch(int index)
{
    game_state& g = state();
    ui& u = gui();
    actor& a = _actors[index];
    // 2.0.0: once you're CALDERRA's CHAMPION, its GYM LEADERS take rematches too, at level 200.
    bool leader = a.tr->role == trainer_role::LEADER && map_region(_map_index) >= 2 &&
                  g.flags.test(champion_flag(map_region(_map_index)));
    if((a.tr->role != trainer_role::ROUTE && ! leader) || a.tr->vanish || a.tr->scene || ! g.beaten.test(a.tr->id) ||
       g.first_able() < 0 || g.tower.active)
    {
        return false;
    }
    int today = int(g.play_frames / frames_per_day);
    if(g.extra.rematch_day != today)
    {
        g.extra.rematch_day = uint16_t(today);
        g.extra.rematched = bitset<trainer_slots>();
    }
    if(g.extra.rematched.test(a.tr->id))
    {
        return false;
    }
    bn::string<128> text;
    upper(text, a.tr->title);
    text.append(": \"I've been training since we last battled. How about a rematch?\"");
    u.show_text(text);
    bool yes = u.yes_no();
    u.clear_text();
    if(yes)
    {
        start_trainer_battle(index);
        _battle->rematch = true;
    }
    return true;
}

}
