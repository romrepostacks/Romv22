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
    ├── front/          enemy + menu sprites — sprites/front/<slug>.png
    ├── back/           your party's battle sprites — sprites/back/<slug>.png
    └── README.md        naming convention + details
```

Everything in `js/app.js` runs as plain global-scope script (no bundler, no
modules) — same as before, just moved out of the inline `<script>` tag into
its own file.

## Custom sprites

See `sprites/README.md`. Short version: drop a PNG named after the species
into `sprites/front/` and/or `sprites/back/`, matching the slug convention
(lowercase, hyphenated — e.g. `mr-mime.png`). Missing files fall back to a
❔ placeholder automatically, so you can fill the roster in over time.

## Save data

Saves live in the browser's `localStorage` for whatever origin you're
running on (`http://localhost:8080` if you use the server, or the `file://`
path if you open it directly — these are different origins, so pick one and
stick with it, or use Export/Import Save Code to move a save between them).
