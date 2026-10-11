# AGENTS.md

Guide for any AI coding agent (Claude, Codex, Copilot, Cursor, Gemini...) or new contributor working in this repo.
Read this first, then `gba/README.md` for the full build and feature list.

## What this is

**Poké Legends: Lands of Nine** (called Party Royale before 2.0.1), an original Pokémon-inspired game for the
**Game Boy Advance**, written in C++ with [Butano](https://github.com/GValiente/butano). The GBA ROM is the only
supported version; the old browser game was retired in 1.7 and its `js/` files survive only as the ROM's data source.

The long-term plan is **9 regions**, one per major version, each with its own story, maps and a level cap 100 higher
than the last (region 1 = 100, region 2 = 200, region 3 = 300 ...).

| Version | Region | Plan |
|---|---|---|
| 1.x | Vellorin | `STORY.md` |
| 2.0.0 | Calderra (Raikou/Entei/Suicune), level 200 | `docs/2.0.0/` |
| 3.0.0 | The Sundered Isles (Groudon/Kyogre), level 300 | `docs/3.0.0/` |
| 4.0.0 | The Skyreach (Rayquaza/Deoxys), level 400 | `docs/4.0.0/` |
| 5.0.0 | Genova (Mewtwo/Mew, first homebrew species), level 500 | `docs/5.0.0/` |
| 6.0.0 | Aeterna (Dialga/Palkia, one valley in two eras), level 600 | `docs/6.0.0/` |
| 7.0.0 | The Hollow Lands (Giratina, a reverse side), level 700 | `docs/7.0.0/` |
| 8.0.0 | Tempesta (the three birds and Lugia, seasons), level 800 | `docs/8.0.0/` |
| 9.0.0 | Origin (Arceus, the eight Champions again), level 900 | `docs/9.0.0/` |

## Where things are

```
├── AGENTS.md            this file
├── README.md            short overview for people
├── STORY.md             region 1 story and the original phase plan
├── docs/
│   ├── <version>/       one folder per major version: plan (.md) and region map (.svg source + .png)
│   └── audits/          dated reviews of structure and capacity
├── gba/                 THE GAME
│   ├── src/, include/   engine: overworld (pr_overworld, pr_field), battle (pr_battle, pr_mon),
│   │                    menus (pr_screens, pr_ui), save (pr_state), title, audio
│   ├── tools/           build pipeline (export.js, build_assets.py), tests (walktest/, savetest/), art scripts
│   ├── data/            generated item/TM data (extras.json), title art
│   ├── news_gba.json    the in-game WHAT'S NEW: player-facing changelog, newest first
│   ├── roms/            released ROMs (force-added; *.gba is gitignored)
│   └── butano/          engine submodule, never edit
├── js/                  game DATA the ROM is built from (not a web app any more)
│   ├── app.js           all regions' maps, story, trainers, species tables, DEX_NUM
│   ├── dexdata.js       generated from PokeAPI: types, stats, learnsets (tools/build-dexdata.js)
│   ├── dexinfo.js, moveextra.js, tileart.js, music.js   generated or hand-made data
├── sprites/pokemon/     <num>.png front, back/, shiny/ (PokeAPI numbering; see sprites/README.md)
└── tools/               generators for js/dexdata.js, js/dexinfo.js, js/tileart.js; tools/art/ draws tile art
```

Data flow: `js/*.js` + `sprites/` → `gba/tools/export.js` (runs the JS in Node) → `gba/tools/build_assets.py`
→ `gba/generated/` (C++ headers, tiles; not committed) → Butano → `gba/party-royale.gba`.

## Build and test

All commands run from `gba/`. Full details in `gba/README.md`.

```
git submodule update --init                  # Butano
sh tools/setup-toolchain.sh                  # once; Wonderful toolchain (devkitARM also works)
make -j8                                     # -> party-royale.gba
python3 tools/walktest/walktest.py OUT       # walks a party on every map in mGBA, reports crashes (~4 min)
sh tools/savetest/run.sh                     # old save converts and round-trips (when the save changes)
```

Tests need `libmgba-dev`. There is no unit test suite; walktest is the smoke test before any release.

## Rules that are easy to break

- **Never renumber anything that a save stores.** Species, moves, items, maps, trainers and item balls are saved
  by index. New ones are **appended** after the existing ones (a new region's maps/trainers go after the last
  region's). Changing an existing index corrupts players' saves.
- **Save layout is fixed** (save version 11, sized for all 9 regions, `gba/include/pr_state.h`, `static_assert`
  on the size). New fields come out of the `spare` bytes; zero must mean "not set". If you must change the layout,
  bump the save version and add a migration in `gba/src/pr_state.cpp`, then run savetest.
- **Older regions stay as they are**: a new region's level cap and harder AI apply only inside it.
- **Versions**: `X.0.0` new region, `0.X.0` major feature, `0.0.X` fixes. Always three parts.
- **Each release** updates `gba/news_gba.json` (player-facing, newest first), the version line in
  `gba/README.md`, and adds the ROM as `gba/roms/lands-of-nine-X.Y.Z.gba` (`git add -f`).
- **GBA limits**: ROM max 32 MB (3.0.0 is 17.4 MB), SRAM 32 KB, map tilesets use up to 14 of 16 BG palette
  banks, icons share 6 sprite palettes. `game_state` globals must be `BN_DATA_EWRAM`.
- Don't add web features; `js/` is data only.

## Branches

`main` is what players get. Feature work goes on its own branch and reaches `main` only with the owner's OK.
`gba-poc` is an old staging branch and may be behind `main`.

## Generated vs hand-written

Hand-written: `gba/src`, `gba/include`, `gba/tools`, `js/app.js`, `js/music.js`, `STORY.md`, `docs/`.
Generated (rebuild, don't hand-edit): `js/dexdata.js`, `js/dexinfo.js`, `js/moveextra.js`, `js/tileart.js`,
`gba/data/extras.json`, `gba/data/title_*.png`, `gba/generated/`. Each generator's header says how to run it.
