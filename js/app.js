const TYPES = ["Normal","Fire","Water","Electric","Grass","Ice","Fighting","Poison","Ground","Flying","Psychic","Bug","Rock","Ghost","Dragon","Dark","Steel","Fairy"];
const CHART = {
 Normal:{Rock:.5,Ghost:0,Steel:.5},
 Fire:{Fire:.5,Water:.5,Grass:2,Ice:2,Bug:2,Rock:.5,Dragon:.5,Steel:2},
 Water:{Fire:2,Water:.5,Grass:.5,Ground:2,Rock:2,Dragon:.5},
 Electric:{Water:2,Electric:.5,Grass:.5,Ground:0,Flying:2,Dragon:.5},
 Grass:{Fire:.5,Water:2,Grass:.5,Poison:.5,Ground:2,Flying:.5,Bug:.5,Rock:2,Dragon:.5,Steel:.5},
 Ice:{Fire:.5,Water:.5,Grass:2,Ice:.5,Ground:2,Flying:2,Dragon:2,Steel:.5},
 Fighting:{Normal:2,Ice:2,Poison:.5,Flying:.5,Psychic:.5,Bug:.5,Rock:2,Ghost:0,Dark:2,Steel:2,Fairy:.5},
 Poison:{Grass:2,Poison:.5,Ground:.5,Rock:.5,Ghost:.5,Steel:0,Fairy:2},
 Ground:{Fire:2,Electric:2,Grass:.5,Poison:2,Flying:0,Bug:.5,Rock:2,Steel:2},
 Flying:{Electric:.5,Grass:2,Fighting:2,Bug:2,Rock:.5,Steel:.5},
 Psychic:{Fighting:2,Poison:2,Psychic:.5,Dark:0,Steel:.5},
 Bug:{Fire:.5,Grass:2,Fighting:.5,Poison:.5,Flying:.5,Psychic:2,Ghost:.5,Dark:2,Steel:.5,Fairy:.5},
 Rock:{Fire:2,Ice:2,Fighting:.5,Ground:.5,Flying:2,Bug:2,Steel:.5},
 Ghost:{Normal:0,Psychic:2,Ghost:2,Dark:.5},
 Dragon:{Dragon:2,Steel:.5,Fairy:0},
 Dark:{Fighting:.5,Psychic:2,Ghost:2,Dark:.5,Fairy:.5},
 Steel:{Fire:.5,Water:.5,Electric:.5,Ice:2,Rock:2,Steel:.5,Fairy:2},
 Fairy:{Fire:.5,Fighting:2,Poison:.5,Dragon:2,Dark:2,Steel:.5}
};
function eff(atkType, defTypes){ let m=1; for(const d of defTypes){ const t=CHART[atkType]; if(t && d in t) m*=t[d]; } return m; }

const ITEMS = {
 none:{n:"None"},
 leftovers:{n:"Leftovers", desc:"Heals 6% max HP at end of each turn"},
 lifeorb:{n:"Life Orb", desc:"+30% damage dealt"},
 scarf:{n:"Choice Scarf", desc:"+50% Speed"},
 sash:{n:"Focus Sash", desc:"Survives a would-be KO from full HP with 1 HP (once)"},
 sitrus:{n:"Sitrus Berry", desc:"Heals 25% max HP once, when HP drops to half or below"}
};

const CURATED_DEX = [
 {name:"Decidueye", types:["Grass","Ghost"], base:{hp:78,atk:107,def:75,spa:100,spd:100,spe:70},
  ability:{n:"Overgrow", type:"boost", boostType:"Grass", desc:"Grass moves +50% power when at 1/3 HP or below"},
  moves:[{n:"Spirit Shackle",t:"Ghost",p:80,c:"phys",a:100},{n:"Leaf Blade",t:"Grass",p:90,c:"phys",a:100},{n:"Sucker Punch",t:"Dark",p:70,c:"phys",a:100},{n:"Thunder Wave",t:"Electric",p:0,c:"status",a:90,status:"par"}]},
 {name:"Incineroar", types:["Fire","Dark"], base:{hp:95,atk:115,def:90,spa:80,spd:90,spe:60},
  ability:{n:"Blaze", type:"boost", boostType:"Fire", desc:"Fire moves +50% power when at 1/3 HP or below"},
  moves:[{n:"Darkest Lariat",t:"Dark",p:85,c:"phys",a:100},{n:"Flare Blitz",t:"Fire",p:120,c:"phys",a:100,sec:{status:"brn",chance:10}},{n:"Cross Chop",t:"Fighting",p:100,c:"phys",a:80},{n:"Fake Out",t:"Normal",p:40,c:"phys",a:100}]},
 {name:"Primarina", types:["Water","Fairy"], base:{hp:80,atk:74,def:74,spa:126,spd:116,spe:60},
  ability:{n:"Torrent", type:"boost", boostType:"Water", desc:"Water moves +50% power when at 1/3 HP or below"},
  moves:[{n:"Sparkling Aria",t:"Water",p:90,c:"spec",a:100},{n:"Moonblast",t:"Fairy",p:95,c:"spec",a:100},{n:"Scald",t:"Water",p:80,c:"spec",a:100,sec:{status:"brn",chance:30}},{n:"Ice Beam",t:"Ice",p:90,c:"spec",a:100,sec:{status:"frz",chance:10}}]},
 {name:"Mimikyu", types:["Ghost","Fairy"], base:{hp:55,atk:90,def:80,spa:50,spd:105,spe:96},
  ability:{n:"Disguise", type:"disguise", desc:"The first hit each battle deals 0 damage"},
  moves:[{n:"Play Rough",t:"Fairy",p:90,c:"phys",a:90},{n:"Shadow Claw",t:"Ghost",p:70,c:"phys",a:100},{n:"Shadow Sneak",t:"Ghost",p:40,c:"phys",a:100},{n:"Toxic",t:"Poison",p:0,c:"status",a:90,status:"psn"}]},
 {name:"Toxapex", types:["Poison","Water"], base:{hp:50,atk:63,def:152,spa:53,spd:142,spe:35},
  ability:{n:"Merciless", type:"merciless", desc:"Always lands a critical hit (1.5x dmg) against a poisoned target"},
  moves:[{n:"Poison Jab",t:"Poison",p:80,c:"phys",a:100,sec:{status:"psn",chance:30}},{n:"Liquidation",t:"Water",p:85,c:"phys",a:100},{n:"Toxic",t:"Poison",p:0,c:"status",a:90,status:"psn"},{n:"Venoshock",t:"Poison",p:65,c:"spec",a:100}]},
 {name:"Lycanroc", types:["Rock"], base:{hp:75,atk:115,def:65,spa:55,spd:65,spe:112},
  ability:{n:"Steadfast", type:"flavor", desc:"No numeric effect yet"},
  moves:[{n:"Stone Edge",t:"Rock",p:100,c:"phys",a:80},{n:"Accelerock",t:"Rock",p:40,c:"phys",a:100},{n:"Crunch",t:"Dark",p:80,c:"phys",a:100},{n:"Drill Run",t:"Ground",p:80,c:"phys",a:95}]},
 {name:"Ninetales-Alola", types:["Ice","Fairy"], base:{hp:73,atk:67,def:75,spa:81,spd:100,spe:109},
  ability:{n:"Snow Warning", type:"flavor", desc:"No numeric effect yet"},
  moves:[{n:"Blizzard",t:"Ice",p:110,c:"spec",a:70,sec:{status:"frz",chance:10}},{n:"Moonblast",t:"Fairy",p:95,c:"spec",a:100},{n:"Sing",t:"Normal",p:0,c:"status",a:55,status:"slp"},{n:"Psychic",t:"Psychic",p:90,c:"spec",a:100}]},
 {name:"Vikavolt", types:["Bug","Electric"], base:{hp:77,atk:70,def:90,spa:145,spd:75,spe:43},
  ability:{n:"Levitate", type:"immune", immuneType:"Ground", desc:"Immune to Ground-type moves"},
  moves:[{n:"Thunderbolt",t:"Electric",p:90,c:"spec",a:100,sec:{status:"par",chance:10}},{n:"Bug Buzz",t:"Bug",p:90,c:"spec",a:100},{n:"Thunder Wave",t:"Electric",p:0,c:"status",a:90,status:"par"},{n:"Flash Cannon",t:"Steel",p:80,c:"spec",a:100}]},
 {name:"Crabominable", types:["Fighting","Ice"], base:{hp:97,atk:132,def:77,spa:62,spd:67,spe:43},
  ability:{n:"Iron Fist", type:"punch", desc:"Punching moves +20% power"},
  moves:[{n:"Ice Hammer",t:"Ice",p:100,c:"phys",a:90},{n:"Close Combat",t:"Fighting",p:120,c:"phys",a:100},{n:"Ice Punch",t:"Ice",p:75,c:"phys",a:100,sec:{status:"frz",chance:10},punch:true},{n:"Rock Slide",t:"Rock",p:75,c:"phys",a:90}]},
 {name:"Ribombee", types:["Bug","Fairy"], base:{hp:60,atk:55,def:60,spa:95,spd:70,spe:124},
  ability:{n:"Shield Dust", type:"flavor", desc:"No numeric effect yet"},
  moves:[{n:"Bug Buzz",t:"Bug",p:90,c:"spec",a:100},{n:"Moonblast",t:"Fairy",p:95,c:"spec",a:100},{n:"Stun Spore",t:"Grass",p:0,c:"status",a:75,status:"par"},{n:"Energy Ball",t:"Grass",p:90,c:"spec",a:100}]},
 {name:"Kommo-o", types:["Dragon","Fighting"], base:{hp:75,atk:110,def:125,spa:100,spd:105,spe:85},
  ability:{n:"Bulletproof", type:"flavor", desc:"No numeric effect yet"},
  moves:[{n:"Clanging Scales",t:"Dragon",p:100,c:"spec",a:100},{n:"Close Combat",t:"Fighting",p:120,c:"phys",a:100},{n:"Flamethrower",t:"Fire",p:90,c:"spec",a:100,sec:{status:"brn",chance:10}},{n:"Poison Jab",t:"Poison",p:80,c:"phys",a:100,sec:{status:"psn",chance:30}}]},
 {name:"Golisopod", types:["Bug","Water"], base:{hp:75,atk:125,def:140,spa:64,spd:57,spe:40},
  ability:{n:"Swarm", type:"boost", boostType:"Bug", desc:"Bug moves +50% power when at 1/3 HP or below"},
  moves:[{n:"First Impression",t:"Bug",p:90,c:"phys",a:100},{n:"Liquidation",t:"Water",p:85,c:"phys",a:100},{n:"Toxic",t:"Poison",p:0,c:"status",a:90,status:"psn"},{n:"Rock Slide",t:"Rock",p:75,c:"phys",a:90}]},
 {name:"Mudsdale", types:["Ground"], base:{hp:100,atk:125,def:100,spa:55,spd:85,spe:35},
  ability:{n:"Stamina", type:"flavor", desc:"No numeric effect yet"},
  moves:[{n:"Earthquake",t:"Ground",p:100,c:"phys",a:100},{n:"Rock Slide",t:"Rock",p:75,c:"phys",a:90},{n:"Superpower",t:"Fighting",p:120,c:"phys",a:100},{n:"Stomping Tantrum",t:"Ground",p:75,c:"phys",a:100}]},
 {name:"Salazzle", types:["Poison","Fire"], base:{hp:68,atk:64,def:60,spa:111,spd:60,spe:117},
  ability:{n:"Corrosion", type:"corrosion", desc:"Poison-type moves can hit Steel-type Pokémon normally"},
  moves:[{n:"Flamethrower",t:"Fire",p:90,c:"spec",a:100,sec:{status:"brn",chance:10}},{n:"Sludge Bomb",t:"Poison",p:90,c:"spec",a:100,sec:{status:"psn",chance:30}},{n:"Toxic",t:"Poison",p:0,c:"status",a:90,status:"psn"},{n:"Fake Out",t:"Normal",p:40,c:"phys",a:100}]},
 {name:"Toucannon", types:["Normal","Flying"], base:{hp:80,atk:120,def:75,spa:75,spd:75,spe:60},
  ability:{n:"Keen Eye", type:"flavor", desc:"No numeric effect yet"},
  moves:[{n:"Drill Peck",t:"Flying",p:80,c:"phys",a:100},{n:"Rock Smash",t:"Fighting",p:40,c:"phys",a:100},{n:"Bullet Seed",t:"Grass",p:25,c:"phys",a:100},{n:"Facade",t:"Normal",p:70,c:"phys",a:100}]},
 {name:"Lurantis", types:["Grass"], base:{hp:70,atk:105,def:90,spa:80,spd:90,spe:45},
  ability:{n:"Contrary", type:"flavor", desc:"No numeric effect yet"},
  moves:[{n:"Leaf Blade",t:"Grass",p:90,c:"phys",a:100},{n:"Superpower",t:"Fighting",p:120,c:"phys",a:100},{n:"Leech Life",t:"Bug",p:80,c:"phys",a:100},{n:"Poison Jab",t:"Poison",p:80,c:"phys",a:100,sec:{status:"psn",chance:30}}]}
];

