#include "pr_state.h"

#include "bn_common.h"
#include <cstddef>
#include "bn_memory.h"
#include "bn_sram.h"

#include "pr_world_data.h"

namespace pr
{

namespace
{
    constexpr char save_tag[8] = { 'P', 'R', 'O', 'Y', 'A', 'L', 'E', '1' };
    constexpr int save_version = 11;

    struct save_block
    {
        char tag[8];
        int version;
        int size;
        game_state game;
        uint32_t checksum;
    };

    static_assert(sizeof(save_block) <= 32 * 1024 - 16, "the save must fit in SRAM, before the clear mark");
    static_assert(items_count <= bag_kinds, "the bag holds bag_kinds kinds of items");
    static_assert(species_count <= dex_size, "the POKéDEX holds dex_size species");
    static_assert(world_data::trainers_count <= trainer_slots, "beaten holds trainer_slots trainers");
    static_assert(world_data::items_count <= item_ball_slots, "picked holds item_ball_slots item balls");
    static_assert(world_data::maps_count <= area_slots, "visited holds a bit per map index");
    static_assert(sizeof(game_state) == 30260, "save version 11's layout never moves: new fields come out of spare");

    template<typename Block>
    uint32_t checksum_of(const Block& block)
    {
        const auto* bytes = reinterpret_cast<const uint8_t*>(&block);
        int length = reinterpret_cast<const uint8_t*>(&block.checksum) - bytes;
        uint32_t sum = 0x1234;
        for(int i = 0; i < length; ++i)
        {
            sum = (sum * 31) + bytes[i];
        }
        return sum;
    }

    // ---------- Save versions 4-10 (GBA 1.0-1.9.x), read once and converted ----------
    // Save version 10's layout, exactly: 46-byte Pokémon, a 116-kind bag and the old bitset sizes.
    namespace v10
    {
        constexpr int items = 116;

        struct mon
        {
            uint16_t species_index;
            uint8_t level;
            uint8_t move_count;
            status st;
            uint8_t sleep_turns;
            held_item item;
            uint8_t flags;
            uint16_t slots[4];          // move index (low 10 bits), PP used (high 6 bits)
            uint16_t hp, max_hp, atk, def, spa, spd, spe, xp;
            char nick[nick_length + 1];
            uint8_t traits;
        };

        struct run
        {
            run_mode mode;
            bool skip_story, adventure, over;
            uint16_t deaths, catches;
            bitset<64> encounter_used;
            uint8_t tower_clears, tower_best, tower_streak, grave_next;
            mon graveyard[graveyard_size];
        };

        struct extras
        {
            uint16_t repel_steps, rematch_day;
            bitset<256> rematched;
            mon daycare[2];
            uint32_t daycare_steps[2];
            uint16_t egg_steps;
            bool egg_waiting;
            uint8_t egg_count;
            egg eggs[egg_slots];
        };

        struct game
        {
            int16_t map, x, y;
            direction facing;
            uint8_t party_count;
            bool surfing;
            uint8_t starter_trio;
            int16_t walk_off, last_heal, restocked;
            uint16_t trainer_id;
            uint32_t story, money, play_frames;
            options opt;
            char name[name_length + 1];
            bn::array<uint8_t, items> items_, pc_items;
            bn::array<mon, max_party> party;
            bn::array<mon, box_slots> box;
            char box_names[box_count][box_name_length + 1];
            uint8_t box_wall[box_count];
            bitset<256> beaten, picked;
            bitset<1024> seen, owned;
            bitset<64> visited;
            run run_;
            tower_state tower;
            extras extra;
        };

        struct block
        {
            char tag[8];
            int version;
            int size;
            game game_;
            uint32_t checksum;
        };

        static_assert(sizeof(mon) == 46 && sizeof(run) == 1400, "save version 10's layout");
        static_assert(sizeof(game) == 22112 && offsetof(game, extra) == 21936, "save version 10's layout");

