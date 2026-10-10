# Party Royale 5.0.0: Genova

Game title: Poké Legends: Lands of Nine. Version scheme: X.0.0 new region · 0.X.0 major features · 0.0.X fixes.
5.0.0 = the fifth region, themed on Mewtwo and Mew, with the level cap raised to 500. Theme agreed with Kyle on
2026-10-10 (plans/regions-4-9-themes.md); Kyle asked on 2026-10-10 to build regions 5 to 9, so this is written as
built rather than as a draft. Map: [genova-map.png](genova-map.png) (source: genova-map.svg, drawn from the game's
data by `gba/tools/art/region_map.py`).

## What earlier versions give us
- **Save:** version 11, unchanged. Genova uses `region = 5`, story flags 12-14 and 30-39 (the MEW sightings).
- **Ids:** Genova's maps, trainers and item balls come after the Skyreach's. Its nine new species come after Deoxys'
  forms; all 16 remaining Arceus plates were added as items at once (after the Sky and Stone plates) so their ids
  never move.
- **Engine work done once for regions 5 to 9:** one Hall of Fame block for every later region, travel guides
  between regions (`GUIDE_ON` / `GUIDE_BACK`), champion flags and the Challenge Tower's cap by region, region map
  colours, the per-region POKéDEX tab, area twins (for Aeterna's eras and the Hollow Lands' reverse side), extra
  people from the region data (`loc.extras`), and a per-region AI tier.

## The legend
GENOVA was built by the **Synthesis Corp.** around one lab, where FOUNDER ARDEN cloned MEWTWO from a MEW eyelash
found in the jungle. Mewtwo broke out years ago and hides in a cave the Corp. sealed. Chairman **Voss** wants a
second clone that obeys. The city loves Arden's statue in the plaza; the lab logs say what he really did.

## Getting there
After the Skyreach League, the **shuttle pilot** at Windward flies you down to **Genova Central**, and back.

## Layout: inner wards and the ring past the walls
| Part | Gyms | Opens |
|---|---|---|
| Inner wards | 1 Foundry Ward (Steel), 2 Canal Ward (Water), 3 Neon Ward (Electric), 4 Helix Ward (Psychic) | when you land |
| West ring: old labs and jungle | 5 Old Town (Ghost), 6 Verdance (Bug/Grass) | 4 Genova badges |
| East ring: skyline and pier | 7 Skyline (Flying), 8 Greenhouse (Grass/Poison) | 4 Genova badges |

22 areas: Genova Central, 8 gym towns, the monorail lines, Sewer Run, Rooftop Run, Overgrown Lab, Jungle Trail,
Canopy Walk (rival Nova), Skyline Way, Synthesis Tower, Sealed Cave, Lost Pier, Arcology Spire and the League.
Gyms in a part go in any order; levels follow Genova badges (400, then 12 per badge, 500 at the League).

## Story
1. Kai and Prof. Linnea meet you in Genova Central. Linnea studies the escaped experiments; people keep seeing a
   pink Pokémon for a second at a time.
2. **Synthesis Tower** (east, over the rooftops): Admin Ridley. The lab terminal there has the "Cinnabar Island" logs.
   Ridley says Voss took the clone DNA to the Sealed Cave.
3. **Canopy Walk:** rival **Nova**, a lab tech's kid who saved an experiment (Helixeon) from being thrown away.
4. **Sealed Cave:** Chairman Voss, then **Mewtwo**.
5. **Lost Pier:** Admin Kessler is searching an old truck. Mew hides under it until you've seen it in all ten places.
6. **Arcology Spire** (8 badges, the Tower and the Cave cleared) to the League: Elite Four Cipher (Steel/Electric),
   Venna (Poison), Rook (Dark), Lyra (Psychic) and Champion **Cassia** at 500.

## The first homebrew Pokémon: the Gene Lab's experiments
Nine species the Corp. spliced from two Pokémon each. Each sprite is the top of one Pokémon on the bottom of
another, recoloured (`gba/tools/art/genova_splices.py`); each keeps its first half's learnset and size.

| # | Name | Types | Spliced from | Evolves |
|---|---|---|---|---|
| 2021 | Chimurr | Normal/Poison | Rattata + Ekans | Chimaul at 34 |
| 2023 | Voltadpole | Electric/Water | Chinchou + Poliwag | Voltoad at 36 |
| 2025 | Graftling | Grass/Steel | Ferroseed + Oddish | Graftree at 40 |
| 2027 | Specterra | Ghost/Ground | Cubone + Shuppet | |
| 2028 | Mimicore | Psychic/Steel | Porygon + Ditto | |
| 2029 | Helixeon | Psychic/Dragon | Mew + Dragonair | (the lab's try at a Mew) |

## Mew sightings
A pink sparkle stands in ten places (Genova Central, both monorail lines, Sewer Run, Rooftop Run, Overgrown Lab,
Jungle Trail, Verdance, Skyline Way and Greenhouse). Touch it and it's gone; each one counts (flags 30-39). With all
ten, Mew comes out from under the truck on the Lost Pier.

## Level cap and AI (Genova only)
- Earlier regions stay as they are. Level = min(500, 400 + 12 × Genova badges).
- Genova's trainers are **analytic**: on top of everything the Skyreach's do, they work out the real damage each
  move does to each of your Pokémon (as a share of its HP) and pick with much less randomness.

## Arceus plates
Two of the 18, as item balls: the **Iron Plate** in the Sewer Run and the **Zap Plate** on Rooftop Run. Nothing
explains them yet; region 9 does.

## Easter eggs
- The truck on the Lost Pier (the old Red/Blue rumour), with Mew really under it.
- A lab terminal in the Synthesis Tower with logs from "Cinnabar Island".
- A tank in the Helix Ward holding a copy of your first partner, which taps the glass back.
- Founder Arden's statue, with something scratched under the plaque.

## Changed from the theme
- The "push the right switch" truck puzzle became the ten sightings: the truck is where Mew waits.
- The draft had 6 to 10 homebrew Pokémon; there are 9 (three evolve).
