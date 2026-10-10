# 3.1.0 compression: size and timing (2026-10-10)

Measured in mGBA (libmgba 0.10.2, the core Delta uses), same 3.1.0 code built twice: Pokémon sprites stored plain, and LZ77-compressed. Script: `gba/tools/walktest/timing.py`.

## ROM size

| Build | Size |
|---|---|
| 3.0.1 | 17.35 MB |
| 3.1.0, sprites plain (data shared, no compression) | 8.27 MB |
| 3.1.0, sprites LZ77 (the release) | 5.58 MB |

Most of the saving wasn't compression. 3.0.1 kept a separate copy of the map and species data for every source file that read it, about 7 copies. 3.1.0 keeps one copy (9.1 MB saved), and LZ77 on the Pokémon sprites saves another 2.7 MB.

## What you'd notice: frames until each screen appears

For each moment, every screen the plain build showed was compared with the frame the compressed build first showed it.

| Moment | Screens compared | Most frames later with compression |
|---|---|---|
| Boot to the title screen | 2 | 0 |
| Continue to the map (gym) | 13 | 0 |
| Gym leader battle starting (12 Pokémon sprites) | 61 | 0 |
| Party screen opening | 14 | 0 |
| Pokémon summary opening | 12 | 0 |
| Sailing to Port Keel (map load) | 45 | 0 |
| Walking up into Victory Road (map change) | 68 | 0 |

## CPU cost of unpacking (timer, all 1,037 species)

| Sprite | Plain | LZ77 |
|---|---|---|
| Front, each | 0.336 ms | 0.418 ms |
| Back, each | 0.336 ms | 0.391 ms |
| Icon, each | 0.484 ms | 0.553 ms |

Unpacking adds about 0.06 to 0.08 ms per sprite. A full 6v6 battle start makes 12 sprites, about 1 ms extra, well inside one 16.7 ms frame. Map loads and map changes don't touch compressed data, so they are unchanged.