        // Older saves: game up to `visited` (save version 4: GBA 1.0), then `run_` (5: GBA 1.1), then `tower` (6:
        // GBA 1.2-1.4). Their bag had 13 kinds of items; version 7 (GBA 1.5) added three and version 8 (GBA 1.6) one
        // more, each moving everything after the bag along; version 9 (GBA 1.8) added the stones, REPELS, rods and TMs
        // to the bag, and `extra` at the end; version 10 (GBA 1.9) added the five held items to the bag.
        constexpr int v4_game_size = 20324;
        constexpr int v5_game_size = 21724;
        constexpr int v6_game_size = 21728;
        constexpr int v7_game_size = 21736;     // GBA 1.5: 16 kinds of items
        constexpr int v8_game_size = 21736;     // GBA 1.6-1.7: 17 (the MASTER BALL)
        constexpr int v9_game_size = 22100;     // GBA 1.8: 111
        constexpr int v9_items_count = 111;
        constexpr int v9_extra_at = 21924;      // offsetof(game, extra) in save version 9
        constexpr int old_items_count = 13;
        constexpr int bag_growth = 2 * (items - old_items_count);
        static_assert(offsetof(game, run_) == v4_game_size + bag_growth, "save version 4 must be game's prefix");
        static_assert(offsetof(game, tower) == v5_game_size + bag_growth, "save version 5 must be game's prefix");

        // Reads a version 4-10 save into `b` as version 10 (the 1.9 loader, unchanged).
        bool read(block& b)
        {
            bn::sram::read(b);
            if(b.version == 10)
            {
                return b.size == int(sizeof(game)) && b.checksum == checksum_of(b);
            }
            if(! ((b.version == 4 && b.size == v4_game_size) || (b.version == 5 && b.size == v5_game_size) ||
                  (b.version == 6 && b.size == v6_game_size) || (b.version == 7 && b.size == v7_game_size) ||
                  (b.version == 8 && b.size == v8_game_size) || (b.version == 9 && b.size == v9_game_size)))
            {
                return false;
            }
            // An older save: its checksum right after its game.
            int old_size = b.size;
            const auto* bytes = reinterpret_cast<const uint8_t*>(&b);
            int length = int(offsetof(block, game_)) + old_size;
            uint32_t sum = 0x1234;
            for(int i = 0; i < length; ++i)
            {
                sum = (sum * 31) + bytes[i];
            }
            uint32_t stored = uint32_t(bytes[length]) | (uint32_t(bytes[length + 1]) << 8) |
                              (uint32_t(bytes[length + 2]) << 16) | (uint32_t(bytes[length + 3]) << 24);
            if(stored != sum)
            {
                return false;
            }
            // The bag grew: move everything after it along, then the PC's items, and clear the new kinds.
            auto* g = reinterpret_cast<uint8_t*>(&b.game_);
            int items_at = int(offsetof(game, items_)), pc_at = int(offsetof(game, pc_items));
            int party_at = int(offsetof(game, party));
            int old_items = b.version == 9 ? v9_items_count : b.version == 8 ? 17 : b.version == 7 ? 16 : old_items_count;
            int old_pc_at = items_at + old_items, old_party_at = old_pc_at + old_items;
            int tail = old_size - old_party_at;
            for(int i = tail - 1; i >= 0; --i)
            {
                g[party_at + i] = g[old_party_at + i];
            }
            for(int i = old_items - 1; i >= 0; --i)
            {
                g[pc_at + i] = g[old_pc_at + i];
            }
            for(int i = items_at + old_items; i < pc_at; ++i)
            {
                g[i] = 0;
            }
            for(int i = pc_at + old_items; i < party_at; ++i)
            {
                g[i] = 0;
            }
            for(int i = party_at + tail; i < int(sizeof(game)); ++i)
            {
                g[i] = 0;
            }
            if(b.version < 5)
            {
                auto* r = reinterpret_cast<uint8_t*>(&b.game_.run_);
                for(int i = 0; i < int(sizeof(run)); ++i)
                {
                    r[i] = 0;
                }
            }
            if(b.version < 6)
            {
                b.game_.tower = tower_state();
                // SPIRECREST TOWN came in as area 34, so every room moved up one.
                if(b.game_.map >= 34)
                {
                    b.game_.map = int16_t(b.game_.map + 1);
                }
            }
            // TRADEWIND VILLAGE and the SAFARI ZONE came in as areas 35 and 36: the rooms moved up two more.
            if(b.version < 7 && b.game_.map >= 35)
            {
                b.game_.map = int16_t(b.game_.map + 2);
            }
            auto* x = reinterpret_cast<uint8_t*>(&b.game_.extra);
            if(b.version < 9)
            {
                // (The old save's last padding bytes landed at the start of `extra`.)
                for(int i = 0; i < int(sizeof(extras)); ++i)
                {
                    x[i] = 0;
                }
            }
            else
            {
                // `extra` is 4-aligned, so it moved a little further than the bag grew: put it in place.
                int from = v9_extra_at + 2 * (items - v9_items_count), to = int(offsetof(game, extra));
                for(int i = int(sizeof(extras)) - 1; i >= 0; --i)
                {
                    g[to + i] = g[from + i];
                }
            }
            return true;
        }

