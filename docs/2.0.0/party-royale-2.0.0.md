# Party Royale 2.0.0 plan (draft)

Version scheme: X.0.0 new region · 0.X.0 major features · 0.0.X fixes.
2.0.0 = a second region, reachable only in Adventure Mode, plus every Pokémon up to Gen 9 (808 → 1025).
Work starts on its own branch once 1.9.0 is out; gba-poc and main stay untouched until then.

Decided: a **full region** the size of Vellorin, with 8 gyms, a rival arc, an Elite Four and a Champion.

## Stage 0: A save format for all 9 regions (built once, in 2.0.0)
Kyle's goal: the save layout never has to change again through 9.0.0, and existing saves carry over.

Today (save version 9, gba/include/pr_state.h): about 22 KB of the cartridge's 32 KB save memory. Each Pokémon takes 48 bytes and there are 430 slots (10 party + 14 boxes of 30), which is about 20 KB of it. Things that would run out before 9 regions:
- Level is one byte (max 255); region 3 needs 300.
- A move is stored in 10 bits (1024 moves max); Gen 9 has about 920 moves before any homebrew ones.
- Pokédex holds 1024 species; Gen 9 is 1025, plus homebrew.
- Story progress is 32 flags; trainers beaten, item balls and rematches hold 256 each; visited areas hold 64. Vellorin alone uses a good share of these.
- The bag is sized to exactly today's items, so every new item shifts everything after it.

New layout (save version 10), sized for 9 regions with room to spare:
- Pokémon: 2-byte level (up to 999), 2-byte move ids with their own PP byte (65,535 moves), 2-byte species (65,535 species, room for homebrew), plus 4 spare bytes for later (gender, held-item extras, forms). About 60 bytes each, about 25 KB for all 430.
- Pokédex: 2,048 species. Story flags: 1,024. Trainers beaten, rematches and item balls: 2,048 each. Areas visited: 1,024. Bag: 256 item kinds.
- A current-region number, and 1 KB of zeroed spare space at the end.
- Every new field must treat zero as "not set yet", so later versions add features inside the spare space without moving anything, and old saves load as they are.
- Total stays under 30 KB, inside the 32 KB chip with room for the existing "game cleared" mark.

Converting saves: the ROM already upgrades old saves when it loads them (every save since 1.0 still works). 2.0.0 adds one more step that reads a 1.8.x/1.9.x save, copies everything across into the new layout, and saves it. Your current game opens in 2.0.0 exactly where you left it. To be safe, keep a copy of your .sav file before the first 2.0.0 load. If 1.9.0 changes the save itself (new Mart items grow the bag), the converter reads that version too.
- Test: convert a real 1.8.1 save and a 1.9.0 save in the emulator, and check that the party, boxes, bag, badges, Pokédex, position and play time all match.

## Stage 1: Species to Gen 9
- Add Gen 8 and 9 to the species table (`DEX_NUM` in js/app.js) and rebuild js/dexdata.js from PokeAPI with tools/build-dexdata.js.
- Sprites: front and shiny from PokeAPI. Gen 8/9 mostly have no back sprite; the build already falls back to the front one, and we add a horizontal flip so it faces the right way.
- Add the Gen 8/9 legendaries and mythicals to the Challenge Tower list (`legend_nums` in gba/tools/build_assets.py).
- Save: the Pokédex `seen` set holds 1024 species, so it grows and gets a new save version with migration (old saves keep working).
- Party icons: check they still fit the 6 shared sprite palettes.

## Stage 2: Moves and abilities
- Append the Gen 8/9 moves to the move table (saved indices stay put). Plain damage moves work right away; moves with unusual effects behave as their closest simple version until a later 2.x.0.
- New abilities the battle engine doesn't model yet act as no ability for now.