// Real representative moves per type, used to auto-generate movesets for the procedural dex below.
const TYPE_MOVEPOOL = {
 Normal:[{n:"Body Slam",t:"Normal",p:85,c:"phys",a:100},{n:"Hyper Voice",t:"Normal",p:90,c:"spec",a:100},{n:"Facade",t:"Normal",p:70,c:"phys",a:100}],
 Fire:[{n:"Flamethrower",t:"Fire",p:90,c:"spec",a:100,sec:{status:"brn",chance:10}},{n:"Flare Blitz",t:"Fire",p:120,c:"phys",a:100,sec:{status:"brn",chance:10}},{n:"Will-O-Wisp",t:"Fire",p:0,c:"status",a:85,status:"brn"}],
 Water:[{n:"Surf",t:"Water",p:90,c:"spec",a:100},{n:"Liquidation",t:"Water",p:85,c:"phys",a:100},{n:"Scald",t:"Water",p:80,c:"spec",a:100,sec:{status:"brn",chance:30}}],
 Electric:[{n:"Thunderbolt",t:"Electric",p:90,c:"spec",a:100,sec:{status:"par",chance:10}},{n:"Wild Charge",t:"Electric",p:90,c:"phys",a:100},{n:"Thunder Wave",t:"Electric",p:0,c:"status",a:90,status:"par"}],
 Grass:[{n:"Energy Ball",t:"Grass",p:90,c:"spec",a:100},{n:"Leaf Blade",t:"Grass",p:90,c:"phys",a:100},{n:"Sleep Powder",t:"Grass",p:0,c:"status",a:75,status:"slp"}],
 Ice:[{n:"Ice Beam",t:"Ice",p:90,c:"spec",a:100,sec:{status:"frz",chance:10}},{n:"Icicle Crash",t:"Ice",p:85,c:"phys",a:90},{n:"Blizzard",t:"Ice",p:110,c:"spec",a:70,sec:{status:"frz",chance:10}}],
 Fighting:[{n:"Close Combat",t:"Fighting",p:120,c:"phys",a:100},{n:"Focus Blast",t:"Fighting",p:120,c:"spec",a:70},{n:"Superpower",t:"Fighting",p:120,c:"phys",a:100}],
 Poison:[{n:"Sludge Bomb",t:"Poison",p:90,c:"spec",a:100,sec:{status:"psn",chance:30}},{n:"Poison Jab",t:"Poison",p:80,c:"phys",a:100,sec:{status:"psn",chance:30}},{n:"Toxic",t:"Poison",p:0,c:"status",a:90,status:"psn"}],
 Ground:[{n:"Earthquake",t:"Ground",p:100,c:"phys",a:100},{n:"Earth Power",t:"Ground",p:90,c:"spec",a:100},{n:"Stomping Tantrum",t:"Ground",p:75,c:"phys",a:100}],
 Flying:[{n:"Air Slash",t:"Flying",p:75,c:"spec",a:95},{n:"Brave Bird",t:"Flying",p:120,c:"phys",a:100},{n:"Hurricane",t:"Flying",p:110,c:"spec",a:70}],
 Psychic:[{n:"Psychic",t:"Psychic",p:90,c:"spec",a:100},{n:"Psycho Cut",t:"Psychic",p:70,c:"phys",a:100},{n:"Hypnosis",t:"Psychic",p:0,c:"status",a:60,status:"slp"}],
 Bug:[{n:"Bug Buzz",t:"Bug",p:90,c:"spec",a:100},{n:"X-Scissor",t:"Bug",p:80,c:"phys",a:100},{n:"Leech Life",t:"Bug",p:80,c:"phys",a:100}],
 Rock:[{n:"Stone Edge",t:"Rock",p:100,c:"phys",a:80},{n:"Power Gem",t:"Rock",p:80,c:"spec",a:100},{n:"Rock Slide",t:"Rock",p:75,c:"phys",a:90}],
 Ghost:[{n:"Shadow Ball",t:"Ghost",p:80,c:"spec",a:100},{n:"Shadow Claw",t:"Ghost",p:70,c:"phys",a:100},{n:"Shadow Sneak",t:"Ghost",p:40,c:"phys",a:100}],
 Dragon:[{n:"Dragon Pulse",t:"Dragon",p:85,c:"spec",a:100},{n:"Outrage",t:"Dragon",p:120,c:"phys",a:100},{n:"Dragon Claw",t:"Dragon",p:80,c:"phys",a:100}],
 Dark:[{n:"Dark Pulse",t:"Dark",p:80,c:"spec",a:100},{n:"Crunch",t:"Dark",p:80,c:"phys",a:100},{n:"Sucker Punch",t:"Dark",p:70,c:"phys",a:100}],
 Steel:[{n:"Flash Cannon",t:"Steel",p:80,c:"spec",a:100},{n:"Iron Head",t:"Steel",p:80,c:"phys",a:100},{n:"Meteor Mash",t:"Steel",p:90,c:"phys",a:90}],
 Fairy:[{n:"Moonblast",t:"Fairy",p:95,c:"spec",a:100},{n:"Play Rough",t:"Fairy",p:90,c:"phys",a:90},{n:"Dazzling Gleam",t:"Fairy",p:80,c:"spec",a:100}]
};
function genMoves(types){
  let pool=[];
  for(const t of types) pool.push(...TYPE_MOVEPOOL[t]);
  while(pool.length<4) pool.push(...TYPE_MOVEPOOL['Normal']);
  return pool.slice(0,4);
}