        void convert(const mon& o, pr::mon& m)
        {
            m = pr::mon();
            m.species_index = o.species_index;
            m.level = o.level;
            m.move_count = o.move_count;
            m.st = o.st;
            m.sleep_turns = o.sleep_turns;
            m.item = o.item;
            m.flags = o.flags;
            m.traits = o.traits;
            for(int i = 0; i < 4; ++i)
            {
                m.moves[i] = uint16_t(o.slots[i] & 0x3ff);
                m.pp_used[i] = uint8_t(o.slots[i] >> 10);
            }
            m.hp = o.hp;
            m.max_hp = o.max_hp;
            m.atk = o.atk;
            m.def = o.def;
            m.spa = o.spa;
            m.spd = o.spd;
            m.spe = o.spe;
            m.xp = o.xp;
            for(int i = 0; i <= nick_length; ++i)
            {
                m.nick[i] = o.nick[i];
            }
        }

        template<int To, int From>
        void convert(const bitset<From>& o, bitset<To>& b)
        {
            static_assert(To >= From);
            for(int i = 0; i < int(o.bytes.size()); ++i)
            {
                b.bytes[i] = o.bytes[i];
            }
        }

        // Field by field into save version 11 (`out` starts as a fresh game).
        void convert(const game& o, game_state& g)
        {
            g.map = o.map;
            g.x = o.x;
            g.y = o.y;
            g.facing = o.facing;
            g.party_count = o.party_count;
            g.surfing = o.surfing;
            g.starter_trio = o.starter_trio;
            g.walk_off = o.walk_off;
            g.last_heal = o.last_heal;
            g.restocked = o.restocked;
            g.trainer_id = o.trainer_id;
            g.story = o.story;
            g.money = o.money;
            g.play_frames = o.play_frames;
            g.opt = o.opt;
            for(int i = 0; i <= name_length; ++i)
            {
                g.name[i] = o.name[i];
            }
            for(int i = 0; i < items; ++i)
            {
                g.items[i] = o.items_[i];
                g.pc_items[i] = o.pc_items[i];
            }
            for(int i = 0; i < max_party; ++i)
            {
                convert(o.party[i], g.party[i]);
            }
            for(int i = 0; i < box_slots; ++i)
            {
                convert(o.box[i], g.box[i]);
            }
            for(int b = 0; b < box_count; ++b)
            {
                for(int i = 0; i <= box_name_length; ++i)
                {
                    g.box_names[b][i] = o.box_names[b][i];
                }
                g.box_wall[b] = o.box_wall[b];
            }
            convert(o.beaten, g.beaten);
            convert(o.picked, g.picked);
            convert(o.seen, g.seen);
            convert(o.owned, g.owned);
            convert(o.visited, g.visited);

            const run& r = o.run_;
            g.run.mode = r.mode;
            g.run.skip_story = r.skip_story;
            g.run.adventure = r.adventure;
            g.run.over = r.over;
            g.run.deaths = r.deaths;
            g.run.catches = r.catches;
            convert(r.encounter_used, g.run.encounter_used);
            g.run.tower_clears = r.tower_clears;
            g.run.tower_best = r.tower_best;
            g.run.tower_streak = r.tower_streak;
            g.run.grave_next = r.grave_next;
            for(int i = 0; i < graveyard_size; ++i)
            {
                convert(r.graveyard[i], g.run.graveyard[i]);
            }
            g.tower = o.tower;

            const extras& x = o.extra;
            g.extra.repel_steps = x.repel_steps;
            g.extra.rematch_day = x.rematch_day;
            convert(x.rematched, g.extra.rematched);
            for(int i = 0; i < 2; ++i)
            {
                convert(x.daycare[i], g.extra.daycare[i]);
                g.extra.daycare_steps[i] = x.daycare_steps[i];
            }
            g.extra.egg_steps = x.egg_steps;
            g.extra.egg_waiting = x.egg_waiting;
            g.extra.egg_count = x.egg_count;
            for(int i = 0; i < egg_slots; ++i)
            {
                g.extra.eggs[i] = x.eggs[i];
            }
        }
    }

