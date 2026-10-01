# Party Royale — Game Boy Advance build

A real GBA ROM of Party Royale, built with [Butano](https://github.com/GValiente/butano) (C++). It runs in
emulators (Delta, mGBA) and on real hardware from a flash cart. Saves go to cartridge SRAM.

Test build **0.2**: the opening of the game (home town and Route 1), built on the web game's own data.

## What's in this build

- **New game the web game's way:** the professor's welcome on a dark stage (with LOTAD), naming yourself,
  then on Route 1 he runs up chased by a ZIGZAGOON. You pick your partner from his bag (Treecko, Torchic or
  Mudkip), fight the ZIGZAGOON, and he thanks you. Emerald's main menu shows the save's details under
  CONTINUE.
- **Duskmere Hollow and Route 1: Fernway Trail**, with the same layout, art and people as the web game,
  joined seamlessly like Emerald's map connections. People wander, and route trainers spot you, show a "!",
  walk up and battle you.
- **Building interiors:** the POKéMON CENTER (the nurse heals you, and there's the PC with a BOX), the POKé
  MART (a free gift first, then BUY and SELL with money) and the houses, including home, where Mom heals
  you.
- **Battles as in the web game:** your whole party against a wild pack (1 up to half your party, at most 4)
  or a trainer's team. Every Pokémon gets a command, then everyone acts in speed order. You pick targets and
  can use the BAG (POKé BALLS and medicine). The web game's damage, type chart, accuracy, status effects,
  catch odds, EXP, level-ups, new moves, evolution and prize money all apply.
- **Emerald's screens and menus:** text prints letter by letter with the ▼ prompt. The START menu, YES/NO
  boxes, battle command and move windows, party screen, summary, bag and save window all sit where Emerald
  puts them, styled after the web game.
- **Walking** with Emerald timing: a step is 16 frames, a run (hold B) is 8, and a new direction turns on the
  spot first. Also ledge hops, tall grass over your legs, signs and item balls.
- **Saving** to cartridge SRAM from START > SAVE.

Not yet: gyms and the rest of the region, the rival, PP and abilities, the POKéDEX screen, music and sound.
Areas past Route 1 show a "not in this test build yet" message. Saves from 0.1 aren't compatible.

## Building

The ROM is built from the web game's own data, so the two stay in step:

1. `tools/export.js` loads `../js/*.js` in Node (with the browser stubbed out). It runs the real map
   generators and draws each area and room exactly as the web game does. It also exports people art,
   trainers, items, species, moves and the type chart.
2. `tools/build_assets.py` turns that into GBA data in `generated/`:
   - a tileset for the outdoors and one per kind of room: 8x8 tiles (flips de-duplicated) packed into 4bpp
     palette banks, and 16x16 metatiles
   - every map's metatiles and tile behaviours, with signs, doors, people, trainers and item balls
   - species, moves, items, window frame tiles, sprites, backgrounds and C++ headers
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
