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
2. **Surf — DONE (0.12.0).** HM03 SURF from Sable (badge 2); ride water with an original sea-creature sprite; water encounters (#10: land/water pools split); OLD ROD from fishermen and fishing (#17); real lakes on Mirror Lake, Hollow Bluffs and Glimmer Coast; sea routes: Route 7 Stormwake Strait (from Portmere) and the Glimmer Sea side area.
3. **Four new towns + gyms — DONE (0.12.0).** Voltara City (Juno, Electric), Route 8 Thornwood Path, Mossgrove Town (Bryn, Grass), Route 9 Frostpine Ridge (snow), Tempest Hideout (Vesper scene; Wren battle and turn), Rimefall Town (Hale, Ice), Route 10 Skyreach Cliffs, Aeriepeak City (Corvin, Dragon/Flying) = 8 badges. Aeriepeak is the end of content until Phase 4 (Dive).
4. **Dive — DONE (0.13.0).** HM08 DIVE from Hale (7th badge). Dark dive spots on Route 7 and the Glimmer Sea; an underwater layer (Stormwake Depths, Coral Trench, Glimmer Deep: seabed, rock, kelp encounters, light shafts to surface; not on the region map); the Sunken Shrine (Wren heals you, Admin Vesper battle, Lugia legendary battle; if caught it is gone for good, if not it can be retried). Beating the shrine ends the storms.
5. **Ending — DONE (0.14.0).** Victory Road (needs 8 badges; Ace Trainers), the Pokémon League (Elite Four Morrow/Dark, Brakk/Fighting, Ferrin/Steel, Aurelle/Psychic-Fairy in gated rooms; whiteout resets the run; Champion Wren), Hall of Fame and credits, then home. The first clear sets the device unlock for Adventure Mode (partyroyale_cleared).
   - **Music:** original chiptune soundtrack in the GBA handheld style, synthesized live with the Web Audio API
     (two pulse channels, a wave/triangle bass and noise drums, like the handheld's sound chip), no audio files.
     All melodies are new compositions, not copies of existing game themes. Tracks: ambient themes (towns, routes,
     sea/surfing, caves, underwater), battle themes (wild, trainer, Gym Leader/rival, Elite/Champion, legendary),
     short jingles (victory, level up, heal, badge, evolution) and a credits theme. Crossfades between areas,
     follows the SOUND option, with a separate MUSIC volume in OPTION.
6. **Nuzlocke mode (web: planned — not started; GBA: done in 1.1).** Chosen when starting a new story (NORMAL / NUZLOCKE), plus a
   SKIP STORY TEXT option for players who already know the route:
   - Classic rules, enforced by the game: a Pokémon that faints is dead (moved to a graveyard; it can't be used
     or revived); only the first wild encounter in each area can be caught (a failed catch or a knockout uses it
     up); every catch must be nicknamed; whiting out (the whole party dead) ends the run.
   - Common clauses: dupes clause (skip species you already have), shiny clause; level caps at each gym leader.
   - A Nuzlocke HUD: encounters used per area on the map, deaths and a run summary on the trainer card.
   - Skip story text: scenes and calls fast-forward or complete themselves, and key story items (e.g. SURF, DIVE)
     are still granted at the same story points so the run can be finished.
7. **Endgame: Adventure Mode (web: planned, not started; GBA: done in 1.1, the tower's guide by the League's doors).** Unlocked by the first clear of the main story (Phase 5).
   - **Continuing:** after the credits the same save carries on as ADVENTURE MODE (shown on the title and the save
     info). The whole region stays open, with the Challenge Tower as the new goal.
   - **The Challenge Tower:** repeatable Elite Four-style runs (four elite trainers and a tower master in a row, no
     healing except items you carry). Each clear raises the tower's rank, so the next run is harder (higher levels,
     bigger and smarter teams, held items). Each clear earns a battle against one legendary from the tower's pool,
     where it can be caught; losing or fleeing puts it back. A caught legendary leaves the pool for good (so does a
     Lugia caught in the story), until every legendary in the game's Pokémon list is caught. The tower shows your
     rank, best streak and legendaries left.
   - **NEW ADVENTURE MODE:** after the first clear, the title menu offers NEW ADVENTURE MODE: it skips the main story
     entirely. You draft a team of 6 Pokémon at level 50 (the draft screen from Free Battle); the other 4 party slots
     stay empty until you catch more. The run starts in the post-game with the region open and the tower available.
     Only this mode starts at level 50 with a draft; a normal NEW GAME is unchanged.
   - **Not in Nuzlocke:** the tower and Adventure Mode are never available in a Nuzlocke run, and NEW ADVENTURE MODE
     can't be combined with Nuzlocke.
8. **Platform-specific UI (planned — not started; web only, not applicable to the GBA).** The interface adapts to the device instead of one layout for all:
   - Phone portrait: the handheld as now; phone landscape: a wide layout (screen in the middle, D-pad left, A/B right).
   - Tablet: a larger screen and controls, split layouts where there is room.
   - Desktop: keyboard-first (key hints on screen, no need for on-screen buttons), mouse support in menus.
   - Game controllers via the Gamepad API (D-pad/stick, A/B, START/SELECT) on any platform.
   - Detected from input type and screen size (pointer, orientation, touch points), with a manual override in OPTION.

## Tablets (in order of the legend)
1. Whisperwood — "When sky and sea raged as one, we sang the silver guardian to sleep."
2. Mirror Lake — "The guardian dreams in a shrine beneath the waves. Let none disturb its rest."
3. Glimmer Coast — "Only one who can walk beneath the sea will find the shrine."
4. Old Quarry — "Should greed wake the guardian, the storms will never end."
