# Party Royale 9.0.0: Origin

Game title: Poké Legends: Lands of Nine. 9.0.0 = the ninth and last region, themed on Arceus, with the level cap
raised to 900. Theme agreed with Kyle on 2026-10-10 (plans/regions-4-9-themes.md: an Arceus finale with two plates
hidden in each region and the first regions getting theirs later); built on Kyle's "Build 5-9", so this is written as
built. Map: [origin-map.png](origin-map.png).

## What earlier versions give us
- **Save:** version 11, unchanged. Origin uses `region = 9` and story flags 25-26 (Arceus, Champion) and 40-42 (the
  old regions' plate shrines).
- **Ids:** maps, trainers and item balls come after Tempesta's; Arceus's 17 type forms after the Galarian birds (they
  use PokeAPI's 493-<type> sprites, saved as 2101-2117).

## The legend
All eight lands surround one mountain. Arceus made them, and scattered its 18 plates across them, two to a land.
Vellorin's, Calderra's and the Isles' were never found, so 9.0.0 adds a plate shrine to each of their first towns
(Duskmere Hollow, Port Calder, Port Keel).

| Land | Plates |
|---|---|
| Vellorin | Flame, Splash (shrine in Duskmere Hollow) |
| Calderra | Meadow, Icicle (shrine in Port Calder) |
| Sundered Isles | Earth, Dread (shrine in Port Keel) |
| Skyreach | Sky, Stone |
| Genova | Iron, Zap |
| Aeterna | Draco, Mind |
| Hollow Lands | Spooky, Toxic |
| Tempesta | Fist, Insect |
| Origin | Pixie (Time Path), Blank (Spear Pillar Steps) |

## Getting there
After the Tempesta League, the **mountain guide** in Fairhaven leads you up to **Origin Gate**, and back.

## Layout: eight halls up the mountain
No gym leaders: each hall is kept by a Champion you've already beaten, with their best team at full strength, and
counts as a badge. Vellorin Hall (Wren) and Calderra Hall (Solenne) to the west, Sundered Hall (Marea) and Skyreach
Hall (Altair) to the east; the Mountain Ascent (4 halls) climbs to Genova Hall (Cassia), with Aeterna Hall (Aeon) and
Hollow Hall (Elegy) on either side; the Spear Pillar Steps lead to Tempesta Hall (Zephyra), then the Hall of Origin
and the League. 19 areas. Levels: 800, then 12 per hall, 900 at the League.

## Story
1. Kai is at Origin Gate, looking down at every land you've been to, and goes on ahead.
2. The eight halls, in any order within each part.
3. **Hall of Origin:** Keeper Ilex tests you. **Arceus** answers only when you hold all 18 plates; before that, the
   light just dims. The altar there turns Arceus to the type of the next plate you hold.
4. **Origin League:** the Elite Four are villains you beat on the way, given a second chance, each with the legend
   they once chased: Director Halley (Deoxys), Chairman Voss (Mewtwo), Chronarch Vex (Dialga and Palkia), Helios
   (Moltres). The Champion is **Kai**, at 900, for real this time.

## Level cap and AI (Origin only)
- Earlier regions stay as they are. Level = min(900, 800 + 12 × halls).
- Origin trainers use everything earlier regions' trainers do, with the least randomness, and the bosses heal more.

## Easter eggs
- The stone ring at Origin Gate with eighteen empty sockets.
- Every hall's route is paved, planted or haunted like the land its Champion came from.