// Gen 1 (Kanto), #1-151. Stats from memory, not individually search-verified like the curated entries above.
// Format: [name, type1, type2-or-null, HP, Atk, Def, SpA, SpD, Spe]
const PROC_RAW = [
["Bulbasaur","Grass","Poison",45,49,49,65,65,45],["Ivysaur","Grass","Poison",60,62,63,80,80,60],["Venusaur","Grass","Poison",80,82,83,100,100,80],
["Charmander","Fire",null,39,52,43,60,50,65],["Charmeleon","Fire",null,58,64,58,80,65,80],["Charizard","Fire","Flying",78,84,78,109,85,100],
["Squirtle","Water",null,44,48,65,50,64,43],["Wartortle","Water",null,59,63,80,65,80,58],["Blastoise","Water",null,79,83,100,85,105,78],
["Caterpie","Bug",null,45,30,35,20,20,45],["Metapod","Bug",null,50,20,55,25,25,30],["Butterfree","Bug","Flying",60,45,50,90,80,70],
["Weedle","Bug","Poison",40,35,30,20,20,50],["Kakuna","Bug","Poison",45,25,50,25,25,35],["Beedrill","Bug","Poison",65,90,40,45,80,75],
["Pidgey","Normal","Flying",40,45,40,35,35,56],["Pidgeotto","Normal","Flying",63,60,55,50,50,71],["Pidgeot","Normal","Flying",83,80,75,70,70,101],
["Rattata","Normal",null,30,56,35,25,35,72],["Raticate","Normal",null,55,81,60,50,70,97],
["Spearow","Normal","Flying",40,60,30,31,31,70],["Fearow","Normal","Flying",65,90,65,61,61,100],
["Ekans","Poison",null,35,60,44,40,54,55],["Arbok","Poison",null,60,85,69,65,79,80],
["Pikachu","Electric",null,35,55,40,50,50,90],["Raichu","Electric",null,60,90,55,90,80,110],
["Sandshrew","Ground",null,50,75,85,20,30,40],["Sandslash","Ground",null,75,100,110,45,55,65],
["Nidoran-F","Poison",null,55,47,52,40,40,41],["Nidorina","Poison",null,70,62,67,55,55,56],["Nidoqueen","Poison","Ground",90,92,87,75,85,76],
["Nidoran-M","Poison",null,46,57,40,40,40,50],["Nidorino","Poison",null,61,72,57,55,55,65],["Nidoking","Poison","Ground",81,102,77,85,75,85],
["Clefairy","Fairy",null,70,45,48,60,65,35],["Clefable","Fairy",null,95,70,73,95,90,60],
["Vulpix","Fire",null,38,41,40,50,65,65],["Ninetales","Fire",null,73,76,75,81,100,100],
["Jigglypuff","Normal","Fairy",115,45,20,45,25,20],["Wigglytuff","Normal","Fairy",140,70,45,85,50,45],
["Zubat","Poison","Flying",40,45,35,30,40,55],["Golbat","Poison","Flying",75,80,70,65,75,90],
["Oddish","Grass","Poison",45,50,55,75,65,30],["Gloom","Grass","Poison",60,65,70,85,75,40],["Vileplume","Grass","Poison",75,80,85,110,90,50],
["Paras","Bug","Grass",35,70,55,45,55,25],["Parasect","Bug","Grass",60,95,80,60,80,30],
["Venonat","Bug","Poison",60,55,50,40,55,45],["Venomoth","Bug","Poison",70,65,60,90,75,90],
["Diglett","Ground",null,10,55,25,35,45,95],["Dugtrio","Ground",null,35,100,50,50,70,120],
["Meowth","Normal",null,40,45,35,40,40,90],["Persian","Normal",null,65,70,60,65,65,115],
["Psyduck","Water",null,50,52,48,65,50,55],["Golduck","Water",null,80,82,78,95,80,85],
["Mankey","Fighting",null,40,80,35,35,45,70],["Primeape","Fighting",null,65,105,60,60,70,95],
["Growlithe","Fire",null,55,70,45,70,50,60],["Arcanine","Fire",null,90,110,80,100,80,95],
["Poliwag","Water",null,40,50,40,40,40,90],["Poliwhirl","Water",null,65,65,65,50,50,90],["Poliwrath","Water","Fighting",90,95,95,70,90,70],
["Abra","Psychic",null,25,20,15,105,55,90],["Kadabra","Psychic",null,40,35,30,120,70,105],["Alakazam","Psychic",null,55,50,45,135,95,120],
["Machop","Fighting",null,70,80,50,35,35,35],["Machoke","Fighting",null,80,100,70,50,60,45],["Machamp","Fighting",null,90,130,80,65,85,55],
["Bellsprout","Grass","Poison",50,75,35,70,30,40],["Weepinbell","Grass","Poison",65,90,50,85,45,55],["Victreebel","Grass","Poison",80,105,65,100,70,70],
["Tentacool","Water","Poison",40,40,35,50,100,70],["Tentacruel","Water","Poison",80,70,65,80,120,100],
["Geodude","Rock","Ground",40,80,100,30,30,20],["Graveler","Rock","Ground",55,95,115,45,45,35],["Golem","Rock","Ground",80,120,130,55,65,45],
["Ponyta","Fire",null,50,85,55,65,65,90],["Rapidash","Fire",null,65,100,70,80,80,105],
["Slowpoke","Water","Psychic",90,65,65,40,40,15],["Slowbro","Water","Psychic",95,75,110,100,80,30],
["Magnemite","Electric","Steel",25,35,70,95,55,45],["Magneton","Electric","Steel",50,60,95,120,70,70],
["Farfetchd","Normal","Flying",52,65,55,58,62,60],
["Doduo","Normal","Flying",35,85,45,35,35,75],["Dodrio","Normal","Flying",60,110,70,60,60,110],
["Seel","Water",null,65,45,55,45,70,45],["Dewgong","Water","Ice",90,70,80,70,95,70],
["Grimer","Poison",null,80,80,50,40,50,25],["Muk","Poison",null,105,105,75,65,100,50],
["Shellder","Water",null,30,65,100,45,25,40],["Cloyster","Water","Ice",50,95,180,85,45,70],
["Gastly","Ghost","Poison",30,35,30,100,35,80],["Haunter","Ghost","Poison",45,50,45,115,55,95],["Gengar","Ghost","Poison",60,65,60,130,75,110],
["Onix","Rock","Ground",35,45,160,30,45,70],
["Drowzee","Psychic",null,60,48,45,43,90,42],["Hypno","Psychic",null,85,73,70,73,115,67],
["Krabby","Water",null,30,105,90,25,25,50],["Kingler","Water",null,55,130,115,50,50,75],
["Voltorb","Electric",null,40,30,50,55,55,100],["Electrode","Electric",null,60,50,70,80,80,150],
["Exeggcute","Grass","Psychic",60,40,80,60,45,40],["Exeggutor","Grass","Psychic",95,95,85,125,75,55],
["Cubone","Ground",null,50,50,95,40,50,35],["Marowak","Ground",null,60,80,110,50,80,45],
["Hitmonlee","Fighting",null,50,120,53,35,110,87],["Hitmonchan","Fighting",null,50,105,79,35,110,76],
["Lickitung","Normal",null,90,55,75,60,75,30],
["Koffing","Poison",null,40,65,95,60,45,35],["Weezing","Poison",null,65,90,120,85,70,60],
["Rhyhorn","Ground","Rock",80,85,95,30,30,25],["Rhydon","Ground","Rock",105,130,120,45,45,40],
["Chansey","Normal",null,250,5,5,35,105,50],
["Tangela","Grass",null,65,55,115,100,40,60],
["Kangaskhan","Normal",null,105,95,80,40,80,90],
["Horsea","Water",null,30,40,70,70,25,60],["Seadra","Water",null,55,65,95,95,45,85],
["Goldeen","Water",null,45,67,60,35,50,63],["Seaking","Water",null,80,92,65,65,80,68],
["Staryu","Water",null,30,45,55,70,55,85],["Starmie","Water","Psychic",60,75,85,100,85,115],
["Mr-Mime","Psychic","Fairy",40,45,65,100,120,90],
["Scyther","Bug","Flying",70,110,80,55,80,105],
["Jynx","Ice","Psychic",65,50,35,115,95,95],
["Electabuzz","Electric",null,65,83,57,95,85,105],
["Magmar","Fire",null,65,95,57,100,85,93],
["Pinsir","Bug",null,65,125,100,55,70,85],
["Tauros","Normal",null,75,100,95,40,70,110],
["Magikarp","Water",null,20,10,55,15,20,80],["Gyarados","Water","Flying",95,125,79,60,100,81],
["Lapras","Water","Ice",130,85,80,85,95,60],
["Ditto","Normal",null,48,48,48,48,48,48],
["Eevee","Normal",null,55,55,50,45,65,55],
["Vaporeon","Water",null,130,65,60,110,95,65],["Jolteon","Electric",null,65,65,60,110,95,130],["Flareon","Fire",null,65,130,60,95,110,65],
["Porygon","Normal",null,65,60,70,85,75,40],
["Omanyte","Rock","Water",35,40,100,90,55,35],["Omastar","Rock","Water",70,60,125,115,70,55],
["Kabuto","Rock","Water",30,80,90,55,45,55],["Kabutops","Rock","Water",60,115,105,65,70,80],
["Aerodactyl","Rock","Flying",80,105,65,60,75,130],
["Snorlax","Normal",null,160,110,65,65,110,30],
["Articuno","Ice","Flying",90,85,100,95,125,85],["Zapdos","Electric","Flying",90,90,85,125,90,100],["Moltres","Fire","Flying",90,100,90,125,85,90],
["Dratini","Dragon",null,41,64,45,50,50,50],["Dragonair","Dragon",null,61,84,65,70,70,70],["Dragonite","Dragon","Flying",91,134,95,100,100,80],
["Mewtwo","Psychic",null,106,110,90,154,90,130],["Mew","Psychic",null,100,100,100,100,100,100]
];
// Gen 2 (Johto), #152-251. Stats from memory, not individually search-verified like the curated entries above.
const PROC_RAW_GEN2 = [
["Chikorita","Grass",null,45,49,65,49,65,45],["Bayleef","Grass",null,60,62,80,63,80,60],["Meganium","Grass",null,80,82,100,83,100,80],
["Cyndaquil","Fire",null,39,52,43,60,50,65],["Quilava","Fire",null,58,64,58,80,65,80],["Typhlosion","Fire",null,78,84,78,109,85,100],
["Totodile","Water",null,50,65,64,44,48,43],["Croconaw","Water",null,65,80,80,59,63,58],["Feraligatr","Water",null,85,105,100,79,83,78],
["Sentret","Normal",null,35,46,34,35,45,20],["Furret","Normal",null,85,76,64,45,55,90],
["Hoothoot","Normal","Flying",60,30,30,36,56,50],["Noctowl","Normal","Flying",100,50,50,86,96,70],
["Ledyba","Bug","Flying",40,20,30,40,80,55],["Ledian","Bug","Flying",55,35,50,55,110,85],
["Spinarak","Bug","Poison",40,60,40,40,40,30],["Ariados","Bug","Poison",70,90,70,60,70,40],
["Crobat","Poison","Flying",85,90,80,70,80,130],
["Chinchou","Water","Electric",75,38,38,56,56,67],["Lanturn","Water","Electric",125,58,58,76,76,67],
["Pichu","Electric",null,20,40,15,35,35,60],
["Cleffa","Fairy",null,50,25,28,45,55,15],
["Igglybuff","Normal","Fairy",90,30,15,40,20,15],
["Togepi","Fairy",null,35,20,65,40,65,20],["Togetic","Fairy","Flying",55,40,85,80,105,40],
["Natu","Psychic","Flying",40,50,45,70,45,70],["Xatu","Psychic","Flying",65,75,70,95,70,95],
["Mareep","Electric",null,55,40,40,65,45,35],["Flaaffy","Electric",null,70,55,55,80,60,45],["Ampharos","Electric",null,90,75,85,115,90,55],
["Bellossom","Grass",null,75,80,95,90,100,50],
["Marill","Water","Fairy",70,20,50,20,50,40],["Azumarill","Water","Fairy",100,50,80,50,80,50],
["Sudowoodo","Rock",null,70,100,115,30,65,30],
["Politoed","Water",null,90,75,75,90,100,70],
["Hoppip","Grass","Flying",35,35,40,35,55,50],["Skiploom","Grass","Flying",55,45,50,45,65,80],["Jumpluff","Grass","Flying",75,55,70,55,95,110],
["Aipom","Normal",null,55,70,55,40,55,85],
["Sunkern","Grass",null,30,30,30,30,30,30],["Sunflora","Grass",null,75,75,55,105,85,30],
["Yanma","Bug","Flying",65,65,45,75,45,95],
["Wooper","Water","Ground",55,45,45,25,25,15],["Quagsire","Water","Ground",95,85,85,65,65,35],
["Espeon","Psychic",null,65,65,60,130,95,110],["Umbreon","Dark",null,95,65,110,60,130,65],
["Murkrow","Dark","Flying",60,85,42,85,42,91],
["Slowking","Water","Psychic",95,75,80,100,110,30],
["Misdreavus","Ghost",null,60,60,60,85,85,85],
["Unown","Psychic",null,48,72,48,72,48,48],
["Wobbuffet","Psychic",null,190,33,58,33,58,33],
["Girafarig","Normal","Psychic",70,80,65,90,65,85],
["Pineco","Bug",null,50,65,90,35,35,15],["Forretress","Bug","Steel",75,90,140,60,60,40],
["Dunsparce","Normal",null,100,70,70,65,65,45],
["Gligar","Ground","Flying",65,75,105,35,65,85],
["Steelix","Steel","Ground",75,85,200,55,65,30],
["Snubbull","Fairy",null,60,80,50,40,40,30],["Granbull","Fairy",null,90,120,75,60,60,45],
["Qwilfish","Water","Poison",65,95,75,55,55,85],
["Scizor","Bug","Steel",70,130,100,55,80,65],
["Shuckle","Bug","Rock",20,10,230,10,230,5],
["Heracross","Bug","Fighting",80,125,75,40,95,85],
["Sneasel","Dark","Ice",55,95,55,35,75,115],
["Teddiursa","Normal",null,60,80,50,50,50,40],["Ursaring","Normal",null,90,130,75,75,75,55],
["Slugma","Fire",null,40,40,40,70,40,20],["Magcargo","Fire","Rock",50,50,120,80,80,30],
["Swinub","Ice","Ground",50,50,40,30,30,50],["Piloswine","Ice","Ground",100,100,80,60,60,50],
["Corsola","Water","Rock",65,55,95,65,95,35],
["Remoraid","Water",null,35,65,35,65,35,65],["Octillery","Water",null,75,105,75,105,75,45],
["Delibird","Ice","Flying",45,55,45,65,45,75],
["Mantine","Water","Flying",65,40,70,80,140,70],
["Skarmory","Steel","Flying",65,80,140,40,70,70],
["Houndour","Dark","Fire",45,60,30,80,50,65],["Houndoom","Dark","Fire",75,90,50,110,80,95],
["Kingdra","Water","Dragon",75,95,95,95,95,85],
["Phanpy","Ground",null,90,60,60,40,40,40],["Donphan","Ground",null,90,120,120,60,60,50],
["Porygon2","Normal",null,85,80,90,105,95,60],
["Stantler","Normal",null,73,95,62,85,65,85],
["Smeargle","Normal",null,55,20,35,20,45,75],
["Tyrogue","Fighting",null,35,35,35,35,35,35],
["Hitmontop","Fighting",null,50,95,95,35,110,70],
["Smoochum","Ice","Psychic",45,30,15,85,65,65],
["Elekid","Electric",null,45,63,37,65,55,95],
["Magby","Fire",null,45,75,37,70,55,83],
["Miltank","Normal",null,95,80,105,40,70,100],
["Blissey","Normal",null,255,10,10,75,135,55],
["Raikou","Electric",null,90,85,75,115,100,115],["Entei","Fire",null,115,115,85,90,75,100],["Suicune","Water",null,100,75,115,90,115,85],
["Larvitar","Rock","Ground",50,64,50,45,50,41],["Pupitar","Rock","Ground",70,84,70,65,70,51],["Tyranitar","Rock","Dark",100,134,110,95,100,61],
["Lugia","Psychic","Flying",106,90,130,90,154,110],["Ho-Oh","Fire","Flying",106,130,90,110,154,90],
["Celebi","Psychic","Grass",100,100,100,100,100,100]
];
function procEntry(row){
  const [name,t1,t2,hp,atk,def,spa,spd,spe] = row;
  const types = t2 ? [t1,t2] : [t1];
  return {name, types, base:{hp,atk,def,spa,spd,spe},
    ability:{n:"—", type:"flavor", desc:"Generic entry — not yet individually curated"},
    moves: genMoves(types)};
}
// Gen 3 (Hoenn), #252-386. Stats from memory, not individually search-verified like the curated entries above.
const PROC_RAW_GEN3 = [
["Treecko","Grass",null,40,45,35,65,55,70],["Grovyle","Grass",null,50,65,45,85,65,95],["Sceptile","Grass",null,70,85,65,105,85,120],
["Torchic","Fire",null,45,60,40,70,50,45],["Combusken","Fire","Fighting",60,85,60,85,60,55],["Blaziken","Fire","Fighting",80,120,70,110,70,80],
["Mudkip","Water",null,50,70,50,50,50,40],["Marshtomp","Water","Ground",70,85,70,60,70,50],["Swampert","Water","Ground",100,110,90,85,90,60],
["Poochyena","Dark",null,35,55,35,30,30,35],["Mightyena","Dark",null,70,90,70,60,60,70],
["Zigzagoon","Normal",null,38,30,41,30,41,60],["Linoone","Normal",null,78,70,61,50,61,100],
["Wurmple","Bug",null,45,45,35,20,30,20],["Silcoon","Bug",null,50,35,55,25,25,15],["Beautifly","Bug","Flying",60,70,50,90,50,65],
["Cascoon","Bug",null,50,35,55,25,25,15],["Dustox","Bug","Poison",60,50,70,50,90,65],
["Lotad","Water","Grass",40,30,30,40,50,30],["Lombre","Water","Grass",60,50,50,60,70,50],["Ludicolo","Water","Grass",80,70,70,90,100,70],
["Seedot","Grass",null,40,40,50,30,30,30],["Nuzleaf","Grass","Dark",70,70,40,60,40,60],["Shiftry","Grass","Dark",90,100,60,90,60,80],
["Taillow","Normal","Flying",40,55,30,30,30,85],["Swellow","Normal","Flying",60,85,60,50,50,125],
["Wingull","Water","Flying",40,30,30,55,30,85],["Pelipper","Water","Flying",60,50,100,85,70,65],
["Ralts","Psychic","Fairy",28,25,25,45,35,40],["Kirlia","Psychic","Fairy",38,35,35,65,55,50],["Gardevoir","Psychic","Fairy",68,65,65,125,115,80],
["Surskit","Bug","Water",40,30,32,50,52,65],["Masquerain","Bug","Flying",70,60,62,80,82,60],
["Shroomish","Grass",null,60,40,60,40,60,35],["Breloom","Grass","Fighting",60,130,80,60,60,70],
["Slakoth","Normal",null,60,60,60,35,35,30],["Vigoroth","Normal",null,80,80,80,55,55,90],["Slaking","Normal",null,150,160,100,95,65,100],
["Nincada","Bug","Ground",31,45,90,30,30,40],["Ninjask","Bug","Flying",61,90,45,50,50,160],["Shedinja","Bug","Ghost",1,90,45,30,30,40],
["Whismur","Normal",null,64,51,23,51,23,28],["Loudred","Normal",null,84,71,43,71,43,48],["Exploud","Normal",null,104,91,63,91,73,68],
["Makuhita","Fighting",null,72,60,30,20,30,25],["Hariyama","Fighting",null,144,120,60,40,60,50],
["Azurill","Normal","Fairy",50,20,40,20,40,20],
["Nosepass","Rock",null,30,45,135,45,90,30],
["Skitty","Normal",null,50,45,45,35,35,50],["Delcatty","Normal",null,70,65,65,55,55,70],
["Sableye","Dark","Ghost",50,75,75,65,65,50],
["Mawile","Steel","Fairy",50,85,85,55,55,50],
["Aron","Steel","Rock",50,70,100,40,40,30],["Lairon","Steel","Rock",60,90,140,50,50,40],["Aggron","Steel","Rock",70,110,180,60,60,50],
["Meditite","Fighting","Psychic",30,40,55,40,55,60],["Medicham","Fighting","Psychic",60,60,75,60,75,80],
["Electrike","Electric",null,40,45,40,65,40,65],["Manectric","Electric",null,70,75,60,105,60,105],
["Plusle","Electric",null,60,50,40,85,75,95],["Minun","Electric",null,60,40,50,75,85,95],
["Volbeat","Bug",null,65,73,55,47,75,85],["Illumise","Bug",null,65,47,55,73,75,85],
["Roselia","Grass","Poison",50,60,45,100,80,65],
["Gulpin","Poison",null,70,43,53,43,53,40],["Swalot","Poison",null,100,73,83,73,83,55],
["Carvanha","Water","Dark",45,90,20,65,20,65],["Sharpedo","Water","Dark",70,120,40,95,40,95],
["Wailmer","Water",null,130,70,35,70,35,60],["Wailord","Water",null,170,90,45,90,45,60],
["Numel","Fire","Ground",60,60,40,65,45,35],["Camerupt","Fire","Ground",70,100,70,105,75,40],
["Torkoal","Fire",null,70,85,140,85,70,20],
["Spoink","Psychic",null,60,25,35,70,80,60],["Grumpig","Psychic",null,80,45,65,90,110,80],
["Spinda","Normal",null,60,60,60,60,60,60],
["Trapinch","Ground",null,45,100,45,45,45,10],["Vibrava","Ground","Dragon",50,70,50,50,50,70],["Flygon","Ground","Dragon",80,100,80,80,80,100],
["Cacnea","Grass",null,50,85,40,85,40,35],["Cacturne","Grass","Dark",70,115,60,115,60,55],
["Swablu","Normal","Flying",45,40,60,40,75,50],["Altaria","Dragon","Flying",75,70,90,70,105,80],
["Zangoose","Normal",null,73,115,60,60,60,90],
["Seviper","Poison",null,73,100,60,100,60,65],
["Lunatone","Rock","Psychic",70,55,65,95,85,70],["Solrock","Rock","Psychic",70,95,85,55,65,70],
["Barboach","Water","Ground",50,48,43,46,41,60],["Whiscash","Water","Ground",110,78,73,76,71,60],
["Corphish","Water",null,43,80,65,50,35,35],["Crawdaunt","Water","Dark",63,120,85,90,55,55],
["Baltoy","Ground","Psychic",40,40,55,40,70,55],["Claydol","Ground","Psychic",60,70,105,70,120,75],
["Lileep","Rock","Grass",66,41,77,61,87,23],["Cradily","Rock","Grass",86,81,97,81,107,43],
["Anorith","Rock","Bug",45,95,50,40,50,75],["Armaldo","Rock","Bug",75,125,100,70,80,45],
["Feebas","Water",null,20,15,20,10,55,80],["Milotic","Water",null,95,60,79,100,125,81],
["Castform","Normal",null,70,70,70,70,70,70],
["Kecleon","Normal",null,60,90,70,60,120,40],
["Shuppet","Ghost",null,44,75,35,63,33,45],["Banette","Ghost",null,64,115,65,83,63,65],
["Duskull","Ghost",null,20,40,90,30,90,25],["Dusclops","Ghost",null,40,70,130,60,130,25],
["Tropius","Grass","Flying",99,68,83,72,87,51],
["Chimecho","Psychic",null,65,50,70,95,80,65],
["Absol","Dark",null,65,130,60,75,60,75],
["Wynaut","Psychic",null,95,23,48,23,48,23],
["Snorunt","Ice",null,50,50,50,50,50,50],["Glalie","Ice",null,80,80,80,80,80,80],
["Spheal","Ice","Water",70,40,50,55,50,25],["Sealeo","Ice","Water",90,60,70,75,70,45],["Walrein","Ice","Water",110,80,90,95,90,65],
["Clamperl","Water",null,35,64,85,74,55,32],["Huntail","Water",null,55,104,105,94,75,52],["Gorebyss","Water",null,55,84,105,114,75,52],
["Relicanth","Water","Rock",100,90,130,45,65,55],
["Luvdisc","Water",null,43,30,55,40,65,97],
["Bagon","Dragon",null,45,75,60,40,30,50],["Shelgon","Dragon",null,65,95,100,60,50,50],["Salamence","Dragon","Flying",95,135,80,110,80,100],
["Beldum","Steel","Psychic",40,55,80,35,60,30],["Metang","Steel","Psychic",60,75,100,55,80,50],["Metagross","Steel","Psychic",80,135,130,95,90,70],
["Regirock","Rock",null,80,100,200,50,100,50],["Regice","Ice",null,80,50,100,100,200,50],["Registeel","Steel",null,80,75,150,75,150,50],
["Latias","Dragon","Psychic",80,80,90,110,130,110],["Latios","Dragon","Psychic",80,90,80,130,110,110],
["Kyogre","Water",null,100,100,90,150,140,90],["Groudon","Ground",null,100,150,140,100,90,90],["Rayquaza","Dragon","Flying",105,150,90,150,90,95],
["Jirachi","Steel","Psychic",100,100,100,100,100,100],["Deoxys","Psychic",null,50,150,50,150,50,150]
];
// Gen 4 (Sinnoh), #387-493. Stats from memory, not individually search-verified like the curated entries above.
const PROC_RAW_GEN4 = [
["Turtwig","Grass",null,55,68,64,45,55,31],["Grotle","Grass",null,75,89,85,55,65,36],["Torterra","Grass","Ground",95,109,105,75,85,56],
["Chimchar","Fire",null,44,58,44,58,44,61],["Monferno","Fire","Fighting",64,78,52,78,52,81],["Infernape","Fire","Fighting",76,104,71,104,71,108],
["Piplup","Water",null,53,51,53,61,56,40],["Prinplup","Water",null,64,66,68,81,76,50],["Empoleon","Water","Steel",84,86,88,111,101,60],
["Starly","Normal","Flying",40,55,30,30,30,60],["Staravia","Normal","Flying",55,75,50,40,40,80],["Staraptor","Normal","Flying",85,120,70,50,60,100],
["Bidoof","Normal",null,59,45,40,35,40,31],["Bibarel","Normal","Water",79,85,60,55,60,71],
["Kricketot","Bug",null,37,25,41,25,41,25],["Kricketune","Bug",null,77,85,51,55,51,65],
["Shinx","Electric",null,45,65,34,40,34,45],["Luxio","Electric",null,60,85,49,60,49,60],["Luxray","Electric",null,80,120,79,95,79,70],
["Budew","Grass","Poison",40,30,35,50,70,55],["Roserade","Grass","Poison",60,70,55,125,105,90],
["Cranidos","Rock",null,67,125,40,30,30,58],["Rampardos","Rock",null,97,165,60,65,50,58],
["Shieldon","Rock","Steel",30,42,118,42,88,30],["Bastiodon","Rock","Steel",60,52,168,47,138,30],
["Burmy","Bug",null,40,29,45,29,45,36],["Wormadam","Bug","Grass",60,59,85,79,105,36],["Mothim","Bug","Flying",70,94,50,94,50,66],
["Combee","Bug","Flying",30,30,42,30,42,70],["Vespiquen","Bug","Flying",70,80,102,80,102,40],
["Pachirisu","Electric",null,60,45,70,45,90,95],
["Buizel","Water",null,55,65,35,60,30,85],["Floatzel","Water",null,85,105,55,85,50,115],
["Cherubi","Grass",null,45,35,45,62,53,35],["Cherrim","Grass",null,70,60,70,87,78,85],
["Shellos","Water",null,76,48,48,57,62,34],["Gastrodon","Water","Ground",111,83,68,92,82,39],
["Ambipom","Normal",null,75,100,66,60,66,115],
["Drifloon","Ghost","Flying",90,50,34,60,44,70],["Drifblim","Ghost","Flying",150,80,44,90,54,80],
["Buneary","Normal",null,55,66,44,44,56,85],["Lopunny","Normal",null,65,76,84,54,96,105],
["Mismagius","Ghost",null,60,60,60,105,105,105],
["Honchkrow","Dark","Flying",100,125,52,105,52,71],
["Glameow","Normal",null,49,55,42,42,37,85],["Purugly","Normal",null,71,82,64,64,59,112],
["Chingling","Psychic",null,45,30,50,65,50,45],
["Stunky","Poison","Dark",63,63,47,41,41,74],["Skuntank","Poison","Dark",103,93,67,71,61,84],
["Bronzor","Steel","Psychic",57,24,86,24,86,23],["Bronzong","Steel","Psychic",67,89,116,79,116,33],
["Bonsly","Rock",null,50,80,95,10,45,10],
["Mime-Jr","Psychic","Fairy",20,25,45,70,90,60],
["Happiny","Normal",null,100,5,5,15,65,30],
["Chatot","Normal","Flying",76,65,45,92,42,91],
["Spiritomb","Ghost","Dark",50,92,108,92,108,35],
["Gible","Dragon","Ground",58,70,45,40,45,42],["Gabite","Dragon","Ground",68,90,65,50,55,82],["Garchomp","Dragon","Ground",108,130,95,80,85,102],
["Munchlax","Normal",null,135,85,40,40,85,5],
["Riolu","Fighting",null,40,70,40,35,40,60],["Lucario","Fighting","Steel",70,110,70,115,70,90],
["Hippopotas","Ground",null,68,72,78,38,42,32],["Hippowdon","Ground",null,108,112,118,68,72,47],
["Skorupi","Poison","Bug",40,50,90,30,55,65],["Drapion","Poison","Dark",70,90,110,60,75,95],
["Croagunk","Poison","Fighting",48,61,40,61,40,50],["Toxicroak","Poison","Fighting",83,106,65,86,65,85],
["Carnivine","Grass",null,74,100,72,90,72,46],
["Finneon","Water",null,49,49,56,49,61,66],["Lumineon","Water",null,69,69,76,69,86,91],
["Mantyke","Water","Flying",45,20,50,60,120,50],
["Snover","Grass","Ice",60,62,50,62,60,40],["Abomasnow","Grass","Ice",90,92,75,92,85,60],
["Weavile","Dark","Ice",70,120,65,45,85,125],
["Magnezone","Electric","Steel",70,70,115,130,90,60],
["Lickilicky","Normal",null,110,85,95,80,95,50],
["Rhyperior","Ground","Rock",115,140,130,55,55,40],
["Tangrowth","Grass",null,100,100,125,110,50,60],
["Electivire","Electric",null,75,123,67,95,85,95],
["Magmortar","Fire",null,75,95,67,125,95,83],
["Togekiss","Fairy","Flying",85,50,95,120,115,80],
["Yanmega","Bug","Flying",86,76,86,116,56,95],
["Leafeon","Grass",null,65,110,130,60,65,95],["Glaceon","Ice",null,65,60,110,130,95,65],
["Gliscor","Ground","Flying",75,95,125,45,75,95],
["Mamoswine","Ice","Ground",110,130,80,70,60,80],
["Porygon-Z","Normal",null,85,80,70,135,75,90],
["Gallade","Psychic","Fighting",68,125,65,65,115,80],
["Probopass","Rock","Steel",60,55,145,75,150,40],
["Dusknoir","Ghost",null,45,100,135,65,135,45],
["Froslass","Ice","Ghost",70,80,70,80,70,110],
["Rotom","Electric","Ghost",50,50,77,95,77,91],
["Uxie","Psychic",null,75,75,130,75,130,95],["Mesprit","Psychic",null,80,105,105,105,105,80],["Azelf","Psychic",null,75,125,70,125,70,115],
["Dialga","Steel","Dragon",100,120,120,150,100,90],["Palkia","Water","Dragon",90,120,100,150,120,100],
["Heatran","Fire","Steel",91,90,106,130,106,77],
["Regigigas","Normal",null,110,160,110,80,110,100],
["Giratina","Ghost","Dragon",150,100,120,100,120,90],
["Cresselia","Psychic",null,120,70,120,75,130,85],
["Phione","Water",null,80,80,80,80,80,80],["Manaphy","Water",null,100,100,100,100,100,100],
["Darkrai","Dark",null,70,90,90,135,90,125],
["Shaymin","Grass",null,100,100,100,100,100,100],
["Arceus","Normal",null,120,120,120,120,120,120]
];
// Gen 5 (Unova), #494-649. Stats from memory, not individually search-verified like the curated entries above.
const PROC_RAW_GEN5 = [
["Victini","Psychic","Fire",100,100,100,100,100,100],
["Snivy","Grass",null,45,45,55,45,55,63],["Servine","Grass",null,60,60,75,60,75,83],["Serperior","Grass",null,75,75,95,75,95,113],
["Tepig","Fire",null,65,63,45,45,45,45],["Pignite","Fire","Fighting",90,93,55,70,55,55],["Emboar","Fire","Fighting",110,123,65,100,65,65],
["Oshawott","Water",null,55,55,45,63,45,45],["Dewott","Water",null,75,75,60,83,60,60],["Samurott","Water",null,95,100,85,108,70,70],
["Patrat","Normal",null,45,55,39,35,39,42],["Watchog","Normal",null,60,85,69,60,69,77],
["Lillipup","Normal",null,45,60,45,25,45,55],["Herdier","Normal",null,65,80,65,35,65,60],["Stoutland","Normal",null,85,110,90,45,90,80],
["Purrloin","Dark",null,41,50,37,50,37,66],["Liepard","Dark",null,64,88,50,88,50,106],
["Pansage","Grass",null,50,53,48,53,48,64],["Simisage","Grass",null,75,98,63,98,63,101],
["Pansear","Fire",null,50,53,48,53,48,64],["Simisear","Fire",null,75,98,63,98,63,101],
["Panpour","Water",null,50,53,48,53,48,64],["Simipour","Water",null,75,98,63,98,63,101],
["Munna","Psychic",null,76,25,45,67,55,24],["Musharna","Psychic",null,116,55,85,107,95,29],
["Pidove","Normal","Flying",50,55,50,36,30,43],["Tranquill","Normal","Flying",62,77,62,50,42,65],["Unfezant","Normal","Flying",80,115,80,65,55,93],
["Blitzle","Electric",null,45,60,32,50,32,76],["Zebstrika","Electric",null,75,100,63,80,63,116],
["Roggenrola","Rock",null,55,75,85,25,25,15],["Boldore","Rock",null,70,105,105,50,40,20],["Gigalith","Rock",null,85,135,130,60,80,25],
["Woobat","Psychic","Flying",65,45,43,55,43,72],["Swoobat","Psychic","Flying",67,57,55,77,55,114],
["Drilbur","Ground",null,60,85,40,30,45,68],["Excadrill","Ground","Steel",110,135,60,50,65,88],
["Audino","Normal",null,103,60,86,60,86,50],
["Timburr","Fighting",null,75,80,55,25,35,35],["Gurdurr","Fighting",null,85,105,85,40,50,40],["Conkeldurr","Fighting",null,105,140,95,55,65,45],
["Tympole","Water",null,50,50,40,50,40,64],["Palpitoad","Water","Ground",75,65,55,65,55,69],["Seismitoad","Water","Ground",105,95,75,85,75,74],
["Throh","Fighting",null,120,100,85,30,85,45],["Sawk","Fighting",null,75,125,75,30,75,85],
["Sewaddle","Bug","Grass",45,53,70,40,60,42],["Swadloon","Bug","Grass",55,63,90,50,80,42],["Leavanny","Bug","Grass",75,103,80,70,80,92],
["Venipede","Bug","Poison",30,45,59,30,39,57],["Whirlipede","Bug","Poison",40,55,99,40,79,47],["Scolipede","Bug","Poison",60,100,89,55,69,112],
["Cottonee","Grass","Fairy",40,27,60,37,50,66],["Whimsicott","Grass","Fairy",60,67,85,77,75,116],
["Petilil","Grass",null,45,35,50,70,50,30],["Lilligant","Grass",null,70,60,75,110,75,90],
["Basculin","Water",null,70,92,65,80,55,98],
["Sandile","Ground","Dark",50,72,35,35,35,65],["Krokorok","Ground","Dark",60,82,45,45,45,74],["Krookodile","Ground","Dark",95,117,70,65,70,92],
["Darumaka","Fire",null,70,90,45,15,45,50],["Darmanitan","Fire",null,105,140,55,30,55,95],
["Maractus","Grass",null,75,86,67,106,67,60],
["Dwebble","Bug","Rock",50,65,85,35,35,55],["Crustle","Bug","Rock",70,105,125,65,75,45],
["Scraggy","Dark","Fighting",50,75,70,35,70,48],["Scrafty","Dark","Fighting",65,90,115,45,115,58],
["Sigilyph","Psychic","Flying",72,58,80,103,80,97],
["Yamask","Ghost",null,38,30,85,55,65,30],["Cofagrigus","Ghost",null,58,50,145,95,105,30],
["Tirtouga","Water","Rock",54,78,103,53,45,22],["Carracosta","Water","Rock",74,108,133,83,65,32],
["Archen","Rock","Flying",55,112,45,74,45,70],["Archeops","Rock","Flying",75,140,65,112,65,110],
["Trubbish","Poison",null,50,50,62,40,62,65],["Garbodor","Poison",null,80,95,82,60,82,75],
["Zorua","Dark",null,40,65,40,80,40,65],["Zoroark","Dark",null,60,105,60,120,60,105],
["Minccino","Normal",null,55,50,40,40,40,75],["Cinccino","Normal",null,75,95,60,65,60,115],
["Gothita","Psychic",null,45,30,50,55,65,45],["Gothorita","Psychic",null,60,45,70,75,85,55],["Gothitelle","Psychic",null,70,55,95,95,110,65],
["Solosis","Psychic",null,45,30,40,105,50,20],["Duosion","Psychic",null,65,40,50,125,60,30],["Reuniclus","Psychic",null,110,65,75,125,85,30],
["Ducklett","Water","Flying",62,44,50,44,50,55],["Swanna","Water","Flying",75,87,63,87,63,98],
["Vanillite","Ice",null,36,50,50,65,60,44],["Vanillish","Ice",null,51,65,65,80,75,59],["Vanilluxe","Ice",null,71,95,85,110,95,79],
["Deerling","Normal","Grass",60,60,50,40,50,75],["Sawsbuck","Normal","Grass",80,100,70,60,70,95],
["Emolga","Electric","Flying",55,75,60,75,60,103],
["Karrablast","Bug",null,50,75,45,40,45,60],["Escavalier","Bug","Steel",70,135,105,60,105,20],
["Foongus","Grass","Poison",69,55,45,55,55,15],["Amoonguss","Grass","Poison",114,85,70,85,80,30],
["Frillish","Water","Ghost",55,40,50,65,85,40],["Jellicent","Water","Ghost",100,60,70,85,105,60],
["Alomomola","Water",null,165,75,80,40,45,65],
["Joltik","Bug","Electric",50,47,50,57,50,65],["Galvantula","Bug","Electric",70,77,60,97,60,108],
["Ferroseed","Grass","Steel",44,50,91,24,86,10],["Ferrothorn","Grass","Steel",74,94,131,54,116,20],
["Klink","Steel",null,40,55,70,45,60,30],["Klang","Steel",null,60,80,95,70,85,50],["Klinklang","Steel",null,60,100,115,70,85,90],
["Tynamo","Electric",null,35,55,40,45,40,60],["Eelektrik","Electric",null,65,85,70,75,70,40],["Eelektross","Electric",null,85,115,80,105,80,50],
["Elgyem","Psychic",null,55,55,55,85,55,30],["Beheeyem","Psychic",null,75,75,75,125,95,40],
["Litwick","Ghost","Fire",50,30,55,65,55,20],["Lampent","Ghost","Fire",60,40,60,95,60,55],["Chandelure","Ghost","Fire",60,55,90,145,90,80],
["Axew","Dragon",null,46,87,60,30,40,57],["Fraxure","Dragon",null,66,117,70,40,50,67],["Haxorus","Dragon",null,76,147,90,60,70,97],
["Cubchoo","Ice",null,55,70,40,60,40,40],["Beartic","Ice",null,95,110,80,70,80,50],
["Cryogonal","Ice",null,80,50,50,95,135,105],
["Shelmet","Bug",null,50,40,85,40,65,25],["Accelgor","Bug",null,80,70,40,100,60,145],
["Stunfisk","Ground","Electric",109,66,84,81,99,32],
["Mienfoo","Fighting",null,45,85,50,55,50,65],["Mienshao","Fighting",null,65,125,60,95,60,105],
["Druddigon","Dragon",null,77,120,90,60,90,48],
["Golett","Ground","Ghost",59,74,50,35,50,35],["Golurk","Ground","Ghost",89,124,80,55,80,55],
["Pawniard","Dark","Steel",45,85,70,40,40,60],["Bisharp","Dark","Steel",65,125,100,60,70,70],
["Bouffalant","Normal",null,95,110,95,40,95,55],
["Rufflet","Normal","Flying",70,83,50,37,50,60],["Braviary","Normal","Flying",100,123,75,57,75,80],
["Vullaby","Dark","Flying",70,55,75,45,65,60],["Mandibuzz","Dark","Flying",110,65,105,55,95,80],
["Heatmor","Fire",null,85,97,66,105,66,65],
["Durant","Bug","Steel",58,109,112,48,48,109],
["Deino","Dark","Dragon",52,65,50,45,50,38],["Zweilous","Dark","Dragon",72,85,70,65,70,58],["Hydreigon","Dark","Dragon",92,105,90,125,90,98],
["Larvesta","Bug","Fire",55,85,55,50,55,60],["Volcarona","Bug","Fire",85,60,65,135,105,100],
["Cobalion","Steel","Fighting",91,90,129,90,72,108],["Terrakion","Rock","Fighting",91,129,90,72,90,108],["Virizion","Grass","Fighting",91,90,72,90,129,108],
["Tornadus","Flying",null,79,115,70,125,80,111],["Thundurus","Electric","Flying",79,115,70,125,80,111],
["Reshiram","Dragon","Fire",100,120,100,150,120,90],["Zekrom","Dragon","Electric",100,150,120,120,100,90],
["Landorus","Ground","Flying",89,125,90,115,80,101],
["Kyurem","Dragon","Ice",125,130,90,130,90,95],
["Keldeo","Water","Fighting",91,72,90,129,90,108],
["Meloetta","Normal","Psychic",100,77,77,128,128,90],
["Genesect","Bug","Steel",71,120,95,120,95,99]
];
// Gen 6 (Kalos), #650-721. Stats from memory, not individually search-verified like the curated entries above.
const PROC_RAW_GEN6 = [
["Chespin","Grass",null,56,61,65,48,45,38],["Quilladin","Grass",null,61,78,95,56,58,57],["Chesnaught","Grass","Fighting",88,107,122,74,75,64],
["Fennekin","Fire",null,40,45,40,62,60,60],["Braixen","Fire",null,59,59,58,90,70,73],["Delphox","Fire","Psychic",75,69,72,114,100,104],
["Froakie","Water",null,41,56,40,62,44,71],["Frogadier","Water",null,54,63,52,83,56,97],["Greninja","Water","Dark",72,95,67,103,71,122],
["Bunnelby","Normal",null,38,36,38,32,36,57],["Diggersby","Normal","Ground",85,56,77,50,77,78],
["Fletchling","Normal","Flying",45,50,43,40,38,62],["Fletchinder","Fire","Flying",62,73,55,56,52,84],["Talonflame","Fire","Flying",78,81,71,74,69,126],
["Scatterbug","Bug",null,38,35,40,27,25,35],["Spewpa","Bug",null,45,22,60,27,30,29],["Vivillon","Bug","Flying",80,52,50,90,50,89],
["Litleo","Fire","Normal",62,50,58,73,54,72],["Pyroar","Fire","Normal",86,68,72,109,66,106],
["Flabebe","Fairy",null,44,38,39,61,79,42],["Floette","Fairy",null,54,45,47,75,98,52],["Florges","Fairy",null,78,65,68,112,154,75],
["Skiddo","Grass",null,66,65,48,62,57,52],["Gogoat","Grass",null,123,100,62,97,81,68],
["Pancham","Fighting",null,67,82,62,46,48,43],["Pangoro","Fighting","Dark",95,124,78,69,71,58],
["Furfrou","Normal",null,75,80,60,65,90,102],
["Espurr","Psychic",null,62,48,54,63,60,68],["Meowstic","Psychic",null,74,48,76,83,81,104],
["Honedge","Steel","Ghost",45,80,100,35,37,28],["Doublade","Steel","Ghost",59,110,150,45,49,35],["Aegislash","Steel","Ghost",60,50,140,50,140,60],
["Spritzee","Fairy",null,78,52,60,63,65,23],["Aromatisse","Fairy",null,101,72,72,99,89,29],
["Swirlix","Fairy",null,62,48,66,59,57,49],["Slurpuff","Fairy",null,82,80,86,85,75,72],
["Inkay","Dark","Psychic",53,54,53,37,46,45],["Malamar","Dark","Psychic",86,92,88,68,75,73],
["Binacle","Rock","Water",42,52,67,39,56,50],["Barbaracle","Rock","Water",72,105,115,54,86,68],
["Skrelp","Poison","Water",50,60,60,60,60,30],["Dragalge","Poison","Dragon",65,75,90,97,123,44],
["Clauncher","Water",null,50,53,62,58,63,44],["Clawitzer","Water",null,71,73,88,120,89,59],
["Helioptile","Electric","Normal",44,38,33,61,43,70],["Heliolisk","Electric","Normal",62,55,52,109,94,109],
["Tyrunt","Rock","Dragon",58,89,77,45,45,48],["Tyrantrum","Rock","Dragon",82,121,119,69,59,71],
["Amaura","Rock","Ice",77,59,50,67,63,46],["Aurorus","Rock","Ice",123,77,72,99,92,58],
["Sylveon","Fairy",null,95,65,65,110,130,60],
["Hawlucha","Fighting","Flying",78,92,75,74,63,118],
["Dedenne","Electric","Fairy",67,58,57,81,67,101],
["Carbink","Rock","Fairy",50,50,150,50,150,50],
["Goomy","Dragon",null,45,50,35,55,75,40],["Sliggoo","Dragon",null,68,75,53,83,113,60],["Goodra","Dragon",null,90,100,70,110,150,80],
["Klefki","Steel","Fairy",57,80,91,80,87,75],
["Phantump","Ghost","Grass",43,70,48,50,60,38],["Trevenant","Ghost","Grass",85,110,76,65,82,56],
["Pumpkaboo","Ghost","Grass",49,66,70,44,55,51],["Gourgeist","Ghost","Grass",65,90,122,58,75,84],
["Bergmite","Ice",null,55,69,85,32,35,28],["Avalugg","Ice",null,95,117,184,44,46,28],
["Noibat","Flying","Dragon",40,30,35,45,40,55],["Noivern","Flying","Dragon",85,70,80,97,80,123],
["Xerneas","Fairy",null,126,131,95,131,98,99],["Yveltal","Dark","Flying",126,131,95,131,98,99],["Zygarde","Dragon","Ground",108,100,121,81,95,95],
["Diancie","Rock","Fairy",50,100,150,100,150,50],
["Hoopa","Psychic","Ghost",80,110,60,150,130,70],
["Volcanion","Fire","Water",80,110,120,130,90,70]
];
// Gen 7 remainder (Alola), #722-809, excluding the 16 already hand-curated above. Stats from memory, not individually search-verified.
const PROC_RAW_GEN7 = [
["Rowlet","Grass","Flying",68,55,55,50,50,42],["Dartrix","Grass","Flying",78,75,75,70,70,52],
["Litten","Fire",null,45,65,40,60,40,70],["Torracat","Fire",null,65,85,50,80,50,90],
["Popplio","Water",null,50,54,54,66,56,40],["Brionne","Water",null,60,69,69,91,81,50],
["Pikipek","Normal","Flying",35,75,30,30,30,65],["Trumbeak","Normal","Flying",55,85,50,40,50,75],
["Yungoos","Normal",null,48,70,30,30,30,45],["Gumshoos","Normal",null,88,110,60,55,60,45],
["Grubbin","Bug",null,47,62,45,55,45,46],["Charjabug","Bug","Electric",57,82,95,55,75,36],
["Crabrawler","Fighting",null,47,82,57,42,47,63],
["Oricorio","Fire","Flying",75,70,70,98,70,93],
["Rockruff","Rock",null,45,65,40,30,40,60],
["Wishiwashi","Water",null,45,20,20,25,25,40],
["Mareanie","Poison","Water",50,53,62,43,52,45],
["Dewpider","Water","Bug",38,40,52,40,72,27],["Araquanid","Water","Bug",68,70,92,50,132,42],
["Fomantis","Grass",null,40,55,35,50,35,35],["Shiinotic","Grass","Fairy",60,45,80,90,100,30],
["Stufful","Normal","Fighting",70,75,50,45,50,50],["Bewear","Normal","Fighting",120,125,80,55,60,60],
["Bounsweet","Grass",null,42,30,38,30,38,32],["Steenee","Grass",null,52,40,48,40,48,62],["Tsareena","Grass",null,72,120,98,50,98,72],
["Comfey","Fairy",null,51,52,90,82,110,100],
["Oranguru","Normal","Psychic",90,60,80,90,110,60],
["Passimian","Fighting",null,100,120,90,40,60,80],
["Wimpod","Bug","Water",25,35,40,20,30,80],
["Sandygast","Ghost","Ground",55,55,80,70,45,15],["Palossand","Ghost","Ground",85,75,110,100,75,35],
["Pyukumuku","Water",null,55,60,130,30,130,5],
["Type-Null","Normal",null,95,95,95,95,95,59],["Silvally","Normal",null,95,95,95,95,95,95],
["Minior","Rock","Flying",60,60,100,60,100,60],
["Komala","Normal",null,65,115,65,75,95,65],
["Turtonator","Fire","Dragon",60,78,135,91,85,36],
["Togedemaru","Electric","Steel",65,98,63,40,73,96],
["Bruxish","Water","Psychic",68,105,70,70,70,92],
["Drampa","Normal","Dragon",78,60,85,135,91,36],
["Dhelmise","Ghost","Grass",70,131,100,86,90,40],
["Jangmo-o","Dragon",null,45,55,65,45,45,45],["Hakamo-o","Dragon","Fighting",55,75,90,65,70,65],
["Tapu-Koko","Electric","Fairy",70,115,85,95,75,130],["Tapu-Lele","Psychic","Fairy",70,85,75,130,115,95],
["Tapu-Bulu","Grass","Fairy",70,130,115,85,95,75],["Tapu-Fini","Water","Fairy",70,75,115,95,130,85],
["Cosmog","Psychic",null,43,29,31,29,31,37],["Cosmoem","Psychic",null,43,29,131,29,131,37],
["Solgaleo","Psychic","Steel",137,137,107,113,89,97],["Lunala","Psychic","Ghost",137,113,89,137,107,97],
["Nihilego","Rock","Poison",109,53,47,127,131,103],["Buzzwole","Bug","Fighting",107,139,139,53,53,79],
["Pheromosa","Bug","Fighting",71,137,37,137,37,151],["Xurkitree","Electric",null,83,89,71,173,71,83],
["Celesteela","Steel","Flying",97,101,103,107,101,61],["Kartana","Grass","Steel",59,181,131,59,31,109],
["Guzzlord","Dark","Dragon",223,101,53,97,53,43],
["Necrozma","Psychic",null,97,107,101,127,89,79],
["Magearna","Steel","Fairy",80,95,115,130,115,65],
["Marshadow","Fighting","Ghost",90,125,80,90,90,125],
["Poipole","Poison",null,67,73,67,73,67,73],["Naganadel","Poison","Dragon",73,73,73,127,73,121],
["Stakataka","Rock","Steel",61,131,211,53,101,13],["Blacephalon","Fire","Ghost",53,127,53,151,79,107],
["Zeraora","Electric",null,88,112,75,102,80,143],
["Meltan","Steel",null,46,65,65,55,35,34],["Melmetal","Steel",null,135,143,143,80,65,34]
];
const DEX = CURATED_DEX.concat(PROC_RAW.map(procEntry)).concat(PROC_RAW_GEN2.map(procEntry)).concat(PROC_RAW_GEN3.map(procEntry)).concat(PROC_RAW_GEN4.map(procEntry)).concat(PROC_RAW_GEN5.map(procEntry)).concat(PROC_RAW_GEN6.map(procEntry)).concat(PROC_RAW_GEN7.map(procEntry));

