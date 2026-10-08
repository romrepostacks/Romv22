#ifndef PR_SCENES_H
#define PR_SCENES_H

#include "bn_vector.h"
#include "pr_ids.h"
#include "pr_game_types.h"

namespace pr
{

enum class encounter_kind : uint8_t
{
    WILD,           // a pack from the tall grass, a cave floor or the water (startWildBattle)
    FIXED,          // a scripted wild Pokémon: the professor's ZIGZAGOON, a bite on the line, the guardian
    TRAINER         // a trainer, rival, Gym Leader, Elite Four, Champion or CHALLENGE TOWER floor
};

// What the overworld hands to a battle.
struct encounter
{
    encounter_kind kind = encounter_kind::WILD;
    int map = 0;                    // trainer: the map they're on, and which of its trainers
    int trainer_index = 0;
    species_id species[4] = {};     // wild: the pack
    int count = 0;
    int level = 2;
    bool legendary = false;         // the guardian: tougher, harder to catch (state.legendary)
    bool water = false;
    bool scripted = false;          // the professor's ZIGZAGOON: never a NUZLOCKE encounter
    bool tower_legend = false;      // the CHALLENGE TOWER's summoned legendary (shiny as rolled at the stone)
    bool rematch = false;           // a route trainer battling again (GBA 1.8): stronger, once a day
    int roamer = -1;                // Calderra's roaming beast (legend_flag), or -1
};

enum class battle_outcome
{
    WON,
    RAN,
    CAUGHT,
    WHITED_OUT
};

// What happened, for afterStory: who was caught (nickname and POKéDEX registration).
struct battle_report
{
    battle_outcome outcome = battle_outcome::WON;
    bn::vector<int16_t, 4> caught_party;      // party index, or 100 + box slot
    bn::vector<uint16_t, 4> dex_new;          // species index
    bool trainer_beaten = false;
    bool run_over = false;                    // NUZLOCKE: the party whited out
};

// Title screen and new game; returns once a game is loaded or started (true), or after FREE BATTLE (false).
bool title_scene();

// Walks the overworld until a battle starts (returns true and fills `battle`) or the player quits to the
// title (returns false).
bool overworld_scene(encounter& battle, const battle_report* last);

battle_report battle_scene(const encounter& battle);

// FREE BATTLE: draft a side from the whole dex and fight a random one.
void free_battle_scene();
// The draft screen alone (NEW ADVENTURE MODE): count Pokémon and their held items; false if backed out.
bool draft_team(int count, uint16_t* species_out, held_item* items_out);

// The next overworld scene starts a CONTINUEd game (startAdventure: the starter event or the area's story).
void set_just_loaded();
// The next overworld scene starts a new game (introFinish's directions).
void set_new_game_started();

}

#endif
