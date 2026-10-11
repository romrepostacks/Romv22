# Party Royale 8.0.0: Tempesta

Game title: Poké Legends: Lands of Nine. 8.0.0 = the eighth region, themed on the three legendary birds and the
seasons, with the level cap raised to 800. Theme agreed with Kyle on 2026-10-10 (plans/regions-4-9-themes.md); built
on Kyle's "Build 5-9", so this is written as built. Map: [tempesta-map.png](tempesta-map.png).

## What earlier versions give us
- **Save:** version 11, unchanged. Tempesta uses `region = 8` and story flags 20-24 (the birds, Champion, Lugia),
  48-51 (Delibird's gift each season) and 52-53 (the season).
- **Ids:** maps, trainers and item balls come after the Hollow Lands'; the Galarian birds after Giratina's Origin form.

## The legend
**Team Solstice** caged Articuno, Zapdos and Moltres on Tempesta's three peaks to keep the land in one endless spring.
Their leader **Helios** grew up in a village that burned every autumn. Without the birds, the farmers can't harvest,
the skiers have no snow, and a kid's kite has been stuck in the sky for a year.

## Getting there
After the Hollow League, the **balloonist** at Gloaming flies you to **Fairhaven**, and back.

## Layout: a round land with three spokes
| Spoke | Places |
|---|---|
| West (winter) | Frostwind Path, **Rimehold** (Bryony, Ice), Snowdrift Trail, **Glacier Point** (Haze, Steel/Ice), **Frost Peak** (Admin Hiems, Articuno) |
| East (summer) | Thunderhead Road, **Stormport** (Nimbus, Electric), Galeway, **Windmill Downs** (Gale, Flying), **Thunder Peak** (Admin Fulgor, Zapdos) |
| North (autumn) | Ember Road, **Cinderfield** (Pippa, Fire), **Goldenreach** (Calla, Grass), Ashen Climb, **Ember Peak** (Helios, Moltres) |
| South | Festival Road (rival Kite), **Bloomtide** (Aurora, Fairy), **Tidewater** (Solace, Water), Whirl Isle (Collector Lawrence, Lugia), Equinox Way (8 badges, all three peaks), Tempesta League |

22 areas around **Fairhaven**, the festival city. Levels follow Tempesta badges (700, then 12 per badge, 800).

## Seasons
- Tempesta starts in **spring**. Freeing a bird (beating its peak's boss) brings its season back: Zapdos summer,
  Moltres autumn, Articuno winter.
- The **Season Keeper** in Fairhaven turns the land to the next season you've freed. Every open-air area's weather
  follows (summer rain, autumn fog, winter snow), so does battle weather (rain, or hail in winter), and each season
  brings its own wild visitors (spring Deerling and Cherubi, summer Shinx and Castform, autumn Pumpkaboo and
  Fletchling, winter Snover and Delibird).
- **Delibird's post office** in Fairhaven sends a parcel once each season: an evolution stone and 3 Rare Candy.

## Story
1. Kai lands with you in the middle of the festival.
2. **Festival Road:** rival **Kite**, whose kite is stuck in the windless sky.
3. The three peaks, in any order: Admins Hiems and Fulgor, then **Helios** on Ember Peak. Each bird is there to catch.
4. **Equinox Way** to the League: Elite Four Cirrus (winter), Tempo (summer), Ashe (autumn), Verna (spring) and
   Champion **Zephyra**, who fields the three birds' Galarian forms, at 800.
5. After the League, **Lugia** rises at Whirl Isle. Before that, the sea only churns.
6. The festival mirror in the League town switches the birds to their **Galarian** forms (PokeAPI's 10169-10171).

## Level cap and AI (Tempesta only)
- Earlier regions stay as they are. Level = min(800, 700 + 12 × Tempesta badges).
- Tempesta trainers count the season's weather when they work out damage.

## Arceus plates
The **Fist Plate** on Galeway and the **Insect Plate** on Ashen Climb.

## Easter eggs
- The three orbs on the peaks and "the beast of the sea shall sing" (Shamouti Island, The Power of One).
- Collector Lawrence and his flying fortress at Whirl Isle.
- An old man on Ashen Climb who caught a Moltres feather as a boy.