const LEVEL = 50;
function statCalc(base, isHp, level){
  level = level||LEVEL;
  if(isHp) return Math.floor(((2*base+31)*level)/100)+level+10;
  return Math.floor(((2*base+31)*level)/100)+5;
}
function makeMon(d,id,item,level){
  level = level||LEVEL;
  return {id, dex:d, name:d.name, nick:null, types:d.types, moves:d.moves, ability:d.ability, item:item||'none', level,
    xp:0, xpNext: level*8,
    maxhp: statCalc(d.base.hp,true,level), hp: statCalc(d.base.hp,true,level),
    atk: statCalc(d.base.atk,false,level), def: statCalc(d.base.def,false,level),
    spa: statCalc(d.base.spa,false,level), spd: statCalc(d.base.spd,false,level), spe: statCalc(d.base.spe,false,level),
    fainted:false, status:null, sleepTurns:0, usedDisguise:false, hitTaken:false, sashUsed:false, berryUsed:false, caught:false};
}
const EVOLUTIONS = {
 Bulbasaur:{to:"Ivysaur",level:16}, Ivysaur:{to:"Venusaur",level:32},
 Charmander:{to:"Charmeleon",level:16}, Charmeleon:{to:"Charizard",level:36},
 Squirtle:{to:"Wartortle",level:16}, Wartortle:{to:"Blastoise",level:36},
 Caterpie:{to:"Metapod",level:7}, Metapod:{to:"Butterfree",level:10},
 Weedle:{to:"Kakuna",level:7}, Kakuna:{to:"Beedrill",level:10},
 Pidgey:{to:"Pidgeotto",level:18}, Pidgeotto:{to:"Pidgeot",level:36},
 Rattata:{to:"Raticate",level:20}, Spearow:{to:"Fearow",level:20},
 Ekans:{to:"Arbok",level:22}, Pikachu:{to:"Raichu",level:24},
 Vulpix:{to:"Ninetales",level:28}, Growlithe:{to:"Arcanine",level:30},
 Poliwag:{to:"Poliwhirl",level:25}, Poliwhirl:{to:"Poliwrath",level:38},
 Abra:{to:"Kadabra",level:16}, Kadabra:{to:"Alakazam",level:34},
 Machop:{to:"Machoke",level:28}, Machoke:{to:"Machamp",level:40},
 Geodude:{to:"Graveler",level:25}, Graveler:{to:"Golem",level:38},
 Slowpoke:{to:"Slowbro",level:37}, Magnemite:{to:"Magneton",level:30},
 Doduo:{to:"Dodrio",level:31}, Seel:{to:"Dewgong",level:34},
 Grimer:{to:"Muk",level:38}, Shellder:{to:"Cloyster",level:30},
 Gastly:{to:"Haunter",level:25}, Haunter:{to:"Gengar",level:36},
 Drowzee:{to:"Hypno",level:26}, Krabby:{to:"Kingler",level:28},
 Voltorb:{to:"Electrode",level:30}, Cubone:{to:"Marowak",level:28},
 Koffing:{to:"Weezing",level:35}, Rhyhorn:{to:"Rhydon",level:42},
 Horsea:{to:"Seadra",level:32}, Goldeen:{to:"Seaking",level:33},
 Staryu:{to:"Starmie",level:34}, Magikarp:{to:"Gyarados",level:20},
 Dratini:{to:"Dragonair",level:30}, Dragonair:{to:"Dragonite",level:55},
 Rowlet:{to:"Dartrix",level:17}, Dartrix:{to:"Decidueye",level:34},
 Litten:{to:"Torracat",level:17}, Torracat:{to:"Incineroar",level:34},
 Popplio:{to:"Brionne",level:17}, Brionne:{to:"Primarina",level:34},
 Pikipek:{to:"Trumbeak",level:14}, Grubbin:{to:"Charjabug",level:20},
 Charjabug:{to:"Vikavolt",level:35}, Crabrawler:{to:"Crabominable",level:34},
 Rockruff:{to:"Lycanroc",level:25}, Mareanie:{to:"Toxapex",level:38},
 Fomantis:{to:"Lurantis",level:34}, Wimpod:{to:"Golisopod",level:30},
 "Jangmo-o":{to:"Hakamo-o",level:35}, "Hakamo-o":{to:"Kommo-o",level:45}
};
function tryEvolve(mon){
  const evo = EVOLUTIONS[mon.name];
  if(!evo || mon.level < evo.level) return;
  const newDex = DEX.find(d=>d.name===evo.to);
  if(!newDex) return;
  const oldDisplay = dname(mon);
  const hpFrac = mon.hp/mon.maxhp;
  mon.name = newDex.name; mon.types = newDex.types; mon.moves = newDex.moves;
  mon.ability = newDex.ability; mon.dex = newDex;
  mon.maxhp = statCalc(newDex.base.hp,true,mon.level);
  mon.hp = Math.max(1,Math.round(hpFrac*mon.maxhp));
  mon.atk = statCalc(newDex.base.atk,false,mon.level); mon.def = statCalc(newDex.base.def,false,mon.level);
  mon.spa = statCalc(newDex.base.spa,false,mon.level); mon.spd = statCalc(newDex.base.spd,false,mon.level);
  mon.spe = statCalc(newDex.base.spe,false,mon.level);
  addLog(`✨ ${oldDisplay} evolved into ${newDex.name}!`);
}
function grantXp(mon, amount){
  if(mon.fainted) return;
  mon.xp = (mon.xp||0) + amount;
  let leveled = false;
  while(mon.level<100 && mon.xp >= mon.xpNext){
    mon.xp -= mon.xpNext;
    mon.level++;
    mon.xpNext = mon.level*8;
    const hpFrac = mon.hp/mon.maxhp;
    mon.maxhp = statCalc(mon.dex.base.hp,true,mon.level);
    mon.hp = Math.max(1,Math.round(hpFrac*mon.maxhp));
    mon.atk = statCalc(mon.dex.base.atk,false,mon.level);
    mon.def = statCalc(mon.dex.base.def,false,mon.level);
    mon.spa = statCalc(mon.dex.base.spa,false,mon.level);
    mon.spd = statCalc(mon.dex.base.spd,false,mon.level);
    mon.spe = statCalc(mon.dex.base.spe,false,mon.level);
    leveled = true;
    tryEvolve(mon);
  }
  if(leveled) addLog(`${dname(mon)} grew to level ${mon.level}!`);
}
function advLevel(){ return Math.min(50, 8 + (adv?adv.loc:0)*4); }

