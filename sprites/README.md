# Custom sprites

Sprites use the same layout as [PokeAPI/sprites](https://github.com/PokeAPI/sprites),
keyed by National Dex number. Any species without a matching file just shows a ❔
placeholder, so it's safe to fill these in gradually.

- `pokemon/<num>.png` — used for enemy sprites and menu/list icons
- `pokemon/back/<num>.png` — used for your own party's sprite during battle (over-the-shoulder view)

## Numbering

`<num>` is the PokeAPI id: the National Dex number for base species, and PokeAPI's
form id for regional forms. The name → number table is `DEX_NUM` in `js/app.js`.

| Species         | Front                  | Back                        |
|-----------------|------------------------|-----------------------------|
| Bulbasaur       | `pokemon/1.png`        | `pokemon/back/1.png`        |
| Pikachu         | `pokemon/25.png`       | `pokemon/back/25.png`       |
| Mr. Mime        | `pokemon/122.png`      | `pokemon/back/122.png`      |
| Ninetales-Alola | `pokemon/10104.png`    | `pokemon/back/10104.png`    |

To refill from a PokeAPI/sprites checkout, copy `sprites/pokemon/<num>.png` and
`sprites/pokemon/back/<num>.png` across as-is.