## Stage 3: The second region
- A new grid of areas, separate from Vellorin's so they never connect, added after every existing area so old saves keep their place.
- Reached in Adventure Mode (proposed: a ferry from Portmere Harbour after the credits). This also replaces the "ADVENTURE MODE is coming in a future update" line.
- New towns, routes, side areas, gym leaders and juniors, rival fights, an Elite Four, a Champion and a new story legendary.
- Gen 8/9 species make up most of the wild pools and trainer teams here, with level curves starting where Vellorin ends.
- New region name, area names and music picks to agree with Kyle.

## Region 2 theme and story (Kyle: themed on Raikou, Entei and Suicune)
Working name: **Calderra**, a region built around a great crater. The legend says a tower at its center burned down centuries ago, and three Pokémon rose from the ashes: one carried the lightning that struck it, one the fire that burned it, and one the rain that put it out.

Layout (locked in by Kyle, 2026-10-08): a tree, not a grid. Map: [calderra-map.png](calderra-map.png), 32 areas.
- One start, **Port Calder**, where the ferry lands. Three branches leave it and all end at the **Crater Rim**.
- Branch 1, **Stormreach** (thunder plains, Raikou): Electric, Flying and Normal gyms. Open from the start.
- Branch 2, **Mistral Lakes** (cold lakes and the north wind, Suicune): Ice and Water gyms. Its entrance at Port Calder opens after branch 1.
- Branch 3, **Cinderdeep** (volcanic highlands, Entei): Rock, Fire and Ground gyms. Its entrance opens after branch 2.
- The Rim ends of branches 2 and 3 are blocked from the Rim side until you have walked that branch from Port Calder once; after that they work both ways as shortcuts.
- From the Rim, once all three beasts are free: the **Ashen Tower** (Ho-Oh), Victory Road and the Calderra League.
- Six optional side areas branch off gym towns (Windmill Fields, Static Fields, Glacier Cave, Hidden Falls, Geode Hollow, Hot Springs).

Story outline:
1. The Champion of Vellorin sails from Portmere to Calderra in Adventure Mode. A new professor studies the Ashen Tower legend.
2. A new villain team, **Team Eclipse**, wants to capture all three beasts and use their power to rebuild the tower for themselves.
3. Each third has gyms and a beast arc: you reach the beast's shrine just after Team Eclipse, fight them off, and the beast appears and flees. It then roams that third of the region (it moves between routes each time you change area), and you can chase and catch it.
4. A rival from Calderra who wants to catch the beasts first, later teaming up with you against Team Eclipse.
5. The finale in the Ashen Tower: Team Eclipse's boss tries to bind the three. With all three beasts settled, Ho-Oh appears at the top of the tower as the region's story legendary.
6. Then the Calderra League: Elite Four and Champion, level cap 200.


Easter eggs to include: a statue of Wren and the Vellorin gym leaders in Calderra's first town; a hidden spot where all three beasts' footprints meet.

## Stage 3b: Level cap and tougher AI (new region only)
- Vellorin stays exactly as it is: foes up to 50, Elite Four up to 70, the same AI.
- Region 2 raises the level cap to 200 (for the player and foes). Foes start around Vellorin's top end and climb toward 200 by its League. Region 3 (3.0.0) goes to 300.
- Fits today's save: a Pokémon's level is stored in one byte (up to 255), so 200 works; 300 needs a bigger field and a save change in 3.0.0. HP and stats at 200 stay inside their 16-bit fields (Blissey is about 1400 HP).
- Needs a look: stat formulas past 100, the XP curve to 200, and screens that assume 3-digit HP and levels.
- Only the new region's trainers get the smarter AI (the Challenge Tower's move scoring today), and its bosses also switch out of bad matchups and use one healing item per battle.

## Stage 4: Test and ship
- Scripted emulator run through every new area (same harness as 1.9.0's playthrough test).
- Old 1.9.0 save loads, migrates and can reach the new region.
- 2.0.0 ROM to gba/roms/, What's New entry, merge with Kyle's OK.

## Open questions for Kyle

- Region name and theme (e.g. snowy north, desert, islands).
- Anything else you want in 2.0.0.