const TYPE_COLORS = {
 Normal:"#A8A878",Fire:"#F08030",Water:"#6890F0",Electric:"#F8D030",Grass:"#78C850",Ice:"#98D8D8",
 Fighting:"#C03028",Poison:"#A040A0",Ground:"#E0C068",Flying:"#A890F0",Psychic:"#F85888",Bug:"#A8B820",
 Rock:"#B8A038",Ghost:"#705898",Dragon:"#7038F8",Dark:"#705848",Steel:"#B8B8D0",Fairy:"#EE99AC"
};
function dname(m){ return m.nick || m.name; }
// Custom-sprite hook: loads sprites/<facing>/<slug>.png next to this file. Never populated
// with ripped/fan-fusion game sprites (see project notes) — this only reads files the
// player supplies themselves. Missing file -> onerror swaps in the ❔ placeholder, so this
// works unmodified whether or not a sprites/ folder exists alongside the HTML.
function slug(name){
  return name.toLowerCase().normalize('NFD').replace(/[̀-ͯ]/g,'')
    .replace(/[^a-z0-9]+/g,'-').replace(/^-+|-+$/g,'');
}
function monSprite(d, facing){
  facing = facing || 'front';
  const path = `sprites/${facing}/${slug(d.name)}.png`;
  return `<div class="mon-sprite"><img src="${path}" alt="${d.name}" onerror="this.style.display='none'; this.nextElementSibling.style.display='flex';"><span class="ph">❔</span></div>`;
}
function showToast(msg){
  if(typeof document.createElement!=='function') return;
  const el = document.createElement('div');
  el.className='toast'; el.textContent=msg;
  document.body.appendChild(el);
  setTimeout(()=>el.remove(), 2200);
}
function togglePartyModal(){ document.getElementById('partyModal').classList.toggle('hidden'); }
function closePartyModal(){ document.getElementById('partyModal').classList.add('hidden'); }

