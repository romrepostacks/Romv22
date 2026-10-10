# Filesystem audit and 9-region capacity check (2026-10-10)

What was checked: the repo `romrepostacks/Romv22` (main, gba-poc, and the 3.0.0 branch `claude/project-thread-mnqoox`) and the project folder. All figures come from the 3.0.0 branch unless noted. Nothing was changed in the repo.

## Where things live today

| Place | What's in it | Notes |
|---|---|---|
| `gba/src`, `gba/include` | The game engine (C++/Butano), about 17k lines | Healthy. Biggest files: `pr_field.cpp` 3.7k lines, `pr_screens.cpp` 3.2k, `pr_battle.cpp` 2.4k |
| `gba/tools` | ROM build pipeline (`export.js`, `build_assets.py` 2k lines), walktest, savetest, art scripts | Healthy |
| `gba/data` | Generated item/TM data, title art | Fine |
| `gba/roms` | **Every released ROM, committed to git** (9 files, 115 MB) | Problem, see 1 |
| `js/app.js` | **All three regions' maps, story, trainers and species**, plus leftover web-app UI code | 6.7k lines, 481 KB. Problem, see 2 |
| `js/dexdata.js` etc. | Generated Pokémon/move data | Fine |
| `sprites/pokemon` | 4,152 sprites (front, back, shiny), 17 MB | Fine |
| `docs/2.0.0` | 2.0.0 plan and Calderra map | 3.0.0 plan is not here, see 3 |
| `/mnt/project-files/plans` | 2.0.0 and 3.0.0 plans and maps | Duplicates `docs/2.0.0` |
| `/mnt/project-files/roms` | 1.8, 1.8.1, 1.9.0, 1.9.1, 3.0.0 | Missing 1.7, 2.0.0, 2.0.1 |

## Structural issues, most important first

1. **ROMs in git.** Every release is a 10 to 17 MB binary committed forever. The packed repo is already 23 MB and grows about 4 MB per release, which every clone and every new thread pays. Fix: publish ROMs as GitHub Releases (one per version, ROM attached) and stop committing them. Old ones can stay in history.
2. **All region content in one web-app file.** `js/app.js` holds Vellorin, Calderra and the Sundered Isles side by side, mixed with retired browser UI code (modals, CSS tile drawing, phone layout). Six more regions would push it past 15k lines. Fix: split into `js/regions/vellorin.js`, `calderra.js`, `sundered.js` (each region's areas, trainers, story), keep species/moves in their own file, and delete the dead web UI. `export.js` already loads files into a sandbox, so it only needs the new file list.
3. **Plans live in two places.** 2.0.0's plan is in both `docs/2.0.0/` and the project folder; 3.0.0's is only in the project folder. Pick one: `docs/<version>/` in the repo (versioned with the code) is the better home, with the project folder for drafts.
4. **Branch drift.** `gba-poc` is stuck at 1.9.1, 13 commits behind main, because 2.0.0 and 2.0.1 went straight to main. The "work lands on gba-poc first" rule is no longer what happens. There are also 7 old `claude/*` branches on GitHub; 4 are fully merged and can be deleted, `v7wdj2` is an unmerged 1.6-era fix, and `mnqoox` is 3.0.0 (being fast-forwarded now).
5. **Names.** Release ROMs are `lands-of-nine-*`, older ones `party-royale-*`, and the build still outputs `party-royale.gba`. Rename the build target to `lands-of-nine.gba` when convenient.
6. **Region-specific constants in code.** About 18 checks like `region == 2` plus `CALDERRA_CHAMPION`, `SUNDERED_CHAMPION`, and `roam[4]` for Calderra's beasts. Manageable at 3 regions; at 9 these want to be per-region data tables rather than more `if`s.

## Capacity for all 9 regions

| Limit | Used at 3 regions | Projected at 9 | Verdict |
|---|---|---|---|
| ROM size (GBA max 32 MB) | 17.4 MB | about 35 MB at the current ~3 MB per region | **Will run out around region 7 or 8** |
| ROM compressibility | gzip shrinks 3.0.0 to 3.6 MB (79% smaller) | | Assets are stored uncompressed, so there's big room |
| SRAM (32 KB) | game_state 30,260 B, layout fixed for 9 regions | same | OK by design |
| Map ids (`area_slots` 1024, includes rooms) | ~250 to 300 | ~850 to 950 (estimate) | OK, tight |
| Trainer ids / item balls (2048 each) | 245 / 249 | ~750 / ~750 | OK |
| Pokédex (2048) | 1,037 species | room for ~1,000 homebrew/forms | OK |
| Tilesets (`uint8_t`, 256) | 130 | ~390 if each region keeps adding ~40 | **Runs out around region 6**, widen to uint16 or share tilesets |
| Level, stats, HP (`uint16_t`) | cap 300 | cap 900; max stat ~6,300 | OK |
| Damage math | done in `double` | | OK |
| XP | flat 35 to 60 per win, `level * 8` to level up | | Inferred: 100 levels per region means thousands of battles per region. Worth checking in the 3.0.0 playtest |

**The one real blocker is ROM size.** Turning on Butano's built-in compression for backgrounds and sprites (LZ77 or Huffman, a one-line setting per asset in the build's JSON) should take well over half the size off, which comfortably fits 9 regions. Best done as a 3.1.0 feature before region 4.

## What we'll be able to do

- 9 regions within one ROM and one save, with the 3.0.0 save loading in every later version (no converter needed, as designed in 2.0.0).
- Homebrew Pokémon: ~1,000 free Pokédex slots, and the Sundered-forms pipeline (species appended after #1025, recolored sprites) already works. Fully original ones need hand-drawn art.
- Level caps to 900 without overflow.
- Per-region story flags (1,024), weather, AI tiers and boss heals are already generic enough to reuse.

## Suggested order

1. After 3.0.0 is on main: move ROMs to GitHub Releases, delete merged branches, reset `gba-poc` to main.
2. 3.1.0: asset compression (frees ROM space) and widen the tileset index.
3. Split `js/app.js` into per-region files before starting region 4.
4. Move region constants into data tables as region 4 is built.
