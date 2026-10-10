# Party Royale 4.0.0 plan: the Skyreach

Game title: Poké Legends: Lands of Nine. Version scheme: X.0.0 new region · 0.X.0 major features · 0.0.X fixes.
4.0.0 = the fourth region, themed on Rayquaza and Deoxys, with the level cap raised to 400. Theme agreed with Kyle on
2026-10-10 (plans/regions-4-9-themes.md); Kyle OK'd this plan and map on 2026-10-10. Built on branch
`claude/project-thread-h2yarj`; notes marked **Built:** say where the build differs from the draft.
Map: [skyreach-map.png](skyreach-map.png) (source: skyreach-map.svg). It's a side view, because this region is about height.

## What earlier versions already give us
- **Save:** version 11 is sized for all 9 regions. Levels are 2 bytes, so 400 fits. The Skyreach uses `region = 4`,
  its own story flags, and new fields from `spare`. No save version bump, no converter.
- **Ids:** the Skyreach's maps, trainers and item balls are appended after the Sundered Isles'. Nothing older moves.
- **Room:** 3.1.0 took the ROM to 5.6 MB and made the tileset index 16-bit, so a region costs about 1 MB now.
- **Systems to reuse:** the captain's travel between regions, level scaling by badge count (`map_level_cap`), the
  `smart` AI and boss heals, shrines and legend catches, region map fast travel, per-region Pokédex tabs, area weather
  carried into battle, and the Sundered forms pipeline (recoloured sprites, species appended after the last one).

## The legend
Rayquaza has slept for ages at the top of the **Sky Pillar**, a ruin taller than the clouds, rising from the middle of
a highland of red mesas and floating rock. Old carvings at the pillar's base show Groudon and Kyogre with Rayquaza
between them. They call it the one who stopped the Sundering.

## Getting there
After you beat the Sundered League, an **airship pilot** at Port Keel offers a flight. The airship docks at
**Windward**, a town at the foot of the Sky Pillar, and can fly you back to Port Keel whenever you like. That first
night a meteor falls into the desert east of town.

## Layout: three altitude bands
The region is stacked rather than spread out. Each band is a ring of gyms and routes around the Sky Pillar:

| Band | Gyms | How you get up |
|---|---|---|
| **Canyon Floor** | 1 Ochre Gulch, 2 Cascade Gorge, 3 Starfall | open when you land |
| **Mesa Tops** | 4 Gale Mesa, 5 Kiln Mesa, 6 Thunderhead | updraft opens with 3 Skyreach badges |
| **Cloud Level** | 7 Frostcrown, 8 The Aerie | updraft opens with 6 Skyreach badges |

- Inside a band you can take the gyms in any order, and levels follow your Skyreach badges as in Calderra and the Isles.
- The bands are what's new: you climb, and you can't skip ahead. Higher bands have more Flying and Dragon Pokémon and
  rougher weather.
- **Wind currents:** tiles that carry you up or across a gap until you land, like sliding on ice. On the mesa and
  cloud routes they make small puzzles (ride the right current to reach a ledge or an item).
- **Pillar Heart** (Victory Road) climbs the inside of the Sky Pillar from Windward to the **Skyreach League** in its
  crown. It opens with all 8 badges and both legends settled.
- **Built:** 25 areas. Wind currents are arrow tiles beside the route paths that push you one more step; Cloud Route 9
  and 10 lead from Frostcrown to the Sky Pillar and The Aerie.
- About 24 areas: Windward, 8 gym towns, 8 routes (Canyon Routes 1 to 3, Mesa Routes 4 to 6, Cloud Routes 7 and 8),
  the Meteor Crater, Zenith Lab, the Sky Pillar, Pillar Heart, the League and Moonfall Hollow.

## Gyms: two types each
Calderra and the Isles used all 18 types once, so every Skyreach leader mixes two types.

| # | Town | Types | Feature |
|---|------|-------|---------|
| 1 | Ochre Gulch | Rock / Ground | canyon mining town |
| 2 | Cascade Gorge | Water / Grass | river canyon with waterfalls |
| 3 | Starfall | Psychic / Fairy | next to the Space Center and the crater |
| 4 | Gale Mesa | Flying / Normal | windmills on the mesa edge |
| 5 | Kiln Mesa | Fire / Fighting | cliff pueblo with pottery kilns |
| 6 | Thunderhead | Electric / Steel | lightning rods on a storm mesa |
| 7 | Frostcrown | Ice / Rock | a snow spire above the clouds |
| 8 | The Aerie | Dragon / Flying | dragon nests on floating rock |

Elite Four: Rock, Psychic, Steel and Dragon. The Champion uses a mix, with a Mega-style ace at 400.