// Custom confirm/prompt modal — native confirm()/prompt() are blocked or throw inside
// the sandboxed iframe artifacts run in, which silently breaks any button relying on them.
let __confirmCb = null;
function showConfirm(msg, cb, withInput, defVal){
  document.getElementById('confirmModalMsg').textContent = msg;
  document.getElementById('confirmModalInputWrap').classList.toggle('hidden', !withInput);
  if(withInput) document.getElementById('confirmModalInput').value = defVal||'';
  __confirmCb = cb;
  document.getElementById('confirmModal').classList.remove('hidden');
}
function confirmModalYes(){
  const withInput = !document.getElementById('confirmModalInputWrap').classList.contains('hidden');
  const val = withInput ? document.getElementById('confirmModalInput').value : true;
  document.getElementById('confirmModal').classList.add('hidden');
  const cb = __confirmCb; __confirmCb = null;
  if(cb) cb(val);
}
function confirmModalNo(){
  document.getElementById('confirmModal').classList.add('hidden');
  __confirmCb = null;
}
function typeBadge(t){ return `<span class="tbadge" style="background:${TYPE_COLORS[t]||'#888'}">${t}</span>`; }
function typeBadges(types){ return types.map(typeBadge).join(' '); }

let state=null, draftPool=[], draftPicked=[], draftItems={}, needA=0, needB=0, draftMode='free';
let adv=null;
const SAVE_KEY='partyroyale_adventure_v1';
function idx(name){ return DEX.findIndex(d=>d.name===name); }
const LOCATIONS=[
 {type:'town', name:"Duskmere Hollow", desc:"Your home town, at the edge of the Vellorin region. A Pokémon Center sits by the old well.", center:true},
 {type:'route', name:"Route 1: Fernway Trail", desc:"Tall grass lines a quiet dirt path. Something's rustling.", pool:[5,6,9,3,15]},
 {type:'trainer', kind:'rival', name:"Fernway Overlook", desc:"Your rival Wren is waiting on the ridge, arms crossed. \"Let's see how far you've come.\"", leaderName:"Wren", leaderTeam:[2,7]},
 {type:'gym', name:"Cindergate Town", desc:"Smoke curls from the gym's chimney. Leader Rell awaits with a scorched-earth team.", center:true, leaderName:"Rell", leaderTeam:[1,5,8]},
 {type:'route', name:"Route 2: Marrow Pass", desc:"A narrow pass between cliffs. The wind carries distant cries.", pool:[7,10,11,0,4,12,13]},
 {type:'gym', name:"Tidalkeep City", desc:"Waves crash against the gym's sea wall. Leader Sable commands the tide.", center:true, leaderName:"Sable", leaderTeam:[4,2,11]},
 {type:'route', name:"Route 3: Hollow Bluffs", desc:"Weathered bluffs overlook the coast. The path forks ahead.", pool:[0,6,10,3,9,14]},
 {type:'gym', name:"Stonebrook Town", desc:"A rugged gym built into the bluffs. Leader Orin trains ground-pounders and toxic tacticians.", center:true, leaderName:"Orin", leaderTeam:[12,13,15]},
 {type:'route', name:"Route 4: The Roost", desc:"Cliffside nests dot the rockface above. Wings flash in the haze.", pool:[14,6,10,9]},
 {type:'trainer', kind:'rival', name:"Windward Ledge", desc:"Wren again, team clearly stronger this time. \"You've grown. Let's finish this properly.\"", leaderName:"Wren", leaderTeam:[9,7,2,11]},
 {type:'route', name:"Route 5: Cragmoor Trail", desc:"Loose scree and echoing caves. Wild Pokémon lurk in the dark.", pool:[idx('Machop'),idx('Geodude'),idx('Gastly'),idx('Onix'),idx('Cubone')]},
 {type:'gym', name:"Wispgate City", desc:"Lantern-lit streets wind up to a gym wreathed in fog. Leader Sable's successor, Iska, trains ghosts and psychics.", center:true, leaderName:"Iska", leaderTeam:[3,10,idx('Gengar')]},
 {type:'route', name:"Route 6: Emberflow Delta", desc:"Steam vents hiss where river meets old lava rock.", pool:[idx('Charmander'),idx('Growlithe'),13,idx('Krabby'),idx('Magikarp')]},
 {type:'town', name:"???", desc:"The trail ahead hasn't been charted yet — more of Vellorin is on the way in a future update.", center:true, endOfContent:true}
];

// --- Overworld tile map: original placeholder sprites (emoji), not any ripped/fan-fusion Pokémon sprite pack. ---
const MAP_COLS=10, MAP_ROWS=6;
function seedRand(str){
  let h=0; for(let i=0;i<str.length;i++) h=(h*31+str.charCodeAt(i))>>>0;
  return function(){ h=(h*1103515245+12345)>>>0; return (h>>>8)/0x1000000; };
}
function genGrid(loc){
  if(loc.__grid) return loc.__grid;
  const rnd = seedRand(loc.name);
  const grid=[];
  for(let y=0;y<MAP_ROWS;y++){
    const row=[];
    for(let x=0;x<MAP_COLS;x++){
      let t='.';
      if(y===0||y===MAP_ROWS-1||x===0) t='#';
      else if(x===MAP_COLS-1) t='E';
      else if(loc.type==='route' && x>1 && rnd()<0.4) t='G';
      row.push(t);
    }
    grid.push(row);
  }
  const midY=Math.floor(MAP_ROWS/2);
  if(loc.center) grid[midY][Math.floor(MAP_COLS/2)-2]='C';
  if(loc.type==='gym'||loc.type==='trainer') grid[midY][Math.floor(MAP_COLS/2)]='N';
  loc.__grid = grid;
  return grid;
}
function spawnPlayer(){ adv.pos = {x:1, y:Math.floor(MAP_ROWS/2)}; }
function spriteHtml(dir, walking){
  return `<div class="spriteflip ${dir==='left'?'flip':''}"><div class="sprite dir-${dir} ${walking?'walking':''}">
    <div class="s-hair"></div><div class="s-face"><span class="s-eye l"></span><span class="s-eye r"></span></div>
    <div class="s-bag"></div><div class="s-body"></div>
  </div></div>`;
}
const TILE_PX = 36;
// Full rebuild: terrain tiles (static per location) plus one persistent, absolutely-positioned
// player sprite overlaid on top. Used on location entry / after battles, NOT on every step —
// movePlayer() below only slides the existing sprite, so steps animate instead of snapping.
function renderTileMap(){
  const loc = LOCATIONS[adv.loc];
  const grid = genGrid(loc);
  const poiIcon = {'C':'🏥','N':(loc.type==='gym'?'🥊':'🧑')};
  let html='';
  for(let y=0;y<MAP_ROWS;y++){
    for(let x=0;x<MAP_COLS;x++){
      let t = grid[y][x];
      if(t==='N' && adv.cleared[loc.name]) t='.';
      const checker = (x+y)%2===0 ? 'a':'b';
      let terrainCls, inner;
      if(t==='#'){ terrainCls='tree'; inner='<div class="treetop"></div><div class="treetrunk"></div>'; }
      else if(t==='G'){ terrainCls=`grass-${checker}`; inner=''; }
      else {
        terrainCls=`path-${checker}`;
        const icon = (t==='E' && y===Math.floor(MAP_ROWS/2)) ? '➡️' : (poiIcon[t]||'');
        inner = icon ? `<div class="poi">${icon}</div>` : '';
      }
      html += `<div class="tile ${terrainCls}" data-x="${x}" data-y="${y}">${inner}</div>`;
    }
  }
  html += `<div id="playerSprite" style="transform:translate(${adv.pos.x*TILE_PX}px,${adv.pos.y*TILE_PX}px)">${spriteHtml(adv.facing||'down', false)}</div>`;
  document.getElementById('tileMap').innerHTML = html;
}
// Lightweight update used for every single step: moves/redraws only the player sprite,
// no full-grid rebuild, so the CSS transition on #playerSprite's transform can animate the slide.
function updatePlayerSprite(walked){
  const el = document.getElementById('playerSprite');
  if(!el) return;
  el.style.transform = `translate(${adv.pos.x*TILE_PX}px,${adv.pos.y*TILE_PX}px)`;
  el.innerHTML = spriteHtml(adv.facing||'down', !!walked);
}
// Brief shake on a grass tile the player steps into — classic "something's in there" cue.
// Guarded: the Node test harness's mock `document` has no querySelector.
function rustleTile(x,y){
  if(typeof document.querySelector!=='function') return;
  const el = document.querySelector(`.tile[data-x="${x}"][data-y="${y}"]`);
  if(!el) return;
  el.classList.remove('rustling');
  void el.offsetWidth; // restart the animation even if it's still mid-play
  el.classList.add('rustling');
}
// Quick full-screen flash on entering a battle — classic Pokémon screen-transition cue.
// Fire-and-forget: the CSS animation runs on its own, nothing here blocks the actual
// battle setup that follows (keeps this safe under the test harness's no-op setTimeout).
function playBattleFlash(){
  const el = document.getElementById('screenFlash');
  if(!el) return;
  el.classList.remove('flash');
  if(el.offsetWidth !== undefined) void el.offsetWidth;
  el.classList.add('flash');
}
function movePlayer(dx,dy){
  if(!adv) return;
  if(document.getElementById('adv').classList.contains('hidden')) return;
  if(!document.getElementById('battle').classList.contains('hidden')) return;
  adv.facing = dy<0?'up':dy>0?'down':dx<0?'left':dx>0?'right':(adv.facing||'down');
  const loc = LOCATIONS[adv.loc];
  const grid = genGrid(loc);
  const nx = adv.pos.x+dx, ny = adv.pos.y+dy;
  if(ny<0||ny>=MAP_ROWS||nx<0||nx>=MAP_COLS){ updatePlayerSprite(false); return; }
  const t = grid[ny][nx];
  if(t==='#'){ updatePlayerSprite(false); return; }
  if(t==='N' && !adv.cleared[loc.name]){ updatePlayerSprite(false); startTrainerBattle(); return; }
  if(t==='E'){ advanceLoc(); return; }
  adv.pos = {x:nx,y:ny};
  if(t==='C'){
    healParty();
    if(adv.restockedLoc !== adv.loc){ adv.items.pokeball=(adv.items.pokeball||0)+5; adv.restockedLoc=adv.loc; showToast('Party healed! +5 Poké Balls'); }
  }
  updatePlayerSprite(true);
  if(t==='G'){
    rustleTile(nx,ny);
    if(Math.random()<0.15){ saveAdv(); startWildBattle(); return; }
  }
  saveAdv();
}
if(typeof document.addEventListener==='function') document.addEventListener('keydown', e=>{
  const box = document.getElementById('msgBox');
  if(box && !box.classList.contains('hidden')){ advanceMsgBox(); return; }
  const k = e.key.toLowerCase();
  if(k==='arrowup'||k==='w') movePlayer(0,-1);
  else if(k==='arrowdown'||k==='s') movePlayer(0,1);
  else if(k==='arrowleft'||k==='a') movePlayer(-1,0);
  else if(k==='arrowright'||k==='d') movePlayer(1,0);
});

