# Party Royale — Game Boy Advance build

A real GBA ROM of Party Royale, built with [Butano](https://github.com/GValiente/butano) (C++). It runs in
emulators (Delta, mGBA) and on real hardware from a flash cart. Saves go to cartridge SRAM.

This is the **proof of concept**: the whole pipeline from the web game to a ROM, on a small slice of
the game.

## What's in this build

- **Title screen**, CONTINUE / NEW GAME, the professor's welcome, and choosing a starter (Treecko,
  Torchic or Mudkip).
- **Duskmere Hollow and Route 1: Fernway Trail**, with the same layout, art and people as the web game,
  joined seamlessly like Emerald's map connections.
- **Walking** with Emerald timing: a step is 16 frames, a run (hold B) is 8, and a new direction turns on the
  spot first. Also bumping, ledge hops, tall grass covering your legs, and talking to people and signs (A).
  Item balls on the route give POKé BALLS.
- **Wild battles** in the tall grass (8% a step), using the web game's species, stats, learnsets, type chart,
  damage formula, accuracy, status effects, catch odds and EXP. You also get level-ups, new moves and
  evolution.
- **The POKéMON CENTER door** heals your party. If you white out, you're sent back there.
- **START menu**: party (summary, set lead), bag, save, quit to title.

Not yet: building interiors, trainers and gyms, wandering NPCs, party-vs-pack battles (this build fights 1v1
with your lead), bag items besides balls, PC boxes, name entry, music and sound. Areas past Route 1
show a "not in this test build yet" message.

## Building

The ROM is built from the web game's own data, so the two stay in step:

1. `tools/export.js` loads `../js/*.js` in Node (with the browser stubbed out). It runs the real map
   generators and draws each area exactly as the web overworld does. It also exports people art, species,
   moves and the type chart.
2. `tools/build_assets.py` turns that into GBA data in `generated/`:
   - shared 8x8 tiles (flips de-duplicated) packed into 4bpp palette banks
   - 16x16 metatiles and per-area maps
   - sprites, battle backgrounds and C++ headers
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

Controls: D-pad to move, B (held) to run, A to talk/read/confirm, B to cancel, START for the menu.

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
