# Party Royale 3.0.0 plan: the Sundered Isles

Game title: Poké Legends: Lands of Nine. Version scheme: X.0.0 new region · 0.X.0 major features · 0.0.X fixes.
3.0.0 = the third region, themed on Groudon and Kyogre, with the level cap raised to 300. Agreed with Kyle on 2026-10-08.
Map: [region3-map.png](region3-map.png) (source: region3-map.svg).

## What 2.0.0 already gives us
- **Save:** version 11 was sized for all 9 regions. Levels are 2 bytes, so 300 fits. Region 3 uses `region = 2`, its own story flags in `flags`, and new fields carved from `spare`. There's no save version bump and no converter.
- **Map ids per region:** Vellorin's and Calderra's ids never move. The Sundered Isles' maps, trainers and items are appended after Calderra's (currently 221 maps).
- **Systems to reuse:** level scaling by badge count (`map_level_cap`), the `smart` AI and `boss_heal`, region map fast travel, the per-region Pokédex tabs, the Move Tutor and Relearner, and battle weather (`battle_weather`, already in the battle engine).

## The legend
The Sundered Isles were once a single land. Long ago, Groudon and Kyogre fought over it: Groudon raised the earth and Kyogre called the sea, until the land broke into the islands of today. They fell asleep at either end of the region, Groudon under **Magma Isle** and Kyogre beneath **the Abyss**, a whirlpool in the open sea.

## Getting there
After you beat the Calderra League, the **ship captain** in Port Calder offers passage. That unlocks the new region. The ship lands at **Port Keel** on the central island, and you can sail back to Port Calder from there whenever you like.

## Layout: a hub with sea routes
- **Keel Isle** (center): Port Keel (Pokémon Center, Mart, Captain, Move Tutor, Relearner) and **Mt. Keel** in the middle, which holds Victory Road and the League. Mt. Keel opens with all 8 badges.
- **Eight gym islands** surround it, each reached by its own water route (Sea Routes 1 to 8). Each route is mostly sea, with surfing, swimmer and sailor trainers, water encounters and small islets along the way. There are also shortcut lanes between neighbouring islands.
- **Free order**, like Calderra: every island is open from the start, and levels follow the number of Sundered Isles badges you have, not where you are.
- **Story sites:** Magma Isle (southwest, Team Quake's base, Groudon) via Mire Isle, and the Abyss (northeast, Team Neptune's base, Kyogre) via Hivewood Key.

| # | Island | Gym type | Feature |
|---|--------|----------|---------|
| 1 | Verdant Isle | Grass | jungle |
| 2 | Hivewood Key | Bug | hollow-tree forest |
| 3 | Ironhaul Isle | Steel | shipyard |
| 4 | Pearl Isle | Fairy | beaches and pearl divers |
| 5 | Wreck Cove | Ghost | shipwreck |
| 6 | Mire Isle | Poison | swamp |
| 7 | Brawler's Rock | Fighting | sea cliffs |
| 8 | Mirage Isle | Psychic | desert and mirages |

The gyms use the 8 types Calderra didn't have. The Elite Four are Water, Ground, Dragon and Dark, with a Champion who uses a mix.

## Story
1. You arrive at Port Keel. The local professor studies the Sundering legend and the island forms it left behind.
2. **Team Quake** wants to wake Groudon, raise the seabed and fuse every island into one flat land built on the ashes of the old ones.
3. **Team Neptune** wants to wake Kyogre and drown the world, so it will be remembered in history like Atlantis.
4. The teams fight each other as much as they fight you. On several islands you run into one, the other, or both at once.
5. Midgame: Team Quake's base on Magma Isle and Team Neptune's base in the Abyss. Each ends with its leader trying to wake their legendary.
6. Finale: both leaders succeed at once. The sea rises and the land shakes, and you calm Groudon and Kyogre, catching each one in its own lair.
7. Kai (from Calderra) comes back as an ally, alongside a new rival from the islands.
8. Then the League on Mt. Keel: Elite Four and Champion at the 300 cap.

## Species: Sundered forms
- About 12 existing Pokémon get a **Sundered form**: a new type, a recolored sprite, and adjusted stats or moves. Examples to pick: a Ground/Water Wooper-style line, Fire/Water forms, Steel coastal forms, Ghost forms tied to the shipwreck.
- They're stored as new species entries appended after #1025. The save's 2-byte species field and the 2,048-entry Pokédex already have room.
- Fully homebrew Pokémon wait for a later region, since each one needs original art.
- Wild pools and trainer teams mix Sundered forms with the water, island and tropical species that suit each island.

## Level cap and AI (Sundered Isles only)
- Vellorin and Calderra stay as they are.
- Level = min(300, 200 + 12 × Sundered Isles badges). The League and Champion are at 300.
- Bosses get two FULL RESTOREs instead of one.
- The `smart` AI also weighs held items (a foe's Choice item, Leftovers and so on) when it picks a move.
- Check: stat formulas, the XP curve to 300, and HP and level that run past 3 digits on screen.

## Extras
1. **Treasure-map region map.** The region map for the Sundered Isles is drawn as a parchment pirate map (compass rose, dashed sea routes, red story trail), with fast travel like the other regions.
2. **Route weather.** Sea routes have rain, storms or fog that carry into battles (using the existing battle weather). Storms grow worse near the Abyss and harsh sun near Magma Isle as the story goes on.
3. **Doubloon Shoal.** A hidden sandbar with buried treasure, the region's X-marks-the-spot Easter egg.

Other Easter eggs: a ship's figurehead of Ho-Oh in Port Keel, and the three Calderra beasts' footprints carved somewhere on the islands.

## Build stages
1. Ship captain unlock and Port Keel (hub, travel back to Port Calder).
2. Sundered forms in the species table and sprites.
3. The eight gym islands and Sea Routes 1 to 8, with gyms.
4. Team Quake and Team Neptune story, Magma Isle and the Abyss, Groudon and Kyogre.
5. Mt. Keel: Victory Road, Elite Four, Champion and the Hall of Fame.
6. Level cap 300, AI changes and boss heals.
7. Extras: treasure-map region map, route weather, Doubloon Shoal and the other Easter eggs.
8. Test and ship: scripted walk through every new area (walktest), a 2.0.1 save loads and can reach the region, the 3.0.0 ROM goes to gba/roms/, a What's New entry, and it merges to main with Kyle's OK.