function saveAdv(){ try{ localStorage.setItem(SAVE_KEY, JSON.stringify(adv)); }catch(e){} }
function patchAdv(a){
  if(!a) return a;
  if(!a.cleared) a.cleared={};
  if(!a.items) a.items = {pokeball:10};
  if(!a.box) a.box=[];
  if(!a.playerName) a.playerName='Trainer';
  for(const m of a.party){
    if(!m.dex) m.dex = DEX.find(d=>d.name===m.name) || DEX[0];
    if(!m.level){ m.level = 50; m.xp = 0; m.xpNext = m.level*8; }
    if(m.nick===undefined) m.nick=null;
  }
  for(const m of a.box){ if(!m.dex) m.dex = DEX.find(d=>d.name===m.name) || DEX[0]; if(m.nick===undefined) m.nick=null; }
  return a;
}
function loadAdv(){ try{ const s=localStorage.getItem(SAVE_KEY); const a=s?JSON.parse(s):null; return patchAdv(a); }catch(e){ return null; } }
function exportSaveCode(){
  if(!adv){ showToast('No adventure in progress to export.'); return; }
  saveAdv();
  const code = btoa(unescape(encodeURIComponent(JSON.stringify(adv))));
  document.getElementById('saveCodeBox').value = code;
  document.getElementById('saveCodeModal').classList.remove('hidden');
  document.getElementById('saveCodeMode').textContent = 'Export';
  document.getElementById('saveCodeHint').textContent = 'Copy this code and save it somewhere. Paste it back in on any computer to restore this exact adventure.';
  document.getElementById('saveCodeAction').classList.add('hidden');
}
function openImportSave(){
  document.getElementById('saveCodeBox').value = '';
  document.getElementById('saveCodeModal').classList.remove('hidden');
  document.getElementById('saveCodeMode').textContent = 'Import';
  document.getElementById('saveCodeHint').textContent = 'Paste a save code below, then click Load.';
  document.getElementById('saveCodeAction').classList.remove('hidden');
}
function closeSaveCode(){ document.getElementById('saveCodeModal').classList.add('hidden'); }
function copySaveCode(){
  const box = document.getElementById('saveCodeBox');
  box.select();
  try{ navigator.clipboard.writeText(box.value); showToast('Copied to clipboard!'); }
  catch(e){ showToast('Could not auto-copy — the code is selected, press Ctrl/Cmd+C.'); }
}
function loadSaveCode(){
  const code = document.getElementById('saveCodeBox').value.trim();
  if(!code){ showToast('Paste a save code first.'); return; }
  try{
    const parsed = JSON.parse(decodeURIComponent(escape(atob(code))));
    if(!parsed || !parsed.party) throw new Error('bad shape');
    adv = patchAdv(parsed);
    saveAdv();
    closeSaveCode();
    showAdvScreens();
    renderAdventure();
  }catch(e){ showToast('That save code looks invalid or corrupted.'); }
}

function startAdventure(){
  const saved = loadAdv();
  if(saved && saved.party && saved.party.length){
    adv = saved;
    showAdvScreens(); renderAdventure(); return;
  }
  beginNewStory();
}

let starterOptions = [];
function beginNewStory(){
  starterOptions = shuffle(CURATED_DEX).slice(0,3);
  ['titleScreen','setup','draft','battle','result','storyResult','adv'].forEach(id=>document.getElementById(id).classList.add('hidden'));
  document.getElementById('starterSelect').classList.remove('hidden');
  document.getElementById('apcNameInput').value = '';
  renderStarterSelect();
}
function renderStarterSelect(){
  document.getElementById('starterGrid').innerHTML = starterOptions.map((d,i)=>
    `<div class="mon mon-row" onclick="chooseStarter(${i})" style="cursor:pointer">
      ${monSprite(d)}
      <div class="mon-body">
        <div class="mon-top"><span>${d.name}</span></div>
        <div class="types">${typeBadges(d.types)}</div>
        <div class="abitag">${d.ability.n} — ${d.ability.desc}</div>
      </div>
    </div>`).join('');
}
function chooseStarter(i){
  const name = (document.getElementById('apcNameInput').value||'').trim().slice(0,16) || 'Trainer';
  const pick = starterOptions[i];
  adv = {playerName:name, party:[makeMon(pick, 0, 'none', 1)], loc:0, cleared:{}, items:{pokeball:10}, box:[]};
  saveAdv();
  renderAdventure();
}

function showAdvScreens(){
  ['titleScreen','setup','draft','battle','result','storyResult','starterSelect'].forEach(id=>document.getElementById(id).classList.add('hidden'));
  document.getElementById('adv').classList.remove('hidden');
}

function healParty(){
  for(const m of adv.party){ m.hp=m.maxhp; m.status=null; m.fainted=false; m.usedDisguise=false; m.sashUsed=false; m.sleepTurns=0; }
}

let mapOpen = false;
function toggleMap(){ mapOpen = !mapOpen; renderMap(); }
function renderMap(){
  const el = document.getElementById('advMap');
  el.classList.toggle('hidden', !mapOpen);
  if(!mapOpen) return;
  el.innerHTML = `<div class="maplist">` + LOCATIONS.map((loc,i)=>{
    const done = (loc.type==='gym'||loc.type==='trainer') ? !!adv.cleared[loc.name] : i<adv.loc;
    const icon = loc.type==='gym'?'🥊':loc.type==='trainer'?'🧑':loc.type==='route'?'🌿':'🏘️';
    return `<div class="maprow ${i===adv.loc?'current':''} ${done&&i!==adv.loc?'done':''}"><span class="dot"></span>${icon} ${loc.name}${done?' ✓':''}</div>`;
  }).join('') + `</div>`;
}

function renderAdventure(){
  showAdvScreens();
  if(!adv.pos) spawnPlayer();
  const loc = LOCATIONS[adv.loc];
  renderTileMap();
  document.getElementById('advTitle').textContent = `${loc.type==='gym'?'🥊':loc.type==='trainer'?'🧑':loc.type==='route'?'🌿':'🏘️'} ${loc.name}`;
  document.getElementById('advDesc').innerHTML = `${loc.desc} <span class="pill">🧑 ${adv.playerName}</span> <span class="pill">🔴 x${adv.items.pokeball||0}</span>${adv.box.length?` <span class="pill">📦 Box: ${adv.box.length}</span>`:''}`;
  renderMap();
  document.getElementById('advParty').innerHTML = adv.party.map((m,mi)=>{
    const xpPct = Math.max(0,Math.min(100,Math.round(100*(m.xp||0)/(m.xpNext||1))));
    return `<div class="mon ${m.fainted?'fainted':''}"><div class="mon-row">
      ${monSprite(m.dex)}
      <div class="mon-body">
        ${namePlate(m, ' HP', ` <button class="secondary" style="font-size:.6rem; padding:2px 5px; margin-left:4px" onclick="renamePartyMon(${mi})">✏️</button>`)}
        ${(m.nick||m.status) ? `<div class="mon-top" style="margin-top:-4px">${m.nick?`<span class="sub">(${m.name})</span> `:''}${statusTag(m)}</div>` : ''}
        <div class="types">${typeBadges(m.types)}</div>
        <div class="xpbar"><div class="xpfill" style="width:${xpPct}%"></div></div>
      </div>
    </div></div>`;
  }).join('');

  let actions = '';
  if(loc.type==='gym' || loc.type==='trainer'){
    actions += adv.cleared[loc.name]
      ? `<span class="sub">${loc.type==='gym'?'Gym Leader':'Rival'} ${loc.leaderName} — defeated ✅</span>`
      : `<span class="sub">Walk up to ${loc.leaderName} (🥊/🧑 on the map) to battle.</span>`;
  } else if(loc.endOfContent){
    actions += `<span class="sub">End of current content — more of Vellorin is coming in a future build.</span>`;
  }
  if(adv.box.length){
    actions += `<button class="secondary" onclick="openBox()">📦 Box (${adv.box.length})</button>`;
  }
  actions += `<button class="secondary" onclick="resetAll()">Exit to menu</button>`;
  document.getElementById('advActions').innerHTML = actions;
}

function advanceLoc(){
  const loc = LOCATIONS[adv.loc];
  if((loc.type==='gym'||loc.type==='trainer') && !adv.cleared[loc.name]){ showToast(`Beat ${loc.leaderName} first!`); return; }
  if(loc.endOfContent){ showToast("The trail ahead hasn't been charted yet!"); return; }
  if(adv.loc < LOCATIONS.length-1) adv.loc++;
  spawnPlayer();
  saveAdv();
  renderAdventure();
}

function startWildBattle(){
  const loc = LOCATIONS[adv.loc];
  if(alive(adv.party).length===0){ showToast('Your whole party has fainted! Rest at the Pokémon Center.'); return; }
  const n = 1+Math.floor(Math.random()*2);
  const picks = shuffle(loc.pool).slice(0,n);
  let id=9000;
  const lv = Math.max(3, advLevel()-2+Math.floor(Math.random()*4));
  const wild = picks.map(i=>makeMon(DEX[i], id++, 'none', lv));
  state = {sideA: adv.party, sideB: wild, log:[], mode:'story'};
  playBattleFlash();
  showAdvScreens();
  document.getElementById('battle').classList.remove('hidden');
  document.getElementById('adv').classList.add('hidden');
  addLog(`A wild encounter begins! (${wild.length} Pokémon, Lv.${lv})`);
  render();
  showMsgBox(state.log.slice(-1));
}

function startTrainerBattle(){
  const loc = LOCATIONS[adv.loc];
  if(alive(adv.party).length===0){ showToast('Your whole party has fainted! Rest at the Pokémon Center.'); return; }
  let id=9000;
  const lv = advLevel();
  const team = loc.leaderTeam.map(i=>makeMon(DEX[i], id++, 'leftovers', lv));
  state = {sideA: adv.party, sideB: team, log:[], mode:'story', trainerLoc: loc};
  playBattleFlash();
  showAdvScreens();
  document.getElementById('battle').classList.remove('hidden');
  document.getElementById('adv').classList.add('hidden');
  addLog(`${loc.type==='gym'?'Gym Leader':'Rival'} ${loc.leaderName} challenges you with ${team.length} Pokémon! (Lv.${lv})`);
  render();
  showMsgBox(state.log.slice(-1));
}

function continueStory(){
  document.getElementById('storyResult').classList.add('hidden');
  renderAdventure();
}

function shuffle(a){a=[...a]; for(let i=a.length-1;i>0;i--){const j=Math.floor(Math.random()*(i+1)); [a[i],a[j]]=[a[j],a[i]];} return a;}

for(let i=1;i<=6;i++){
  document.getElementById('sizeA').innerHTML += `<option value="${i}" ${i===3?'selected':''}>${i}</option>`;
  document.getElementById('sizeB').innerHTML += `<option value="${i}" ${i===3?'selected':''}>${i}</option>`;
}
{
  const tf = document.getElementById('draftTypeFilter');
  Object.keys(TYPE_COLORS).forEach(t=>{ tf.innerHTML += `<option value="${t}">${t}</option>`; });
}
function refreshContinueBtn(){
  const hasSave = !!loadAdv();
  document.getElementById('continueBtn').textContent = hasSave ? '▶ Continue Adventure' : '▶ Start Adventure';
  document.getElementById('newAdvBtn').classList.toggle('hidden', !hasSave);
}
function newAdventurePrompt(){
  showConfirm('Start a new adventure? This will overwrite your current saved game (export a save code first if you want to keep it).', ok=>{
    if(ok) beginNewStory();
  });
}
refreshContinueBtn();

function showTitle(){
  document.getElementById('titleScreen').classList.remove('hidden');
  document.getElementById('setup').classList.add('hidden');
  refreshContinueBtn();
}
function showSetup(){
  document.getElementById('titleScreen').classList.add('hidden');
  document.getElementById('setup').classList.remove('hidden');
  document.getElementById('newGameBtn').style.display = loadAdv() ? '' : 'none';
}
function newAdventure(){
  showConfirm('Erase your saved adventure and start a new one?', ok=>{
    if(!ok) return;
    try{ localStorage.removeItem(SAVE_KEY); }catch(e){}
    adv = null;
    beginNewStory();
  });
}

function goToDraft(mode){
  draftMode = mode;
  needA = +document.getElementById('sizeA').value;
  needB = +document.getElementById('sizeB').value;
  draftPool = shuffle(DEX);
  draftPicked = []; draftItems = {};
  document.getElementById('setup').classList.add('hidden');
  document.getElementById('draft').classList.remove('hidden');
  document.getElementById('draftSearch').value='';
  document.getElementById('draftTypeFilter').value='';
  renderDraft();
}

function itemOptions(sel){
  return Object.keys(ITEMS).map(k=>`<option value="${k}" ${k===sel?'selected':''}>${ITEMS[k].n}</option>`).join('');
}

function renderDraft(){
  document.getElementById('draftCounter').textContent = `Pick ${needA} Pokémon for your side (${draftPicked.length}/${needA} selected)`;
  const q = (document.getElementById('draftSearch').value||'').toLowerCase();
  const typeFilter = document.getElementById('draftTypeFilter').value;
  const sortBy = document.getElementById('draftSort').value;
  let rows = draftPool.map((d,i)=>({d,i}));
  if(q) rows = rows.filter(r=>r.d.name.toLowerCase().includes(q));
  if(typeFilter) rows = rows.filter(r=>r.d.types.includes(typeFilter));
  if(sortBy==='name') rows.sort((a,b)=>a.d.name.localeCompare(b.d.name));
  else if(sortBy==='total') rows.sort((a,b)=>{
    const t = x=>x.d.base.hp+x.d.base.atk+x.d.base.def+x.d.base.spa+x.d.base.spd+x.d.base.spe;
    return t(b)-t(a);
  });
  document.getElementById('draftGrid').innerHTML = rows.map(({d,i})=>{
    const picked = draftPicked.includes(i);
    return `<div class="mon ${picked?'picked':''}" onclick="toggleDraft(${i})" style="cursor:pointer">
      <div class="mon-row">
        ${monSprite(d)}
        <div class="mon-body">
          <div class="mon-top"><span>${d.name}</span><span>${picked?'✅':''}</span></div>
          <div class="types">${typeBadges(d.types)}</div>
          <div class="abitag">${d.ability.n} — ${d.ability.desc}</div>
          ${picked ? `<div class="itemsel" onclick="event.stopPropagation()"><label>Item</label><select onchange="draftItems[${i}]=this.value">${itemOptions(draftItems[i])}</select></div>` : ''}
        </div>
      </div>
    </div>`;
  }).join('') || `<div class="sub">No Pokémon match that search/filter.</div>`;
  document.getElementById('confirmDraftBtn').disabled = draftPicked.length !== needA;
}
function toggleDraft(i){
  const idx = draftPicked.indexOf(i);
  if(idx>=0) draftPicked.splice(idx,1);
  else { if(draftPicked.length>=needA) return; draftPicked.push(i); if(!(i in draftItems)) draftItems[i]='none'; }
  renderDraft();
}

