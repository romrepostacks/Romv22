# Party Royale

An original, Pokémon-inspired party-battler/overworld game. Runs entirely in
the browser off static files — no build step, no external services, no
accounts. Progress saves to your browser's local storage (per-origin), plus
you can export/import a portable save code.

## Run it

Requires only Node.js (no packages to install):

```
node server.js
```

or, equivalently:

```
npm start
```

Then open **http://localhost:8080** in your browser.

(You can also just double-click `index.html` to open it directly with
`file://` — the game itself works either way. The server is there mainly so
custom sprite images always load reliably and so the origin stays stable
across sessions.)

## Folder structure

```
party-royale/
├── index.html        the page shell — markup only
├── css/style.css      all visual styling
├── js/app.js          all game logic (data, battle engine, overworld, save system, UI)
├── server.js          zero-dependency static file server
├── package.json
└── sprites/
    ├── pokemon/        enemy + menu sprites — sprites/pokemon/<num>.png
    │   └── back/       your party's battle sprites — sprites/pokemon/back/<num>.png
    └── README.md        numbering + details
```

Everything in `js/app.js` runs as plain global-scope script (no bundler, no
modules) — same as before, just moved out of the inline `<script>` tag into
its own file.

## Custom sprites

See `sprites/README.md`. Short version: sprites follow the PokeAPI/sprites
layout — `sprites/pokemon/<num>.png` and `sprites/pokemon/back/<num>.png`,
where `<num>` is the National Dex number (e.g. `25.png`). Missing files fall back to a
❔ placeholder automatically, so you can fill the roster in over time.

## Pokémon data

`js/dexdata.js` holds real types, base stats, abilities, level-up learnsets
(Ultra Sun/Moon) and level-based evolutions for every species, generated from
[PokeAPI](https://pokeapi.co)'s CSV dump. To rebuild it, download the CSVs from
<https://github.com/PokeAPI/pokeapi/tree/master/data/v2/csv> into a folder and run:

```
node tools/build-dexdata.js path/to/csv-folder
```

Only moves the battle engine can simulate are kept (damaging moves, and status
moves that inflict burn/poison/paralysis/sleep/freeze).

## Save data

Saves live in the browser's `localStorage` for whatever origin you're
running on (`http://localhost:8080` if you use the server, or the `file://`
path if you open it directly — these are different origins, so pick one and
stick with it, or use Export/Import Save Code to move a save between them).

## Game Boy Advance version

`gba/` builds a real GBA ROM (for Delta, mGBA or a flash cart) from this game's own maps, art and
Pokémon data. Version 1.1 is the whole game: every area, the story through the Elite Four and the
Hall of Fame, party battles with move animations, catching, field moves, the POKéDEX, PC boxes, music and
saving, plus NUZLOCKE mode and ADVENTURE MODE with the CHALLENGE TOWER (STORY.md phases 6 and 7, so far
only on the GBA). See `gba/README.md`.
