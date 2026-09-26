# Custom sprites

Drop your own artwork in here and the game will pick it up automatically —
no code changes needed. Any species without a matching file just shows a ❔
placeholder, so it's safe to fill these in gradually.

- `front/<slug>.png` — used for enemy sprites and menu/list icons
- `back/<slug>.png` — used for your own party's sprite during battle (over-the-shoulder view)

## Naming convention (the "slug")

Lowercase the species name, strip accents, and turn every run of spaces/punctuation
into a single hyphen:

| Species        | Filename            |
|----------------|----------------------|
| Bulbasaur      | `bulbasaur.png`      |
| Mr. Mime       | `mr-mime.png`        |
| Nidoran♀       | `nidoran-f.png`      |
| Ho-Oh          | `ho-oh.png`          |
| Ninetales-Alola| `ninetales-alola.png`|

Any image format works as long as it's named `.png` (the code only checks the
extension `.png` — feel free to save a re-exported PNG even if you drew it
elsewhere). Recommended size: square, at least 96×96px, transparent background.