    // A fresh game (introFinish).
    void fresh(game_state& g)
    {
        auto* bytes = reinterpret_cast<uint8_t*>(&g);
        for(int i = 0; i < int(sizeof(game_state)); ++i)
        {
            bytes[i] = 0;
        }
        g.walk_off = -1;
        g.last_heal = -1;
        g.restocked = -1;
        g.money = 3000;
        g.opt = options();
        g.starter_trio = 2;
        g.run = run_state();
        g.tower = tower_state();
    }

    struct save_header
    {
        char tag[8];
        int version;
        int size;
    };

    // The save blocks are ~22-31 KB, so they live in EWRAM rather than on the stack (one at a time).
    union block_storage
    {
        save_block current;
        v10::block old;

        block_storage()
        {
        }
    };
    BN_DATA_EWRAM_BSS block_storage block_buffer;

    // Reads the save into `out`: a version 11 one as it is, an older one converted (it stays in its old format in
    // SRAM until the game is saved).
    bool read_save(game_state* out)
    {
        save_header h;
        bn::sram::read(h);
        for(int i = 0; i < 8; ++i)
        {
            if(h.tag[i] != save_tag[i])
            {
                return false;
            }
        }
        if(h.version == save_version)
        {
            save_block& b = block_buffer.current;
            bn::sram::read(b);
            if(b.size != int(sizeof(game_state)) || b.checksum != checksum_of(b))
            {
                return false;
            }
            if(out)
            {
                *out = b.game;
            }
            return true;
        }
        if(h.version < 4 || h.version > 10 || ! v10::read(block_buffer.old))
        {
            return false;
        }
        if(out)
        {
            fresh(*out);
            v10::convert(block_buffer.old.game_, *out);
        }
        return true;
    }

    // The device unlock, in the SRAM's last 16 bytes.
    struct clear_mark
    {
        char tag[8];
        uint32_t cleared;
        uint32_t check;
    };
    constexpr int clear_mark_offset = 32 * 1024 - int(sizeof(clear_mark));
    static_assert(sizeof(save_block) <= clear_mark_offset, "the save must leave room for the clear mark");
    constexpr char clear_tag[8] = { 'P', 'R', 'C', 'L', 'E', 'A', 'R', '1' };

