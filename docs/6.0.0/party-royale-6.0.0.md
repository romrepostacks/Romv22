# Party Royale 6.0.0: Aeterna

Game title: Poké Legends: Lands of Nine. 6.0.0 = the sixth region, themed on Dialga and Palkia, with the level cap
raised to 600. Theme agreed with Kyle on 2026-10-10 (plans/regions-4-9-themes.md); built on Kyle's "Build 5-9", so
this is written as built. Map: [aeterna-map.png](aeterna-map.png) (source: aeterna-map.svg, drawn from the game's
data by `gba/tools/art/region_map.py`; past areas are drawn nudged down and right of their present).

## What earlier versions give us
- **Save:** version 11, unchanged. Aeterna uses `region = 6` and story flags 15-17 (Dialga, Palkia, Champion),
  27 (the seed planted) and 28 (the Mind Plate taken).
- **Ids:** Aeterna's maps, trainers and item balls come after Genova's. Dialga's and Palkia's Origin forms come
  after Genova's experiments.
- **Twins** (new engine piece, reused by 7.0.0): an area can be a copy of another (`twinOf` in js/app.js, built by
  `twinMap`). It has the same layout, its own name, people, gym, wild Pokémon and trainers' teams, and its exits
  lead to the twins of its original's neighbours. A **rift** (person role `TWIN`) steps you to the same spot in the
  twin. The region map shows the side you're on.

## The legend
DIALGA and PALKIA fought over the valley long ago and tore it in two. The rifts are the tear: step through one and
you're in the same valley, ages earlier. **TEAM PARADOX** wants to stop time on one perfect moment, forever.
Their leader, **Chronarch Vex**, only wants to stay in a moment he lost.

## Getting there
After the Genova League, the **conductor** at Genova Central takes the old line out to **Aeterna Village**, and back.

## Layout: the valley now and then
| Now | Then (twin) |
|---|---|
| Aeterna Village (hub) | Old Aeterna |
| Millstream Road | Ancient Millstream |
| Harvest, gym 1 Barley (Ground) | Harvest Hold, gym Tor (Rock) |
| Orchard Lane | Wildwood Path (Celebi's shrine) |
| Bloomfield, gym Posy (Grass/Fairy) | Stonebloom, gym Kara (Fighting) |
| Riverbend Road (the tree) | First River (the seed) |
| Clocktower, gym Horatio (Steel/Psychic) | Sundial Temple, gym Ignatia (Fire/Dragon) |
| Lakeshore Path, rival Tess (4 badges) | Old Lakeshore (4 badges) |
| Mirelake, gym Undine (Water) | Mire Throne, gym Regina (Dragon) |
| Temporal Spire: Admin Lacuna, **Dialga** | Spatial Temple: Chronarch Vex, **Palkia** |
| Timeless Path (8 badges, both legends calmed) | |
| Aeterna League | |

22 areas. Rifts stand in Aeterna Village, Orchard Lane, Riverbend Road and Mirelake (and the same spots in the past).
Gyms go in any order; levels follow Aeterna badges (500, then 12 per badge, 600 at the League).

## Story
1. Kai meets you in the village; the church clock has two faces that never agree. A rift by the well leads to Old
   Aeterna, where a boy has just sold Kai an apple.
2. **Lakeshore Path:** rival **Tess**, whose grandpa wrote the valley's history, explains the tear.
3. **Temporal Spire:** Admin Lacuna, then **Dialga**. Vex has gone back to the **Spatial Temple** that stood there
   before the spire, reached from Mire Throne in the past: Vex, then **Palkia**.
4. **Timeless Path** to the League: Elite Four Fossa (ancient Rock/Ground), Ion (future Steel/Electric), Clio
   (Psychic), Saros (Dragon), and Champion **Aeon**, who leads with Dialga and Palkia, at 600.

## Paradox Pokémon and Origin forms
The present's wild areas have the Iron Pokémon (Iron Treads, Moth, Bundle, Leaves, Jugulis, Hands); the past has
the ancient ones (Great Tusk, Scream Tail, Brute Bonnet, Flutter Mane, Slither Wing). The altars in the Spire and
the Temple switch Dialga and Palkia between their normal and **Origin** forms (PokeAPI's sprites 10245 and 10246).

## Level cap and AI (Aeterna only)
- Earlier regions stay as they are. Level = min(600, 500 + 12 × Aeterna badges).
- On top of Genova's damage maths, Aeterna's trainers go for a knockout when your Pokémon would outspeed them.

## Arceus plates
The **Draco Plate** in the Spatial Temple, and the **Mind Plate**: pick up the seed on the First River's bank in the
past, plant it, and in the present a huge tree stands on the Riverbend Road with the plate in its roots.

## Easter eggs
- The church clock's two faces, and an old woman stuck in the same day.
- Celebi peeking out of a little shrine on Wildwood Path, then gone.
- Three kinds of paw prints under the Clocktower's clock, and a note about the Brass and Ashen towers.
- The boy in Old Aeterna who plants an apple core; the orchard is there in the present.
