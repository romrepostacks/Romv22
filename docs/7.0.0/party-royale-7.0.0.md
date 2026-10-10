# Party Royale 7.0.0: The Hollow Lands

Game title: Poké Legends: Lands of Nine. 7.0.0 = the seventh region, themed on Giratina, with the level cap raised
to 700. Theme agreed with Kyle on 2026-10-10 (plans/regions-4-9-themes.md); built on Kyle's "Build 5-9", so this is
written as built. Map: [hollow-map.png](hollow-map.png) (drawn from the game's data by `gba/tools/art/region_map.py`;
the reverse side is the lower panel).

## What earlier versions give us
- **Save:** version 11, unchanged. The Hollow Lands use `region = 7` and story flags 18-19 (Giratina, Champion).
- **Ids:** maps, trainers and item balls come after Aeterna's; Giratina's Origin form after Palkia's.
- **Twins** from 6.0.0, now **mirrored** (`mirror: true`): the reverse side of a road is its layout flipped left to
  right. A **shadow** (person role `TWIN`) steps you through, to the mirrored spot.

## The legend
The Hollow Lands are as close as the living world gets to the reverse side, where Giratina was banished. People who
lost someone come here. **Dr. Lorne** of **Team Hollow** lost his sister to the reverse side and wants to tear it
open to bring her back. He isn't evil, and his grunts mostly just miss people.

## Getting there
After the Aeterna League, the **ferryman** at Aeterna Village rows you across the grey lake to **Gloaming**, and back.

## Layout: the ring
| Part | Places |
|---|---|
| South and west of the ring | Gloaming (hub), Dimmer Road, **Wanewick** (Mara, Ghost), Pale Way, **Greyhaven** (Fenwick, Normal), Hushed Road (rival Lumen), **Stillwater** (Mirela, Ice/Ghost) |
| North (4 badges) | Lost Road, **Duskfall** (Noctis, Dark), Echo Road |
| East and south | **Lanternfall** (Ember, Fire/Ghost), Ashfall Road, Pyre Tower (Admin Cinder), **Mossgrave** (Thorne, Grass/Ghost), Weeping Road, **Echo Bay** (Coral, Water/Ghost), Shore Road |
| The middle | **Cenotaph** (Mortimer, Psychic), Mourning Path (8 badges, both shrines), Hollow League |
| The reverse side | the eight roads, mirrored and colourless, each with its own wild Pokémon; the reverse Lost and Echo Roads lead to the **Distortion World** |

29 areas. Levels follow Hollow badges (600, then 12 per badge, 700 at the League).

## Story
1. Kai meets you in Gloaming. A grave in the churchyard has no name, only question marks.
2. **Hushed Road:** rival **Lumen**, who fell through to the reverse side years ago and came back only halfway;
   nobody but you can see them.
3. **Pyre Tower:** Admin Cinder tries to open the reverse side where it's thinnest. Lorne has gone through himself.
4. **Distortion World** (through the reverse Lost Road or Echo Road): Dr. Lorne, then **Giratina**. Its altar
   switches Giratina to its **Origin** form.
5. **Mourning Path** to the League: Elite Four Dirge (Ghost), Shade (Dark), Pallor (Ice), Knell (Dragon) and
   Champion **Elegy**, who lost someone too and became Champion instead, at 700.

## Level cap and AI (Hollow Lands only)
- Earlier regions stay as they are. Level = min(700, 600 + 12 × Hollow badges).
- On top of Aeterna's, Hollow trainers favour a knockout on a Pokémon that could hit them hard back.

## Arceus plates
The **Spooky Plate** in the Pyre Tower and the **Toxic Plate** on the reverse Weeping Road.

## Easter eggs
- The MISSINGNO grave in Gloaming, buried in Rare Candy.
- Cubone's mother's grave in the Pyre Tower, and a tune from a town you've never been to.
- On the reverse Lost Road, someone who looks exactly like you waves goodbye.
