# Poké Legends: Lands of Nine — Game Boy Advance build

A real GBA ROM of Poké Legends: Lands of Nine (called Party Royale before 2.0.1), built with [Butano](https://github.com/GValiente/butano) (C++). It runs in
emulators (Delta, mGBA) and on real hardware from a flash cart. Saves go to cartridge SRAM.

Version **9.0.0**: the whole game, from the professor's welcome to the Hall of Fame, built from the retired
web game's data (`js/`, `sprites/`), plus the two story phases planned after it (STORY.md phases 6 and 7):
NUZLOCKE mode and ADVENTURE MODE with the CHALLENGE TOWER, shiny Pokémon, and PP, stat and weather moves.
The GBA is now the only supported version of the game.

## What's in it

- **Calderra (2.0, Adventure Mode):** a second region reached by ferry from PORTMERE, themed on RAIKOU, ENTEI and
  SUICUNE. Three branches from PORT CALDER open in any order (STORMREACH, the MISTRAL LAKES, CINDERDEEP) with
  eight gyms, TEAM ECLIPSE, the rival KAI, HO-OH's ASHEN TOWER and its own LEAGUE. Every Pokémon up to Gen 9,
  a Gen 8/9 starter from PROF. EMBER, roaming beasts, a MOVE TUTOR, and a level cap of 200 with smarter trainers.
- **All of Vellorin:** every town, route, cave and sea area of the web game (34 areas, plus three GBA-only ones) and every building
  inside them, with the same layouts, art and people, joined seamlessly like Emerald's map connections.
  Water and flowers animate, and each area has its weather (rain, snow, ash, fog, the depths) and the time
  of day tinting the world, as in the web game.
- **The story:** the professor's welcome, your partner from a random trio of starters, your rival WREN,
  TEAM TEMPEST and ADMIN VESPER, the TIDEWARDENS' tablets, the professor's calls, the eight gyms with their
  juniors, the guardian LUGIA at the Sunken Shrine, VICTORY ROAD, the ELITE FOUR and the CHAMPION, then the
  Hall of Fame and the credits.
- **Field moves:** SURF (from SABLE's badge), DIVE down to the sea floor and back (from HALE's), and the OLD
  ROD from a fisherman.
- **Battles as in the web game:** your whole party against a wild pack or a trainer's team, every Pokémon
  commanded, then everyone acting in speed order. Damage, the type chart, accuracy, status, abilities
  (type immunities and boosts, Iron Fist, Merciless, Corrosion, Disguise), held items (LEFTOVERS,
  LIFE ORB, FOCUS SASH, SITRUS BERRY, CHOICE SCARF), EXP and EXP SHARE, level-ups, new moves, evolution
  and prize money all follow the web game. Every move has the web game's animation.
- **Moves beyond the web game (GBA only):** every move has its real PP (shown in battle and on the
  summary; a POKéMON CENTER, home or the PC restores it; out of PP, a Pokémon STRUGGLEs). The learnsets
  gain the main series' stat moves (GROWL, LEER, SWORDS DANCE, CALM MIND, DRAGON DANCE...), healing moves
  (RECOVER, ROOST, SYNTHESIS...) and the four weathers (SUNNY DAY, RAIN DANCE, SANDSTORM, HAIL, for five
  turns). Stat stages work as in the main series, as do side effects (PSYCHIC lowering SP. DEF, CLOSE
  COMBAT lowering the user's), draining (ABSORB, GIGA DRAIN), recoil (DOUBLE-EDGE) and spread moves that hit
  every foe (EARTHQUAKE, ROCK SLIDE, GROWL) at 3/4 power. FIGHT asks for the target first, then shows the
  moves coloured for it: red super effective, yellow a normal hit, dark not very effective, grey no effect
  (or it would fail, or no PP); B goes back to change the target. Data: `tools/build_moves.js` from
  PokeAPI's CSVs into `data/moves_extra.json`.
- **Fast travel (GBA only):** on the POKéNAV map (START > POKéNAV), move to any town or route you've
  been to and press A to travel there: to the door of its POKéMON CENTER, or onto the path where you'd
  enter a route. (Not from inside the CHALLENGE TOWER or the POKéMON LEAGUE.)
- **Extras (GBA only):** TRADEWIND VILLAGE below WISPGATE CITY (open once its Gym Leader is beaten), whose
  TRADER swaps a Pokémon for a random one of about the same level; the SAFARI ZONE above PORTMERE HARBOUR
  ($5000 to enter), where every species, legendaries included, is equally likely; hidden items (a glint
  now and then; face the spot and press A) with RARE CANDIES, GREAT BALLS and ULTRA BALLS; GREAT BALLS
  (x1.5) and ULTRA BALLS (x2) in the POKé MART from the 2nd and 5th badges.
  The MASTER BALL never fails; WREN gives you one at the Sunken Shrine, before the guardian.
- **Catching as in Emerald:** each species' real catch rate with Gen 3's formula (the guardian uses the web
  game's odds). Then the POKéDEX registration page and the nickname prompt.
- **Menus:** the POKéDEX (list, INFO, AREA, SIZE), the party screen (6 Pokémon, 10 from your fourth badge),
  summary, bag, the PC's 14 boxes (WITHDRAW, DEPOSIT, MOVE, RELEASE, names and wallpapers), the trainer
  card, the POKéNAV region map, OPTION (text speed, sound, music, EXP SHARE, weather) and saving.
- **NUZLOCKE mode (phase 6):** chosen at NEW GAME (NORMAL / NUZLOCKE), with a SKIP STORY TEXT option for
  either. The rules are enforced: a Pokémon that faints has fallen and goes to the graveyard (no REVIVE);
  only the first wild encounter in each area can be caught, and one already owned doesn't count (dupes
  clause); every catch is nicknamed; your Pokémon can't level past the next GYM LEADER; whiting out ends the
  run. The region map marks spent areas, the TRAINER CARD's back keeps deaths, catches and encounters, and
  the PC has a GRAVEYARD.
- **ADVENTURE MODE (phase 7):** after the credits a normal save carries on as ADVENTURE MODE, and the road
  south of DUSKMERE HOLLOW opens (it's gated until you're CHAMPION) to SPIRECREST TOWN, with its own
  POKéMON CENTER, MART and the CHALLENGE TOWER. Inside, five themed floors (Fire, Water, Electric, Ghost,
  and the TOWER MASTER's Dragon floor) each hold one trainer with a team of the floor's type; there's no
  healing inside but what you carry, and leaving ends the challenge. Each clear raises the rank (higher
  levels, bigger teams, held items, smarter foes). At the summit, the SUMMONING STONE calls a legendary
  from the pool of 68 into a chamber themed for its type; a caught one leaves the pool for good. The first
  clear also unlocks NEW ADVENTURE MODE on the title: skip the story, draft 6 Pokémon at level 50 (the FREE
  BATTLE draft) and start in SPIRECREST TOWN as CHAMPION. Neither is offered in a NUZLOCKE run.
- **Shiny Pokémon:** any wild Pokémon (and your starter, and the tower's legendary) can be shiny: 1 in
  4096, twice that in ADVENTURE MODE and 1.25 times in a NUZLOCKE run. Shinies use their real shiny colours
  (from PokeAPI's sprites, in sprites/pokemon/shiny) in battle and on the summary (marked
  SHINY), and sparkle when they appear. In a NUZLOCKE run, a shiny can always be caught (the shiny clause).
- **Skip story text:** scenes, the professor's calls and the rivals' goodbyes complete themselves; SURF,
  DIVE and the other story rewards still come at the same points.
- **Items and side activities (1.8):** evolution stones and the LINKING CORD for trade evolutions, 73 TMs
  (by badges) and the MOVE TUTOR in every POKéMON CENTER, REPELS and the ESCAPE ROPE, the GOOD and SUPER
  RODS, once-a-day rematches with route trainers, and the DAY CARE in CINDERGATE TOWN, which raises two
  POKéMON and finds EGGS (no genders: any two that share an egg group, or DITTO, can breed).
  The EGGS you carry show in the party screen with their hatch progress (1.8.1), with an EGG icon (1.9.0).
- **Held items (1.9.0):** give your POKéMON LEFTOVERS, a LIFE ORB, CHOICE SCARF, FOCUS SASH or SITRUS BERRY
  from the party screen (ITEM > GIVE / TAKE) or the BAG. The MART's HELD ITEMS counter sells them from the
  2nd badge (LIFE ORB and CHOICE SCARF from the 4th). The FOCUS SASH and SITRUS BERRY are used up.
- **FREE BATTLE** and **WHAT'S NEW** on the title screen, as in the web game.
- **Music and sound:** the web game's tunes and sound effects on the GBA's sound chip.
- **Saving** to cartridge SRAM from START > SAVE, plus autosaves where the web game autosaves. 1.0 to 1.8.1
  saves load in 2.0.x; saves from 0.3 and earlier aren't compatible.

Phase 8 (per-device layouts and controllers) was planned for the web app, which has been retired.

Catching differs from the web game on purpose: the web game uses a flat 25% chance at full HP for every
species, so a ball thrown at a healthy Pokémon usually failed. The GBA build uses Emerald's formula instead
(for example about 34% at full HP for Route 1's common Pokémon, 78% at half HP and nearly certain at 1/5).

## Building

The ROM is built from the web game's own data, so the two stay in step:

1. `tools/export.js` loads `../js/*.js` in Node (with the browser stubbed out). It runs the real map
   generators and draws each area and room exactly as the web game does. It also exports people art,
   trainers, items, species, moves and the type chart.
2. `tools/build_assets.py` turns that into GBA data in `generated/`:
   - a tileset per area (with strips of its neighbours) and per kind of room: 8x8 tiles (flips
     de-duplicated) packed into 4bpp palette banks, 16x16 metatiles and animation frames
   - every map's metatiles and tile behaviours, with signs, doors, people, trainers and item balls
   - species, moves (with their animation scripts), items (with TMs, evolution items, egg groups and
     TM compatibility from `data/extras.json`, made once by `tools/build_extras.py` from PokeAPI's CSVs), music, window frame tiles, sprites, backgrounds
     and C++ headers
3. Butano compiles `src/` and the generated data into `party-royale.gba`.

The Makefile runs steps 1–2 automatically before every build. It skips them when nothing changed.

Requirements: Node.js, Python 3 with Pillow, and the Wonderful GBA toolchain with BlocksDS's tools. On
Linux x86_64:

```
git submodule update --init      # Butano
sh tools/setup-toolchain.sh      # once
export WONDERFUL_TOOLCHAIN=/opt/wonderful PATH=/opt/wonderful/bin:$PATH
make -j8
```

To check that no map crashes, `python3 tools/walktest/walktest.py` (needs `libmgba-dev`) drops a level 100
party at every map's spawn point, walks it around at random in mGBA and reports any Butano assert.

(devkitARM also works with Butano: install devkitPro's `gba-dev` and drop the `WONDERFUL_TOOLCHAIN`
export.)

## Playing

- **Delta (iPhone/iPad):** put `party-royale.gba` in Files, then import it in Delta (+ → Files). Delta
  detects the SRAM save automatically.
- **mGBA:** open the file.
- **Real hardware:** copy it to a flash cart (EverDrive GBA, EZ-Flash). Set the save type to SRAM if the
  cart asks.

Controls: D-pad to move, B (held) to run, A to talk/read/confirm, B to cancel, START for the menu. On the
naming screen, SELECT switches case, B deletes and START jumps to OK.

## Layout

```
gba/
├── Makefile              Butano project; runs tools/build_assets.py first
├── butano/               Butano engine (git submodule)
├── include/, src/        the game: overworld, battle, title, UI, Pokémon rules, save
├── tools/export.js       web game -> data.json + area pictures
├── tools/build_assets.py data -> GBA tiles, palettes, sprites, C++ headers
└── generated/            build output of the tools (not committed)
```