    BN_DATA_EWRAM game_state current;      // not _BSS: game_state has default member values (GCC 16 rejects them in .sbss)
    bool active = false;
    bn::random random_generator;
}

game_state& state()
{
    return current;
}

void reset_state()
{
    fresh(current);
}

bool game_active()
{
    return active;
}

void set_game_active(bool on)
{
    active = on;
}

time_of_day current_time_of_day()
{
    // Each time of day lasts 7.5 minutes of play (450 s), so a full day is 30 minutes.
    return time_of_day((current.play_frames / 60 / 450) % 4);
}

const char* time_of_day_name(time_of_day t)
{
    constexpr const char* names[] = { "MORNING", "DAY", "EVENING", "NIGHT" };
    return names[int(t)];
}

bn::random& rng()
{
    return random_generator;
}

int game_state::able_count() const
{
    int n = 0;
    for(int i = 0; i < party_count; ++i)
    {
        n += ! party[i].fainted();
    }
    return n;
}

int game_state::first_able() const
{
    for(int i = 0; i < party_count; ++i)
    {
        if(! party[i].fainted())
        {
            return i;
        }
    }
    return -1;
}

void game_state::heal_party()
{
    for(int i = 0; i < party_count; ++i)
    {
        party[i].heal();
    }
}

int game_state::party_cap() const
{
    return badges() >= 4 ? max_party : small_party;
}

bool game_state::add_mon(const mon& m, int& box_slot)
{
    box_slot = -1;
    if(party_count < party_cap())
    {
        party[party_count++] = m;
        return true;
    }
    for(int i = 0; i < box_slots; ++i)
    {
        if(box[i].empty())
        {
            box[i] = m;
            box[i].heal();      // the Box heals (saveAdv)
            box_slot = i;
            return true;
        }
    }
    return false;
}

int game_state::box_used() const
{
    int n = 0;
    for(const mon& m : box)
    {
        n += ! m.empty();
    }
    return n;
}

void game_state::add_item(item_id id, int count)
{
    items[int(id)] = uint8_t(bn::min(255, items[int(id)] + count));
}

int game_state::average_level() const
{
    if(! party_count)
    {
        return 5;
    }
    int sum = 0;
    for(int i = 0; i < party_count; ++i)
    {
        sum += party[i].level;
    }
    // Math.round(sum / n)
    return (2 * sum + party_count) / (2 * party_count);
}

int game_state::badges() const
{
    return region_badges(1);
}

int game_state::region_badges(int region) const
{
    int n = 0;
    for(int i : world_data::area_maps)
    {
        const map_def& m = world_data::maps[i];
        if(m.gate == gate_kind::GYM && m.leader_id >= 0 && m.area->region == region)
        {
            n += beaten.test(m.leader_id);
        }
    }
    return n;
}

void game_state::mark_seen(int species)
{
    seen.set(species);
}

bool game_state::mark_owned(int species)
{
    seen.set(species);
    bool first = ! owned.test(species);
    owned.set(species);
    return first;
}

bool device_cleared()
{
    clear_mark m;
    bn::sram::read_offset(m, clear_mark_offset);
    for(int i = 0; i < 8; ++i)
    {
        if(m.tag[i] != clear_tag[i])
        {
            return false;
        }
    }
    return m.cleared == 1 && m.check == 0x5eed1234u;
}

void set_device_cleared()
{
    clear_mark m;
    for(int i = 0; i < 8; ++i)
    {
        m.tag[i] = clear_tag[i];
    }
    m.cleared = 1;
    m.check = 0x5eed1234u;
    bn::sram::write_offset(m, clear_mark_offset);
}

bool roll_shiny()
{
    // Out of 16384: 4 (1/4096), 8 in ADVENTURE MODE, 5 in a NUZLOCKE run.
    // 2.0.0: twice that with the whole national POKéDEX owned.
    const run_state& run = state().run;
    int chances = run.adventure ? 8 : run.nuzlocke() ? 5 : 4;
    if(state().owned.count() >= species_count)
    {
        chances *= 2;
    }
    return rng().get_int(16384) < chances;
}

int map_region(int map)
{
    if(map < 0 || map >= world_data::maps_count)
    {
        return 1;
    }
    const map_def& m = world_data::maps[map];
    return m.is_room() ? world_data::maps[m.exit_map].area->region : m.area->region;
}

int calderra_level(int badges)
{
    return region_level(2, badges);
}

int region_cap(int region)
{
    return bn::max(1, region) * 100;
}

int region_level(int region, int badges)
{
    return bn::min(region_cap(region), region_cap(region) - 100 + 12 * badges);
}

int map_level_cap(int map)
{
    int region = map_region(map);
    if(region >= 2)
    {
        return region_level(region, state().region_badges(region));
    }
    return world_data::maps[map].level_cap;
}

species_id roamer_species(int f)
{
    constexpr species_id beasts[] = { species_id::RAIKOU, species_id::SUICUNE, species_id::ENTEI };
    return beasts[f];
}

void start_roaming(int f)
{
    state().roam[f] = 1;        // somewhere; roamers_move picks the route
    roamers_move();
}

void roamers_move()
{
    game_state& g = state();
    for(int f = 0; f < 3; ++f)
    {
        if(g.roam[f] && ! g.flags.test(f) && world_data::roam_counts[f])
        {
            g.roam[f] = int16_t(1 + world_data::roam_areas[f][rng().get_int(world_data::roam_counts[f])]);
        }
    }
}

int legend_flag(species_id legend)
{
    switch(legend)
    {
    case species_id::RAIKOU:
        return 0;
    case species_id::SUICUNE:
        return 1;
    case species_id::ENTEI:
        return 2;
    case species_id::HO_OH:
        return 3;
    case species_id::GROUDON:
        return flag::GROUDON;
    case species_id::KYOGRE:
        return flag::KYOGRE;
    case species_id::RAYQUAZA:
        return flag::RAYQUAZA;
    case species_id::DEOXYS:
        return flag::DEOXYS;
    default:
        return -1;
    }
}

bool legend_caught(species_id legend)
{
    const game_state& g = state();
    int f = legend_flag(legend);
    return f >= 0 ? g.flags.test(f) : g.has(story::LEGEND_CAUGHT);
}

bool shrines_cleared(int region)
{
    const game_state& g = state();
    for(int i : world_data::area_maps)
    {
        const map_def& m = world_data::maps[i];
        if(m.area->region == region && (m.area->flags & area_flag::SHRINE) && ! (m.leader_id >= 0 && g.beaten.test(m.leader_id)))
        {
            return false;
        }
    }
    return true;
}

int level_cap_now()
{
    const game_state& g = state();
    if(! g.run.nuzlocke())
    {
        return region_cap(g.region + 1);
    }
    int region = map_region(g.map);
    if(region >= 2)
    {
        // Calderra and later: the next leader's level (region_level of one more badge), then the top for the League.
        int b = g.region_badges(region);
        return b >= 8 ? region_cap(region) : region_level(region, b + 1);
    }
    // The first gym (in the region's order) whose leader you haven't beaten; after the eighth, the League.
    for(int i : world_data::area_maps)
    {
        const map_def& m = world_data::maps[i];
        if(m.area->region == 1 && m.area->kind == area_kind::GYM && m.leader_id >= 0 && ! g.beaten.test(m.leader_id))
        {
            return m.level_cap;
        }
    }
    for(int i : world_data::area_maps)
    {
        const map_def& m = world_data::maps[i];
        if(m.area->region == 1 && (m.area->flags & area_flag::CHAMPION) && ! (m.leader_id >= 0 && g.beaten.test(m.leader_id)))
        {
            return m.level_cap;
        }
    }
    return 100;
}

bool save_exists()
{
    return read_save(nullptr);
}

bool peek_save(game_state& out)
{
    return read_save(&out);
}

bool load_game()
{
    return read_save(&current);
}

void save_game()
{
    save_block& block = block_buffer.current;
    // The Box heals (saveAdv, tester #27).
    for(mon& m : current.box)
    {
        if(! m.empty())
        {
            m.heal();
        }
    }
    for(int i = 0; i < 8; ++i)
    {
        block.tag[i] = save_tag[i];
    }
    block.version = save_version;
    block.size = int(sizeof(game_state));
    block.game = current;
    block.checksum = checksum_of(block);
    bn::sram::write(block);
}

}