function confirmDraft(){
  let id=0;
  const sideA = draftPicked.map(i=>makeMon(draftPool[i], id++, draftItems[i]||'none'));
  const remaining = draftPool.map((d,i)=>i).filter(i=>!draftPicked.includes(i));
  const enemyIdx = shuffle(remaining).slice(0,needB);
  const itemKeys = Object.keys(ITEMS);
  const sideB = enemyIdx.map(i=>makeMon(draftPool[i], id++, itemKeys[Math.floor(Math.random()*itemKeys.length)]));
  state = {sideA, sideB, log:[], mode:'free'};
  document.getElementById('draft').classList.add('hidden');
  document.getElementById('result').classList.add('hidden');
  document.getElementById('battle').classList.remove('hidden');
  addLog(`Battle start: ${sideA.length} vs ${sideB.length}!`);
  render();
}

function addLog(t){ state.log.push(t); const el=document.getElementById('log'); el.innerHTML = state.log.map(l=>`<div>${l}</div>`).join(''); el.scrollTop = el.scrollHeight; }

// Paced message box: reveals a batch of lines one at a time with a tap/keypress-to-continue
// prompt, like the classic dialogue box, instead of dumping a whole turn's log at once.
// The full scrollback log (addLog above) still updates immediately in parallel — this only
// paces what's shown in the popup and gates the Submit button until the player's caught up.
let msgQueue = [];
let msgQueueDone = null;
function showMsgBox(lines, onDone){
  if(!lines || !lines.length){ if(onDone) onDone(); return; }
  msgQueue = lines.slice();
  msgQueueDone = onDone || null;
  const btn = document.getElementById('submitBtn');
  if(btn) btn.disabled = true;
  advanceMsgBox();
}
function advanceMsgBox(){
  const box = document.getElementById('msgBox');
  if(!box) return;
  if(msgQueue.length===0){
    box.classList.add('hidden');
    const btn = document.getElementById('submitBtn');
    if(btn) btn.disabled = false;
    const cb = msgQueueDone; msgQueueDone = null;
    if(cb) cb();
    return;
  }
  document.getElementById('msgBoxText').innerHTML = msgQueue.shift();
  box.classList.remove('hidden');
}
function alive(side){ return side.filter(m=>!m.fainted && !m.caught); }

function renamePartyMon(i){
  const m = adv.party[i];
  showConfirm(`Nickname for ${m.name}? (leave blank to clear)`, input=>{
    m.nick = input.trim() ? input.trim().slice(0,12) : null;
    saveAdv();
    renderAdventure();
    document.getElementById('partyModal').classList.remove('hidden');
  }, true, m.nick||'');
}

function openBox(){
  document.getElementById('boxList').innerHTML = adv.box.map((m,i)=>
    `<div class="mon"><div class="mon-row">
       ${monSprite(m.dex)}
       <div class="mon-body">
         <div class="mon-top"><span>${dname(m)}<span class="lvbadge">Lv.${m.level}</span></span></div>
         <div class="types">${typeBadges(m.types)}</div>
         ${adv.party.length<6
            ? `<button class="secondary" onclick="swapBox(${i})">Add to party</button>`
            : `Swap in for: ` + adv.party.map((p,pi)=>`<button class="secondary" style="font-size:.7rem" onclick="swapBox(${i},${pi})">${dname(p)}</button>`).join(' ')}
       </div>
     </div></div>`).join('') || `<div class="sub">Box is empty.</div>`;
  document.getElementById('boxModal').classList.remove('hidden');
}
function closeBox(){ document.getElementById('boxModal').classList.add('hidden'); }
function swapBox(boxIdx, partyIdx){
  const boxed = adv.box[boxIdx];
  if(!boxed) return;
  if(adv.party.length < 6){
    adv.box.splice(boxIdx,1);
    adv.party.push(boxed);
  } else if(partyIdx!==undefined){
    adv.box[boxIdx] = adv.party[partyIdx];
    adv.party[partyIdx] = boxed;
  } else return;
  saveAdv();
  openBox();
  renderAdventure();
}

function render(){ renderSide('sideA', state.sideA, true); renderSide('sideB', state.sideB, false); }

function statusTag(m){
  if(!m.status) return '';
  const label = {par:'PAR',brn:'BRN',psn:'PSN',slp:'SLP',frz:'FRZ'}[m.status];
  return `<span class="status-tag st-${m.status}">${label}</span>`;
}
function namePlate(m, extra, corner){
  const pct = Math.max(0, Math.round(100*m.hp/m.maxhp));
  const color = pct>50?'var(--good)':pct>20?'#facc15':'var(--bad)';
  return `<div class="nameplate">
    <div class="np-top"><span class="np-name">${dname(m)}${m.fainted?' 💀':''}</span><span class="np-lv">Lv${m.level||50}${corner||''}</span></div>
    <div class="np-hprow"><span class="np-hplabel">HP</span><div class="hpbar"><div class="hpfill" style="width:${pct}%; background:${color}"></div></div></div>
    <div class="hptext">${m.hp}/${m.maxhp}${extra||''}</div>
  </div>`;
}

function renderSide(elId, side, interactive){
  const el = document.getElementById(elId);
  el.innerHTML = side.map(m=>{
    const pct = Math.max(0, Math.round(100*m.hp/m.maxhp));
    const color = pct>50?'var(--good)':pct>20?'#facc15':'var(--bad)';
    let moveOpts = '';
    const isWild = state.mode==='story' && !state.trainerLoc;
    if(interactive && !m.fainted){
      const ballOpt = isWild ? `<option value="ball">🔴 Throw Poké Ball (${adv.items.pokeball||0} left)</option>` : '';
      moveOpts = `<div class="movesel">
        <select id="move-${m.id}">${m.moves.map((mv,i)=>`<option value="${i}">${mv.n}${mv.p?` (${mv.p} pow)`:' (status)'}</option>`).join('')}${ballOpt}</select>
        <select id="target-${m.id}">${alive(state.sideB).map(t=>`<option value="${t.id}">${dname(t)} (${Math.round(100*t.hp/t.maxhp)}%)</option>`).join('')}</select>
      </div>`;
    }
    const itemTag = m.item !== 'none' ? ` · <span style="color:var(--sub)">${ITEMS[m.item].n}</span>` : '';
    return `<div class="mon ${m.fainted?'fainted':''}">
      <div class="mon-row">
        ${monSprite(m.dex, interactive?'back':'front')}
        <div class="mon-body">
          ${namePlate(m, ' HP')}
          ${m.status ? `<div class="mon-top" style="margin-top:-4px">${statusTag(m)}</div>` : ''}
          <div class="types">${typeBadges(m.types)} · ${m.ability.n}${itemTag}</div>
        </div>
      </div>
      ${moveOpts}
    </div>`;
  }).join('');
}

function applyStatus(target, status, chance){
  if(target.status || target.fainted) return;
  if(Math.random()*100 > chance) return;
  target.status = status;
  if(status==='slp') target.sleepTurns = 1+Math.floor(Math.random()*3);
  addLog(`${dname(target)} was afflicted with ${({par:'paralysis',brn:'a burn',psn:'poison',slp:'sleep',frz:'freeze'})[status]}!`);
}

function damage(user, move, target){
  const atkStat = move.c==='phys' ? user.atk : user.spa;
  let defStat = move.c==='phys' ? target.def : target.spd;
  const stab = user.types.includes(move.t) ? 1.5 : 1;
  let e = eff(move.t, target.types);
  if(target.ability.type==='immune' && target.ability.immuneType===move.t) e = 0;
  if(user.ability.type==='corrosion' && move.t==='Poison' && e===0) e = eff(move.t, target.types.filter(t=>t!=='Steel'&&t!=='Poison')) || 1;
  const rand = 0.85 + Math.random()*0.15;
  let power = move.p;
  if(user.ability.type==='boost' && user.ability.boostType===move.t && user.hp <= user.maxhp/3) power *= 1.5;
  if(user.ability.type==='punch' && move.punch) power *= 1.2;
  let dmg = Math.floor((((2*LEVEL/5+2)*power*(atkStat/defStat))/50 + 2) * stab * e * rand);
  if(user.status==='brn' && move.c==='phys') dmg = Math.floor(dmg/2);
  if(user.item==='lifeorb') dmg = Math.floor(dmg*1.3);
  if(user.ability.type==='merciless' && target.status==='psn') dmg = Math.floor(dmg*1.5);
  if(e===0) dmg=0;
  return {dmg, eff:e};
}

function canAct(m){
  if(m.status==='par' && Math.random()<0.25){ addLog(`${dname(m)} is paralyzed and can't move!`); return false; }
  if(m.status==='slp'){ m.sleepTurns--; if(m.sleepTurns<=0){ m.status=null; addLog(`${dname(m)} woke up!`); } else { addLog(`${dname(m)} is fast asleep.`); return false; } }
  if(m.status==='frz'){ if(Math.random()<0.2){ m.status=null; addLog(`${dname(m)} thawed out!`); } else { addLog(`${dname(m)} is frozen solid!`); return false; } }
  return true;
}

function effSpeed(m){ return m.status==='par' ? m.spe/2 : m.spe; }

function submitTurn(){
  const actorsA = alive(state.sideA);
  if(actorsA.length===0) return;
  const logStart = state.log.length;
  const actions = [];
  for(const m of actorsA){
    const moveSel = document.getElementById(`move-${m.id}`);
    const targetSel = document.getElementById(`target-${m.id}`);
    if(!moveSel || !targetSel) continue;
    const targetId = +targetSel.value;
    const target = state.sideB.find(t=>t.id===targetId);
    if(!target || target.fainted) continue;
    if(moveSel.value==='ball'){ actions.push({user:m, ball:true, target}); continue; }
    const move = m.moves[+moveSel.value];
    actions.push({user:m, move, target});
  }
  for(const m of alive(state.sideB)){
    const opts = alive(state.sideA);
    if(opts.length===0) continue;
    const move = m.moves[Math.floor(Math.random()*m.moves.length)];
    const target = opts[Math.floor(Math.random()*opts.length)];
    actions.push({user:m, move, target});
  }
  actions.sort((a,b)=>effSpeed(b.user) - effSpeed(a.user));

  for(const act of actions){
    if(act.user.fainted || act.target.fainted || act.target.caught) continue;
    if(act.ball){
      if(!adv.items.pokeball){ addLog(`No Poké Balls left!`); continue; }
      adv.items.pokeball--;
      const hpFrac = act.target.hp/act.target.maxhp;
      const statusBonus = act.target.status ? 1.5 : 1;
      const chance = Math.max(0.1, Math.min(0.95, 0.95 - hpFrac*0.7)) * statusBonus;
      addLog(`${dname(act.user)} threw a Poké Ball at ${dname(act.target)}!`);
      if(Math.random() < chance){
        act.target.caught = true;
        addLog(`Gotcha! ${dname(act.target)} was caught!`);
        if(adv.party.length < 6) adv.party.push(act.target);
        else { adv.box.push(act.target); addLog(`${dname(act.target)} was sent to the Box (party full).`); }
        saveAdv();
      } else {
        addLog(`Oh no! The wild ${dname(act.target)} broke free!`);
      }
      continue;
    }
    if(!canAct(act.user)) continue;
    if(Math.random()*100 > act.move.a){ addLog(`${dname(act.user)} used ${act.move.n} — missed!`); continue; }

    if(act.move.c==='status'){
      applyStatus(act.target, act.move.status, 100);
      continue;
    }

    if(act.target.ability.type==='disguise' && !act.target.usedDisguise){
      act.target.usedDisguise = true;
      addLog(`${dname(act.target)}'s Disguise absorbed the hit — no damage!`);
      continue;
    }

    const r = damage(act.user, act.move, act.target);
    let dmg = r.dmg;
    if(act.target.item==='sash' && !act.target.sashUsed && act.target.hp===act.target.maxhp && dmg>=act.target.hp){
      dmg = act.target.hp - 1; act.target.sashUsed = true;
      addLog(`${dname(act.target)} hung on using its Focus Sash!`);
    }
    act.target.hp = Math.max(0, act.target.hp - dmg);
    let tag = r.eff===0?' (no effect)':r.eff>1?' (super effective!)':r.eff<1?' (not very effective)':'';
    addLog(`${dname(act.user)} used ${act.move.n} on ${dname(act.target)} for ${dmg}${tag}`);
    if(act.move.sec) applyStatus(act.target, act.move.sec.status, act.move.sec.chance);
    if(act.target.hp<=0 && !act.target.fainted){ act.target.fainted=true; addLog(`${dname(act.target)} fainted!`); }
  }

  for(const side of [state.sideA, state.sideB]){
    for(const m of alive(side)){
      if(m.status==='brn' || m.status==='psn'){ const d = Math.floor(m.maxhp/12); m.hp = Math.max(0, m.hp-d); addLog(`${dname(m)} is hurt by its ${m.status==='brn'?'burn':'poison'}! (-${d})`); if(m.hp<=0){ m.fainted=true; addLog(`${dname(m)} fainted!`); } }
      if(m.item==='leftovers' && !m.fainted){ const h = Math.floor(m.maxhp*0.06); m.hp = Math.min(m.maxhp, m.hp+h); if(h>0) addLog(`${dname(m)} restored HP with Leftovers. (+${h})`); }
      if(m.item==='sitrus' && !m.berryUsed && !m.fainted && m.hp <= m.maxhp/2){ const h = Math.floor(m.maxhp*0.25); m.hp = Math.min(m.maxhp, m.hp+h); m.berryUsed = true; addLog(`${dname(m)} restored HP with its Sitrus Berry! (+${h})`); }
    }
  }

  render();
  showMsgBox(state.log.slice(logStart), checkEnd);
}

function checkEnd(){
  const a = alive(state.sideA).length, b = alive(state.sideB).length;
  if(a===0 || b===0){
    document.getElementById('battle').classList.add('hidden');
    if(state.mode==='story'){
      document.getElementById('storyResult').classList.remove('hidden');
      if(a>0){
        document.getElementById('storyResultText').textContent = '🏆 Victory!';
        const xpAmount = state.trainerLoc ? (state.trainerLoc.type==='gym'?60:35) : 18*state.sideB.length;
        for(const m of adv.party) grantXp(m, xpAmount);
        if(state.trainerLoc){ adv.cleared[state.trainerLoc.name]=true; document.getElementById('storyResultSub').textContent = `You defeated ${state.trainerLoc.type==='gym'?'Gym Leader':''} ${state.trainerLoc.leaderName}! (+${xpAmount} XP)`; }
        else document.getElementById('storyResultSub').textContent = `The wild Pokémon retreated. (+${xpAmount} XP)`;
      } else {
        document.getElementById('storyResultText').textContent = '💀 Your party was defeated...';
        document.getElementById('storyResultSub').textContent = 'You were carried back and fully healed.';
        healParty();
      }
      saveAdv();
    } else {
      document.getElementById('result').classList.remove('hidden');
      document.getElementById('resultText').textContent = a>0 ? '🏆 Your side wins!' : (b>0 ? '💀 Enemy side wins.' : 'Double knockout — draw!');
    }
  }
}

function resetAll(){
  state=null; adv=null;
  document.getElementById('result').classList.add('hidden');
  document.getElementById('battle').classList.add('hidden');
  document.getElementById('draft').classList.add('hidden');
  document.getElementById('adv').classList.add('hidden');
  document.getElementById('storyResult').classList.add('hidden');
  document.getElementById('setup').classList.add('hidden');
  document.getElementById('starterSelect').classList.add('hidden');
  document.getElementById('titleScreen').classList.remove('hidden');
  refreshContinueBtn();
}