## Story
1. You land at Windward. **Professor Cirrus** at the Starfall Space Center tracks the meteor that falls that night.
2. **Team Zenith** takes the crater. Its leader, **Director Halley**, wants the meteor's DNA to build "perfect" Pokémon.
3. Deoxys wakes inside the meteor and copies whatever it fights. Zenith grunts carry Pokémon "tuned" with its DNA.
4. A rival from the Skyreach, **Sora**, a young pilot, battles you on Mesa Route 5 and later helps you. Kai has a cameo
   in Windward.
5. Midgame: **Zenith Lab** on the eastern mesa, where Halley has the meteor core. You beat Halley, and Deoxys breaks
   loose and flies to the top of the Sky Pillar.
6. Finale on the **Sky Pillar**: Rayquaza comes down to fight the thing from space. You battle Halley one last time
   and then calm Rayquaza and catch it at the summit. Deoxys falls back to the **Meteor Crater**, where you catch it.
7. Then the League in the pillar's crown: Elite Four and Champion at 400.

## Deoxys forms
After you catch Deoxys, meteorite shards in four places change its form when it's in your party: the Meteor Crater
(Normal), Ochre Gulch (Attack), Frostcrown (Defense) and Gale Mesa (Speed). Each form is its own species entry,
appended after the Sundered forms, with PokeAPI's form sprites.

## Skyreach forms
About 8 existing Pokémon get a **Skyreach form**, made the same way as the Sundered forms (new types, recoloured
sprite, adjusted stats). Examples to pick from: a floating Rock/Flying Geodude line, a Fire/Flying Ponyta line, an
Ice/Flying Swablu line, a Rock/Dragon Rhyhorn line. Fully homebrew Pokémon still wait for Genova (5.0.0).

## Level cap and AI (Skyreach only)
- Vellorin, Calderra and the Sundered Isles stay as they are.
- Level = min(400, 300 + 12 × Skyreach badges). The League and Champion are at 400.
- Bosses keep two FULL RESTOREs, as in the Isles.
- New for this region: the `smart` AI **switches out** of a bad matchup when another Pokémon on its team takes the hit
  better, and it saves its strongest Pokémon for last.
- **Built:** battles put every Pokémon on the field at once, so there is nothing to switch to. Instead Skyreach
  trainers are **keen**: they go first for whichever of your Pokémon threatens them with a super-effective move.
- Check: stat formulas and the XP curve to 400, and the 4-digit HP and stat numbers on screen.

## Arceus plates
Two of the 18 plates are hidden here: the **Sky Plate** on the highest floating rock past The Aerie, reached by a
chain of wind currents, and the **Stone Plate** deep in the Ochre Gulch mine. Nothing explains them yet; region 9 does.
They're appended as new items (the save's bag holds 256 item kinds).
**Built:** the Sky Plate is on Cloud Route 10 and the Stone Plate on Canyon Route 2, as item balls.

## Extras
1. **Altitude region map.** The region map is a side view like the draft: canyon floor, mesas and clouds, with the
   pillar in the middle and fast travel like the other regions.
2. **Strong winds.** A new weather on the cloud routes and the Sky Pillar that carries into battle: Flying types lose
   their weaknesses, like Rayquaza's Delta Stream. Thunderhead gets rain, the canyon floor gets sandstorms.
   **Built:** no canyon sandstorms; Frostcrown has snow, which becomes hail in battle.
3. **Easter eggs:** the Space Center's launch board counts down to "4.0.0"; in **Moonfall Hollow** by the crater, a lone
   Clefairy dances only on full-moon play-time minutes; Groudon and Kyogre are carved at the pillar's base with
   Rayquaza between them.

## Build stages
1. The airship pilot at Port Keel and Windward (hub, flight back to Port Keel).
2. Skyreach forms and Deoxys' form species, with sprites.
3. Canyon Floor: Canyon Routes 1 to 3 and gyms 1 to 3.
4. Wind current tiles and the updrafts, then Mesa Tops and Cloud Level with gyms 4 to 8.
5. Team Zenith story: the Meteor Crater, Zenith Lab, the Sky Pillar, Rayquaza and Deoxys, the four shards.
6. Pillar Heart, Elite Four, Champion and the Hall of Fame.
7. Level cap 400, AI switching and ace-last, strong winds weather.
8. Extras: altitude region map, plates, Easter eggs.
9. Test and ship: walktest over every new area, a 3.1.0 save loads and can reach the region, the 4.0.0 ROM goes to
   gba/roms/, a What's New entry, and it merges to main with Kyle's OK.

## Open for Kyle
- Bands that unlock at 3 and 6 badges (the draft), or every gym open from the start like the last two regions?
- Two types per gym (the draft), or single types repeated from earlier regions?
- Skyreach forms (about 8), or skip them and keep only Deoxys' forms?
