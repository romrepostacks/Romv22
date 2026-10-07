# Party Royale

An original, Pokémon-inspired party-battler/overworld game for the **Game Boy Advance**. It runs in emulators
(Delta, mGBA) and on real hardware from a flash cart. The ROM is built from `gba/`; see `gba/README.md` for
what's in it and how to build it.

The browser version has been retired. Its code stays only as the game's data source: `gba/tools/export.js`
loads `js/*.js` (maps, art, story, Pokémon and moves) and `sprites/` and turns them into the ROM's data.

## Folder structure

```
├── gba/              the GBA game (Butano, C++) and its build tools
├── js/               game data the ROM is built from (maps, art, story, Pokémon, moves, music)
├── sprites/          Pokémon sprites (sprites/pokemon/<num>.png, back/ and shiny/)
├── tools/            generators for js/dexdata.js, js/dexinfo.js and js/tileart.js
└── STORY.md          the story and the phase plan
```

## Pokémon data

`js/dexdata.js` holds real types, base stats, abilities, level-up learnsets (Ultra Sun/Moon) and level-based
evolutions for every species, generated from [PokeAPI](https://pokeapi.co)'s CSV dump. To rebuild it,
download the CSVs from <https://github.com/PokeAPI/pokeapi/tree/master/data/v2/csv> into a folder and run:

```
node tools/build-dexdata.js path/to/csv-folder
```
