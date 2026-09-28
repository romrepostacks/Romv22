# Party Royale — Story & World Plan (Vellorin)

All names are placeholders the owner can change. Original story; Pokémon species are the game's existing pool.

## The three threads
- **Team Tempest (villains).** Want to wake **Lugia**, guardian of the sea and sky, to command the storms.
  Admin **Vesper** leads them in Vellorin. Grunts appear from Marrow Pass onward; their HQ is hidden in Portmere.
- **The Tidewardens (ancient mystery).** A lost people who sang Lugia to sleep in a **Sunken Shrine** beneath the sea.
  Their stone tablets across Vellorin tell the legend piece by piece; PROF. HAWTHORN follows along by POKéNAV call.
- **Wren (rival).** Driven to be strongest; tempted by Vesper's offer of power, turns back, helps at the shrine,
  and waits at the end as the Champion.

## Phases
1. **Story groundwork — DONE (0.10.0).** Story flags (`adv.story`), area-entry scenes (`SCENES`, `storyEnter`),
   Tempest grunts on Marrow Pass / Hollow Bluffs / Emberflow Delta, tablets on the four side areas, POKéNAV calls
   after badges 1, 2 and 4, story townsfolk in the gym towns, Wren's post-battle lines, **Portmere Harbour**
   (Vesper's dock scene + grunt battle). The ferry is shut: end of content for now.
1.5. **Route design pass (from tester feedback #11, #14, #15) — DONE (0.11.0).** Routes become guided corridors
   instead of open fields: forest walls with the path through them, side paths and pockets to explore; tall grass
   you have to cross at points on the main path; trainers watching the path; ledges as real one-way shortcuts;
   rocks that shape the path; item variety (Potions, Antidotes, Super Potions, Poké Balls) at the end of side paths;
   a lower wild encounter rate.
2. **Surf — PAUSED until the owner says go** (after 1.5; folds in #10 land/water encounter split and #17 bigger water + fishing). Sable (badge 2) gives SURF; ride water tiles; surf sprite; water encounters; sea routes out of
   Glimmer Coast / Portmere.
3. **Four new towns + gyms** (Electric, Grass, Ice, Dragon/Flying → 8 badges) and routes between them;
   Tempest's hideout (with Wren's turn).
4. **Dive.** DIVE from the 6th/7th gym; dark-water dive spots on sea routes; underwater maps (seaweed works like
   tall grass, their own Pokémon, pixel-art seabed); the **Sunken Shrine** and the Lugia climax with Wren's help.
5. **Ending.** Victory Road, the Pokémon League (four elite trainers + Champion Wren), credits.
6. **Platform-specific UI (planned — not started).** The interface adapts to the device instead of one layout for all:
   - Phone portrait: the handheld as now; phone landscape: a wide layout (screen in the middle, D-pad left, A/B right).
   - Tablet: a larger screen and controls, split layouts where there is room.
   - Desktop: keyboard-first (key hints on screen, no need for on-screen buttons), mouse support in menus.
   - Game controllers via the Gamepad API (D-pad/stick, A/B, START/SELECT) on any platform.
   - Detected from input type and screen size (pointer, orientation, touch points), with a manual override in OPTION.
7. **Nuzlocke mode (planned — not started).** Chosen when starting a new story (NORMAL / NUZLOCKE), plus a
   SKIP STORY TEXT option for players who already know the route:
   - Classic rules, enforced by the game: a Pokémon that faints is dead (moved to a graveyard; it can't be used
     or revived); only the first wild encounter in each area can be caught (a failed catch or a knockout uses it
     up); every catch must be nicknamed; whiting out (the whole party dead) ends the run.
   - Common clauses: dupes clause (skip species you already have), shiny clause; level caps at each gym leader.
   - A Nuzlocke HUD: encounters used per area on the map, deaths and a run summary on the trainer card.
   - Skip story text: scenes and calls fast-forward or complete themselves, and key story items (e.g. SURF, DIVE)
     are still granted at the same story points so the run can be finished.

## Tablets (in order of the legend)
1. Whisperwood — "When sky and sea raged as one, we sang the silver guardian to sleep."
2. Mirror Lake — "The guardian dreams in a shrine beneath the waves. Let none disturb its rest."
3. Glimmer Coast — "Only one who can walk beneath the sea will find the shrine."
4. Old Quarry — "Should greed wake the guardian, the storms will never end."
