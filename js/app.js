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
// Real data from js/dexdata.js (generated from PokeAPI by tools/build-dexdata.js) replaces the
// hand-typed tables above: types, base stats, ability, level-up learnset and evolutions. The
// curated entries keep their hand-tuned abilities, which already have battle effects.
const STRUGGLE = {n:"Struggle", t:"Normal", p:50, c:"phys", a:100};
if(typeof DEXDATA!=='undefined') for(const d of DEX){
  const r = DEXDATA[slug(d.name)];
  if(!r) continue;
  d.types = r.types; d.base = r.base; d.learn = r.learn; d.evo = r.evo;
  if(!CURATED_DEX.includes(d)) d.ability = r.ability;
  d.moves = movesAt(d, LEVEL);
}
// The moves a wild/trainer Pokémon knows at a level: its last four level-up moves, like the games.
function movesAt(d, level){
  if(!d.learn) return d.moves.slice();
  const known = [];
  for(const [lv, mi] of d.learn){
    if(lv > level) break;
    const i = known.indexOf(mi); if(i>=0) known.splice(i,1);
    known.push(mi);
  }
  // Nothing usable yet (e.g. Magikarp only has Splash): take its first real attack, else Struggle.
  if(!known.length && d.learn.length) known.push(d.learn[0][1]);
  return known.length ? known.slice(-4).map(i=>MOVEDATA[i]) : [STRUGGLE];
}
function statCalc(base, isHp, level){
  level = level||LEVEL;
  if(isHp) return Math.floor(((2*base+31)*level)/100)+level+10;
  return Math.floor(((2*base+31)*level)/100)+5;
}
function makeMon(d,id,item,level){
  level = level||LEVEL;
  return {id, dex:d, name:d.name, nick:null, types:d.types, moves:movesAt(d, level), movesV:2, ability:d.ability, item:item||'none', level,
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
// Stats from base stats + level, keeping the same fraction of HP.
function recalcStats(mon){
  const b = mon.dex.base, hpFrac = mon.maxhp ? mon.hp/mon.maxhp : 1;
  mon.maxhp = statCalc(b.hp,true,mon.level);
  mon.hp = mon.fainted ? 0 : Math.max(1, Math.round(hpFrac*mon.maxhp));
  mon.atk = statCalc(b.atk,false,mon.level); mon.def = statCalc(b.def,false,mon.level);
  mon.spa = statCalc(b.spa,false,mon.level); mon.spd = statCalc(b.spd,false,mon.level);
  mon.spe = statCalc(b.spe,false,mon.level);
}
// Evolution: real level-up evolutions from the data; the old hand-written table still covers
// stone/trade/friendship evolutions (at a stand-in level) since there are no items for those yet.
function tryEvolve(mon){
  const evo = mon.dex.evo ? {to: DEX.find(d=>slug(d.name)===mon.dex.evo.to), level: mon.dex.evo.level}
    : EVOLUTIONS[mon.name] ? {to: DEX.find(d=>d.name===EVOLUTIONS[mon.name].to), level: EVOLUTIONS[mon.name].level} : null;
  if(!evo || !evo.to || mon.level < evo.level) return;
  const oldDisplay = dname(mon), newDex = evo.to;
  mon.name = newDex.name; mon.types = newDex.types; mon.ability = newDex.ability; mon.dex = newDex;
  recalcStats(mon);
  addLog(`✨ ${oldDisplay} evolved into ${newDex.name}!`);
  learnMovesAt(mon, mon.level);
}
// New level-up moves: fill empty slots, otherwise forget the oldest move (the games ask; this doesn't).
function learnMovesAt(mon, level){
  for(const [lv, mi] of (mon.dex.learn||[])){
    if(lv!==level) continue;
    const mv = MOVEDATA[mi];
    mon.moves = mon.moves.filter(m=>m.n!=='Struggle');
    if(mon.moves.some(m=>m.n===mv.n)) continue;
    if(mon.moves.length>=4){ const old = mon.moves.shift(); addLog(`${dname(mon)} forgot ${old.n} and learned ${mv.n}!`); }
    else addLog(`${dname(mon)} learned ${mv.n}!`);
    mon.moves.push(mv);
  }
}
function grantXp(mon, amount){
  if(mon.fainted) return;
  mon.xp = (mon.xp||0) + amount;
  while(mon.level<100 && mon.xp >= mon.xpNext){
    mon.xp -= mon.xpNext;
    mon.level++;
    mon.xpNext = mon.level*8;
    recalcStats(mon);
    addLog(`${dname(mon)} grew to level ${mon.level}!`);
    learnMovesAt(mon, mon.level);
    tryEvolve(mon);
  }
}
// Level cap for the current location — foes never exceed it, so you can out-level a route.
function advLevel(){
  const L = adv ? LOCATIONS[adv.loc] : null;
  return Math.min(50, 8 + (L ? (L.tier ?? adv.loc) : 0)*4);
}
function partyAvgLevel(){ return adv && adv.party.length ? adv.party.reduce((s,m)=>s+(m.level||5),0)/adv.party.length : 5; }
// Wild Pokémon sit a little under your party's level (like a route you're meant to be on);
// trainers match it, minus one to offset their numbers advantage.
function wildLevel(){ return Math.max(2, Math.min(advLevel(), Math.round(partyAvgLevel()) - 2 + Math.floor(Math.random()*3))); }
function trainerLevel(){ return Math.max(3, Math.min(advLevel(), Math.round(partyAvgLevel()) - 1)); }

const TYPE_COLORS = {
 Normal:"#A8A878",Fire:"#F08030",Water:"#6890F0",Electric:"#F8D030",Grass:"#78C850",Ice:"#98D8D8",
 Fighting:"#C03028",Poison:"#A040A0",Ground:"#E0C068",Flying:"#A890F0",Psychic:"#F85888",Bug:"#A8B820",
 Rock:"#B8A038",Ghost:"#705898",Dragon:"#7038F8",Dark:"#705848",Steel:"#B8B8D0",Fairy:"#EE99AC"
};
function dname(m){ return m.nick || m.name; }
// Custom-sprite hook: loads sprites in PokeAPI/sprites layout, keyed by National Dex number —
// sprites/pokemon/<num>.png (front) and sprites/pokemon/back/<num>.png. Only reads files the
// player supplies themselves. Missing file -> onerror swaps in the ❔ placeholder, so this
// works unmodified whether or not a sprites/ folder exists alongside the HTML.
function slug(name){
  return name.toLowerCase().normalize('NFD').replace(/[̀-ͯ]/g,'')
    .replace(/[^a-z0-9]+/g,'-').replace(/^-+|-+$/g,'');
}
// slug -> National Dex number (PokeAPI id; regional forms use PokeAPI's 10xxx form ids).
const DEX_NUM = {
 "bulbasaur":1,"ivysaur":2,"venusaur":3,"charmander":4,"charmeleon":5,"charizard":6,"squirtle":7,"wartortle":8,
 "blastoise":9,"caterpie":10,"metapod":11,"butterfree":12,"weedle":13,"kakuna":14,"beedrill":15,"pidgey":16,
 "pidgeotto":17,"pidgeot":18,"rattata":19,"raticate":20,"spearow":21,"fearow":22,"ekans":23,"arbok":24,
 "pikachu":25,"raichu":26,"sandshrew":27,"sandslash":28,"nidoran-f":29,"nidorina":30,"nidoqueen":31,"nidoran-m":32,
 "nidorino":33,"nidoking":34,"clefairy":35,"clefable":36,"vulpix":37,"ninetales":38,"jigglypuff":39,"wigglytuff":40,
 "zubat":41,"golbat":42,"oddish":43,"gloom":44,"vileplume":45,"paras":46,"parasect":47,"venonat":48,
 "venomoth":49,"diglett":50,"dugtrio":51,"meowth":52,"persian":53,"psyduck":54,"golduck":55,"mankey":56,
 "primeape":57,"growlithe":58,"arcanine":59,"poliwag":60,"poliwhirl":61,"poliwrath":62,"abra":63,"kadabra":64,
 "alakazam":65,"machop":66,"machoke":67,"machamp":68,"bellsprout":69,"weepinbell":70,"victreebel":71,"tentacool":72,
 "tentacruel":73,"geodude":74,"graveler":75,"golem":76,"ponyta":77,"rapidash":78,"slowpoke":79,"slowbro":80,
 "magnemite":81,"magneton":82,"farfetchd":83,"doduo":84,"dodrio":85,"seel":86,"dewgong":87,"grimer":88,
 "muk":89,"shellder":90,"cloyster":91,"gastly":92,"haunter":93,"gengar":94,"onix":95,"drowzee":96,
 "hypno":97,"krabby":98,"kingler":99,"voltorb":100,"electrode":101,"exeggcute":102,"exeggutor":103,"cubone":104,
 "marowak":105,"hitmonlee":106,"hitmonchan":107,"lickitung":108,"koffing":109,"weezing":110,"rhyhorn":111,"rhydon":112,
 "chansey":113,"tangela":114,"kangaskhan":115,"horsea":116,"seadra":117,"goldeen":118,"seaking":119,"staryu":120,
 "starmie":121,"mr-mime":122,"scyther":123,"jynx":124,"electabuzz":125,"magmar":126,"pinsir":127,"tauros":128,
 "magikarp":129,"gyarados":130,"lapras":131,"ditto":132,"eevee":133,"vaporeon":134,"jolteon":135,"flareon":136,
 "porygon":137,"omanyte":138,"omastar":139,"kabuto":140,"kabutops":141,"aerodactyl":142,"snorlax":143,"articuno":144,
 "zapdos":145,"moltres":146,"dratini":147,"dragonair":148,"dragonite":149,"mewtwo":150,"mew":151,"chikorita":152,
 "bayleef":153,"meganium":154,"cyndaquil":155,"quilava":156,"typhlosion":157,"totodile":158,"croconaw":159,"feraligatr":160,
 "sentret":161,"furret":162,"hoothoot":163,"noctowl":164,"ledyba":165,"ledian":166,"spinarak":167,"ariados":168,
 "crobat":169,"chinchou":170,"lanturn":171,"pichu":172,"cleffa":173,"igglybuff":174,"togepi":175,"togetic":176,
 "natu":177,"xatu":178,"mareep":179,"flaaffy":180,"ampharos":181,"bellossom":182,"marill":183,"azumarill":184,
 "sudowoodo":185,"politoed":186,"hoppip":187,"skiploom":188,"jumpluff":189,"aipom":190,"sunkern":191,"sunflora":192,
 "yanma":193,"wooper":194,"quagsire":195,"espeon":196,"umbreon":197,"murkrow":198,"slowking":199,"misdreavus":200,
 "unown":201,"wobbuffet":202,"girafarig":203,"pineco":204,"forretress":205,"dunsparce":206,"gligar":207,"steelix":208,
 "snubbull":209,"granbull":210,"qwilfish":211,"scizor":212,"shuckle":213,"heracross":214,"sneasel":215,"teddiursa":216,
 "ursaring":217,"slugma":218,"magcargo":219,"swinub":220,"piloswine":221,"corsola":222,"remoraid":223,"octillery":224,
 "delibird":225,"mantine":226,"skarmory":227,"houndour":228,"houndoom":229,"kingdra":230,"phanpy":231,"donphan":232,
 "porygon2":233,"stantler":234,"smeargle":235,"tyrogue":236,"hitmontop":237,"smoochum":238,"elekid":239,"magby":240,
 "miltank":241,"blissey":242,"raikou":243,"entei":244,"suicune":245,"larvitar":246,"pupitar":247,"tyranitar":248,
 "lugia":249,"ho-oh":250,"celebi":251,"treecko":252,"grovyle":253,"sceptile":254,"torchic":255,"combusken":256,
 "blaziken":257,"mudkip":258,"marshtomp":259,"swampert":260,"poochyena":261,"mightyena":262,"zigzagoon":263,"linoone":264,
 "wurmple":265,"silcoon":266,"beautifly":267,"cascoon":268,"dustox":269,"lotad":270,"lombre":271,"ludicolo":272,
 "seedot":273,"nuzleaf":274,"shiftry":275,"taillow":276,"swellow":277,"wingull":278,"pelipper":279,"ralts":280,
 "kirlia":281,"gardevoir":282,"surskit":283,"masquerain":284,"shroomish":285,"breloom":286,"slakoth":287,"vigoroth":288,
 "slaking":289,"nincada":290,"ninjask":291,"shedinja":292,"whismur":293,"loudred":294,"exploud":295,"makuhita":296,
 "hariyama":297,"azurill":298,"nosepass":299,"skitty":300,"delcatty":301,"sableye":302,"mawile":303,"aron":304,
 "lairon":305,"aggron":306,"meditite":307,"medicham":308,"electrike":309,"manectric":310,"plusle":311,"minun":312,
 "volbeat":313,"illumise":314,"roselia":315,"gulpin":316,"swalot":317,"carvanha":318,"sharpedo":319,"wailmer":320,
 "wailord":321,"numel":322,"camerupt":323,"torkoal":324,"spoink":325,"grumpig":326,"spinda":327,"trapinch":328,
 "vibrava":329,"flygon":330,"cacnea":331,"cacturne":332,"swablu":333,"altaria":334,"zangoose":335,"seviper":336,
 "lunatone":337,"solrock":338,"barboach":339,"whiscash":340,"corphish":341,"crawdaunt":342,"baltoy":343,"claydol":344,
 "lileep":345,"cradily":346,"anorith":347,"armaldo":348,"feebas":349,"milotic":350,"castform":351,"kecleon":352,
 "shuppet":353,"banette":354,"duskull":355,"dusclops":356,"tropius":357,"chimecho":358,"absol":359,"wynaut":360,
 "snorunt":361,"glalie":362,"spheal":363,"sealeo":364,"walrein":365,"clamperl":366,"huntail":367,"gorebyss":368,
 "relicanth":369,"luvdisc":370,"bagon":371,"shelgon":372,"salamence":373,"beldum":374,"metang":375,"metagross":376,
 "regirock":377,"regice":378,"registeel":379,"latias":380,"latios":381,"kyogre":382,"groudon":383,"rayquaza":384,
 "jirachi":385,"deoxys":386,"turtwig":387,"grotle":388,"torterra":389,"chimchar":390,"monferno":391,"infernape":392,
 "piplup":393,"prinplup":394,"empoleon":395,"starly":396,"staravia":397,"staraptor":398,"bidoof":399,"bibarel":400,
 "kricketot":401,"kricketune":402,"shinx":403,"luxio":404,"luxray":405,"budew":406,"roserade":407,"cranidos":408,
 "rampardos":409,"shieldon":410,"bastiodon":411,"burmy":412,"wormadam":413,"mothim":414,"combee":415,"vespiquen":416,
 "pachirisu":417,"buizel":418,"floatzel":419,"cherubi":420,"cherrim":421,"shellos":422,"gastrodon":423,"ambipom":424,
 "drifloon":425,"drifblim":426,"buneary":427,"lopunny":428,"mismagius":429,"honchkrow":430,"glameow":431,"purugly":432,
 "chingling":433,"stunky":434,"skuntank":435,"bronzor":436,"bronzong":437,"bonsly":438,"mime-jr":439,"happiny":440,
 "chatot":441,"spiritomb":442,"gible":443,"gabite":444,"garchomp":445,"munchlax":446,"riolu":447,"lucario":448,
 "hippopotas":449,"hippowdon":450,"skorupi":451,"drapion":452,"croagunk":453,"toxicroak":454,"carnivine":455,"finneon":456,
 "lumineon":457,"mantyke":458,"snover":459,"abomasnow":460,"weavile":461,"magnezone":462,"lickilicky":463,"rhyperior":464,
 "tangrowth":465,"electivire":466,"magmortar":467,"togekiss":468,"yanmega":469,"leafeon":470,"glaceon":471,"gliscor":472,
 "mamoswine":473,"porygon-z":474,"gallade":475,"probopass":476,"dusknoir":477,"froslass":478,"rotom":479,"uxie":480,
 "mesprit":481,"azelf":482,"dialga":483,"palkia":484,"heatran":485,"regigigas":486,"giratina":487,"cresselia":488,
 "phione":489,"manaphy":490,"darkrai":491,"shaymin":492,"arceus":493,"victini":494,"snivy":495,"servine":496,
 "serperior":497,"tepig":498,"pignite":499,"emboar":500,"oshawott":501,"dewott":502,"samurott":503,"patrat":504,
 "watchog":505,"lillipup":506,"herdier":507,"stoutland":508,"purrloin":509,"liepard":510,"pansage":511,"simisage":512,
 "pansear":513,"simisear":514,"panpour":515,"simipour":516,"munna":517,"musharna":518,"pidove":519,"tranquill":520,
 "unfezant":521,"blitzle":522,"zebstrika":523,"roggenrola":524,"boldore":525,"gigalith":526,"woobat":527,"swoobat":528,
 "drilbur":529,"excadrill":530,"audino":531,"timburr":532,"gurdurr":533,"conkeldurr":534,"tympole":535,"palpitoad":536,
 "seismitoad":537,"throh":538,"sawk":539,"sewaddle":540,"swadloon":541,"leavanny":542,"venipede":543,"whirlipede":544,
 "scolipede":545,"cottonee":546,"whimsicott":547,"petilil":548,"lilligant":549,"basculin":550,"sandile":551,"krokorok":552,
 "krookodile":553,"darumaka":554,"darmanitan":555,"maractus":556,"dwebble":557,"crustle":558,"scraggy":559,"scrafty":560,
 "sigilyph":561,"yamask":562,"cofagrigus":563,"tirtouga":564,"carracosta":565,"archen":566,"archeops":567,"trubbish":568,
 "garbodor":569,"zorua":570,"zoroark":571,"minccino":572,"cinccino":573,"gothita":574,"gothorita":575,"gothitelle":576,
 "solosis":577,"duosion":578,"reuniclus":579,"ducklett":580,"swanna":581,"vanillite":582,"vanillish":583,"vanilluxe":584,
 "deerling":585,"sawsbuck":586,"emolga":587,"karrablast":588,"escavalier":589,"foongus":590,"amoonguss":591,"frillish":592,
 "jellicent":593,"alomomola":594,"joltik":595,"galvantula":596,"ferroseed":597,"ferrothorn":598,"klink":599,"klang":600,
 "klinklang":601,"tynamo":602,"eelektrik":603,"eelektross":604,"elgyem":605,"beheeyem":606,"litwick":607,"lampent":608,
 "chandelure":609,"axew":610,"fraxure":611,"haxorus":612,"cubchoo":613,"beartic":614,"cryogonal":615,"shelmet":616,
 "accelgor":617,"stunfisk":618,"mienfoo":619,"mienshao":620,"druddigon":621,"golett":622,"golurk":623,"pawniard":624,
 "bisharp":625,"bouffalant":626,"rufflet":627,"braviary":628,"vullaby":629,"mandibuzz":630,"heatmor":631,"durant":632,
 "deino":633,"zweilous":634,"hydreigon":635,"larvesta":636,"volcarona":637,"cobalion":638,"terrakion":639,"virizion":640,
 "tornadus":641,"thundurus":642,"reshiram":643,"zekrom":644,"landorus":645,"kyurem":646,"keldeo":647,"meloetta":648,
 "genesect":649,"chespin":650,"quilladin":651,"chesnaught":652,"fennekin":653,"braixen":654,"delphox":655,"froakie":656,
 "frogadier":657,"greninja":658,"bunnelby":659,"diggersby":660,"fletchling":661,"fletchinder":662,"talonflame":663,"scatterbug":664,
 "spewpa":665,"vivillon":666,"litleo":667,"pyroar":668,"flabebe":669,"floette":670,"florges":671,"skiddo":672,
 "gogoat":673,"pancham":674,"pangoro":675,"furfrou":676,"espurr":677,"meowstic":678,"honedge":679,"doublade":680,
 "aegislash":681,"spritzee":682,"aromatisse":683,"swirlix":684,"slurpuff":685,"inkay":686,"malamar":687,"binacle":688,
 "barbaracle":689,"skrelp":690,"dragalge":691,"clauncher":692,"clawitzer":693,"helioptile":694,"heliolisk":695,"tyrunt":696,
 "tyrantrum":697,"amaura":698,"aurorus":699,"sylveon":700,"hawlucha":701,"dedenne":702,"carbink":703,"goomy":704,
 "sliggoo":705,"goodra":706,"klefki":707,"phantump":708,"trevenant":709,"pumpkaboo":710,"gourgeist":711,"bergmite":712,
 "avalugg":713,"noibat":714,"noivern":715,"xerneas":716,"yveltal":717,"zygarde":718,"diancie":719,"hoopa":720,
 "volcanion":721,"rowlet":722,"dartrix":723,"decidueye":724,"litten":725,"torracat":726,"incineroar":727,"popplio":728,
 "brionne":729,"primarina":730,"pikipek":731,"trumbeak":732,"toucannon":733,"yungoos":734,"gumshoos":735,"grubbin":736,
 "charjabug":737,"vikavolt":738,"crabrawler":739,"crabominable":740,"oricorio":741,"ribombee":743,"rockruff":744,"lycanroc":745,
 "wishiwashi":746,"mareanie":747,"toxapex":748,"mudsdale":750,"dewpider":751,"araquanid":752,"fomantis":753,"lurantis":754,
 "shiinotic":756,"salazzle":758,"stufful":759,"bewear":760,"bounsweet":761,"steenee":762,"tsareena":763,"comfey":764,
 "oranguru":765,"passimian":766,"wimpod":767,"golisopod":768,"sandygast":769,"palossand":770,"pyukumuku":771,"type-null":772,
 "silvally":773,"minior":774,"komala":775,"turtonator":776,"togedemaru":777,"mimikyu":778,"bruxish":779,"drampa":780,
 "dhelmise":781,"jangmo-o":782,"hakamo-o":783,"kommo-o":784,"tapu-koko":785,"tapu-lele":786,"tapu-bulu":787,"tapu-fini":788,
 "cosmog":789,"cosmoem":790,"solgaleo":791,"lunala":792,"nihilego":793,"buzzwole":794,"pheromosa":795,"xurkitree":796,
 "celesteela":797,"kartana":798,"guzzlord":799,"necrozma":800,"magearna":801,"marshadow":802,"poipole":803,"naganadel":804,
 "stakataka":805,"blacephalon":806,"zeraora":807,"meltan":808,"melmetal":809,"ninetales-alola":10104
};
function spritePath(d, facing){
  const num = DEX_NUM[slug(d.name)] || 0;
  return `sprites/pokemon/${facing==='back'?'back/':''}${num}.png`;
}
function monSprite(d, facing){
  const path = spritePath(d, facing);
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
function dexByName(n){ return DEX.find(d=>d.name===n) || DEX[0]; }
// The Vellorin region is a grid of areas; `at` is each area's [x,y] cell. Story areas come first,
// in order (their index is the progress tier, and saves store it), then optional side areas.
// Wild pools and trainer teams are real species by name.
const LOCATIONS=[
 {type:'town', name:"Duskmere Hollow", at:[0,0], desc:"Your home town, at the edge of the Vellorin region. A Pokémon Center sits by the old well.", center:true},
 {type:'route', name:"Route 1: Fernway Trail", at:[1,0], desc:"Tall grass lines a quiet dirt path. Something's rustling.", pool:['Pidgey','Rattata','Zigzagoon','Bidoof','Pikipek','Caterpie']},
 {type:'trainer', kind:'rival', name:"Fernway Overlook", at:[2,0], desc:"Your rival Wren is waiting on the ridge, arms crossed. \"Let's see how far you've come.\"", leaderName:"Wren", leaderTeam:['Pidgey','Eevee']},
 {type:'gym', name:"Cindergate Town", at:[2,-1], desc:"Smoke curls from the gym's chimney. Leader Rell awaits with a scorched-earth team.", center:true, leaderName:"Rell", leaderTeam:['Slugma','Growlithe','Torkoal']},
 {type:'route', name:"Route 2: Marrow Pass", at:[3,-1], desc:"A narrow pass between cliffs. The wind carries distant cries.", pool:['Geodude','Spearow','Zubat','Sandshrew','Machop','Rockruff'], theme:'rocky'},
 {type:'gym', name:"Tidalkeep City", at:[4,-1], desc:"Waves crash against the gym's sea wall. Leader Sable commands the tide.", center:true, leaderName:"Sable", leaderTeam:['Horsea','Staryu','Wailmer']},
 {type:'route', name:"Route 3: Hollow Bluffs", at:[4,0], desc:"Weathered bluffs overlook the coast. The path forks ahead.", pool:['Wingull','Krabby','Tentacool','Psyduck','Slowpoke','Corphish']},
 {type:'gym', name:"Stonebrook Town", at:[4,1], desc:"A rugged gym built into the bluffs. Leader Orin trains ground-pounders and toxic tacticians.", center:true, leaderName:"Orin", leaderTeam:['Diglett','Nidorina','Sandslash']},
 {type:'route', name:"Route 4: The Roost", at:[3,1], desc:"Cliffside nests dot the rockface above. Wings flash in the haze.", pool:['Spearow','Taillow','Starly','Hoothoot','Pidgeotto','Swablu']},
 {type:'trainer', kind:'rival', name:"Windward Ledge", at:[2,1], desc:"Wren again, team clearly stronger this time. \"You've grown. Let's finish this properly.\"", leaderName:"Wren", leaderTeam:['Pidgeotto','Espeon','Growlithe','Pikachu']},
 {type:'route', name:"Route 5: Cragmoor Trail", at:[2,2], desc:"Loose scree and echoing caves. Wild Pokémon lurk in the dark.", pool:['Machop','Geodude','Gastly','Onix','Cubone'], theme:'rocky'},
 {type:'gym', name:"Wispgate City", at:[3,2], desc:"Lantern-lit streets wind up to a gym wreathed in fog. Leader Sable's successor, Iska, trains ghosts and psychics.", center:true, leaderName:"Iska", leaderTeam:['Haunter','Kadabra','Gengar']},
 {type:'route', name:"Route 6: Emberflow Delta", at:[4,2], desc:"Steam vents hiss where river meets old lava rock.", pool:['Charmander','Growlithe','Numel','Krabby','Magikarp','Slugma']},
 {type:'town', name:"???", at:[5,2], desc:"The trail ahead hasn't been charted yet — more of Vellorin is on the way in a future update.", center:true, endOfContent:true},
 // Side areas (optional; `tier` sets their level cap to match where they branch off)
 {type:'route', name:"Whisperwood", at:[0,-1], tier:1, desc:"Old trees crowd out the sky. Bug and Grass Pokémon thrive in the shade.", pool:['Caterpie','Weedle','Oddish','Bellsprout','Wurmple','Seedot','Grubbin'], theme:'forest'},
 {type:'route', name:"Mirror Lake", at:[1,1], tier:2, desc:"A still lake reflects the sky. Water Pokémon splash near the shore.", pool:['Psyduck','Poliwag','Lotad','Marill','Surskit','Wooper'], theme:'lake'},
 {type:'route', name:"Glimmer Coast", at:[5,-1], tier:6, desc:"Sea spray and sparkling sand. Shells glint in the tide pools.", pool:['Wingull','Shellder','Staryu','Krabby','Horsea','Corsola'], theme:'lake'},
 {type:'route', name:"Old Quarry", at:[3,0], tier:7, desc:"Abandoned cuts in the rock. Boulders everywhere — some of them move.", pool:['Geodude','Onix','Aron','Nosepass','Roggenrola','Larvitar'], theme:'rocky'}
];
// Story areas link in order; the way onward from a gym or rival stays shut until they're beaten.
// Every link works both ways, so you can always walk back.
const OPPOSITE = {up:'down', down:'up', left:'right', right:'left'};
// Party Royale's party: up to 10, and every battle is your whole party against theirs.
const MAX_PARTY = 10;
function linkAreas(i, j, gate){
  const a = LOCATIONS[i], b = LOCATIONS[j];
  const dx = b.at[0]-a.at[0], dy = b.at[1]-a.at[1];
  const dir = dx>0 ? 'right' : dx<0 ? 'left' : dy>0 ? 'down' : 'up';
  (a.links ||= []).push({dir, to:j, gate});
  (b.links ||= []).push({dir:OPPOSITE[dir], to:i, gate:false});
}
for(let i=0;i<13;i++) linkAreas(i, i+1, LOCATIONS[i].type==='gym' || LOCATIONS[i].type==='trainer');
for(const [i,j] of [[0,14],[1,15],[5,16],[6,17]]) linkAreas(i, j, false);

// --- Overworld (GBA style) ---
// Hand-laid towns / generated routes with painted buildings, a 15×10-tile camera that follows the
// player, exits on any side leading to neighbouring areas, and frame-synced hold-to-walk movement.
// All art here is original: CSS tiles/buildings and hand-made 12×16 pixel-art trainers.
// Emerald timing at 60 fps: a walk step is 16 frames, a run step or a turn-in-place is 8.
const T = 32, VIEW_W = 15, VIEW_H = 10, WALK_MS = 16000/60, RUN_MS = 8000/60, TURN_MS = 8000/60, JUMP_MS = 32000/60;
function seedRand(str){
  let h=0; for(let i=0;i<str.length;i++) h=(h*31+str.charCodeAt(i))>>>0;
  return function(){ h=(h*1103515245+12345)>>>0; return (h>>>8)/0x1000000; };
}

// Tiles: T tree · . grass · : path · " tall grass · ~ water · * flowers · F fence · S sign ·
// b bush · r rock · L ledge (hop down it) · N route sign · B building (solid) · D door
// Ledges: the direction you can hop over them.
const LEDGE_DIR = {'L':'down', '>':'right', '<':'left'};
const TILE_CLS = {'x':'gymwall','I':'item','L':'ledge','>':'ledge-e','<':'ledge-w','N':'rsign','r':'rock','b':'bush','T':'tree','.':'grass',':':'path','"':'tall','~':'water','*':'flower','F':'fence','S':'sign','E':'exit','B':'grass','D':'path',
  '#':'wall','_':'floor','o':'rug','M':'mat','c':'counter','h':'healer','P':'pc','k':'bookshelf','v':'tv','e':'bed','t':'table','p':'plant','s':'shelf','u':'statue',
  'n':'wall wmon','m':'wall wmap','w':'wall wwin','K':'wall wclock','C':'counter cend','q':'seat','Q':'seat yellow','g':'glasstable'};
const WALKABLE = new Set(['.',':','"','*','E','D','_','o','M']);
const TOWN_ROWS = [
  'TTTTTTTTTTTTTTTTTTTTTT',
  'TT..................TT',
  'T....................T',
  'T....................T',
  'T....................T',
  'T....................T',
  'T::::::::::::::::::::T',
  'T::::::::::::::::::::T',
  'T..S............**...T',
  'T.**.......~~~~.**...T',
  'T.**.......~~~~......T',
  'T..........~~~~...TT.T',
  'TFFFFF............TT.T',
  'TTTTTTTTTTTTTTTTTTTTTT'];
// Openings in the tree border, Emerald style: a few tiles wide, so the next area is in view
// before you get there. `x,y` is the first open border tile and `len` runs along the edge.
// A neighbour is drawn so its matching opening starts at the same place.
// Clear the opening and one tile in. Routes pass ',' (grass that scenery won't be put on; see
// buildRoute), so the gap never gets plugged; paths already there are kept.
function carveOpening(tiles, dir, o, fill){
  const [ix,iy] = STEP_IN[dir], horiz = dir==='left' || dir==='right';
  for(let i=0;i<o.len;i++) for(let d=0;d<2;d++){
    const row = tiles[o.y + (horiz?i:0) + iy*d], x = o.x + (horiz?0:i) + ix*d;
    if(row[x]!==':') row[x] = fill;
  }
}
const HOUSE_ROOFS = [['#58a860','#408848'],['#b07040','#905830'],['#8860c0','#6848a0'],['#d09040','#b07028']];
const HOUSE_LINES = [
  ["Nobody's home. A Pikachu clock ticks on the wall."],
  ["A kid is glued to a battle on TV.", "\"Super effective moves do double damage! Everyone knows that!\""],
  ["Something's baking. It smells like Oran Berries."],
  ["An old man looks up from his paper.", "\"Wild Pokémon hide in tall grass. Walk through it and they'll find you!\""],
  ["\"Weaken a wild Pokémon before you throw a Poké Ball. Works every time... mostly.\""]];
const STEP_IN = {left:[1,0], right:[-1,0], up:[0,1], down:[0,-1]};
const TOWNSFOLK = ['youngster','lass','oldman','girl','boy','gentleman','oldwoman'];
const TOWN_TALK = [
  ["Did you know? Pokémon Centers heal your whole party for free."],
  ["The Poké Mart clerk gives new trainers free Poké Balls. Nice, right?"],
  ["I saw a trainer with a Pokémon twice my size. I need to train harder!"],
  ["Wild Pokémon only jump out of tall grass. Stick to the path if your team's tired."],
  ["Each Gym Leader uses one or two types. Plan your team around that!"],
  ["Walk off the edge of town and you'll reach the next area. You can always come back."]];
const ROUTE_TALK = {
  youngster:["I like shorts! They're comfy and easy to wear!", "...Also, hold Shift to run. Or press B."],
  bugcatcher:["Bug Pokémon evolve fast! Caterpie becomes Metapod at level 7!"],
  fisher:["The water Pokémon here come up into the lakeside grass.", "No rod needed!"],
  hiker:["Rock and Ground Pokémon hate Water and Grass moves. Remember that!"]};   // from an exit tile into the map

// Towns are 36×26: the compact centre (TOWN_ROWS: Pokémon Center, Mart/Gym, houses, pond) sits in
// outskirts with a few more houses, trees and flowers, and roads running out to each exit.
const TOWN_W = 36, TOWN_H = 26, TOWN_OX = 7, TOWN_OY = 6;
function buildTown(loc){
  const rnd = seedRand(loc.name);
  const W = TOWN_W, H = TOWN_H, OX = TOWN_OX, OY = TOWN_OY;
  const inside = (x,y)=> x>=2 && y>=2 && x<W-2 && y<H-2;
  const tiles = Array.from({length:H}, (_,y)=>Array.from({length:W}, (_,x)=> inside(x,y) ? '.' : 'T'));
  // About half the towns (seeded) mirror the centre left–right, so the Pokémon Center, Mart/Gym,
  // pond and north–south lane swap sides. P(x) places a centre column, R(x,w) a centre rectangle.
  const flip = seedRand(loc.name + '/flip')() < 0.5;
  const P = x=>OX + (flip ? 21-x : x), R = (x,w)=>OX + (flip ? 21-(x+w-1) : x), LANE = P(flip ? 9 : 7);
  // The centre; its own ring of trees becomes open ground.
  TOWN_ROWS.forEach((row,y)=>row.split('').forEach((ch,x)=>{
    const edge = x===0 || y===0 || x===row.length-1 || y===TOWN_ROWS.length-1;
    tiles[y+OY][P(x)] = edge ? '.' : ch;
  }));
  // Roads: west/east on the centre's road rows, a 3-wide lane north/south; openings in the border.
  const OPEN = {left:{x:0,y:OY+5,len:4}, right:{x:W-1,y:OY+5,len:4}, up:{x:LANE,y:0,len:3}, down:{x:LANE,y:H-1,len:3}};
  const exits = [], ARROW = {left:'←', right:'→', up:'↑', down:'↓'};
  const signs = [{x:P(3), y:8+OY, text:loc.name.toUpperCase()}];
  for(const l of loc.links){
    // A sign beside the road just inside each exit, as on routes.
    const [sx, sy] = {left:[3, OY+5], right:[W-4, OY+5], up:[LANE-1, 3], down:[LANE+3, H-4]}[l.dir];
    tiles[sy][sx] = 'N';
    signs.push({x:sx, y:sy, route:true, text:`${loc.name.toUpperCase()}\n${ARROW[l.dir]} ${LOCATIONS[l.to].name.toUpperCase()}`});
    if(l.dir==='left')  for(let x=0; x<=OX; x++) tiles[OY+6][x] = tiles[OY+7][x] = ':';
    if(l.dir==='right') for(let x=OX+21; x<W; x++) tiles[OY+6][x] = tiles[OY+7][x] = ':';
    if(l.dir==='up')    for(let y=0; y<OY+6; y++) for(let x=LANE; x<LANE+3; x++) tiles[y][x] = ':';
    if(l.dir==='down')  for(let y=OY+8; y<H; y++) for(let x=LANE; x<LANE+3; x++) tiles[y][x] = ':';
    carveOpening(tiles, l.dir, OPEN[l.dir], '.');
    exits.push({...OPEN[l.dir], ...l});
  }
  const house = (x,y)=>({kind:'house', x, y, w:4, h:3, door:{x:x+1,y:y+2}, roof:HOUSE_ROOFS[Math.floor(rnd()*4)], lines:HOUSE_LINES[Math.floor(rnd()*HOUSE_LINES.length)]});
  const b = [];
  if(loc.center) b.push({kind:'center', x:R(2,5), y:2+OY, w:5, h:4, door:{x:P(4),y:5+OY}});
  if(loc.type==='gym') b.push({kind:'gym', x:R(10,6), y:2+OY, w:6, h:4, door:{x:P(12),y:5+OY}});
  else b.push(house(R(10,4), 3+OY));
  if(loc.center) b.push({kind:'mart', x:R(17,4), y:3+OY, w:4, h:3, door:{x:P(18),y:5+OY}});
  else b.push(house(R(17,4), 3+OY));
  // Outskirt houses (two or three, seeded) in the corners, away from the roads.
  const spots = [[3,2],[26,2],[3,20],[27,20]].filter(()=>rnd()<0.75);
  for(const [x,y] of spots){
    let free = true;
    for(let yy=y; yy<y+4; yy++) for(let xx=x-1; xx<x+5; xx++) if(tiles[yy][xx]!=='.') free = false;
    if(free) b.push(house(x, y));
  }
  for(const h of b) if(h.kind==='house') h.resident = TOWNSFOLK[Math.floor(rnd()*TOWNSFOLK.length)];
  const npcs = [[14,8],[5,10]].map(([x,y])=>({kind:TOWNSFOLK[Math.floor(rnd()*TOWNSFOLK.length)], x:P(x), y:y+OY, facing:'down', wander:true,
    lines:TOWN_TALK[Math.floor(rnd()*TOWN_TALK.length)], home:{x:P(x), y:y+OY}}));
  npcs.push({kind:TOWNSFOLK[Math.floor(rnd()*TOWNSFOLK.length)], x:OX+24, y:OY+15, facing:'left', wander:true, lines:TOWN_TALK[Math.floor(rnd()*TOWN_TALK.length)], home:{x:OX+24, y:OY+15}});
  const map = finishMap(tiles, b, npcs, exits, {x:P(5), y:7+OY}, signs);
  // Trees and flowers in the outskirts, kept two tiles clear of roads, buildings and people, and
  // undone if they'd cut anything off.
  const near = (x,y)=>{
    for(let yy=y-2; yy<=y+2; yy++) for(let xx=x-2; xx<=x+2; xx++){ const ch = tiles[yy] && tiles[yy][xx]; if(ch===':' || ch==='B' || ch==='D' || ch==='S') return true; }
    return npcs.some(n=>Math.abs(n.x-x)<=2 && Math.abs(n.y-y)<=2);
  };
  const placed = [];
  // Each town's outskirts lean toward its Gym's type: rocky (fire/ground), woods (ghost), a strip of
  // sea shore along the south (water); home and the rest get trees and flowers.
  const flavor = (GYM_STYLE[loc.leaderName] || {kind:''}).kind.replace('leader', '').toLowerCase();
  const PALETTE = {fire:'rrrTb', ground:'rrTTb*', ghost:'TTTTb', water:'**bT'}[flavor] || 'TTTT**b';
  const tries = flavor==='ghost' ? 120 : 70;
  if(flavor==='water') for(let y=H-5; y<H-2; y++) for(let x=2; x<W-2; x++){
    if(x>=LANE-1 && x<=LANE+3) continue;   // leave the south lane
    if(tiles[y][x]==='.' && !near(x,y)){ tiles[y][x] = '~'; placed.push([x,y]); }
  }
  for(let i=0; i<tries; i++){
    const x = 2+Math.floor(rnd()*(W-4)), y = 2+Math.floor(rnd()*(H-4));
    const inCentre = x>=OX-1 && x<=OX+22 && y>=OY-1 && y<=OY+14;
    if(inCentre || tiles[y][x]!=='.' || near(x,y)) continue;
    tiles[y][x] = PALETTE[Math.floor(rnd()*PALETTE.length)]; placed.push([x,y]);
  }
  const seen = new Set([map.spawn.y*W+map.spawn.x]), todo = [[map.spawn.x, map.spawn.y]];
  while(todo.length){ const [x,y] = todo.pop();
    for(const [dx,dy] of Object.values(DIRS)){ const nx=x+dx, ny=y+dy, ch=tiles[ny]&&tiles[ny][nx];
      if(WALKABLE.has(ch) && !seen.has(ny*W+nx)){ seen.add(ny*W+nx); todo.push([nx,ny]); } } }
  const cut = b.some(h=>!seen.has((h.door.y+1)*W+h.door.x)) || exits.some(e=>!seen.has(e.y*W+e.x) && !seen.has((e.y+1)*W+e.x) && !seen.has(e.y*W+e.x+1));
  if(cut) for(const [x,y] of placed) tiles[y][x] = '.';
  return map;
}
const ROUTE_THEMES = {
  plain: {grass:6, trees:22, rocks:0.12, ponds:[[3,2]]},
  forest:{grass:7, trees:55, rocks:0.05, ponds:[]},
  lake:  {grass:4, trees:16, rocks:0.1,  ponds:[[7,4],[3,2]]},
  rocky: {grass:4, trees:10, rocks:0.55, ponds:[]}};
// Routes are long corridors like Emerald's: about 125×42 running east–west, 42×125 north–south,
// or ~80×68 where they turn or branch (seeded ±10% per route). A two-tree-thick border with an
// opening per exit, a two-wide path that meanders in L-shaped legs from each opening to the middle,
// then ponds, tree clusters, tall grass, scenery, signs, ledges, items and locals, all scaled with area.
function routeOpen(W, H){
  const cx = Math.floor(W/2), cy = Math.floor(H/2);
  return {left:{x:0,y:cy-2,len:4}, right:{x:W-1,y:cy-2,len:4}, up:{x:cx-1,y:0,len:3}, down:{x:cx-1,y:H-1,len:3}};
}
function buildRoute(loc){
  const rnd = seedRand(loc.name);
  const th = ROUTE_THEMES[loc.theme||'plain'];
  const dirs = loc.links.map(l=>l.dir);
  const horiz = dirs.some(d=>d==='left'||d==='right'), vert = dirs.some(d=>d==='up'||d==='down');
  const vary = n=>Math.round(n*(0.9 + rnd()*0.2));
  const W = horiz && !vert ? vary(125) : vert && !horiz ? vary(42) : vary(80);
  const H = horiz && !vert ? vary(42) : vert && !horiz ? vary(125) : vary(68);
  const cx = Math.floor(W/2), cy = Math.floor(H/2), A = W*H, scale = A/416;   // 416 = the old 26×16 route
  const inside = (x,y)=> x>=2 && y>=2 && x<W-2 && y<H-2;
  const tiles = Array.from({length:H}, (_,y)=>Array.from({length:W}, (_,x)=> inside(x,y) ? '.' : 'T'));
  const OPEN = routeOpen(W, H);
  const spots = {left:{x:0,y:cy}, right:{x:W-1,y:cy}, up:{x:cx,y:0}, down:{x:cx,y:H-1}};
  const clamp = (v,a,b)=>Math.max(a, Math.min(b, v));
  // Path from an opening to the middle, through waypoints every ~14 tiles that swing to the side.
  const exits = [], paths = {};
  for(const l of loc.links){
    const sp = spots[l.dir], lr = l.dir==='left' || l.dir==='right';
    carveOpening(tiles, l.dir, OPEN[l.dir], ',');
    let x = sp.x + STEP_IN[l.dir][0], y = sp.y + STEP_IN[l.dir][1];
    tiles[sp.y][sp.x] = ':';
    const along = lr ? Math.abs(cx-x) : Math.abs(cy-y), legs = Math.max(1, Math.round(along/14));
    const way = [];
    for(let i=1;i<legs;i++){
      const t = i/legs;
      let wx = Math.round(x+(cx-x)*t), wy = Math.round(y+(cy-y)*t);
      if(lr) wy = clamp(wy + Math.round((rnd()*2-1)*(H/2-7)), 4, H-6); else wx = clamp(wx + Math.round((rnd()*2-1)*(W/2-7)), 4, W-6);
      way.push([wx,wy]);
    }
    way.push([cx,cy]);
    const trail = [];
    const put = (px,py)=>{ trail.push({x:px,y:py}); for(const [ax,ay] of [[0,0],[1,0],[0,1],[1,1]]) if(inside(px+ax,py+ay) || tiles[py+ay] && tiles[py+ay][px+ax]===',') tiles[py+ay][px+ax] = ':'; };
    for(const [wx,wy] of way){
      // An L: part of the way along the route, across to the waypoint's line, then the rest.
      const mid = lr ? Math.round(x + (wx-x)*rnd()) : Math.round(y + (wy-y)*rnd());
      if(lr){ while(x!==mid){ put(x,y); x += Math.sign(mid-x); } while(y!==wy){ put(x,y); y += Math.sign(wy-y); } while(x!==wx){ put(x,y); x += Math.sign(wx-x); } }
      else { while(y!==mid){ put(x,y); y += Math.sign(mid-y); } while(x!==wx){ put(x,y); x += Math.sign(wx-x); } while(y!==wy){ put(x,y); y += Math.sign(wy-y); } }
    }
    put(cx,cy);
    paths[l.dir] = trail;
    exits.push({...OPEN[l.dir], ...l});
  }
  const open = (x,y)=> tiles[y] && tiles[y][x]==='.';
  const rect = (w,h)=>[2+Math.floor(rnd()*(W-4-w)), 2+Math.floor(rnd()*(H-4-h))];
  // Ponds (need clear ground plus a one-tile margin).
  for(let k=0; k<Math.max(1, Math.round(scale/3)); k++) for(const [pw0,ph0] of th.ponds){
    const pw = pw0 + Math.floor(rnd()*3), ph = ph0 + Math.floor(rnd()*2);
    for(let tries=0; tries<30; tries++){
      const [x0,y0] = rect(pw,ph);
      let ok = true;
      for(let yy=y0-1; yy<=y0+ph && ok; yy++) for(let xx=x0-1; xx<=x0+pw; xx++) if(!open(xx,yy)){ ok=false; break; }
      if(ok){ for(let yy=y0; yy<y0+ph; yy++) for(let xx=x0; xx<x0+pw; xx++) tiles[yy][xx] = '~'; break; }
    }
  }
  // Tree clusters: little woods that give a big route some shape.
  for(let k=0; k<Math.round(A/260*(th.trees/22)); k++){
    const ox = 3+Math.floor(rnd()*(W-6)), oy = 3+Math.floor(rnd()*(H-6)), r = 1.5 + rnd()*3;
    for(let yy=Math.floor(oy-r); yy<=oy+r; yy++) for(let xx=Math.floor(ox-r); xx<=ox+r; xx++)
      if(open(xx,yy) && (xx-ox)*(xx-ox)+(yy-oy)*(yy-oy) <= r*r && rnd()<0.85) tiles[yy][xx] = 'T';
  }
  // Tall grass patches, bigger than before.
  const grassPatches = Math.round((loc.type==='trainer' ? 2 : th.grass) * scale * 0.7);
  for(let i=0;i<grassPatches;i++){
    const w = 4 + Math.floor(rnd()*7), h = 3 + Math.floor(rnd()*5);
    const [x0,y0] = rect(w,h);
    for(let yy=y0; yy<y0+h; yy++) for(let xx=x0; xx<x0+w; xx++) if(open(xx,yy)) tiles[yy][xx] = '"';
  }
  // Scattered scenery (only ever on plain grass, so paths stay open end to end).
  for(let i=0;i<th.trees*scale*0.6;i++){
    const x = 2+Math.floor(rnd()*(W-4)), y = 2+Math.floor(rnd()*(H-4));
    if(!open(x,y)) continue;
    const r = rnd();
    tiles[y][x] = r<th.rocks ? 'r' : r<th.rocks+0.45 ? 'T' : r<th.rocks+0.65 ? 'b' : '*';
  }
  // Nothing below may wall off part of the map: `fits` tries a change and keeps it only if every
  // tile you could walk to before (bar the ones it covers) can still be walked to, both ways.
  const reachSet = ()=>{
    const seen = new Set([cy*W+cx]), todo = [[cx,cy]];
    while(todo.length){ const [x,y] = todo.pop();
      for(const [dx,dy] of Object.values(DIRS)){ const nx=x+dx, ny=y+dy, ch=tiles[ny]&&tiles[ny][nx];
        if((WALKABLE.has(ch) || ch===',') && !seen.has(ny*W+nx)){ seen.add(ny*W+nx); todo.push([nx,ny]); } } }
    return seen;
  };
  const fits = (cells, ch)=>{
    const before = reachSet().size, old = cells.map(([x,y])=>tiles[y][x]);
    cells.forEach(([x,y])=>{ tiles[y][x] = ch; });
    if(reachSet().size === before - old.filter(o=>WALKABLE.has(o)).length) return true;
    cells.forEach(([x,y],i)=>{ tiles[y][x] = old[i]; });
    return false;
  };
  // A route sign just inside each entrance, beside the path: "ROUTE 1 / → NEXT PLACE" (Emerald style).
  const ARROW = {left:'←', right:'→', up:'↑', down:'↓'}, signs = [];
  for(const l of loc.links){
    const [ix,iy] = STEP_IN[l.dir], sp = spots[l.dir], lr = l.dir==='left' || l.dir==='right';
    for(const side of [-1,1,-2,2]){
      const x = sp.x + ix*3 + (lr?0:side), y = sp.y + iy*3 + (lr?side:0), ch = tiles[y] && tiles[y][x];
      if(!ch || ch===':' || ch===',' || ch==='L' || !fits([[x,y]], 'N')) continue;
      signs.push({x, y, route:true, text:`${loc.name.toUpperCase()}\n${ARROW[l.dir]} ${LOCATIONS[l.to].name.toUpperCase()}`});
      break;
    }
  }
  // Ledges: short one-way drops on open grass (south-facing, and some east/west), never across the
  // path and never where the drop would strand you.
  const southLedges = Math.max(1, Math.round(A/450));
  for(let tries=0, placed=0; tries<southLedges*25 && placed<southLedges; tries++){
    const len = 3 + Math.floor(rnd()*6), x0 = 2 + Math.floor(rnd()*(W-4-len)), y = 3 + Math.floor(rnd()*(H-7));
    const cells = [];
    for(let x=x0; x<x0+len; x++) if(tiles[y][x]==='.' && WALKABLE.has(tiles[y-1][x]) && WALKABLE.has(tiles[y+1][x])) cells.push([x,y]);
    if(cells.length===len && fits(cells, 'L')) placed++;
  }
  const sideLedges = Math.round(A/1400);
  for(let tries=0, placed=0; tries<sideLedges*25 && placed<sideLedges; tries++){
    const len = 3 + Math.floor(rnd()*4), x = 3 + Math.floor(rnd()*(W-6)), y0 = 2 + Math.floor(rnd()*(H-4-len)), ch = rnd()<0.5 ? '>' : '<';
    const cells = [];
    for(let y=y0; y<y0+len; y++) if(tiles[y][x]==='.' && WALKABLE.has(tiles[y][x-1]) && WALKABLE.has(tiles[y][x+1])) cells.push([x,y]);
    if(cells.length===len && fits(cells, ch)) placed++;
  }
  // Spots someone can stand on or something can lie on: plain grass you can walk to.
  const reach = reachSet();
  const freeSpot = ()=>{
    for(let tries=0; tries<200; tries++){
      const x = 3+Math.floor(rnd()*(W-6)), y = 3+Math.floor(rnd()*(H-6));
      if(tiles[y][x]==='.' && reach.has(y*W+x) && !npcs.some(n=>Math.abs(n.x-x)+Math.abs(n.y-y)<4)) return {x,y};
    }
    return null;
  };
  // The rival stands partway down the path to the gated exit, watching back toward the middle.
  const npcs = [];
  const gated = loc.links.find(l=>l.gate);
  if(loc.type==='trainer' && gated){
    const trail = paths[gated.dir], spot = trail[Math.floor(trail.length*0.45)];
    npcs.push({kind:'rival', x:spot.x, y:spot.y, facing:OPPOSITE[gated.dir], trainer:true, vanish:true});
  }
  // Locals with tips, spread along the route.
  const who = {forest:'bugcatcher', lake:'fisher', rocky:'hiker'}[loc.theme] || 'youngster';
  const locals = ['youngster', 'lass', who, 'gentleman', 'girl'];
  for(let k=0; k<Math.max(1, Math.round(A/1100)); k++){
    const sp = freeSpot(); if(!sp) break;
    const kind = locals[k % locals.length];
    npcs.push({kind, x:sp.x, y:sp.y, facing:'down', lines:ROUTE_TALK[kind] || TOWN_TALK[k % TOWN_TALK.length], home:{...sp}, wander:k%2===1});
  }
  // Trainers: a few per route, 3–5 tiles off the path, facing it, so walking the path gets you spotted.
  const TR_CLASSES = {plain:['YOUNGSTER','LASS','BUG CATCHER'], forest:['BUG CATCHER','LASS','YOUNGSTER'], lake:['FISHERMAN','LASS','YOUNGSTER'], rocky:['HIKER','YOUNGSTER','LASS']}[loc.theme||'plain'];
  const TR_KIND = {YOUNGSTER:'youngster', LASS:'lass', 'BUG CATCHER':'bugcatcher', HIKER:'hiker', FISHERMAN:'fisher'};
  // Lasses are girls; the other classes here are boys, as in Emerald.
  const TR_NAMES = {girl:['CALLIE','TIANA','DANA','OLIVIA','KAREN','ROSA','NINA','IVY','JUNE'], boy:['JOEY','BEN','RICK','ALLEN','MIKE','TOBY','LUKE','GREG','OWEN','SAM']};
  const TR_INTRO = ["Our eyes met! That means we battle!", "Hey! You look tough. Let's see!", "I just caught these guys. Try them out!", "You're not getting past without a battle!", "My Pokémon and I trained all day for this!"];
  const TR_AFTER = ["I'll train harder, I promise.", "You're really strong! Good luck out there.", "Losing is part of training, right?", "Next time, I'll win for sure!"];
  const pool = areaPool(loc), trails = Object.values(paths);
  for(let k=0, made=0; k<200 && made<Math.max(2, Math.round(A/1300)); k++){
    const trail = trails[Math.floor(rnd()*trails.length)];
    if(trail.length<12) continue;
    const p = trail[6 + Math.floor(rnd()*(trail.length-10))], d = ['up','down','left','right'][Math.floor(rnd()*4)];
    const dist = 3 + Math.floor(rnd()*3), [dx,dy] = DIRS[d], x = p.x+dx*dist, y = p.y+dy*dist;
    if(!tiles[y] || tiles[y][x]!=='.' || !reach.has(y*W+x) || npcs.some(n=>Math.abs(n.x-x)+Math.abs(n.y-y)<6)) continue;
    let clear = true;   // an open line of sight from them back to the path
    for(let s=1; s<dist; s++) if(!WALKABLE.has(tiles[y-dy*s][x-dx*s])) clear = false;
    if(!clear) continue;
    const cls = TR_CLASSES[Math.floor(rnd()*TR_CLASSES.length)];
    const size = 2 + Math.floor(rnd()*4), team = Array.from({length:size}, ()=>pool[Math.floor(rnd()*pool.length)]);
    npcs.push({kind:TR_KIND[cls], x, y, facing:OPPOSITE[d], trainer:true, id:`${loc.name}#${made}`, title:`${cls} ${(names=>names[Math.floor(rnd()*names.length)])(TR_NAMES[cls==='LASS' ? 'girl' : 'boy'])}`,
      team, intro:TR_INTRO[Math.floor(rnd()*TR_INTRO.length)], after:TR_AFTER[Math.floor(rnd()*TR_AFTER.length)], home:{x,y}});
    made++;
  }
  // Item balls lying in the grass (A picks them up, once).
  const items = [];
  for(let k=0; k<Math.max(1, Math.round(A/1300)); k++){
    const sp = freeSpot(); if(!sp) break;
    tiles[sp.y][sp.x] = 'I'; items.push(sp);
  }
  for(const row of tiles) row.forEach((ch,x)=>{ if(ch===',') row[x] = '.'; });   // openings: plain grass again
  const first = spots[exits[0].dir];
  return finishMap(tiles, [], npcs, exits, {x:first.x+STEP_IN[exits[0].dir][0]*2, y:first.y+STEP_IN[exits[0].dir][1]*2}, signs);
}
function finishMap(tiles, buildings, npcs, exits, spawn, signs){
  for(const b of buildings){
    for(let y=b.y; y<b.y+b.h; y++) for(let x=b.x; x<b.x+b.w; x++) tiles[y][x] = 'B';
    tiles[b.door.y][b.door.x] = 'D';
    if(tiles[b.door.y+1][b.door.x]==='.') tiles[b.door.y+1][b.door.x] = ':';
  }
  return {w:tiles[0].length, h:tiles.length, tiles, buildings, npcs, exits, signs, spawn};
}
function getMap(loc){
  if(!loc.__map) loc.__map = (loc.type==='route'||loc.type==='trainer') ? buildRoute(loc) : buildTown(loc);
  // Item balls you've already picked up stay gone (adv.picked holds "Area@x,y").
  if(typeof adv!=='undefined' && adv && adv.picked) for(const k in adv.picked){
    if(!k.startsWith(loc.name+'@')) continue;
    const [x,y] = k.slice(loc.name.length+1).split(',').map(Number), row = loc.__map.tiles[y];
    if(row && row[x]==='I') row[x] = '.';
  }
  return loc.__map;
}

// ---------- Interiors ----------
// Rooms you walk into through a building's door. Tiles: # wall · _ floor · o rug · M exit mat ·
// c counter (you can talk across it) · h healing machine · P PC · k bookshelf · v TV · e bed ·
// t table · p plant · s shelf · u statue. Rooms float on black, like the GBA.
const ROOMS = {
  center:['##############','######n###m###','#k___h______P#','#_CccccccC___#','#____oooo____#','#qQ_______qgQ#','#p__________p#','#____________#','######MM######'],
  mart:  ['###########','##ww#K#ww##','#_ss_ss_ss#','#_________#','#_ss_ss_p_#','#cc_______#','#_c_______#','#####M#####'],
  house: ['#########','###w##w##','#kv___ep#','#_____e_#','#__tt___#','#_______#','####M####'],
  gym:   ['###############','###############','#u___________u#','#_____________#','#xxxxx_xxxxxxx#','#_____________#','#_xxxxxxxxxx__#',
          '#_____________#','#xxxxxxx_xxxxx#','#_____________#','#__xxxxxxxxxxx#','#_____________#','#u_____o_____u#','#######M#######']};
// Junior trainers per gym type (Emerald-like classes) and the Pokémon they use.
const GYM_JUNIORS = {
  fire:  {cls:'KINDLER',    kind:'boy',    team:['Vulpix','Ponyta','Slugma','Growlithe','Numel']},
  water: {cls:'SWIMMER',    kind:'lass',   team:['Horsea','Goldeen','Staryu','Psyduck','Poliwag','Marill']},
  ground:{cls:'CAMPER',     kind:'hiker',  team:['Diglett','Sandshrew','Geodude','Phanpy','Cubone']},
  ghost: {cls:'HEX MANIAC', kind:'oldwoman', team:['Gastly','Shuppet','Duskull','Misdreavus','Abra','Natu']}};
const GYM_STYLE = {Rell:{kind:'leaderFire', type:'Fire'}, Sable:{kind:'leaderWater', type:'Water'}, Orin:{kind:'leaderGround', type:'Ground and Poison'}, Iska:{kind:'leaderGhost', type:'Ghost and Psychic'}};
function getInterior(loc, bi){
  loc.__rooms ||= {};
  if(loc.__rooms[bi]) return loc.__rooms[bi];
  const b = getMap(loc).buildings[bi];
  // Gym walls: pools in the Water gym, rock or boulders elsewhere (styled by theme).
  const theme = b.kind==='gym' ? (GYM_STYLE[loc.leaderName] || {kind:'leaderFire'}).kind.replace('leader', '').toLowerCase() : null;
  const tiles = ROOMS[b.kind].map(r=>r.split('').map(ch=>ch==='x' && theme==='water' ? '~' : ch));
  const h = tiles.length, w = tiles[0].length, matX = tiles[h-1].indexOf('M');
  const npcs = [];
  if(b.kind==='center') npcs.push({kind:'nurse', x:7, y:2, facing:'down', role:'nurse'});
  if(b.kind==='mart') npcs.push({kind:'clerk', x:1, y:6, facing:'right', role:'clerk'},
    {kind:'girl', x:7, y:3, facing:'up', wander:true, lines:["Poké Balls are the one thing you can never have too many of."]});
  if(b.kind==='house') npcs.push({kind:b.resident || 'oldwoman', x:3, y:5, facing:'down', wander:true, lines:b.lines});
  if(b.kind==='gym'){
    const g = GYM_STYLE[loc.leaderName] || {kind:'leaderFire', type:'tough'};
    npcs.push({kind:g.kind, x:7, y:2, facing:'down', trainer:true, gymLeader:true},
      {kind:'gentleman', x:9, y:12, facing:'left', lines:[`Hey, challenger! ${loc.leaderName} uses ${g.type}-type Pokémon.`, "Bring Pokémon with the right type advantage and you'll be fine!"]});
    // Two juniors whose lines of sight cross the way up: one watching row 9 from the right, one row 5 from the left.
    const J = GYM_JUNIORS[theme] || GYM_JUNIORS.fire, pick = (k,n)=>Array.from({length:n}, (_,i)=>J.team[(k*2+i) % J.team.length]);
    const names = J.kind==='lass' || J.kind==='oldwoman' ? ['MAYA','LENA'] : ['TOMMY','CARL'];
    [[12,9,'left'],[2,5,'right']].forEach(([x,y,facing],k)=>npcs.push({kind:J.kind, x, y, facing, trainer:true, id:`${loc.name}#gym${k}`,
      title:`${J.cls} ${names[k]}`, team:pick(k, k+3), intro:`${loc.leaderName} is the best! You'll have to get past me first!`,
      after:`${loc.leaderName} is waiting at the top. Good luck!`}));
  }
  for(const n of npcs) n.home = {x:n.x, y:n.y};
  return loc.__rooms[bi] = {w, h, tiles, buildings:[], npcs, exits:[], signs:[], interior:b.kind, theme, spawn:{x:matX, y:h-2}, mat:{x:matX, y:h-1}};
}
function tileAt(map,x,y){ return (y<0||y>=map.h||x<0||x>=map.w) ? (map.interior ? '#' : 'T') : map.tiles[y][x]; }
// Map connections, like Emerald's: each linked area sits against this map's edge, offset so the two
// openings line up. They're drawn around you and you simply walk across — no fade.
function neighbours(li){
  const loc = LOCATIONS[li];
  if(loc.__nb) return loc.__nb;
  const map = getMap(loc);
  return loc.__nb = map.exits.map(e=>{
    const m = getMap(LOCATIONS[e.to]), back = m.exits.find(b=>b.to===li);
    const ox = e.dir==='right' ? map.w : e.dir==='left' ? -m.w : e.x - back.x;
    const oy = e.dir==='down' ? map.h : e.dir==='up' ? -m.h : e.y - back.y;
    return {exit:e, map:m, ox, oy};
  });
}
// The neighbour covering (x,y) in this map's coordinates, if any.
function nbAt(x, y){
  return neighbours(adv.loc).find(n=>x>=n.ox && y>=n.oy && x<n.ox+n.map.w && y<n.oy+n.map.h);
}
function inMap(map, x, y){ return x>=0 && y>=0 && x<map.w && y<map.h; }
// Tile at (x,y), looking into the connected areas when it's past this map's edge.
function worldTile(x, y){
  const map = curMap();
  if(map.interior || inMap(map, x, y)) return tileAt(map, x, y);
  const n = nbAt(x, y);
  return n ? tileAt(n.map, x-n.ox, y-n.oy) : 'T';
}
// The map you're on right now: the area, or the room you're inside.
function curMap(){ const loc = LOCATIONS[adv.loc]; return adv.inside!=null ? getInterior(loc, adv.inside) : getMap(loc); }
// People on the current map. Beaten rivals leave; everyone else stays put (Gym Leaders included).
function curNpcs(){ const loc = LOCATIONS[adv.loc]; return curMap().npcs.filter(n=>!(n.vanish && adv.cleared[loc.name])); }
function activeTrainer(n){ return n.trainer && !adv.cleared[n.id || LOCATIONS[adv.loc].name]; }
function spawnPlayer(){ adv.inside = null; const map = getMap(LOCATIONS[adv.loc]); adv.pos = {...map.spawn}; adv.facing = 'down'; }

// ---------- Pixel-art characters: everyone is on the player's 16×21 Gen 3 frame ----------
// Gen 3 NPCs stand as tall as the player, so each character is a 12-row head on the player's own body
// and walk frames (rows 12–20 of playerArt, see below), recoloured by a palette:
// K outline · R hat · W white · S skin · e eyes · H hair · B top · D legs (set D = B for a dress) · Y bag/trim.
const HEADS = {"cap":{"down":["................",".....KKKKKK.....","...KKRRRRRRKK...","..KRRRWWRRRRRK..","..KRRRRRRRRRRK..",".KKKKKKKKKKKKKK.","..KHSSSSSSSSHK..","..KSSSSSSSSSSK..",".KHKSSSSSSSSKHK.",".KHKSSeSSeSSKHK.","..KgSSeSSeSSgK..","...KjSSSSSSjK..."],
  "up":["................",".....KKKKKK.....","...KKRRRRRRKK...","..KRRRRRRRRRRK..","..KRRRRRRRRRRK..","..KRRRRRRRRRRK..","..KHHHHHHHHHHK..","..KHHHHHHHHHHK..",".KSKHHHHHHHHKSK.",".KSKHHHHHHHHKSK.","..KKHHHHHHHHKK..","...KHHHHHHHHK..."],
  "left":["................",".....KKKKKK.....","....KRRRRRRKK...","...KRRWWRRRRRK..","...KRRRRRRRRRK..","KKKKKKKKKRRRRK..","..KSSSSSHHHHK...","..KSSSSSSHHHK...","..KSSeSSSSHHK...","..KSSeSSSSHK....","..KgSSSSSSK.....","...KjSSSSjK....."]},"short":{"down":["................",".....KKKKKK.....","...KKHHHHHHKK...","..KHHHHHHHHHHK..","..KHHHHHHHHHHK..","..KHHHHHHHHHHK..","..KHHSSSSSSHHK..","..KHSSSSSSSSHK..",".KHKSSSSSSSSKHK.",".KHKSSeSSeSSKHK.","..KgSSeSSeSSgK..","...KjSSSSSSjK..."],
  "up":["................",".....KKKKKK.....","...KKHHHHHHKK...","..KHHHHHHHHHHK..","..KHHHHHHHHHHK..","..KHHHHHHHHHHK..","..KHHHHHHHHHHK..","..KHHHHHHHHHHK..",".KSKHHHHHHHHKSK.",".KSKHHHHHHHHKSK.","..KKHHHHHHHHKK..","...KHHHHHHHHK..."],
  "left":["................",".....KKKKKK.....","....KHHHHHHKK...","...KHHHHHHHHHK..","..KHHHHHHHHHHK..","..KSHHHHHHHHHK..","..KSSSSSHHHHHK..","..KSSSSSSHHHK...","..KSSeSSSSHHK...","..KSSeSSSSHK....","..KgSSSSSSK.....","...KjSSSSjK....."]},"long":{"down":["................",".....KKKKKK.....","...KKHHHHHHKK...","..KHHHHHHHHHHK..","..KHHHHHHHHHHK..",".KHHHHHHHHHHHHK.",".KHHHSSSSSSHHHK.",".KHHSSSSSSSSHHK.",".KHKSSSSSSSSKHK.",".KHKSSeSSeSSKHK.",".KHgSSeSSeSSgHK.",".KHKjSSSSSSjKHK."],
  "up":["................",".....KKKKKK.....","...KKHHHHHHKK...","..KHHHHHHHHHHK..","..KHHHHHHHHHHK..",".KHHHHHHHHHHHHK.",".KHHHHHHHHHHHHK.",".KHHHHHHHHHHHHK.",".KHHHHHHHHHHHHK.",".KHHHHHHHHHHHHK.","..KHHHHHHHHHHK..",".KHHHHHHHHHHHHK."],
  "left":["................",".....KKKKKK.....","....KHHHHHHKK...","...KHHHHHHHHHK..","..KHHHHHHHHHHK..","..KSHHHHHHHHHK..","..KSSSSSHHHHHK..","..KSSSSSSHHHHK..","..KSSeSSSSHHHK..","..KSSeSSSSHHHK..","..KgSSSSSSHHHK..","...KjSSSSjKHHK.."]},"bald":{"down":["................","......KKKK......","....KKSSSSKK....","...KSSSSSSSSK...","..KSSSSSSSSSSK..","..KSSSSSSSSSSK..","..KHSSSSSSSSHK..","..KHSSSSSSSSHK..",".KHKSSSSSSSSKHK.",".KHKSSeSSeSSKHK.","..KgSSeSSeSSgK..","...KjSWWWWSjK..."],
  "up":["................","......KKKK......","....KKSSSSKK....","...KSSSSSSSSK...","..KSSSSSSSSSSK..","..KSSSSSSSSSSK..","..KSSSSSSSSSSK..","..KHSSSSSSSSHK..",".KSKHHHHHHHHKSK.",".KSKHHHHHHHHKSK.","..KKHHHHHHHHKK..","...KHHHHHHHHK..."],
  "left":["................","......KKKK......","....KKSSSSK.....","...KSSSSSSSK....","..KSSSSSSSSSK...","..KSSSSSSSSHK...","..KSSSSSSSHHK...","..KSSSSSSSHHK...","..KSSeSSSSHK....","..KSSeSSSHHK....","..KWWSSSSSHK....","...KjSSSSjK....."]},"nurse":{"down":["................",".....KKKKKK.....","...KKWWWWWWKK...","..KWWWWRRWWWWK..","..KHWWWWWWWWHK..",".KHHHHHHHHHHHHK.",".KHHHSSSSSSHHHK.",".KHHSSSSSSSSHHK.",".KHKSSSSSSSSKHK.",".KHKSSeSSeSSKHK.",".KHgSSeSSeSSgHK.",".KHKjSSSSSSjKHK."],
  "up":["................",".....KKKKKK.....","...KKWWWWWWKK...","..KWWWWWWWWWWK..","..KHWWWWWWWWHK..",".KHHHHHHHHHHHHK.",".KHHHHHHHHHHHHK.",".KHHHHHHHHHHHHK.",".KHHHHHHHHHHHHK.",".KHHHHHHHHHHHHK.","..KHHHHHHHHHHK..",".KHHHHHHHHHHHHK."],
  "left":["................",".....KKKKKK.....","....KWWWWWWKK...","...KWWWRRWWWWK..","..KHWWWWWWWWWK..","..KSHHHHHHHHHK..","..KSSSSSHHHHHK..","..KSSSSSSHHHHK..","..KSSeSSSSHHHK..","..KSSeSSSSHHHK..","..KgSSSSSSHHHK..","...KjSSSSjKHHK.."]}};
// The player's body letters → NPC palette letters (lowercase = the darker shade).
const BODY_MAP = {k:'K', D:'B', d:'b', W:'W', T:'Y', t:'y', S:'S', G:'D', q:'q', R:'z'};
const shade = (hex, f) => '#'+[1,3,5].map(i=>Math.round(parseInt(hex.substr(i,2),16)*f).toString(16).padStart(2,'0')).join('');
// The cast (loosely following the classic trainer classes); each is a head + palette.
const CHARS = {
  player:   {head:'cap',   K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#f8c8a0',H:'#583828',B:'#3868d0',D:'#404860',Y:'#f0c040'},
  rival:    {head:'cap',   K:'#282830',R:'#40a060',W:'#f8f8f8',S:'#f8c8a0',H:'#e0a040',B:'#6848a8',D:'#303038',Y:'#a8a8b8'},
  youngster:{head:'cap',   K:'#282830',R:'#3878d8',W:'#f8f8f8',S:'#f8c8a0',H:'#402818',B:'#f0c030',D:'#3050a0',Y:'#f0c030'},
  bugcatcher:{head:'cap',  K:'#282830',R:'#e8d070',W:'#f8f8f8',S:'#f8c8a0',H:'#402818',B:'#78b848',D:'#c0a060',Y:'#78b848'},
  hiker:    {head:'cap',   K:'#282830',R:'#a06830',W:'#e8d8b0',S:'#e8b080',H:'#302018',B:'#c07038',D:'#584030',Y:'#806040'},
  fisher:   {head:'cap',   K:'#282830',R:'#d8d8c8',W:'#f8f8f8',S:'#e8b080',H:'#302018',B:'#4898a8',D:'#305060',Y:'#c8a060'},
  lass:     {head:'long',  K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#f8d0b0',H:'#c86828',B:'#e05878',D:'#e05878',Y:'#e05878'},
  girl:     {head:'long',  K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#f8d0b0',H:'#f0c840',B:'#58b068',D:'#58b068',Y:'#58b068'},
  boy:      {head:'short', K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#f8c8a0',H:'#302018',B:'#e06838',D:'#404860',Y:'#e06838'},
  gentleman:{head:'short', K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#f8c8a0',H:'#a0a0a8',B:'#484858',D:'#383840',Y:'#484858'},
  oldman:   {head:'bald',  K:'#282830',R:'#e04040',W:'#f0f0f0',S:'#f0c098',H:'#b8b8c0',B:'#907050',D:'#504030',Y:'#907050'},
  oldwoman: {head:'long',  K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#f0c098',H:'#c8c8d0',B:'#8870b0',D:'#8870b0',Y:'#8870b0'},
  nurse:    {head:'nurse', K:'#282830',R:'#e04848',W:'#f8f8f8',S:'#f8d0b0',H:'#f890b0',B:'#f8c0d0',D:'#f8c0d0',Y:'#f8c0d0'},
  prof:     {head:'short', K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#f8c8a0',H:'#6a5040',B:'#f0f0f0',D:'#5a4a38',Y:'#c8c8d0'},
  clerk:    {head:'short', K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#f8c8a0',H:'#302018',B:'#4878d8',D:'#30406a',Y:'#4878d8'},
  leaderFire:  {head:'short', K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#e8b080',H:'#e84828',B:'#303030',D:'#e84828',Y:'#303030'},
  leaderWater: {head:'long',  K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#f8d0b0',H:'#3888e0',B:'#f8f8f8',D:'#3888e0',Y:'#f8f8f8'},
  leaderGround:{head:'bald',  K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#d89868',H:'#604028',B:'#a07838',D:'#584028',Y:'#a07838'},
  leaderGhost: {head:'long',  K:'#282830',R:'#e04040',W:'#f8f8f8',S:'#f0e0e8',H:'#6848a8',B:'#383050',D:'#383050',Y:'#383050'}};
const charCache = {};
// The player: an original trainer on Gen 3's 16×21 overworld frame, following Brendan's row-by-row
// widths from the pokeemerald sheet: the head is widest at the eyes (14 px, hair tufts outside the face
// outline) and wider than the 12-px shoulders; the arms hang outside at 14 px; a 10-px leg block with
// two 3-px feet and a 2-px gap. High contrast: black outlines, near-black clothes, white hair, red.
// Ours: spiky silver hair, crimson headband (tail trails behind from the side), navy jacket with white
// collar and teal trim, teal pack, charcoal trousers, crimson shoes. Walk frames dip 1 px.
const PLAYER_PAL = {K:'#283860', a:'#f8f8f8', b:'#c0c8dc', k:'#080808', R:'#d02838', S:'#f8d0a8', g:'#e8a878', j:'#b87050',
  i:'#101c38', W:'#f0f0f0', D:'#1c2440', d:'#10162c', T:'#1f9a86', t:'#126050', G:'#3a3a48', q:'#24242e'};
const HAIR = ['..........K.....','.........KaK....','.....KKKKaaKK...','....KaabbbbbbK..','...KbaaaabbbK...'];
const PLAYER_TOP = {
  down:[...HAIR,'...kRRRRRRRRk...','..kbaSSSSSSabk..','..kaSSSSSSSSak..','.kbkSSSSSSSSkbk.','.kbkSSiSSiSSkbk.','..kgSSiSSiSSgk..',
        '...kjSSSSSSjk...','...kDkWWWWkDk...','..kDDDTWWTDDDk..','.kDkDDDTTDDDkDk.','.kSkDDDTTDDdkSk.','..kkddddddddkk..'],
  up:  [...HAIR,'...kRRRRRRRRk...','..kbaaaaaaaabk..','..kaaaaaaaaaak..','.kbaaaaaaaaaabk.','.kbbaaaaaaaabbk.','..kbbbaaaabbbk..',
        '...kbbbbbbbbk...','...kDkSSSSkDk...','..kDTTTTTTTTDk..','.kDkTTTTTTTTkDk.','.kSkTTttttTTkSk.','..kkttttttttkk..'],
  left:['..........K.....','.........KaK....','.....KKKKaaK....','...KKaabbbbbKK..','..KKaaaaaabbbbK.','..kRRRRRRRRkbbbK','..kaSSSSabbbkRk.',
        '..kSSSSSSabbkRRk','..kSSiSSSSabbk..','..kSSiSSSSbbk...','..kgSSSSSSbk....','...kjSSSSjk.....',
        '....kkWWDDk.....','....kWDDDDTTk...','....kDkDDDTTk...','....kDkDDdTTk...','....kSkdddTk....']};
const PLAYER_LEGS = {
  down:[['...kGGGGGGGGk...','...kGGGqqGGGk...','...kRRRkkRRRk...','....kkk..kkk....'],
        ['...kGGGGGGGGk...','...kGGGk.kRRRk..','...kRRRk..kkk...','....kkk.........']],
  left:[['.....kGGGGGk....','.....kGGGGqk....','....kRRRRRk.....','.....kkkkk......'],
        ['....kGGkkGGk....','...kGGk..kGGk...','..kRRk....kRRk..','..kkk......kkk..'],
        ['.....kGGGGk.....','....kGGkGGk.....','...kRRk.kRRk....','...kkk..kkk.....']]};
PLAYER_LEGS.up = PLAYER_LEGS.down;
// Step frames, following Brendan's walk (pokeemerald walking.png) row by row. Front/back: the hips
// shift and the stepping leg swings in under the body so one foot shows, centred and a row lower,
// while the near arm swings forward over the hips and the far one drops behind (only the pack shows). Side: a wide diagonal
// stride, back foot trailing low on one step, front foot landing low on the other; the arm swings just 1 px. (Frame 2 of the
// front/back walk is frame 1 mirrored; the whole step frame also dips 1 px, see charSvg.)
const PLAYER_STEP = {
  down:['...kDkWWWWkDkTk.','..kDkDDTTDDkkTk.','..kDDDDTTDDDdk..','..kDDkdddddk....','..kSSkGGGGGk....',
        '...kkkkkGGGk....','.....kGGGGGk....','.......kRRRk....','........kkk.....'],
  up:  ['..kDTTTTTTTk....','...kkTTTTTTDk...','...kDTttttTkk...','...kkttttttkDk..','...kGGGGGGGkSk..',
        '....kkGGGGkkD...','....kkkkkkk.....','....kRRRk.......','.....kkk........'],
  left1:['.....kDkDDTTk...','....kDkkDDdTTk..','...kSkDDkddTTk..','..kGkkGGkkkTk...','..kRGkkkkkkkGk..',
         '...kRRkkkkkGGk..','....kk...kRRk...','..........kk....'],
  left2:['.....kDDDDTTk...','....kkDkDDdTTk..','....kkDkdddTTk..','...kGkSkGGkTk...','..kGkGGkkkkkGk..',
         '..kRRkkkkkkRRk..','...kRRk....kk...','....kk..........']};
function playerArt(d, frame, run){
  let art = PLAYER_TOP[d].concat(PLAYER_LEGS[d][0]);
  if(frame && d==='left') art = art.slice(0, 13).concat(PLAYER_STEP['left'+frame]);
  else if(frame) art = art.slice(0, 12).concat(PLAYER_STEP[d]);
  if(run && d==='left') art = art.map((r,i)=>i<12 ? r.slice(1)+'.' : r);   // lean into the run
  return art;
}

// frame 0 = standing, 1/2 = alternating steps (2 is 1 mirrored for up/down, so the other foot leads).
// `run` leans the head forward from the side.
function charSvg(kind, dir, frame, run){
  const key = kind+dir+frame+(run?'r':'');
  if(charCache[key]) return charCache[key];
  const d = dir==='right' ? 'left' : dir;
  const flipP = (dir==='right') !== (frame===2 && (dir==='up'||dir==='down'));
  if(kind==='player'){
    let r = '';
    playerArt(d, frame, run).forEach((row,y)=>{ for(let x=0;x<16;x++){ const ch=row[x]; if(ch!=='.') r += `<rect x="${x}" y="${y}" width="1" height="1" fill="${PLAYER_PAL[ch]}"/>`; } });
    // 16×21 at 32×42, centred on the actor and 10 px up so the feet stay put; walk frames sit 2 px lower (the Gen 3 dip).
    return charCache[key] = `<svg viewBox="0 0 16 21" width="32" height="42" shape-rendering="crispEdges" style="margin:${frame ? -8 : -10}px 0 0 -4px;${flipP?'transform:scaleX(-1)':''}">${r}</svg>`;
  }
  const P = CHARS[kind] || CHARS.boy;
  if(!P.pal) P.pal = {...P, K:'#101010', e:'#101828', g:shade(P.S,.9), j:shade(P.S,.72), b:shade(P.B,.72), y:shade(P.Y,.72), q:shade(P.D,.8), z:shade(P.D,.5)};
  let head = HEADS[P.head][d];
  if(run && d==='left') head = head.map(r=>r.slice(1)+'.');   // lean into the run
  const art = head.concat(playerArt(d, frame, run).slice(12).map(r=>r.replace(/[^.]/g, ch=>BODY_MAP[ch])));
  let r = '';
  art.forEach((row,y)=>{ for(let x=0;x<16;x++){ const ch=row[x]; if(ch!=='.') r += `<rect x="${x}" y="${y}" width="1" height="1" fill="${P.pal[ch]}"/>`; } });
  return charCache[key] = `<svg viewBox="0 0 16 21" width="32" height="42" shape-rendering="crispEdges" style="margin:${frame ? -8 : -10}px 0 0 -4px;${flipP?'transform:scaleX(-1)':''}">${r}</svg>`;
}

// Tall grass in the DS-era style: one round, leafy clump per 16×16 tile (five shaded leaves with
// a teal shadow under it) on the mint ground, tiled square. Drawn once, shared as a CSS variable.
const GROUND = {base:'#48d890', dot:'#60e8a0', dark:'#38c880'};
function tallGrassSvg(){
  const C = {shadow:'#18a058', edge:'#1c7a10', dark:'#208800', mid:'#30a810', light:'#60c038', hi:'#98e060'};
  const px = [];
  for(let y=0;y<16;y++){ px.push([]); for(let x=0;x<16;x++) px[y].push(GROUND.base); }
  // Soft teal shadow under the clump.
  for(let y=0;y<16;y++) for(let x=0;x<16;x++){ const u=(x-7.5)/7.2, v=(y-11.5)/3.6; if(u*u+v*v<=1) px[y][x] = C.shadow; }
  // Leaves: [centre x, centre y, angle (radians, 0 = pointing right), half-length, half-width]; back ones first.
  const leaves = [[4.5,5,-2.3,4.3,2.5],[11,5,-0.85,4.3,2.5],[3.5,9,3.0,4,2.4],[12.5,9,0.15,4,2.4],[7.8,8,-1.57,4.4,2.7],[5.5,11.5,2.4,3.6,2.3],[10.5,11.5,0.75,3.6,2.3]];
  for(const [cx,cy,a,L,Wd] of leaves){
    const ca = Math.cos(a), sa = Math.sin(a);
    for(let y=0;y<16;y++) for(let x=0;x<16;x++){
      const dx = x+0.5-cx, dy = y+0.5-cy;
      const u = dx*ca + dy*sa, v = -dx*sa + dy*ca;           // along / across the leaf
      const r = (u/L)*(u/L) + (v/Wd)*(v/Wd);
      if(r>1) continue;
      // Dark rim, dark midrib, lit upper half (brightest near the middle), mid-green lower half.
      px[y][x] = r>0.62 ? C.edge : Math.abs(v)<0.55 ? C.dark : dy<0 ? (r<0.3 ? C.hi : C.light) : C.mid;
    }
  }
  let rects = '';
  px.forEach((row,y)=>row.forEach((c,x)=>{ rects += `<rect x="${x}" y="${y}" width="1" height="1" fill="${c}"/>`; }));
  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" shape-rendering="crispEdges">${rects}</svg>`;
}
if(typeof document!=='undefined' && document.documentElement && document.documentElement.style)
  document.documentElement.style.setProperty('--tallgrass', `url("data:image/svg+xml,${encodeURIComponent(tallGrassSvg())}")`);

// ---------- Buildings ----------
function buildingHtml(b, ox=0, oy=0, bi=null){
  const wins = [];
  for(let i=0;i<b.w;i++) if(b.x+i!==b.door.x) wins.push(`<div class="win" style="left:${i*T+7}px"></div>`);
  const roof = b.kind==='center' ? ['#e05050','#c03838'] : b.kind==='mart' ? ['#4878d8','#3060b8'] : b.kind==='gym' ? ['#708898','#586878'] : b.roof;
  const plate = b.kind==='center' ? '<div class="plate plate-center"><span class="pc-ball"></span>POKéMON</div>'
    : b.kind==='mart' ? '<div class="plate plate-mart">MART</div>'
    : b.kind==='gym' ? '<div class="plate plate-gym">GYM</div>' : '<div class="chimney"></div>';
  return `<div class="bld bld-${b.kind}"${bi!=null ? ` data-bi="${bi}"` : ''} style="left:${(b.x+ox)*T}px; top:${(b.y+oy)*T}px; width:${b.w*T}px; height:${b.h*T}px; --roof:${roof[0]}; --roof2:${roof[1]}">
    <div class="roof"></div><div class="wall">${wins.join('')}</div>${plate}
    <div class="door door-${b.kind}" style="left:${(b.door.x-b.x)*T+5}px"></div></div>`;
}

// ---------- Rendering + camera ----------
// Path and water are auto-tiled: each tile looks at its neighbours to draw a rim on open edges and
// round its outer corners, so areas read as smooth shapes instead of a grid.
const EDGE_GROUPS = {
  path: {same:new Set([':','E','D']), rim:'#c9b478', r:10},
  water:{same:new Set(['~']), rim:'#e8f4ff', r:12},
  rug:  {same:new Set(['o']), rim:'#f8d060', r:3}};
function edgeStyle(map, x, y, g){
  const same = (dx,dy)=>{ const nx=x+dx, ny=y+dy; return nx<0||ny<0||nx>=map.w||ny>=map.h || g.same.has(map.tiles[ny][nx]); };
  const t=!same(0,-1), b=!same(0,1), l=!same(-1,0), r=!same(1,0);
  const sh = [];
  if(t) sh.push(`inset 0 3px 0 ${g.rim}`); if(b) sh.push(`inset 0 -3px 0 ${g.rim}`);
  if(l) sh.push(`inset 3px 0 0 ${g.rim}`); if(r) sh.push(`inset -3px 0 0 ${g.rim}`);
  if(!sh.length) return '';
  return `box-shadow:${sh.join(',')};border-radius:${t&&l?g.r:0}px ${t&&r?g.r:0}px ${b&&r?g.r:0}px ${b&&l?g.r:0}px`;
}
function tileHtml(map, ch, x, y, wx=x, wy=y){
  let cls = TILE_CLS[ch];
  const v = ((x*73856093) ^ (y*19349663)) >>> 0;   // stable per-tile variation
  let extra = '', style = '';
  if(cls==='grass') extra = ` v${v%4}`;
  if(cls==='bed' && (y===0 || map.tiles[y-1][x]!=='e')) extra = ' top';   // pillow end
  // Walls: the face you see (wall with floor below it) vs. the dark top/sides.
  if(cls==='wall' && (y+1>=map.h || '#nmwKM'.includes(map.tiles[y+1][x]))) cls = 'walltop';
  if(cls==='path'||cls==='exit') style = edgeStyle(map,x,y,EDGE_GROUPS.path);
  else if(cls==='water') style = edgeStyle(map,x,y,EDGE_GROUPS.water);
  else if(cls==='rug') style = edgeStyle(map,x,y,EDGE_GROUPS.rug);
  return `<div class="t t-${cls}${extra}" data-x="${wx}" data-y="${wy}" style="left:${wx*T}px;top:${wy*T}px;${style}"></div>`;
}
function npcHtml(n, i){
  return `<div class="ow-actor npc" id="npc-${i}" style="transform:translate(${n.x*T+4}px,${n.y*T-6}px); z-index:${20+2*n.y}">${charSvg(n.kind, n.facing, 0)}<div class="ow-bang hidden">!</div></div>`;
}
// Overworld weather, Emerald-style: volcanic ash near Cindergate (like Route 113), fog around
// Wispgate (like Route 120), rain on the coast.
const WEATHER = {"Cindergate Town":'ash', "Route 2: Marrow Pass":'ash', "Wispgate City":'fog', "Route 5: Cragmoor Trail":'fog', "Route 3: Hollow Bluffs":'rain', "Glimmer Coast":'rain'};
let owShownLoc = null;
function renderOverworld(){
  const loc = LOCATIONS[adv.loc], map = curMap();
  const world = document.getElementById('owWorld'), view = document.getElementById('owView');
  view.className = 'ow-view' + (map.interior ? ` indoors room-${map.interior}${map.theme ? ` gym-${map.theme}` : ''}` : WEATHER[loc.name] ? ` wx-${WEATHER[loc.name]}` : '');
  if(map.theme==='ghost') view.style.setProperty('--sight', (70 + 45*map.npcs.filter(n=>n.id && adv.cleared[n.id]).length) + 'px');
  let html = '<div id="owTiles"></div>';
  html += curNpcs().map(n=>npcHtml(n, map.npcs.indexOf(n))).join('');
  html += `<div class="ow-actor" id="owPlayer"><div class="ow-body"></div><div class="ow-reflect"></div></div>`;
  world.innerHTML = html;
  renderTiles(adv.pos.x, adv.pos.y);
  owAnim = null; owMoving = false;
  owFit();
  drawPlayer(0);
  placePlayer();
  updateReflect();
  if(owShownLoc !== adv.loc && !map.interior){
    owShownLoc = adv.loc;
    const pop = document.getElementById('owPopup');
    pop.textContent = loc.name;
    pop.classList.remove('show'); void pop.offsetWidth; pop.classList.add('show');
    clearTimeout(pop.__t); pop.__t = setTimeout(()=>pop.classList.remove('show'), 2200);
  }
}
// Only the tiles around the camera exist in the page: a 25×21 window centred on the player, taken
// from this map and whichever connected maps it overlaps (so big maps stay cheap). It's redrawn
// when you've moved 3 tiles from its centre; tiles sit at fixed world positions, so a redraw
// never shows. Tall-grass fronts and buildings (this map's and the neighbours') go in the same layer.
const WIN_X = 12, WIN_Y = 10;
let owTileCenter = {x:0, y:0};
function mapAt(x, y){
  const map = curMap();
  if(inMap(map, x, y)) return {m:map, lx:x, ly:y};
  if(map.interior) return null;
  const n = nbAt(x, y);
  return n ? {m:n.map, lx:x-n.ox, ly:y-n.oy} : null;
}
function renderTiles(cx, cy){
  const layer = document.getElementById('owTiles');
  if(!layer) return;
  owTileCenter = {x:cx, y:cy};
  let html = '', fronts = '';
  for(let y=cy-WIN_Y; y<=cy+WIN_Y; y++) for(let x=cx-WIN_X; x<=cx+WIN_X; x++){
    const a = mapAt(x, y);
    if(!a) continue;
    const ch = a.m.tiles[a.ly][a.lx];
    html += tileHtml(a.m, ch, a.lx, a.ly, x, y);
    if(ch==='"') fronts += `<div class="tg-front" data-x="${x}" data-y="${y}" style="left:${x*T}px;top:${y*T+16}px;z-index:${21+2*y}"></div>`;
  }
  const map = curMap();
  let blds = map.buildings.map((b,i)=>buildingHtml(b, 0, 0, i)).join('');
  if(!map.interior) for(const n of neighbours(adv.loc)) blds += n.map.buildings.map(b=>buildingHtml(b, n.ox, n.oy)).join('');
  layer.innerHTML = html + fronts + blds;
}
function keepTilesAround(){
  if(Math.abs(adv.pos.x-owTileCenter.x)>=3 || Math.abs(adv.pos.y-owTileCenter.y)>=3) renderTiles(adv.pos.x, adv.pos.y);
}
// Scale the 480×320 view to fit, choosing a scale where one tile is a whole number of device
// pixels — otherwise the browser leaves hairline seams between tiles.
let owK = 1, owDpr = 1;
function owFit(){
  const scr = document.getElementById('owScreen'), view = document.getElementById('owView');
  const avail = scr && scr.parentNode ? scr.parentNode.clientWidth - 10 : 0;   // minus the 5px frame
  if(!avail) return;
  owDpr = (typeof window!=='undefined' && window.devicePixelRatio) || 1;
  const tileDev = Math.min(Math.floor(avail*owDpr/VIEW_W), Math.floor(1.5*T*owDpr));
  owK = tileDev/(T*owDpr);
  view.style.transform = `scale(${owK})`;
  scr.style.width = VIEW_W*T*owK + 'px';
  scr.style.height = VIEW_H*T*owK + 'px';
  if(adv && document.getElementById('owPlayer')) placePlayer();
}
if(typeof window!=='undefined' && window.addEventListener) window.addEventListener('resize', owFit);
let owFrameShown = null;
function drawPlayer(frame, run){
  const el = document.getElementById('owPlayer');
  const key = (adv.facing||'down') + frame + (run?'r':'');
  if(!el || owFrameShown===key && el.__drawn) return;
  el.querySelector('.ow-body').innerHTML = el.querySelector('.ow-reflect').innerHTML = charSvg('player', adv.facing||'down', frame, run);
  owFrameShown = key; el.__drawn = true;
}
// Put player + camera at fractional tile coords. The camera keeps the player centred; both use
// the same whole-pixel offset so tiles stay crisp and the player never jitters against them.
function setActorPos(px, py){
  const el = document.getElementById('owPlayer'), world = document.getElementById('owWorld');
  if(!el || !world) return;
  // Snap the camera to whole device pixels (in view units that's multiples of 1/(k·dpr)).
  const unit = 1/(owK*owDpr), snap = v => Math.round(v/unit)*unit;
  const cx = (VIEW_W*T - T)/2, cy = (VIEW_H*T - T)/2;
  const wx = snap(cx - px*T), wy = snap(cy - py*T);
  world.style.transform = `translate(${wx}px,${wy}px)`;
  world.parentNode.style.backgroundPosition = `${wx}px ${wy}px`;   // the forest past the maps scrolls too
  el.style.transform = `translate(${cx - wx + 4}px,${cy - wy - 6}px)`;
}
function placePlayer(){
  const el = document.getElementById('owPlayer');
  if(!el) return;
  setActorPos(adv.pos.x, adv.pos.y);
  el.style.zIndex = 20 + 2*adv.pos.y;
}
function rustleTile(x,y){
  if(typeof document.querySelector!=='function') return;
  document.querySelectorAll(`#owTiles > [data-x="${x}"][data-y="${y}"]`).forEach(el=>{
    el.classList.remove('rustling'); void el.offsetWidth; el.classList.add('rustling');
  });
}
// Encounter transition (Emerald): the field flashes white twice, then black bars slide in from
// alternate sides to wipe it (~1 s), and only then the battle screen.
function battleIntro(done){
  const view = document.getElementById('owView');
  if(!view || document.getElementById('adv').classList.contains('hidden')) return done();
  owBusy = true; held.length = 0;
  const fx = document.createElement('div');
  fx.className = 'bt-intro';
  fx.innerHTML = Array.from({length:8}, (_,i)=>`<i style="top:${i*40}px; --from:${i%2 ? '' : '-'}100%"></i>`).join('');
  view.appendChild(fx);
  setTimeout(()=>{ fx.remove(); owBusy = false; done(); }, 1050);
}

// ---------- Movement ----------
// Steps are animated on requestAnimationFrame and chained back-to-back on the same clock, so
// holding a direction gives one smooth continuous walk instead of step-pause-step.
const DIRS = {up:[0,-1], down:[0,1], left:[-1,0], right:[1,0]};
const KEYDIR = {arrowup:'up', w:'up', arrowdown:'down', s:'down', arrowleft:'left', a:'left', arrowright:'right', d:'right'};
const held = [];          // held directions, most recent last
let owMoving = false, owBusy = false, owRun = false, owStepFoot = false, owAnim = null;
const owNow = ()=> (typeof performance!=='undefined' ? performance.now() : Date.now());
function owActive(){
  if(!adv || owBusy) return false;
  if(document.getElementById('adv').classList.contains('hidden')) return false;
  return ['battle'].every(id=>document.getElementById(id).classList.contains('hidden'))
    && ['partyModal','boxModal','saveCodeModal','confirmModal'].every(id=>document.getElementById(id).classList.contains('hidden'));
}
function pressDir(d){
  const i = held.indexOf(d); if(i>=0) held.splice(i,1);
  held.push(d);
  owTryStep(owNow());
}
function releaseDir(d){ const i = held.indexOf(d); if(i>=0) held.splice(i,1); }
function npcAt(x, y){ return curNpcs().find(n=>n.x===x && n.y===y); }
// `chained` = continuing straight on from the previous step (no turn pause).
function owTryStep(startAt, chained){
  if(owMoving || !owActive()) return;
  const d = held[held.length-1];
  if(!d) return;
  // Don't carry over more than a frame of lag, or a stalled tab would make the next step jump.
  const t0 = Math.max(startAt || owNow(), owNow() - 20);
  const here = {fx:adv.pos.x, fy:adv.pos.y, start:t0};
  // From a standstill, a new direction first turns you on the spot; keep holding and you walk.
  if(!chained && d!==adv.facing){ adv.facing = d; return owStart({...here, dur:TURN_MS, inPlace:true}); }
  adv.facing = d;
  const loc = LOCATIONS[adv.loc], map = curMap();
  const nx = adv.pos.x + DIRS[d][0], ny = adv.pos.y + DIRS[d][1];
  const ch = worldTile(nx, ny);
  const npc = npcAt(nx, ny);
  if(npc && activeTrainer(npc)){ drawPlayer(0); return triggerTrainer(npc); }
  const cross = !map.interior && !inMap(map, nx, ny) ? nbAt(nx, ny) : null;
  if(cross && adv.starterPending && cross.exit.to!==1){
    drawPlayer(0);
    return owSay(["It's dangerous to go out without POKéMON!", `${PROF} went toward ROUTE 1...`]);
  }
  if(cross && cross.exit.gate && !adv.cleared[loc.name]){
    drawPlayer(0);
    return owSay([`You should challenge ${loc.type==='gym'?'Gym Leader':'your rival'} ${loc.leaderName} before moving on.`]);
  }
  // Walking down onto a ledge hops you over it to the tile beyond: 32 frames for the two tiles.
  const lx = nx + DIRS[d][0], ly = ny + DIRS[d][1];
  if(LEDGE_DIR[ch]===d && WALKABLE.has(worldTile(lx, ly)) && !npcAt(lx, ly)){
    adv.pos = {x:lx, y:ly};
    const el = document.getElementById('owPlayer');
    if(el) el.style.zIndex = 20 + 2*adv.pos.y;
    sfx('jump');
    return owStart({...here, dur:JUMP_MS, jump:true, ch:worldTile(lx, ly)});
  }
  // Blocked (by scenery or a person): walk in place against it for as long as you hold on.
  if(npc || !WALKABLE.has(ch)){ sfx('bump'); return owStart({...here, dur:WALK_MS, inPlace:true}); }
  adv.pos = {x:nx, y:ny};
  const el = document.getElementById('owPlayer');
  // Layer by the destination row: grass fronts on that row (and the one you leave) cover your legs.
  if(el) el.style.zIndex = 20 + 2*ny;
  if(ch==='"') rustleTile(nx, ny);
  const run = owRun && !map.interior;   // running shoes don't work indoors
  owStart({...here, dur:run ? RUN_MS : WALK_MS, run, ch});
}
// Emerald shows your reflection when the tile below you is water.
function updateReflect(){
  const el = document.getElementById('owPlayer');
  if(el) el.classList.toggle('reflect', worldTile(adv.pos.x, adv.pos.y+1)==='~');
}
function owStart(a){
  updateReflect();
  owStepFoot = !owStepFoot;   // feet alternate step to step
  a.foot = owStepFoot ? 1 : 2;
  owAnim = a; owMoving = true;
  owStep(owNow());
  requestAnimationFrame(owFrame);
}
// Gen 3 walk: step pose for the first half of the tile, standing pose for the second half.
// Running holds the stride a little longer (5 of 8 frames) and leans the whole way.
function owStep(t){
  const a = owAnim;
  const k = Math.min(1, Math.max(0, (t - a.start)/a.dur));
  setActorPos(a.fx + (adv.pos.x-a.fx)*k, a.fy + (adv.pos.y-a.fy)*k);
  if(a.jump){
    // Two walk cycles' worth of legs while airborne; the sprite rises up to 12 GBA pixels
    // (24 here) above its shadow, which stays on the ground.
    const q = Math.floor(k*4);
    drawPlayer(k<1 && q%2===0 ? (q ? 3-a.foot : a.foot) : 0);
    liftPlayer(Math.round(12*Math.sin(Math.PI*k))*2);
  }
  else drawPlayer(k < (a.run ? 5/8 : 1/2) ? a.foot : 0, a.run);
  return k;
}
function liftPlayer(px){
  const body = document.querySelector('#owPlayer .ow-body');
  if(body) body.style.transform = px ? `translateY(${-px}px)` : '';
}
// A puff of dust where you land from a ledge.
function dustAt(x, y){
  const world = document.getElementById('owWorld');
  if(!world) return;
  const el = document.createElement('div');
  el.className = 'ow-dust';
  el.style.cssText = `left:${x*T}px; top:${y*T+18}px; z-index:${21+2*y}`;
  world.appendChild(el);
  setTimeout(()=>el.remove(), 400);
}
function owFrame(t){
  const a = owAnim;
  if(!a) return;
  if(owStep(t) < 1){ requestAnimationFrame(owFrame); return; }
  owAnim = null; owMoving = false;
  placePlayer();
  if(a.jump) dustAt(adv.pos.x, adv.pos.y);
  if(a.inPlace) owTryStep(a.start + a.dur, true);
  else owArrive(a.ch, a.start + a.dur);
  if(!owMoving) drawPlayer(0);   // stopped: stand up straight
  if(startQueued && !owMoving){ startQueued = false; startMenu(); }
}
// Stepped over the edge into a connected area: it becomes the current map, with no fade.
function crossArea(){
  const n = nbAt(adv.pos.x, adv.pos.y);
  adv.loc = n.exit.to;
  adv.pos = {x:adv.pos.x - n.ox, y:adv.pos.y - n.oy};
  adv.visited[LOCATIONS[adv.loc].name] = true;
  saveAdv();
  renderAdventure();
  if(adv.starterPending && adv.loc===1){ held.length = 0; starterEvent(); }
}
function owArrive(ch, endedAt){
  if(!curMap().interior && !inMap(curMap(), adv.pos.x, adv.pos.y)) crossArea();
  else keepTilesAround();
  const map = curMap();
  const keepWalking = held.length > 0;
  if(!keepWalking || ch!==':' && ch!=='.' && ch!=='_') saveAdv();
  if(ch==='D'){ const bi = map.buildings.findIndex(b=>b.door.x===adv.pos.x && b.door.y===adv.pos.y); if(bi>=0) return enterBuilding(bi); }
  if(ch==='M') return leaveBuilding();
  if(ch==='"'){
    if(Math.random()<0.12){ held.length = 0; owRun = false; saveAdv(); startWildBattle(); return; }
  }
  // Trainers spot you when you walk into their line of sight (up to 5 tiles, nothing in between).
  for(const n of curNpcs()){
    if(!activeTrainer(n)) continue;
    const [fx,fy] = DIRS[n.facing];
    for(let s=1; s<=5; s++){
      const tx = n.x+fx*s, ty = n.y+fy*s;
      if(!WALKABLE.has(tileAt(map,tx,ty))) break;
      if(tx===adv.pos.x && ty===adv.pos.y){ saveAdv(); return triggerTrainer(n); }
    }
  }
  owTryStep(endedAt, true);
}
// Fade to black, run `fn` (which moves you), then fade back in.
function owFade(fn){
  const view = document.getElementById('owView');
  owBusy = true; held.length = 0;
  view.classList.add('fade');
  setTimeout(()=>{
    owBusy = false;
    fn();
    saveAdv();
    renderAdventure();
    setTimeout(()=>document.getElementById('owView').classList.remove('fade'), 40);
  }, 300);
}
// The door opens (3 steps of 4 frames), you vanish through it, then the fade.
function enterBuilding(bi){
  owBusy = true; held.length = 0;
  const bld = document.querySelector(`#owTiles .bld[data-bi="${bi}"]`), door = bld && bld.querySelector('.door');
  if(door) door.classList.add('open');
  sfx('door');
  setTimeout(()=>{ const p = document.getElementById('owPlayer'); if(p) p.style.visibility = 'hidden'; }, 200);
  setTimeout(()=>owFade(()=>{
    adv.inside = bi;
    const room = getInterior(LOCATIONS[adv.loc], bi);
    adv.pos = {...room.spawn}; adv.facing = 'up';
  }), 300);
}
// Coming out: you appear in the open doorway and step down onto the path, then the door shuts.
function leaveBuilding(){
  const bi = adv.inside;
  owFade(()=>{
    const b = getMap(LOCATIONS[adv.loc]).buildings[bi];
    adv.inside = null;
    adv.pos = {x:b.door.x, y:b.door.y+1}; adv.facing = 'down';
  });
  setTimeout(()=>{
    const bld = document.querySelector(`#owTiles .bld[data-bi="${bi}"]`), door = bld && bld.querySelector('.door');
    if(door) door.classList.add('open');
    sfx('door');
    if(!owMoving) owStart({fx:adv.pos.x, fy:adv.pos.y-1, start:owNow(), dur:WALK_MS, ch:tileAt(curMap(), adv.pos.x, adv.pos.y)});
    setTimeout(()=>door && door.classList.remove('open'), WALK_MS + 100);
  }, 350);
}
// Whiting out (Emerald): you come to outside the door of the last Pokémon Center you healed at,
// one tile below it, facing down, with the party healed. Home is the default.
function sendToCenter(){
  const li = adv.lastHeal!=null && LOCATIONS[adv.lastHeal].center ? adv.lastHeal : 0;
  const b = getMap(LOCATIONS[li]).buildings.find(b=>b.kind==='center');
  adv.loc = li; adv.inside = null;
  adv.pos = {x:b.door.x, y:b.door.y+1}; adv.facing = 'down';
}
function faceNpcToPlayer(n){
  n.facing = n.x<adv.pos.x ? 'right' : n.x>adv.pos.x ? 'left' : n.y<adv.pos.y ? 'down' : 'up';
  const el = document.getElementById(`npc-${curMap().npcs.indexOf(n)}`);
  if(el) el.firstElementChild.outerHTML = charSvg(n.kind, n.facing, 0);
}
// Spotted (Emerald): "!" pops over the trainer for about a second, they walk up tile by tile
// until they're next to you, you face each other, then the challenge line.
function triggerTrainer(npc){
  const loc = LOCATIONS[adv.loc];
  held.length = 0; owBusy = true;
  const el = document.getElementById(`npc-${curMap().npcs.indexOf(npc)}`), bang = el && el.querySelector('.ow-bang');
  if(bang) bang.classList.remove('hidden');
  sfx('spot');
  let foot = 1;
  const walkUp = ()=>{
    if(Math.abs(adv.pos.x-npc.x) + Math.abs(adv.pos.y-npc.y) <= 1) return challenge();
    npc.facing = npc.x<adv.pos.x ? 'right' : npc.x>adv.pos.x ? 'left' : npc.y<adv.pos.y ? 'down' : 'up';
    npc.x += DIRS[npc.facing][0]; npc.y += DIRS[npc.facing][1];
    if(el){
      el.firstElementChild.outerHTML = charSvg(npc.kind, npc.facing, foot = 3-foot);
      el.style.transform = `translate(${npc.x*T+4}px,${npc.y*T-6}px)`;
      el.style.zIndex = 20 + 2*npc.y;
      setTimeout(()=>{ if(el.isConnected) el.firstElementChild.outerHTML = charSvg(npc.kind, npc.facing, 0); }, WALK_MS/2);
    }
    setTimeout(walkUp, WALK_MS);
  };
  const challenge = ()=>{
    adv.facing = npc.x<adv.pos.x ? 'left' : npc.x>adv.pos.x ? 'right' : npc.y<adv.pos.y ? 'up' : 'down';
    drawPlayer(0);
    faceNpcToPlayer(npc);
    owBusy = false;
    if(npc.id) return owSay([`${npc.title}: "${npc.intro}"`], ()=>startTrainerBattle(npc));
    const quote = npc.gymLeader ? `So, a new challenger has come to the ${loc.name.split(' ')[0]} Gym. Show me what your Pokémon can do!`
      : (loc.desc.match(/"([^"]+)"/)||[])[1] || "Let's battle!";
    owSay([`${loc.leaderName}: "${quote}"`], ()=>startTrainerBattle());
  };
  setTimeout(()=>{ if(bang) bang.classList.add('hidden'); walkUp(); }, 1000);
}
// What you get by pressing A at things that aren't people.
const THING_TEXT = {
  bookshelf:["It's crammed full of books about Pokémon."], tv:["A Pokémon battle is on TV. The crowd is going wild!"],
  bed:["A comfy-looking bed. No time for a nap now!"], shelf:["Shelves stocked with Potions, Antidotes and Poké Balls."],
  healer:["A machine for healing Pokémon."], plant:["A well-watered potted plant."], table:["A plain wooden table."],
  seat:["A soft, round cushion seat."], glasstable:["A glass table. There's a POKéMON magazine on it."]};
function owInteract(){
  if(!owActive() || owMoving) return;
  const loc = LOCATIONS[adv.loc], map = curMap();
  const [dx,dy] = DIRS[adv.facing||'down'];
  let tx = adv.pos.x+dx, ty = adv.pos.y+dy;
  if(tileAt(map,tx,ty)==='c'){ tx += dx; ty += dy; }   // talk across a counter
  const npc = npcAt(tx, ty);
  if(npc) return talkTo(npc);
  const sign = map.signs.find(s=>s.x===tx && s.y===ty);
  if(sign) return owSay(sign.route ? [sign.text] : [sign.text, loc.desc.replace(/\s*"[^"]*"\s*/g,' ').trim()]);
  const ch = tileAt(map, adv.pos.x+dx, adv.pos.y+dy);
  if(ch==='P') return usePC();
  // An item ball: take it (Emerald: "Obtained ..." then "put away ... in the ... POCKET.").
  if(ch==='I'){
    const x = adv.pos.x+dx, y = adv.pos.y+dy;
    (adv.picked ||= {})[`${loc.name}@${x},${y}`] = true;
    map.tiles[y][x] = '.';
    adv.items.pokeball = (adv.items.pokeball||0) + 1;
    saveAdv(); renderTiles(adv.pos.x, adv.pos.y);
    return obtainItem('POKé BALL', 1, 'POKé BALLS', ()=>renderAdventure());
  }
  if(ch==='u') return owSay([`${loc.name.toUpperCase()} POKéMON GYM`, `Leader: ${loc.leaderName}`, adv.cleared[loc.name] ? `Winning trainers: ${adv.playerName}` : 'Winning trainers: ...']);
  const said = THING_TEXT[(TILE_CLS[ch]||'').split(' ')[0]];
  if(said) owSay(said);
}
function talkTo(n){
  const loc = LOCATIONS[adv.loc];
  faceNpcToPlayer(n);
  if(activeTrainer(n)) return triggerTrainer(n);
  if(n.id) return owSay([`${n.title}: "${n.after}"`]);
  if(n.gymLeader) return owSay([`${loc.leaderName}: "You've already beaten me. The road ahead is waiting for you!"`]);
  if(n.role==='nurse') return nurseTalk(n);
  if(n.role==='clerk'){
    if(adv.restockedLoc !== adv.loc){
      adv.restockedLoc = adv.loc; adv.items.pokeball = (adv.items.pokeball||0) + 5; saveAdv();
      return owSay(['Welcome to the Poké Mart!', "You're new in town? Here, take these on the house!"], ()=>obtainItem('POKé BALLS', 5, 'POKé BALLS', ()=>renderAdventure()));
    }
    return martOpen();
  }
  owSay(n.lines || ['...']);
}

// ---------- People who wander ----------
// Every so often, a wandering NPC takes one step (staying near home, never onto you or a doorway).
let owWanderTimer = null;
function startWander(){
  clearInterval(owWanderTimer);
  owWanderTimer = setInterval(()=>{
    if(!adv || !owActive() || document.hidden) return;
    const map = curMap();
    for(const n of curNpcs()){
      if(!n.wander || Math.random() > 0.25) continue;
      const d = ['up','down','left','right'][Math.floor(Math.random()*4)];
      const nx = n.x + DIRS[d][0], ny = n.y + DIRS[d][1];
      n.facing = d;
      const el = document.getElementById(`npc-${map.npcs.indexOf(n)}`);
      const ch = tileAt(map, nx, ny);
      const free = WALKABLE.has(ch) && !'EDM'.includes(ch) && !(nx===adv.pos.x && ny===adv.pos.y) && !npcAt(nx, ny)
        && Math.abs(nx-n.home.x)<=2 && Math.abs(ny-n.home.y)<=2;
      if(free){ n.x = nx; n.y = ny; }
      if(el){
        el.firstElementChild.outerHTML = charSvg(n.kind, n.facing, free ? (n.step = !n.step) ? 1 : 2 : 0);
        el.style.transform = `translate(${n.x*T+4}px,${n.y*T-6}px)`;
        el.style.zIndex = 20 + 2*n.y;
        if(free) setTimeout(()=>{ if(el.isConnected) el.firstElementChild.outerHTML = charSvg(n.kind, n.facing, 0); }, 180);
      }
    }
  }, 700);
}
if(typeof window!=='undefined' && typeof setInterval==='function') startWander();

// ---------- Overworld text box ----------
// Text prints letter by letter at Emerald's MID speed; A mid-line finishes the line.
// MID text speed (Emerald's default): about 5 frames per letter.
// Frames per letter for OPTION > TEXT SPEED (SLOW / MID / FAST), MID by default.
const TEXT_FRAMES = {SLOW:9, MID:5, FAST:2};
const textMs = ()=>(TEXT_FRAMES[adv && adv.textSpeed] || 5)*1000/60;
let owQueue = [], owQueueDone = null, owTyping = null, owHold = false, owLineDone = null;
function owSay(lines, done){
  owQueue = lines.slice(); owQueueDone = done || null;
  owBusy = true; held.length = 0;
  owAdvance();
}
function owFinishLine(){
  const body = document.getElementById('owTextBody');
  clearInterval(owTyping); owTyping = null;
  body.textContent = body.__full;
  document.getElementById('owText').classList.remove('typing');
  if(owLineDone){ const f = owLineDone; owLineDone = null; f(); }
}
// A line the game moves on from by itself (no A press); input is ignored until owHold is cleared.
function owSayHold(text, then){
  owBusy = true; held.length = 0;
  owQueue = []; owQueueDone = null; owHold = true; owLineDone = then;
  owShowLine(text);
}
function owAdvance(){
  const box = document.getElementById('owText');
  if(uiMenus.length || pcBox || dexScr || ptyScr || uiScr || owHold) return;   // a menu (or the game) is using the box
  if(owTyping) return owFinishLine();
  if(!owQueue.length){
    box.classList.add('hidden');
    owBusy = false;
    const cb = owQueueDone; owQueueDone = null;
    if(cb) cb();
    return;
  }
  owShowLine(owQueue.shift());
}
function owShowLine(text){
  const box = document.getElementById('owText'), body = document.getElementById('owTextBody');
  clearInterval(owTyping);
  body.__full = text; body.textContent = '';
  box.classList.remove('hidden'); box.classList.add('typing');
  let i = 0;
  owTyping = setInterval(()=>{ body.textContent = text.slice(0, ++i); if(i >= text.length) owFinishLine(); }, textMs());
}
function owTextOpen(){ const b = document.getElementById('owText'); return b && !b.classList.contains('hidden') && !document.getElementById('adv').classList.contains('hidden'); }


// ---------- Pokémon Center nurse (Emerald script) ----------
// Welcome → "Would you like to rest your POKéMON?" YES/NO → she turns to the machine, sets down one
// Poké Ball per party member, the machine flashes to the healing jingle, she turns back, thanks you
// and bows. NO skips straight to the bow.
function nurseTalk(n){
  owSay(['Hello, and welcome to\nthe POKéMON CENTER.', 'We restore your tired\nPOKéMON to full health.'], ()=>{
    owBusy = true;
    owPrompt('Would you like to rest your POKéMON?');
    uiMenu(document.getElementById('owView'), ['YES', 'NO'], k=>{
      if(k!==0) return owSay(['We hope to see you again!']);
      // No button press here: the heal starts as soon as the line has printed.
      owSayHold("Okay, I'll take your\nPOKéMON for a few seconds.", ()=>nurseHeal(n));
    }, 'gm-yesno');
  });
}
function nurseFace(n, dir){
  n.facing = dir;
  const el = document.getElementById(`npc-${curMap().npcs.indexOf(n)}`);
  if(el) el.firstElementChild.outerHTML = charSvg(n.kind, dir, 0);
  return el;
}
// Timings from pokeemerald: a ball every 25 frames (the first at once), 32 frames' pause, then the
// 160-frame jingle with the machine glowing for 150 of them; she heals you after turning back.
function nurseHeal(n){
  const F = 1000/60, machine = document.querySelector('#owWorld .t-healer'), count = adv.party.length;
  nurseFace(n, 'left');
  for(let i=0;i<count;i++) setTimeout(()=>{ sfx('ball'); if(machine) machine.insertAdjacentHTML('beforeend', `<span class="heal-ball" style="left:${7+(i%2)*10}px; top:${7+Math.floor(i/2)*6}px"></span>`); }, (4 + 25*i)*F);
  const jingle = (4 + 25*(count-1) + 32)*F;
  setTimeout(()=>{ sfx('heal'); if(machine){ machine.classList.remove('healing'); void machine.offsetWidth; machine.classList.add('healing'); } }, jingle);
  setTimeout(()=>{ if(machine){ machine.classList.remove('healing'); machine.querySelectorAll('.heal-ball').forEach(b=>b.remove()); } }, jingle + 150*F);
  setTimeout(()=>{
    nurseFace(n, 'down');
    healParty(); adv.lastHeal = adv.loc; saveAdv();
    owHold = false;
    owSay(['Thank you for waiting.'], ()=>{
      // She bows (52 frames) while this line is up.
      const el = nurseFace(n, 'down');
      if(el){ el.classList.add('bow'); setTimeout(()=>el.classList.remove('bow'), 52*F); }
      owSay(["We've restored your\nPOKéMON to full health."], ()=>owSay(['We hope to see you again!']));
    });
  }, jingle + 164*F);
}

// ---------- START menu (Emerald: a window on the right, cursor remembered) ----------
let startIdx = 0;
let startQueued = false;
function startMenu(){
  if(owMoving && owActive() && !uiMenus.length){ startQueued = true; held.length = 0; return; }   // open when this step lands
  if(!owActive() || owMoving || uiMenus.length) return;
  owBusy = true; held.length = 0;
  const name = adv.playerName.toUpperCase();
  // POKéDEX shows up once you've seen something.
  const items = (Object.keys(adv.seen||{}).length ? ['POKéDEX'] : []).concat((adv.party.length ? ['POKéMON'] : []).concat(['BAG', 'POKéNAV', name, 'SAVE', 'OPTION', 'FEEDBACK', 'EXIT']));
  sfx('open');
  const m = uiMenu(document.getElementById('owView'), items, k=>{
    owBusy = false;
    if(k>=0) startIdx = k;
    const pick = items[k];
    if(pick==='POKéDEX') return dexOpen();
    if(pick==='POKéMON') return partyOpen();
    if(pick==='BAG') return bagOpen();
    if(pick==='OPTION') return optionOpen();
    if(pick==='FEEDBACK') return feedbackOpen();
    if(pick==='POKéNAV') return toggleMap();
    if(pick===name){
      return cardOpen();
    }
    if(pick==='SAVE') return startSave();
  }, 'gm-start');
  m.i = startIdx; uiMenuDraw(m);
}
// Emerald's save: an info window top-left (place, PLAYER, BADGES, POKéDEX), YES/NO, "SAVING...",
// then "{PLAYER} saved the game." and back to the field by itself. NO goes back to the START menu.
function startSave(){
  owBusy = true;
  const view = document.getElementById('owView'), info = document.createElement('div');
  const badges = LOCATIONS.filter(l=>l.type==='gym' && adv.cleared[l.name]).length;
  const own = new Set(adv.party.concat(adv.box.filter(Boolean)).map(p=>p.name)).size;
  info.className = 'gba-menu gm-saveinfo';
  info.innerHTML = `<div class="si-place">${LOCATIONS[adv.loc].name}</div><div>PLAYER <b>${adv.playerName}</b></div><div>BADGES <b>${badges}</b></div><div>POKéDEX <b>${own}</b></div>`;
  view.appendChild(info);
  owPrompt('Would you like to save the game?');
  uiMenu(view, ['YES', 'NO'], k=>{
    if(k!==0){ info.remove(); owPromptClose(); return startMenu(); }
    owSayHold("SAVING...\nDON'T TURN OFF THE POWER.", ()=>setTimeout(()=>{
      saveAdv();
      sfx('save');
      owSayHold(`${adv.playerName} saved the game.`, ()=>setTimeout(()=>{ owHold = false; info.remove(); owPromptClose(); }, 1000));
    }, 500));
  }, 'gm-yesno');
}

// ---------- Pokédex (Emerald layout, from pokedex.c) ----------
// Striped green background, a yellow list on the right whose 6th row is always the selected one,
// a three-sprite rolodex in the middle, SEEN/OWN bottom-left. The list runs from No. 1 to the
// highest species you've seen; unseen entries are dashes. adv.seen / adv.owned are keyed by name.
const dexNum = d=>DEX_NUM[slug(d.name)] || 0;
let DEX_LIST = null;   // species in national order (built on first use)
function markSeen(m){ (adv.seen ||= {})[m.name] = true; }
function markOwned(m){ markSeen(m); const first = !(adv.owned ||= {})[m.name]; adv.owned[m.name] = true; return first; }
const DEX_TABS = ['INFO', 'AREA', 'SIZE', 'CANCEL'];
let dexScr = null, dexIdx = -1;   // -1: first opening starts on the first species seen
function dexOpen(){
  DEX_LIST ||= DEX.filter(d=>dexNum(d)>0 && dexNum(d)<10000).sort((a,b)=>dexNum(a)-dexNum(b));
  const seenNums = DEX_LIST.filter(d=>adv.seen && adv.seen[d.name]).map(dexNum);
  if(!seenNums.length) return;
  owBusy = true;
  const el = document.createElement('div');
  el.className = 'dex';
  document.getElementById('owView').appendChild(el);
  dexScr = {el, list:DEX_LIST.filter(d=>dexNum(d)<=Math.max(...seenNums)), page:null};
  if(dexIdx<0) dexIdx = dexScr.list.findIndex(d=>adv.seen[d.name]);
  dexIdx = Math.min(dexIdx, dexScr.list.length-1);
  dexDraw();
}
function dexDraw(){
  const s = dexScr, L = s.list, d = L[dexIdx];
  const seen = n=>adv.seen && adv.seen[n], own = n=>adv.owned && adv.owned[n];
  const img = (dd, cls)=> dd && seen(dd.name) ? `<img class="${cls}" src="${spritePath(dd)}" alt="">` : `<div class="${cls} dex-unk">?</div>`;
  if(s.page){
    // Entry page (Emerald INFO): No./name, category, HT in feet'inches", WT in lbs., with the
    // ability text in the description box. Tabs INFO / SIZE / CANCEL along the top (Left/Right).
    const o = own(d.name), ab = d.ability || (DEXDATA[slug(d.name)]||{}).ability || {n:'', desc:''};
    const info = (typeof DEXINFO!=='undefined' && DEXINFO[dexNum(d)]) || {h:0, w:0, g:''};
    const inches = Math.round(info.h*3.937), ht = `${Math.floor(inches/12)}'${String(inches%12).padStart(2,'0')}"`;
    const wt = `${(info.w*0.220462).toFixed(1)} lbs.`;
    const tabs = DEX_TABS.map((t,k)=>`<span class="${k===s.tab?'on':''}${t==='CANCEL'?' de-cancel':''}">${t}</span>`).join('');
    let body;
    if(s.page==='area'){
      // AREA (Emerald): the region map with every area where it lives wild pulsing, or AREA UNKNOWN.
      const xs = LOCATIONS.map(l=>l.at[0]), ys = LOCATIONS.map(l=>l.at[1]), x0 = Math.min(...xs), y0 = Math.min(...ys);
      const cols = Math.max(...xs)-x0+1, rows = Math.max(...ys)-y0+1;
      const home = LOCATIONS.filter(l=>l.pool && l.pool.includes(d.name));
      let cells = '';
      LOCATIONS.forEach(l=>{
        const gx = (l.at[0]-x0)*2+1, gy = (l.at[1]-y0)*2+1;
        cells += `<div class="da-cell da-${l.type}${home.includes(l) ? ' hab' : ''}" style="grid-column:${gx};grid-row:${gy}"></div>`;
        for(const k of l.links) if(k.dir==='right' || k.dir==='down')
          cells += `<div class="da-link ${k.dir==='right'?'h':'v'}" style="grid-column:${gx+(k.dir==='right'?1:0)};grid-row:${gy+(k.dir==='down'?1:0)}"></div>`;
      });
      body = `<div class="de-area"><div class="ds-cap">${d.name.toUpperCase()}</div>
        <div class="da-map" style="grid-template-columns:repeat(${cols},44px 10px);grid-template-rows:repeat(${rows},26px 10px)">${cells}</div>
        ${home.length ? '' : '<div class="da-unknown">AREA UNKNOWN</div>'}</div>`;
    } else if(s.page==='size'){
      // Silhouettes side by side, scaled by height against a 1.6 m trainer (the taller fills the box).
      const mh = Math.max(info.h, 1), ph = 16, big = Math.max(mh, ph);
      body = `<div class="de-size"><div class="ds-cap">SIZE COMPARED TO ${adv.playerName.toUpperCase()}</div>
        <div class="ds-fig"><img src="${spritePath(d)}" alt="" style="height:${Math.round(200*mh/big)}px"></div>
        <div class="ds-fig ds-you" style="height:${Math.round(200*ph/big)}px">${charSvg('player','down',0)}</div></div>`;
    } else body = `<div class="de-card">${img(d,'de-sprite')}
        <div class="de-text"><div>No${String(dexNum(d)).padStart(3,'0')} <b>${d.name.toUpperCase()}</b></div>
          <div>${o && info.g ? info.g.toUpperCase() : '?????'} POKéMON</div>
          <div>HT <b>${o && info.h ? ht : `??'??"`}</b></div>
          <div>WT <b>${o && info.w ? wt : '????.? lbs.'}</b></div></div></div>
      <div class="de-desc">${o ? (ab.desc||'').replace('(No battle effect yet.)', '').trim() : ''}</div>`;
    s.el.innerHTML = `<div class="dex-entry${o?'':' only-seen'}${s.reg?' reg':''}"><div class="de-tabs">${s.reg ? 'POKéDEX registration completed.' : tabs}</div>${body}</div>`;
    return;
  }
  const rows = [];
  for(let r=0;r<11;r++){ const e = L[dexIdx - 5 + r];
    rows.push(`<div class="dl-row${r===5?' on':''}">${e ? `${own(e.name)?'<i class="dl-ball"></i>':''}<span>No${String(dexNum(e)).padStart(3,'0')}</span> ${seen(e.name) ? e.name.toUpperCase() : '----------'}` : ''}</div>`); }
  const seenN = Object.keys(adv.seen||{}).length, ownN = Object.keys(adv.owned||{}).length;
  s.el.innerHTML = `<div class="dex-ball"></div>
    <div class="dex-count">SEEN <b>${seenN}</b><br>OWN <b>${ownN}</b></div>
    <div class="dex-viewer">${img(L[dexIdx-1],'dv-prev')}${img(d,'dv-cur')}${img(L[dexIdx+1],'dv-next')}</div>
    <div class="dex-list">${rows.join('')}</div>`;
}
function dexKey(k){
  const s = dexScr, L = s.list;
  if(s.page){
    if(s.reg && (k==='a' || k==='b')){ s.el.remove(); dexScr = null; owBusy = false; return dexRegisterNext(); }
    if(s.reg) return;
    if(k==='b'){ s.page = null; return dexDraw(); }
    if(k==='left' || k==='right'){ s.tab = (s.tab + (k==='left' ? DEX_TABS.length-1 : 1)) % DEX_TABS.length; return dexDraw(); }
    if(k==='a'){
      const t = DEX_TABS[s.tab];
      if(t==='CANCEL'){ s.page = null; return dexDraw(); }
      // SIZE needs the Pokémon to be caught; otherwise the failure buzz, as in Emerald.
      if(t==='SIZE' && !adv.owned[L[dexIdx].name]) return sfx('bump');
      s.page = t==='SIZE' ? 'size' : t==='AREA' ? 'area' : 'info';
      return dexDraw();
    }
    // Up/Down on an entry jump to the previous/next seen species.
    const step = k==='up' ? -1 : k==='down' ? 1 : 0;
    if(step){ for(let i=dexIdx+step; i>=0 && i<L.length; i+=step) if(adv.seen[L[i].name]){ dexIdx = i; break; } dexDraw(); }
    return;
  }
  if(k==='b'){ s.el.remove(); dexScr = null; owBusy = false; return startMenu(); }   // back to START, as in Emerald
  if(k==='a'){ if(adv.seen[L[dexIdx].name]){ s.page = 'info'; s.tab = 0; dexDraw(); } return; }
  const step = {up:-1, down:1, left:-7, right:7}[k];
  if(step){ dexIdx = Math.max(0, Math.min(L.length-1, dexIdx+step)); dexDraw(); }
}

// ---------- POKéMON (party) screen, Emerald layout (party_menu.h, doubled) ----------
// Lead in a big box on the left, slots 2–6 stacked on the right, CANCEL bottom-right, "Choose a
// POKéMON." bottom-left. The selected box swaps palette and its icon hops, faster when healthier.
let ptyScr = null;   // {el, i (0–5, 6 = CANCEL), swap (index being moved), sum (summary page)}
function hpClass(m){ const f = m.hp/m.maxhp; return m.fainted || m.hp<=0 ? 'fnt' : f>0.5 ? 'g' : f>0.2 ? 'y' : 'r'; }
function partyOpen(){
  owBusy = true;
  const el = document.createElement('div');
  el.className = 'pty';
  document.getElementById('owView').appendChild(el);
  ptyScr = {el, i:0, swap:null, sum:null};
  partyDraw();
}
function partyBox(m, k){
  const s = ptyScr, cls = hpClass(m), f = Math.max(0, m.hp/m.maxhp);
  // Icon frame time by HP (6/8/14/22 frames), doubled for the two-frame hop.
  const speed = {g: f>=1 ? 6 : 8, y:14, r:22}[cls];
  return `<div class="pb ${k?'pb-small':'pb-lead'} pb-${cls}${s.i===k?' on':''}${s.swap===k?' swapping':''}" data-k="${k}" style="${k ? slotPos(k) : ''}">
    <img src="${spritePath(m.dex)}" alt="" style="${speed ? `animation-duration:${speed*2/60}s` : 'animation:none'}">
    <div class="pb-name">${dname(m)}</div><div class="pb-lv">Lv${m.level}</div>
    <div class="pb-hp"><span>HP</span><i><b class="hp-${cls}" style="width:${f*100}%"></b></i></div>
    <div class="pb-hpn">${Math.max(0,m.hp)}/${m.maxhp}</div></div>`;
}
// Slot k (1–9): column-major, five to a column, right of the lead box.
const slotPos = k=>`left:${192 + Math.floor((k-1)/5)*146}px; top:${16 + ((k-1)%5)*50}px`;
function partyDraw(){
  const s = ptyScr;
  if(s.sum) return summaryDraw();
  s.el.innerHTML = adv.party.map(partyBox).join('')
    + [...Array(MAX_PARTY).keys()].slice(1).filter(k=>!adv.party[k]).map(k=>`<div class="pb pb-small pb-empty" style="${slotPos(k)}"></div>`).join('')
    + `<div class="pty-cancel${s.i===MAX_PARTY?' on':''}" data-k="${MAX_PARTY}">CANCEL</div>
       <div class="pty-msg">${s.swap!=null ? 'Move to where?' : s.msg || 'Choose a POKéMON.'}</div>`;
  s.el.onclick = e=>{ const b = e.target.closest('[data-k]'); if(b){ s.i = +b.dataset.k; uiKey('a'); } };
}
function partyKey(k){
  const s = ptyScr, n = adv.party.length;
  if(s.sum) return summaryKey(k);
  if(k==='b'){
    if(s.swap!=null){ s.swap = null; return partyDraw(); }
    s.el.remove(); ptyScr = null; owBusy = false; return startMenu();
  }
  if(k==='a'){
    if(s.i===MAX_PARTY){ if(s.swap!=null){ s.swap = null; return partyDraw(); } return partyKey('b'); }
    if(s.swap!=null) return partySwap(s.swap, s.i);
    const m = adv.party[s.i];
    s.msg = `Do what with ${dname(m)}?`; partyDraw();
    uiMenu(s.el, ['SUMMARY', 'SWITCH', 'CANCEL'], c=>{
      s.msg = null;
      if(c===0){ s.sum = {page:0}; return partyDraw(); }
      if(c===1 && n>1) s.swap = s.i;
      partyDraw();
    }, 'gm-br');
    return;
  }
  // Up/Down step through the slots and CANCEL; Left/Right move between the lead and the two columns.
  const order = [...Array(n).keys(), MAX_PARTY];
  const at = order.indexOf(s.i);
  if(k==='up') s.i = order[(at + order.length - 1) % order.length];
  if(k==='down') s.i = order[(at + 1) % order.length];
  if(k==='left') s.i = s.i>=6 && s.i<MAX_PARTY ? s.i-5 : 0;
  if(k==='right') s.i = s.i===0 ? (n>1 ? 1 : 0) : s.i>=1 && s.i<=5 && s.i+5<n ? s.i+5 : s.i;
  partyDraw();
}
// SWITCH: both boxes slide off-screen and back with their places traded (~21 frames each way).
function partySwap(a, b){
  const s = ptyScr;
  s.swap = null;
  if(a===b) return partyDraw();
  const boxes = [a,b].map(k=>s.el.querySelector(`.pb[data-k="${k}"]`)).filter(Boolean);
  boxes.forEach(el=>el.animate([{transform:'none'},{transform:`translateX(${el.classList.contains('pb-lead') ? -200 : 300}px)`}], {duration:350, fill:'forwards'}));
  setTimeout(()=>{
    [adv.party[a], adv.party[b]] = [adv.party[b], adv.party[a]];
    saveAdv(); partyDraw();
    [a,b].map(k=>s.el.querySelector(`.pb[data-k="${k}"]`)).forEach(el=>el && el.animate([{transform:`translateX(${el.classList.contains('pb-lead') ? -200 : 300}px)`},{transform:'none'}], {duration:350}));
  }, 350);
}
// SUMMARY: POKéMON INFO / POKéMON SKILLS / BATTLE MOVES. Left/Right change page (no wrap), sliding
// in from the right; Up/Down change Pokémon; B goes back to the party.
const SUM_PAGES = ['POKéMON INFO', 'POKéMON SKILLS', 'BATTLE MOVES'];
function summaryDraw(dir){
  const s = ptyScr, m = adv.party[s.i], p = s.sum.page, cls = hpClass(m);
  const body = p===0 ? `<div class="sm-rows"><div>TYPE <b>${m.types.map(t=>t.toUpperCase()).join('/')}</b></div>
        <div>ABILITY <b>${(m.ability&&m.ability.n||'-').toUpperCase()}</b></div><div>OT <b>${adv.playerName}</b></div>
        <div>EXP. POINTS <b>${m.xp||0}</b></div><div>NEXT LV. <b>${Math.max(0,(m.xpNext||0)-(m.xp||0))}</b></div></div>`
    : p===1 ? `<div class="sm-rows"><div>HP <b>${Math.max(0,m.hp)}/${m.maxhp}</b></div><div>ATTACK <b>${m.atk}</b></div><div>DEFENSE <b>${m.def}</b></div>
        <div>SP. ATK <b>${m.spa}</b></div><div>SP. DEF <b>${m.spd}</b></div><div>SPEED <b>${m.spe}</b></div></div>`
    : `<div class="sm-rows">${m.moves.map(mv=>`<div><span class="tbadge" style="background:${TYPE_COLORS[mv.t]||'#888'}">${mv.t}</span> ${mv.n.toUpperCase()} <b>${mv.p?'PWR '+mv.p:'—'}</b></div>`).join('')}</div>`;
  s.el.innerHTML = `<div class="sm">
    <div class="sm-head">${SUM_PAGES.map((t,k)=>`<span class="${k===p?'on':''}">${k===p?t:''}</span>`).join('')}<i>◀▶ PAGE</i></div>
    <div class="sm-left"><img src="${spritePath(m.dex)}" alt=""><div>${dname(m)}</div><div>Lv${m.level}</div>
      <div class="pb-hp"><span>HP</span><i><b class="hp-${cls}" style="width:${Math.max(0,m.hp/m.maxhp)*100}%"></b></i></div></div>
    <div class="sm-page">${body}</div></div>`;
  if(dir) s.el.querySelector('.sm-page').animate([{transform:`translateX(${dir>0?256:-256}px)`},{transform:'none'}], {duration:8000/60, easing:'linear'});
}
function summaryKey(k){
  const s = ptyScr;
  if(k==='b' || k==='a'){ s.sum = null; return partyDraw(); }
  if(k==='left' && s.sum.page>0){ s.sum.page--; return summaryDraw(-1); }
  if(k==='right' && s.sum.page<SUM_PAGES.length-1){ s.sum.page++; return summaryDraw(1); }
  if(k==='up' || k==='down'){ s.i = (s.i + (k==='up' ? adv.party.length-1 : 1)) % adv.party.length; summaryDraw(); }
}

// ---------- Sound effects ----------
// Short original chip-style tones made with Web Audio (no recorded game audio). Each entry is a
// list of [Hz, seconds, wave, volume]; 0 Hz is a rest. OPTION > SOUND turns them off.
const SFX = {
  select:[[1318,.045]], open:[[880,.035],[1318,.05]], bump:[[98,.09,'triangle',.2]],
  jump:[[523,.05],[784,.07]], ball:[[1046,.04,'triangle',.12]], save:[[784,.1],[988,.1],[1175,.1],[1568,.25]],
  pcOn:[[660,.05],[990,.05],[1320,.07]], pcOff:[[1320,.05],[990,.05],[660,.07]], pcLogin:[[990,.04],[1320,.06]],
  spot:[[1568,.05],[2093,.1]], obtain:[[784,.12],[784,.06],[784,.06],[1046,.3],[0,.05],[988,.12],[1175,.35,'triangle',.08]], door:[[392,.05,'triangle',.12],[294,.08,'triangle',.12]],
  heal:[[523,.16],[659,.16],[784,.16],[1046,.32],[0,.08],[880,.16],[988,.16],[1046,.16],[1318,.6,'triangle',.1]]};
let audioCtx = null;
function sfx(name){
  const notes = SFX[name];
  if(!notes || (adv && adv.sound===false) || typeof window==='undefined') return;
  try{ audioCtx ||= new (window.AudioContext || window.webkitAudioContext)(); }catch(e){ return; }
  let t = audioCtx.currentTime;
  for(const [hz, dur, wave='square', vol=.05] of notes){
    if(hz){
      const o = audioCtx.createOscillator(), g = audioCtx.createGain();
      o.type = wave; o.frequency.value = hz;
      g.gain.setValueAtTime(vol, t); g.gain.exponentialRampToValueAtTime(.0001, t + dur);
      o.connect(g).connect(audioCtx.destination); o.start(t); o.stop(t + dur);
    }
    t += dur;
  }
}

// ---------- Full-screen pages opened from START (BAG, OPTION) ----------
// uiScr is the open page: {el, key(k)}. Closing goes back to the START menu, as in Emerald.
let uiScr = null;
function scrOpen(cls, key){
  owBusy = true;
  const el = document.createElement('div');
  el.className = cls;
  document.getElementById('owView').appendChild(el);
  return uiScr = {el, key};
}
function scrClose(){ uiScr.el.remove(); uiScr = null; owBusy = false; startMenu(); }

// BAG: five pockets; Left/Right change pocket (the bag hops), Up/Down move the cursor (it shakes).
const POCKETS = ['ITEMS', 'POKé BALLS', 'TMs & HMs', 'BERRIES', 'KEY ITEMS'];
const ITEM_INFO = {pokeball:{name:'POKé BALL', pocket:1, desc:'A tool for catching wild POKéMON.', price:200},
  potion:{name:'POTION', pocket:0, desc:'Restores the HP of a POKéMON by 20 points.', price:300, heal:20},
  superpotion:{name:'SUPER POTION', pocket:0, desc:'Restores the HP of a POKéMON by 50 points.', price:700, heal:50},
  antidote:{name:'ANTIDOTE', pocket:0, desc:'Heals a poisoned POKéMON.', price:100, cure:'psn'},
  parlyzheal:{name:'PARLYZ HEAL', pocket:0, desc:'Heals a paralyzed POKéMON.', price:200, cure:'par'},
  awakening:{name:'AWAKENING', pocket:0, desc:'Awakens a sleeping POKéMON.', price:250, cure:'slp'},
  burnheal:{name:'BURN HEAL', pocket:0, desc:'Heals a POKéMON of a burn.', price:250, cure:'brn'}};
const CURED = {psn:'poisoning', par:'paralysis', slp:'sleep', brn:'its burn'};
// POTION heals 20 HP (not a fainted Pokémon). Returns the message, or null if it would do nothing.
// Use a medicine on m. Returns the message, or null if it would do nothing (nothing is used up).
function useItem(id, m){
  const it = ITEM_INFO[id];
  if(!it || !adv.items[id] || m.fainted || m.hp<=0) return null;
  if(it.heal){
    if(m.hp>=m.maxhp) return null;
    adv.items[id]--;
    const before = m.hp; m.hp = Math.min(m.maxhp, m.hp + it.heal);
    return `${dname(m)}'s HP was restored by ${m.hp - before} points.`;
  }
  if(it.cure && m.status===it.cure){
    adv.items[id]--; m.status = null; m.sleepTurns = 0;
    return `${dname(m)} was cured of ${CURED[it.cure]}.`;
  }
  return null;
}
const isMedicine = id=>ITEM_INFO[id] && (ITEM_INFO[id].heal || ITEM_INFO[id].cure);
let bagPocket = 1;
function bagOpen(){
  const s = scrOpen('bag', k=>{
    const list = bagList();
    if(k==='b' || k==='a' && s.i===list.length) return scrClose();
    if(k==='a'){
      const it = list[s.i];
      if(!it || !isMedicine(it.id)) return;   // only medicine works from the field
      return uiMenu(s.el, adv.party.map(m=>`${dname(m)} ${Math.max(0,m.hp)}/${m.maxhp}`).concat('CANCEL'), p=>{
        const m = adv.party[p]; if(!m) return;
        const said = useItem(it.id, m);
        s.msg = said || "It won't have any effect."; saveAdv(); bagDraw();
      }, 'gm-br');
    }
    if(k==='left' || k==='right'){ bagPocket = (bagPocket + (k==='left' ? 4 : 1)) % 5; s.i = 0; bagDraw('hop'); }
    if(k==='up' || k==='down'){ s.i = (s.i + (k==='up' ? list.length : 1)) % (list.length+1); bagDraw('shake'); }
  });
  s.i = 0; bagDraw();
}
function bagList(){ return Object.entries(adv.items).filter(([id,n])=>n>0 && ITEM_INFO[id] && ITEM_INFO[id].pocket===bagPocket).map(([id,n])=>({...ITEM_INFO[id], id, n})); }
function bagDraw(anim){
  const s = uiScr, list = bagList(), cur = list[s.i];
  s.el.innerHTML = `<div class="bag-title">◀ ${POCKETS[bagPocket]} ▶</div>
    <div class="bag-dots">${POCKETS.map((p,k)=>`<i class="${k===bagPocket?'on':''}"></i>`).join('')}</div>
    <div class="bag-sprite bag-p${bagPocket}"></div>
    <div class="bag-list">${list.map((it,k)=>`<div class="${k===s.i?'on':''}">${it.name}<b>×${it.n}</b></div>`).join('')}<div class="${s.i===list.length?'on':''}">CLOSE BAG</div></div>
    <div class="bag-desc">${s.msg || (cur ? cur.desc : 'CLOSE BAG')}</div>`;
  s.msg = null;
  const spr = s.el.querySelector('.bag-sprite');
  if(anim==='hop') spr.animate([{transform:'none'},{transform:'translateY(-10px)'},{transform:'none'}], {duration:5000/60*2});
  if(anim==='shake') spr.animate([{transform:'rotate(0)'},{transform:'rotate(-6deg)'},{transform:'rotate(6deg)'},{transform:'rotate(0)'}], {duration:12000/60});
}

// OPTION: only the rows that mean something here. Left/Right change the value (wraps); the
// chosen value is red; B or CANCEL saves and closes. Emerald plays no sounds on this page.
function optionOpen(){
  const rows = [['TEXT SPEED', 'textSpeed', ['SLOW','MID','FAST'], 'MID'], ['SOUND', 'sound', ['ON','OFF'], 'ON']];
  const val = r=> r[1]==='sound' ? (adv.sound===false ? 'OFF' : 'ON') : (adv[r[1]] || r[3]);
  const s = scrOpen('opt', k=>{
    if(k==='b' || k==='a' && s.i===rows.length){ saveAdv(); return scrClose(); }
    if(k==='up' || k==='down') s.i = (s.i + (k==='up' ? rows.length : 1)) % (rows.length+1);
    const r = rows[s.i];
    if(r && (k==='left' || k==='right')){
      const v = r[2][(r[2].indexOf(val(r)) + (k==='left' ? r[2].length-1 : 1)) % r[2].length];
      if(r[1]==='sound') adv.sound = v==='ON'; else adv[r[1]] = v;
    }
    draw();
  });
  s.silent = true;   // Emerald plays no sounds on this page
  const draw = ()=>{ s.el.innerHTML = `<div class="opt-title">OPTION</div><div class="opt-rows">
    ${rows.map((r,k)=>`<div class="${s.i===k?'on':''}">${r[0]}<span>${r[2].map(v=>`<em class="${v===val(r)?'sel':''}">${v}</em>`).join('')}</span></div>`).join('')}
    <div class="${s.i===rows.length?'on':''}">CANCEL</div></div>`; };
  s.i = 0; draw();
}

// ---------- Playtest feedback ----------
// SEND posts to FEEDBACK_ENDPOINT, a relay (relay/worker.js, a Cloudflare Worker) that files the GitHub
// issue with a token only it holds, so testers stay anonymous. With no endpoint set, or if the relay
// can't be reached, it falls back to a pre-filled GitHub issue link (that needs a GitHub account).
// Either way the game adds where they are and what they carry.
const GAME_VERSION = '0.9.5-playtest';   // bump on each push so reports show which build they came from
const FEEDBACK_REPO = 'romrepostacks/romv22';   // set to the GitHub repo that should receive issues
const FEEDBACK_ENDPOINT = 'https://party-royale-feedback.kylemeadows.workers.dev';                    // the Worker's URL, e.g. https://party-royale-feedback.<you>.workers.dev
const FEEDBACK_KINDS = ['Bug', 'Looks wrong', 'Feels off', 'Idea', 'Praise'];
function feedbackContext(){
  const loc = LOCATIONS[adv.loc];
  return ['', '', '---', `Version: ${GAME_VERSION}`, `Where: ${loc ? loc.name : '?'}${adv.inside ? ' (inside '+adv.inside+')' : ''} @ ${adv.pos ? adv.pos.x+','+adv.pos.y : '?'}`,
    `Party: ${(adv.party||[]).map(m=>m.name+' L'+m.level).join(', ') || 'none'}`, `Badges: ${LOCATIONS.filter(l=>l.type==='gym' && adv.cleared[l.name]).length}`,
    `Device: ${innerWidth}×${innerHeight} @${devicePixelRatio}x, ${matchMedia('(display-mode: standalone)').matches ? 'installed' : 'browser'}`, `UA: ${navigator.userAgent}`].join('\n');
}
function feedbackOpen(){
  owBusy = true;
  const el = document.createElement('div');
  el.className = 'fb gba-menu';
  el.innerHTML = `<div class="fb-title">FEEDBACK</div>
    <div class="fb-kinds">${FEEDBACK_KINDS.map((k,i)=>`<label><input type="radio" name="fbk" value="${k}" ${i?'':'checked'}>${k}</label>`).join('')}</div>
    <textarea maxlength="2000" placeholder="What happened, or what should feel different?"></textarea>
    <input class="fb-hp" name="website" tabindex="-1" autocomplete="off" aria-hidden="true">
    <div class="fb-btns"><button class="fb-send">SEND</button><button class="fb-cancel">CANCEL</button></div>
    <div class="fb-note">${FEEDBACK_ENDPOINT ? 'Sent anonymously, no account needed. Game details are included.' : 'Opens GitHub with your note and game details filled in.'}</div>`;
  document.getElementById('owView').appendChild(el);
  const ta = el.querySelector('textarea');
  setTimeout(()=>ta.focus(), 0);
  const close = ()=>{ el.remove(); owBusy = false; startMenu(); };
  el.querySelector('.fb-cancel').onclick = close;
  el.addEventListener('keydown', e=>{ if(e.key==='Escape') close(); });
  const send = el.querySelector('.fb-send'), note = el.querySelector('.fb-note');
  send.onclick = ()=>{
    const text = ta.value.trim();
    if(!text){ ta.focus(); return; }
    const kind = el.querySelector('input[name=fbk]:checked').value;
    const q = new URLSearchParams({title: `[${kind}] ${text.split('\n')[0].slice(0, 60)}`, body: text + feedbackContext()});
    const github = `https://github.com/${FEEDBACK_REPO}/issues/new?${q}`;
    const thanks = ()=>{ close(); owSay(['Thanks! Your feedback helps shape the game.']); };
    if(!FEEDBACK_ENDPOINT){ window.open(github, '_blank', 'noopener'); return thanks(); }
    send.disabled = true; send.textContent = 'SENDING…';
    fetch(FEEDBACK_ENDPOINT, {method:'POST', headers:{'Content-Type':'application/json'},
      body: JSON.stringify({kind, text, context: feedbackContext(), website: el.querySelector('.fb-hp').value})})
      .then(r=>{ if(!r.ok) throw new Error(r.status); thanks(); })
      .catch(()=>{   // the popup has to come from a tap, so offer the GitHub link rather than opening it
        send.disabled = false; send.textContent = 'SEND';
        note.innerHTML = `Couldn't send just now. Try again in a minute, or <a href="${github}" target="_blank" rel="noopener">post it on GitHub</a>.`;
      });
  };
}

// Poké Mart (Emerald): "How may I serve you?" BUY / SEE YA!, a price list with your money shown
// top-left, a quantity, "That will be ₽X. OK?", then "Here you go! Thank you very much."
function martOpen(){
  adv.money ??= 3000;
  owBusy = true;
  const view = document.getElementById('owView'), wallet = document.createElement('div');
  wallet.className = 'gba-menu gm-saveinfo';
  const showMoney = ()=>{ wallet.innerHTML = `MONEY <b>₽${adv.money}</b>`; };
  showMoney(); view.appendChild(wallet);
  const bye = ()=>{ wallet.remove(); owPromptClose(); owSay(['Please come again!']); };
  const top = ()=>{
    owPrompt('Welcome! How may I serve you?');
    uiMenu(view, ['BUY', 'SELL', 'SEE YA!'], k=>k===0 ? list() : k===1 ? sell() : bye(), 'gm-br');
  };
  // SELL: anything you carry, at half price.
  const sell = ()=>{
    const have = Object.keys(ITEM_INFO).filter(id=>adv.items[id]>0);
    if(!have.length){ owPrompt("You don't have anything to sell."); return setTimeout(top, 1200); }
    owPrompt('What would you like to sell?');
    uiMenu(view, have.map(id=>`${ITEM_INFO[id].name} ×${adv.items[id]}`).concat('CANCEL'), k=>{
      const id = have[k]; if(!id) return top();
      const it = ITEM_INFO[id], each = it.price/2, qs = [...new Set([1, 5, adv.items[id]])].filter(q=>q<=adv.items[id]);
      owPrompt(`How many would you like to sell?`);
      uiMenu(view, qs.map(q=>`×${q}  ₽${q*each}`).concat('CANCEL'), qi=>{
        const q = qs[qi]; if(!q) return sell();
        owPrompt(`I can pay ₽${q*each}. Would that be OK?`);
        uiMenu(view, ['YES', 'NO'], y=>{
          if(y!==0) return sell();
          adv.items[id] -= q; adv.money += q*each; saveAdv(); showMoney();
          owPrompt(`Turned over the ${it.name} and received ₽${q*each}.`);
          setTimeout(sell, 1100);
        }, 'gm-yesno');
      }, 'gm-br');
    }, 'gm-br');
  };
  const badges = LOCATIONS.filter(l=>l.type==='gym' && adv.cleared[l.name]).length;
  const stock = ['pokeball', 'potion', 'antidote', 'parlyzheal', 'awakening'].concat(badges>=2 ? ['superpotion', 'burnheal'] : []);
  const list = ()=>{
    owPrompt('What would you like?');
    uiMenu(view, stock.map(id=>`${ITEM_INFO[id].name}  ₽${ITEM_INFO[id].price}`).concat('CANCEL'), k=>{
      const id = stock[k]; if(!id) return top();
      const it = ITEM_INFO[id], max = Math.min(99, Math.floor(adv.money/it.price));
      if(!max){ owPrompt("You don't have enough money."); return setTimeout(list, 1200); }
      const qs = [1, 5, 10].filter(q=>q<=max);
      owPrompt(`${it.name}? Certainly. How many would you like?`);
      uiMenu(view, qs.map(q=>`×${q}  ₽${q*it.price}`).concat('CANCEL'), qi=>{
        const q = qs[qi]; if(!q) return list();
        owPrompt(`${it.name}, and you want ${q}? That will be ₽${q*it.price}. OK?`);
        uiMenu(view, ['YES', 'NO'], y=>{
          if(y!==0) return list();
          adv.money -= q*it.price; adv.items[id] = (adv.items[id]||0) + q; saveAdv(); showMoney();
          sfx('ball');
          owPrompt('Here you go! Thank you very much.');
          setTimeout(list, 900);
        }, 'gm-yesno');
      }, 'gm-br');
    }, 'gm-br');
  };
  top();
}

// Emerald's item fanfare: "Obtained ..." while the jingle plays, then where it went.
function obtainItem(what, n, pocket, done){
  sfx('obtain');
  owSayHold(`Obtained ${n>1 ? n+' ' : 'the '}${what}!`, ()=>setTimeout(()=>{
    owHold = false;
    owSay([`${adv.playerName} put away the ${what}\nin the ${pocket} POCKET.`], done);
  }, 700));
}

// TRAINER CARD: green (no stars yet), name / ID No. / POKéDEX / TIME with a blinking colon, and
// the eight badge slots. A flips it (a vertical squash), B closes back to START.
if(typeof setInterval==='function') setInterval(()=>{ if(typeof adv!=='undefined' && adv && !document.hidden) adv.playSec = (adv.playSec||0) + 1; }, 1000);
function cardOpen(){
  adv.tid ||= String(Math.floor(Math.random()*65536)).padStart(5,'0');
  sfx('pcLogin');
  const s = scrOpen('tcard', k=>{
    if(k==='b'){ clearInterval(s.tick); return scrClose(); }
    if(k==='a'){
      const card = s.el.querySelector('.tc');
      card.animate([{transform:'scaleY(1)'},{transform:'scaleY(0)'}], {duration:150}).onfinish = ()=>{
        s.back = !s.back; draw();
        s.el.querySelector('.tc').animate([{transform:'scaleY(0)'},{transform:'scaleY(1)'}], {duration:150});
      };
    }
  });
  const gyms = LOCATIONS.filter(l=>l.type==='gym');
  const draw = ()=>{
    const t = adv.playSec||0, colon = t%2 ? ' ' : ':';
    s.el.innerHTML = s.back
      ? `<div class="tc tc-back"><div class="tc-head">${adv.playerName}</div>
          <div class="tc-row">AREAS VISITED<b>${Object.keys(adv.visited||{}).length}</b></div>
          <div class="tc-row">RIVAL BATTLES WON<b>${LOCATIONS.filter(l=>l.type==='trainer' && adv.cleared[l.name]).length}</b></div>
          <div class="tc-row">POKéMON IN BOXES<b>${adv.box.filter(Boolean).length}</b></div></div>`
      : `<div class="tc"><div class="tc-head">TRAINER CARD<span>IDNo.${adv.tid}</span></div>
          <div class="tc-row">NAME<b>${adv.playerName}</b></div>
          <div class="tc-row">MONEY<b>₽${adv.money ?? 3000}</b></div>
          <div class="tc-row">POKéDEX<b>${Object.keys(adv.owned||{}).length}</b></div>
          <div class="tc-row">TIME<b>${Math.floor(t/3600)}<span class="tc-colon">${colon}</span>${String(Math.floor(t/60)%60).padStart(2,'0')}</b></div>
          <div class="tc-player">${charSvg('player', 'down', 0)}</div>
          <div class="tc-badges">${gyms.map(g=>`<i class="${adv.cleared[g.name]?'on':''}"></i>`).join('')}${'<i></i>'.repeat(Math.max(0,8-gyms.length))}</div></div>`;
  };
  s.tick = setInterval(()=>{ if(uiScr===s && !s.back) draw(); }, 1000);
  draw();
}

// ---------- GBA menus + the PC (Pokémon Storage System) ----------
// Follows pokeemerald: boot flicker → "{PLAYER} booted up the PC." → SOMEONE'S PC / LOG OFF →
// WITHDRAW / DEPOSIT / SEE YA! with a live description → the box screen (14 boxes × 30, 6 per row).
// Boxes live in adv.box as one list: box b holds entries b*30 … b*30+29.
const uiMenus = [];   // open menus, top one gets the keys
function uiMenu(host, items, cb, cls, onMove){
  const el = document.createElement('div');
  el.className = 'gba-menu ' + (cls||'');
  host.appendChild(el);
  const m = {items, i:0, cb, el, onMove};
  el.addEventListener('click', e=>{ const r = e.target.closest('[data-k]'); if(r){ m.i = +r.dataset.k; uiMenuPick(m, m.i); } });
  uiMenus.push(m); uiMenuDraw(m);
  return m;
}
function uiMenuDraw(m){
  m.el.innerHTML = m.items.map((t,k)=>`<div class="gm-row${k===m.i?' on':''}" data-k="${k}">${t}</div>`).join('');
  if(m.onMove) m.onMove(m.i);
}
function uiMenuPick(m, k){ m.el.remove(); uiMenus.splice(uiMenus.indexOf(m), 1); m.cb(k); }
// Text in the bottom box that stays up while a menu is open (no typing, no ▼).
function owPrompt(text){
  const box = document.getElementById('owText');
  document.getElementById('owTextBody').textContent = text;
  box.classList.remove('hidden'); box.classList.add('typing');
}
function owPromptClose(){ document.getElementById('owText').classList.add('hidden'); owBusy = false; }

function usePC(){
  sfx('pcOn');
  owBusy = true; held.length = 0;
  // The screen tile flickers on: 5 toggles, one every 6 frames.
  const [dx,dy] = DIRS[adv.facing];
  const pc = document.querySelector(`#owTiles > [data-x="${adv.pos.x+dx}"][data-y="${adv.pos.y+dy}"]`);
  for(let i=1;i<=5;i++) setTimeout(()=>pc && pc.classList.toggle('on'), i*100);
  setTimeout(()=>owSay([`${adv.playerName} booted up the PC.`], pcTopMenu), 520);
}
function pcTopMenu(){
  owBusy = true;
  owPrompt('Which PC should be accessed?');
  uiMenu(document.getElementById('owView'), ["SOMEONE'S PC", `${adv.playerName.toUpperCase()}'s PC`, 'LOG OFF'], k=>{
    if(k===1){ sfx('pcLogin'); return owSay([`Accessed ${adv.playerName}'s PC.`], playerPC); }
    if(k!==0) return pcLogOff();
    sfx('pcLogin');
    owSay(["Accessed SOMEONE'S PC.", 'POKéMON Storage System opened.'], pcStorageMenu);
  }, 'gm-topleft');
}
// {PLAYER}'s PC (Emerald): ITEM STORAGE → WITHDRAW ITEM / DEPOSIT ITEM, with a POTION stored to begin with.
function playerPC(){
  owBusy = true;
  adv.pcItems ??= {potion:1};
  const view = document.getElementById('owView');
  const items = src=>Object.keys(ITEM_INFO).filter(id=>src[id]>0);
  const move = (from, to, verb)=>{
    const have = items(from);
    if(!have.length){ owPrompt(verb==='WITHDRAW' ? 'There are no items.' : "You don't have any items."); return setTimeout(storage, 1100); }
    owPrompt(verb==='WITHDRAW' ? 'Withdraw which item?' : 'Deposit which item?');
    uiMenu(view, have.map(id=>`${ITEM_INFO[id].name} ×${from[id]}`).concat('CANCEL'), k=>{
      const id = have[k]; if(!id) return storage();
      from[id]--; to[id] = (to[id]||0) + 1; saveAdv();
      owPrompt(verb==='WITHDRAW' ? `Withdrew ${ITEM_INFO[id].name}.` : `Deposited ${ITEM_INFO[id].name}.`);
      setTimeout(()=>move(from, to, verb), 700);
    }, 'gm-topleft');
  };
  const storage = ()=>{
    owPrompt('What would you like to do?');
    uiMenu(view, ['WITHDRAW ITEM', 'DEPOSIT ITEM', 'CANCEL'], k=>{
      if(k===0) return move(adv.pcItems, adv.items, 'WITHDRAW');
      if(k===1) return move(adv.items, adv.pcItems, 'DEPOSIT');
      pcTopMenu();
    }, 'gm-topleft', i=>owPrompt(['Take out items from the PC.', 'Store items in the PC.', 'Go back to the previous menu.'][i]));
  };
  storage();
}
function pcLogOff(){
  sfx('pcOff');
  document.querySelectorAll('#owWorld .t-pc.on').forEach(el=>el.classList.remove('on'));
  owPromptClose();
}
const PC_OPTS = [['WITHDRAW POKéMON', 'Move POKéMON stored in BOXES to\nyour party.'], ['DEPOSIT POKéMON', 'Store POKéMON in your party in BOXES.'],
  ['MOVE POKéMON', 'Organize the POKéMON in BOXES and\nin your party.'], ['SEE YA!', 'Return to the previous menu.']];
function pcStorageMenu(start){
  owBusy = true;
  const m = uiMenu(document.getElementById('owView'), PC_OPTS.map(o=>o[0]), k=>{
    if(k<0 || k===3) return pcTopMenu();
    // Guards, as in Emerald: the description line is replaced and the menu stays up.
    const stop = k===0 && adv.party.length>=MAX_PARTY ? 'Your party is full!' : k===1 && adv.party.length<=1 ? 'There is just one POKéMON with you.' : k===0 && !adv.box.length ? 'There are no POKéMON in the BOXES.' : null;
    if(stop){ pcStorageMenu(k); owPrompt(stop); return; }
    owPromptClose(); owBusy = true;
    boxOpen(['withdraw', 'deposit', 'move'][k]);
  }, 'gm-topleft', i=>owPrompt(PC_OPTS[i][1]));
  if(start){ m.i = start; uiMenuDraw(m); }
}

// Emerald's 16 wallpapers, in its order; the WALLPAPER menu groups them in fours.
const WALLPAPERS = [['#78c060','#58a048'],['#a8b0c0','#8890a8'],['#e8c878','#d0a858'],['#d8b060','#b89040'],['#b89878','#987858'],['#e07050','#b85038'],['#e0f0f8','#b8d8f0'],['#8878a8','#685888'],['#f0e0a0','#78c8f0'],['#3868a8','#284888'],['#68b0e0','#4890c8'],['#a0d0f8','#f8f8f8'],['#f8b8c8','#f8f8f8'],['#f8a0a0','#f8f8f8'],['#98a0a8','#707880'],['#d8d8d8','#c0c0c0']];
const WALL_NAMES = [['SCENERY 1', ['FOREST','CITY','DESERT','SAVANNA']], ['SCENERY 2', ['CRAG','VOLCANO','SNOW','CAVE']], ['SCENERY 3', ['BEACH','SEAFLOOR','RIVER','SKY']], ['ETCETERA', ['POLKA-DOT','POKéCENTER','MACHINE','SIMPLE']]];
const boxName = i=>(adv.boxNames && adv.boxNames[i]) || `BOX${i+1}`;
const boxWall = i=>WALLPAPERS[adv.boxWall && adv.boxWall[i]!=null ? adv.boxWall[i] : i % 16];
// {mode, box, r, c (r=-1: box title; c=-1: party column in MOVE mode), p (party row), held (MOVE: the Pokémon in the hand), el}
let pcBox = null;
function boxOpen(mode){
  const el = document.createElement('div');
  el.className = 'pc-box';
  document.getElementById('owView').appendChild(el);
  pcBox = {mode, box:0, r:0, c:0, p:0, el};
  boxDraw();
}
const inParty = ()=>pcBox.mode==='deposit' || pcBox.c<0;
function boxMon(){ const b = pcBox; return inParty() ? adv.party[b.p] : b.r>=0 ? adv.box[b.box*30 + b.r*6 + b.c] : null; }
function boxDraw(){
  const b = pcBox, wp = boxWall(b.box), m = boxMon() || b.held;
  const slots = [];
  for(let s=0;s<30;s++){ const mon = adv.box[b.box*30+s];
    slots.push(`<div class="pb-slot" data-s="${s}">${mon ? `<img src="${spritePath(mon.dex)}" alt="">` : ''}</div>`); }
  const hand = b.mode==='deposit' ? `left:${b.p===MAX_PARTY ? 440 : 294+Math.floor(b.p/5)*143}px; top:${10+(b.p===MAX_PARTY ? 5 : b.p%5)*34}px` : b.c<0 ? `left:${34+(b.p%2)*78}px; top:${158+Math.floor(b.p/2)*31}px`
    : b.r<0 ? `left:310px; top:8px` : `left:${196+b.c*48}px; top:${46+b.r*44}px`;
  // MOVE mode shows the party as a 2×3 grid where the info panel usually is.
  const info = b.mode==='move'
    ? `<div class="pb-pmini">${[...Array(MAX_PARTY).keys()].map(i=>`<div class="${b.c<0 && b.p===i ? 'on' : ''}" data-p="${i}">${adv.party[i] ? `<img src="${spritePath(adv.party[i].dex)}" alt="">` : ''}</div>`).join('')}</div>`
    : `<div class="pb-info">${m ? `${dname(m)}<br><small>/${m.name.toUpperCase()}</small><br><small>Lv${m.level}</small>` : ''}</div>`;
  b.el.innerHTML = `
    <div class="pb-left"><div class="pb-label${m?' on':''}">POKéMON DATA</div>
      <div class="pb-sprite">${m ? `<img src="${spritePath(m.dex)}" alt="">` : ''}</div>${info}</div>
    ${b.mode==='deposit'
      ? `<div class="pb-party">${[...Array(MAX_PARTY).keys()].map(i=>`<div class="pb-prow${i===b.p?' on':''}" data-p="${i}">${adv.party[i] ? `<img src="${spritePath(adv.party[i].dex)}" alt="">${dname(adv.party[i])} <small>Lv${adv.party[i].level}</small>` : ''}</div>`).join('')}
         <div class="pb-prow pb-cancel${b.p===MAX_PARTY?' on':''}" data-p="${MAX_PARTY}">CANCEL</div></div>`
      : `<div class="pb-title" style="--wp:${wp[0]}"><span class="pb-arrow" data-a="-1">◀</span>${boxName(b.box)}<span class="pb-arrow" data-a="1">▶</span></div>
         <div class="pb-grid" style="background:repeating-linear-gradient(45deg,${wp[0]} 0 12px,${wp[1]} 12px 24px)">${slots.join('')}</div>`}
    <div class="pb-hand${b.held ? ' fist' : ''}" style="${hand}">${b.held ? `<img src="${spritePath(b.held.dex)}" alt="">` : ''}</div>
    <div class="pb-msg">${b.msg || (b.mode==='deposit' ? 'Which POKéMON will you deposit?'
      : b.held ? `Holding ${dname(b.held)}. Where to?`
      : `${adv.box.slice(b.box*30,b.box*30+30).filter(Boolean).length}/30  ◀▶ on the title to change BOX`)}</div>`;
  b.el.onclick = e=>{
    const s = e.target.closest('[data-s]'), p = e.target.closest('[data-p]'), a = e.target.closest('[data-a]');
    if(a) return boxScroll(+a.dataset.a);
    if(s){ b.r = Math.floor(s.dataset.s/6); b.c = s.dataset.s%6; boxDraw(); return uiKey('a'); }
    if(e.target.closest('.pb-title')){ b.r = -1; if(b.c<0) b.c = 0; boxDraw(); return uiKey('a'); }
    if(p){ b.p = +p.dataset.p; if(b.mode==='move') b.c = -1; boxDraw(); return uiKey('a'); }
  };
}
function boxScroll(d){
  pcBox.box = (pcBox.box + d + 14) % 14;
  const g = pcBox.el.querySelector('.pb-grid');
  boxDraw();
  const n = pcBox.el.querySelector('.pb-grid');
  if(g && n) n.animate([{transform:`translateX(${d*100}%)`},{transform:'none'}], {duration:32000/60/2, easing:'linear'});
}
// One-line messages inside the box screen; A/B moves on.
function boxSay(lines, done){ pcBox.say = {lines:lines.slice(), done}; boxSayNext(); }
function boxSayNext(){
  const s = pcBox.say;
  if(!s.lines.length){ pcBox.say = null; pcBox.msg = null; boxDraw(); return s.done && s.done(); }
  pcBox.msg = s.lines.shift(); boxDraw();
}
function boxClose(){
  pcBox.el.remove(); pcBox = null;
  renderAdventure(); saveAdv();
  pcStorageMenu();
}
function boxAction(){
  const b = pcBox, m = boxMon();
  if(b.mode==='deposit' && b.p===MAX_PARTY) return boxClose();
  if(b.r<0 && b.c>=0 && b.mode!=='deposit') return boxTitleMenu();
  if(b.held) return boxPlace();
  if(!m) return;
  const first = {deposit:'STORE', withdraw:'WITHDRAW', move:'MOVE'}[b.mode];
  b.msg = `${dname(m)} is selected.`; boxDraw();
  uiMenu(b.el, [first, 'RELEASE', 'CANCEL'], k=>{
    b.msg = null;
    if(k===0) return b.mode==='deposit' ? boxStore(m) : b.mode==='withdraw' ? boxWithdraw(m) : boxGrab(m);
    if(k===1) return boxRelease(m);
    boxDraw();
  }, 'gm-br');
}
// MOVE: the hand takes the Pokémon out of its slot (party lists close up behind it)...
function boxGrab(m){
  const b = pcBox;
  if(inParty()){
    if(adv.party.length<=1) return boxSay(["That's your last POKéMON!"]);
    adv.party.splice(b.p, 1);
  } else adv.box[b.box*30 + b.r*6 + b.c] = null;
  b.held = m; boxDraw();
}
// ...and puts it down: an empty spot takes it, an occupied one swaps (you then hold the other).
function boxPlace(){
  const b = pcBox, m = b.held;
  if(inParty()){
    const other = adv.party[b.p];
    if(other) adv.party[b.p] = m; else adv.party.push(m);
    b.held = other || null;
  } else {
    const s = b.box*30 + b.r*6 + b.c, other = adv.box[s];
    adv.box[s] = m; b.held = other || null;
  }
  while(adv.box.length && !adv.box[adv.box.length-1]) adv.box.pop();
  saveAdv(); boxDraw();
}
// A on the box title: WALLPAPER (Emerald's four theme groups) / NAME / CANCEL.
function boxTitleMenu(){
  const b = pcBox;
  b.msg = 'What do you want to do?'; boxDraw();
  uiMenu(b.el, ['WALLPAPER', 'NAME', 'CANCEL'], k=>{
    b.msg = null; boxDraw();
    if(k===0){
      b.msg = 'Pick a theme.'; boxDraw();
      return uiMenu(b.el, WALL_NAMES.map(g=>g[0]).concat('CANCEL'), g=>{
        if(g<0 || g>3){ b.msg = null; return boxDraw(); }
        b.msg = 'Pick the wallpaper.'; boxDraw();
        uiMenu(b.el, WALL_NAMES[g][1], w=>{
          b.msg = null;
          if(w>=0){ (adv.boxWall ||= [])[b.box] = g*4 + w; saveAdv(); }
          boxDraw();
          const grid = b.el.querySelector('.pb-grid');
          if(grid && w>=0) grid.animate([{filter:'brightness(3)'},{filter:'none'}], {duration:400});   // fade through white
        }, 'gm-br');
      }, 'gm-br');
    }
    if(k===1) showConfirm(`${boxName(b.box)}'s name?`, v=>{
      if(typeof v==='string'){ (adv.boxNames ||= [])[b.box] = v.trim().toUpperCase().slice(0,8) || null; saveAdv(); }
      boxDraw();
    }, true, boxName(b.box));
  }, 'gm-br');
}
function boxWithdraw(m){
  if(adv.party.length>=MAX_PARTY) return boxSay(["Your party's full!"]);
  adv.box[adv.box.indexOf(m)] = null; adv.party.push(m);
  while(adv.box.length && !adv.box[adv.box.length-1]) adv.box.pop();
  boxSay([`${dname(m)} was withdrawn.`]);
}
function boxStore(m){
  if(adv.party.length<=1) return boxSay(["That's your last POKéMON!"]);
  let s = adv.box.findIndex(x=>!x);   // first free slot, box by box
  if(s<0) s = adv.box.length;
  if(s >= 14*30) return boxSay(['The BOXES are full.']);
  adv.box[s] = m; adv.party.splice(adv.party.indexOf(m), 1);
  pcBox.p = Math.min(pcBox.p, adv.party.length-1);
  boxSay([`${dname(m)} was deposited in BOX${Math.floor(s/30)+1}.`]);
}
function boxRelease(m){
  const fromParty = inParty();
  if(fromParty && adv.party.length<=1) return boxSay(["That's your last POKéMON!"]);
  pcBox.msg = 'Release this POKéMON?'; boxDraw();
  uiMenu(pcBox.el, ['YES', 'NO'], k=>{
    if(k!==0){ pcBox.msg = null; return boxDraw(); }
    // The icon shrinks away over 120 frames, then the goodbyes.
    const img = pcBox.el.querySelector(pcBox.mode==='deposit' ? '.pb-prow.on img' : fromParty ? '.pb-pmini .on img' : `.pb-slot[data-s="${pcBox.r*6+pcBox.c}"] img`) || pcBox.el.querySelector('.pb-sprite img');
    pcBox.busy = true;
    const done = ()=>{
      pcBox.busy = false;
      if(fromParty){ adv.party.splice(adv.party.indexOf(m), 1); pcBox.p = Math.min(pcBox.p, adv.party.length-1); }
      else { adv.box[adv.box.indexOf(m)] = null; while(adv.box.length && !adv.box[adv.box.length-1]) adv.box.pop(); }
      boxSay([`${dname(m)} was released.`, `Bye-bye, ${dname(m)}!`]);
    };
    if(img && img.animate) img.animate([{transform:'scale(1)'},{transform:'scale(.06)'}], {duration:2000, fill:'forwards'}).onfinish = done;
    else done();
  }, 'gm-yesno');
}
// Keys for menus and the box screen: 'up','down','left','right','a','b'. True if used.
// Every key a menu or screen uses plays the select blip (Emerald's SE_SELECT); OPTION is silent.
function uiKey(k){
  const used = uiKeyInner(k);
  if(used && !(uiScr && uiScr.silent)) sfx('select');
  return used;
}
function uiKeyInner(k){
  const m = uiMenus[uiMenus.length-1];
  if(m){
    if(k==='up' || k==='down'){ m.i = (m.i + (k==='up' ? m.items.length-1 : 1)) % m.items.length; uiMenuDraw(m); }
    else if(k==='a') uiMenuPick(m, m.i);
    else if(k==='b') uiMenuPick(m, m.items.length===2 && m.items[1]==='NO' ? 1 : -1);
    return true;
  }
  if(dexScr){ dexKey(k); return true; }
  if(ptyScr){ partyKey(k); return true; }
  if(uiScr){ uiScr.key(k); return true; }
  if(!pcBox) return false;
  const b = pcBox;
  if(b.busy) return true;
  if(b.say){ if(k==='a' || k==='b') boxSayNext(); return true; }
  if(k==='b' && b.held) return boxSay(["You're holding a POKéMON!"]), true;
  if(k==='b'){
    b.msg = 'Exit from the BOX?'; boxDraw();
    uiMenu(b.el, ['YES', 'NO'], y=>{ b.msg = null; if(y===0) boxClose(); else boxDraw(); }, 'gm-yesno');
    return true;
  }
  if(k==='a') return boxAction(), true;
  if(b.mode==='deposit'){
    if(k==='up') b.p = (b.p+MAX_PARTY) % (MAX_PARTY+1); if(k==='down') b.p = (b.p+1) % (MAX_PARTY+1);
    if((k==='left' || k==='right') && b.p<MAX_PARTY) b.p = (b.p+5) % MAX_PARTY;
  } else if(b.c<0){
    // MOVE mode, party grid (2 columns × 3 rows); right from its right column goes back to the box.
    if(k==='up') b.p = (b.p+MAX_PARTY-2) % MAX_PARTY; if(k==='down') b.p = (b.p+2) % MAX_PARTY;
    if(k==='left') b.p -= b.p%2;
    if(k==='right'){ if(b.p%2) { b.c = 0; b.r = Math.max(0, Math.min(4, Math.floor(b.p/2)*2)); } else b.p++; }
  } else {
    if(b.r<0 && (k==='left' || k==='right')) return boxScroll(k==='left' ? -1 : 1), true;
    if(k==='left' && b.c===0 && b.mode==='move'){ b.c = -1; b.p = Math.max(0, Math.min(b.r*2+1, b.held ? Math.min(5, adv.party.length) : adv.party.length-1)); }
    else if(k==='left') b.c = (b.c+5) % 6;
    if(k==='right') b.c = (b.c+1) % 6;
    if(k==='up') b.r = b.r<0 ? 4 : b.r-1; if(k==='down') b.r = b.r>=4 ? -1 : b.r+1;
  }
  boxDraw();
  return true;
}

// ---------- Input ----------
if(typeof document.addEventListener==='function'){
  document.addEventListener('keydown', e=>{
    if(e.target.closest && e.target.closest('.fb')) return;   // typing feedback
    const box = document.getElementById('msgBox');
    if(box && !box.classList.contains('hidden')){
      if(!e.repeat && ['enter',' ','z','x','escape','arrowdown'].includes(e.key.toLowerCase())){ e.preventDefault(); advanceMsgBox(); }
      return;
    }
    if(!document.getElementById('confirmModal').classList.contains('hidden')) return;   // typing a name
    if(uiMenus.length || pcBox || dexScr || ptyScr || uiScr){
      const u = {arrowup:'up',w:'up',arrowdown:'down',s:'down',arrowleft:'left',a:'left',arrowright:'right',d:'right',enter:'a',' ':'a',z:'a',x:'b',escape:'b',backspace:'b'}[e.key.toLowerCase()];
      if(u){ e.preventDefault(); if(!e.repeat || /up|down|left|right/.test(u)) uiKey(u); }
      return;
    }
    if(battleKey(e)) return;
    const k = e.key.toLowerCase();
    if(owTextOpen()){
      if(['enter',' ','z','x'].includes(k)){ e.preventDefault(); if(!e.repeat){ if(!owHold) sfx('select'); owAdvance(); } }
      else if(KEYDIR[k]) e.preventDefault();
      return;
    }
    if(k==='shift'){ owRun = true; return; }
    if(!owActive()) return;
    if(KEYDIR[k]){ e.preventDefault(); if(!e.repeat) pressDir(KEYDIR[k]); }
    else if((k==='enter'||k===' '||k==='z') && !e.repeat){
      const f = document.activeElement;
      if(k!=='z' && f && /^(BUTTON|INPUT|SELECT|TEXTAREA)$/.test(f.tagName) && !f.closest('.gb-controls')) return; // let focused controls work
      e.preventDefault(); if(k==='enter') startMenu(); else owInteract();
    }
  });
  document.addEventListener('keyup', e=>{
    const k = e.key.toLowerCase();
    if(k==='shift') owRun = false;
    if(KEYDIR[k]) releaseDir(KEYDIR[k]);
  });
  if(typeof window!=='undefined' && window.addEventListener) window.addEventListener('blur', ()=>{ held.length = 0; owRun = false; });
  // On-screen D-pad (hold to walk), A (read / talk / advance text) and B (hold to run).
  if(typeof document.querySelectorAll==='function') document.querySelectorAll('.gb-controls [data-dir]').forEach(btn=>{
    const d = btn.dataset.dir;
    btn.addEventListener('pointerdown', e=>{ e.preventDefault(); btn.setPointerCapture && btn.setPointerCapture(e.pointerId); if(uiKey(d) || owTextOpen()) return; pressDir(d); });
    ['pointerup','pointercancel','lostpointercapture'].forEach(ev=>btn.addEventListener(ev, ()=>releaseDir(d)));
  });
  // iPhone: cancelling pointer events doesn't stop a held touch selecting text, showing the loupe or the
  // callout (issue #3); only cancelling the touch itself does. Pointer events still arrive, so every
  // control below listens for pointerdown, never click (a cancelled touch sends no click).
  const pad = document.querySelector('.gb-controls');
  if(pad){
    pad.addEventListener('touchstart', e=>e.preventDefault(), {passive:false});
    pad.addEventListener('contextmenu', e=>e.preventDefault());
  }
  const sBtn = document.getElementById('btnStart');
  if(sBtn) sBtn.addEventListener('pointerdown', e=>{ e.preventDefault(); if(uiMenus.length) uiKey('b'); else if(!owTextOpen()) startMenu(); });
  const aBtn = document.getElementById('btnA'), bBtn = document.getElementById('btnB');
  if(aBtn) aBtn.addEventListener('pointerdown', e=>{ e.preventDefault(); if(uiKey('a')) return; if(owTextOpen()){ if(!owHold) sfx('select'); owAdvance(); } else owInteract(); });
  if(bBtn){
    bBtn.addEventListener('pointerdown', e=>{ e.preventDefault(); if(uiKey('b')) return; owRun = true; if(owTextOpen()) owAdvance(); });
    ['pointerup','pointercancel','pointerleave'].forEach(ev=>bBtn.addEventListener(ev, ()=>{ owRun = false; }));
  }
}

function saveAdv(){ try{ localStorage.setItem(SAVE_KEY, JSON.stringify(adv)); }catch(e){} }
function patchAdv(a){
  if(!a) return a;
  for(const m of [...(a.party||[]), ...(a.box||[])]) if(m) delete m.caught;
  if(!a.cleared) a.cleared={};
  if(!a.items) a.items = {pokeball:10};
  if(!a.box) a.box=[];
  if(!a.playerName) a.playerName='Trainer';
  if(!a.visited) a.visited = {};
  for(const m of a.party.concat(a.box)){
    // Always re-link to the live dex entry (saves hold a stale copy of it).
    m.dex = DEX.find(d=>d.name===m.name) || DEX[0];
    if(!m.level){ m.level = 50; m.xp = 0; m.xpNext = m.level*8; }
    if(m.nick===undefined) m.nick=null;
    // Saves from before the real-data update: real types/ability/stats, and the moves it would know.
    if(m.movesV!==2){ m.types = m.dex.types; m.ability = m.dex.ability; m.moves = movesAt(m.dex, m.level); recalcStats(m); m.movesV = 2; }
  }
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
  if(saved && saved.party && (saved.party.length || saved.starterPending)){
    adv = saved;
    showAdvScreens(); renderAdventure();
    if(adv.starterPending && adv.loc===1) starterEvent();
    return;
  }
  beginNewStory();
}

let starterOptions = [];
// A real Grass/Fire/Water starter trio, picked from a random generation each new game.
const STARTER_TRIOS = [["Bulbasaur","Charmander","Squirtle"],["Chikorita","Cyndaquil","Totodile"],["Treecko","Torchic","Mudkip"],
  ["Turtwig","Chimchar","Piplup"],["Snivy","Tepig","Oshawott"],["Chespin","Fennekin","Froakie"],["Rowlet","Litten","Popplio"]];
// A new game opens like Emerald: the professor's welcome and your name, then you start in your
// hometown with no Pokémon. The starter comes from his Bag on Route 1 (starterEvent).
const PROF = 'PROF. HAWTHORN';
const INTRO_LINES = ["Hi there! Sorry to keep you waiting!", "Welcome to the world of POKéMON!",
  `I'm ${PROF}. Around here, people call me the POKéMON PROFESSOR.`,
  "This world is home to creatures called POKéMON. People and POKéMON live side by side here in the VELLORIN region.",
  "Some play with them, some work with them, and some battle alongside them.", "Me? I study POKéMON for a living.",
  "But enough about me. Tell me about you. What's your name?"];
function beginNewStory(){
  const trio = STARTER_TRIOS[Math.floor(Math.random()*STARTER_TRIOS.length)];
  starterOptions = trio.map(n=>DEX.find(d=>d.name===n)).filter(Boolean);
  ['titleScreen','setup','draft','battle','result','storyResult','adv'].forEach(id=>document.getElementById(id).classList.add('hidden'));
  const panel = document.getElementById('starterSelect');
  panel.classList.remove('hidden');
  panel.innerHTML = `<div class="intro-stage"><div class="intro-prof">${charSvg('prof','down',0)}</div>
    <img class="intro-mon" src="${spritePath(dexByName('Lotad'))}" alt="">
    <div class="intro-text" id="introText"></div>
    <div class="intro-name hidden" id="introName"><input id="apcNameInput" type="text" placeholder="Trainer" maxlength="12"><button id="introOk">OK</button></div></div>`;
  let i = 0;
  const say = ()=>{ document.getElementById('introText').textContent = INTRO_LINES[i]; if(i===INTRO_LINES.length-1) askName(); };
  const next = ()=>{ if(i < INTRO_LINES.length-1){ i++; sfx('select'); say(); } };
  const askName = ()=>{
    document.getElementById('introName').classList.remove('hidden');
    const input = document.getElementById('apcNameInput'); input.focus();
    const done = ()=>{
      const name = (input.value||'').trim().slice(0,12) || 'Trainer';
      document.removeEventListener('keydown', key);
      document.getElementById('introName').classList.add('hidden');
      const outro = [`${name}! That's a fine name.`, `${name}, your very own POKéMON adventure is about to begin!`,
        "Dreams, friendships, rivals... they're all waiting out there. I'll see you soon!"];
      let k = 0;
      document.getElementById('introText').textContent = outro[0];
      panel.onclick = ()=>{ if(++k < outro.length){ sfx('select'); document.getElementById('introText').textContent = outro[k]; } else { panel.onclick = null; introFinish(name); } };
    };
    document.getElementById('introOk').onclick = e=>{ e.stopPropagation(); done(); };
    input.onkeydown = e=>{ if(e.key==='Enter'){ e.stopPropagation(); done(); } };
  };
  const key = e=>{ if(document.getElementById('introName').classList.contains('hidden') && ['Enter',' ','z','Z'].includes(e.key)){ e.preventDefault(); panel.onclick && panel.onclick(); } };
  panel.onclick = next;
  document.addEventListener('keydown', key);
  say();
}
// Which way ROUTE 1 lies from home, in compass and on-screen terms (towns can be mirrored).
function routeOneWay(){
  const l = LOCATIONS[0].links.find(l=>/^route 1\b/i.test(LOCATIONS[l.to].name)) || LOCATIONS[0].links[0];
  const way = {right:['east', 'right'], left:['west', 'left'], up:['north', 'up'], down:['south', 'down']}[l.dir];
  return `ROUTE 1 is ${way[0]} of town. Follow the road ${l.dir==='up'||l.dir==='down' ? way[1] : 'to the '+way[1]}!`;
}
function introFinish(name){
  document.getElementById('starterSelect').classList.add('hidden');
  adv = {playerName:name, party:[], loc:0, cleared:{}, visited:{}, items:{pokeball:10}, box:[], money:3000,
    starterPending:true, starterTrio:starterOptions.map(d=>d.name)};
  saveAdv();
  renderAdventure();
  owSay([`${PROF} went out toward ROUTE 1 to study wild POKéMON.`, `Maybe you should go and find him! ${routeOneWay()}`]);
}

// ---- Route 1: "H-help me!" ----
function starterEvent(){
  if(owBusy && document.getElementById('evChaser')) return;
  const map = curMap(), near = [[3,0],[3,-1],[3,1],[4,0],[2,0]].map(([dx,dy])=>({x:adv.pos.x+dx, y:adv.pos.y+dy}))
    .find(p=>WALKABLE.has(tileAt(map,p.x,p.y)) && WALKABLE.has(tileAt(map,p.x+1,p.y)));
  const spot = near || {x:adv.pos.x+2, y:adv.pos.y};
  map.npcs.push({kind:'prof', x:spot.x, y:spot.y, facing:'left', event:true, lines:["Please! In my BAG!"], home:{...spot}});
  renderAdventure();
  const zig = dexByName('Zigzagoon');
  document.getElementById('owTiles').insertAdjacentHTML('afterend',
    `<div class="ow-actor ev-chaser" id="evChaser" style="transform:translate(${(spot.x+1)*T}px,${spot.y*T-12}px); z-index:${20+2*spot.y}"><img src="${spritePath(zig)}" alt=""></div>`);
  owSay(['H-help me!', `${PROF}: Hello! You over there! Please! Help me!`, "A wild ZIGZAGOON is after me! In my BAG! There's a POKé BALL in there!"], starterBag);
}
// Emerald's Bag screen: the open Bag with three Poké Balls; Left/Right to choose, A to look.
function starterBag(){
  let i = 1;
  const s = scrOpen('startbag', k=>{
    if(k==='left' || k==='right'){ i = (i + (k==='left' ? 2 : 1)) % 3; return draw(); }
    if(k!=='a') return;
    const d = starterOptions.find(o=>o.name===adv.starterTrio[i]) || dexByName(adv.starterTrio[i]);
    s.el.querySelector('.sb-show').innerHTML = `<img src="${spritePath(d)}" alt="">`;
    owPrompt(`Do you choose this POKéMON? The ${d.types[0].toUpperCase()} POKéMON ${d.name.toUpperCase()}!`);
    uiMenu(document.getElementById('owView'), ['YES', 'NO'], y=>{
      owPromptClose(); owBusy = true;
      if(y!==0){ s.el.querySelector('.sb-show').innerHTML = ''; return; }
      s.el.remove(); uiScr = null;
      const mon = makeMon(d, 0, 'none', 5);
      adv.party = [mon]; markOwned(mon); adv.starterPending = false; adv.starterThanks = true; saveAdv();
      owSay([`${adv.playerName} chose ${d.name.toUpperCase()}!`], ()=>{ document.getElementById('evChaser')?.remove(); startWildBattle({names:['Zigzagoon'], level:2}); });
    }, 'gm-yesno');
  });
  const draw = ()=>{ s.el.innerHTML = `<div class="sb-bag"></div>${[0,1,2].map(k=>`<div class="sb-ball${k===i?' on':''}" style="left:${150+k*70}px"></div>`).join('')}
    <div class="pb-hand" style="left:${162+i*70}px; top:62px"></div><div class="sb-show"></div>`; };
  draw();
}
// After the first battle, the professor thanks you and heads off.
function starterThanks(){
  adv.starterThanks = false;
  const map = getMap(LOCATIONS[1]);
  map.npcs = map.npcs.filter(n=>!n.event);
  saveAdv();
  owSay([`${PROF}: Whew... I went into the tall grass to look at wild POKéMON, and it jumped me!`, "You saved me. Thanks a lot!",
    `That ${adv.party[0].name.toUpperCase()} seems to like you. Please, keep it!`, "Travel with it and fill up your POKéDEX. I'll be watching your progress!"], ()=>renderAdventure());
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
// Region map: areas on their grid cells, with a connector wherever two areas link. Places you
// haven't been to yet show as "???".
function renderMap(){
  const el = document.getElementById('advMap');
  el.classList.toggle('hidden', !mapOpen);
  if(!mapOpen) return;
  const xs = LOCATIONS.map(l=>l.at[0]), ys = LOCATIONS.map(l=>l.at[1]);
  const x0 = Math.min(...xs), y0 = Math.min(...ys), cols = Math.max(...xs)-x0+1, rows = Math.max(...ys)-y0+1;
  let cells = '';
  LOCATIONS.forEach((loc,i)=>{
    const gx = (loc.at[0]-x0)*2+1, gy = (loc.at[1]-y0)*2+1;
    const seen = adv.visited[loc.name] || i===adv.loc;
    const icon = loc.type==='gym'?'🥊':loc.type==='trainer'?'🧑':loc.type==='route'?'🌿':'🏘️';
    const done = (loc.type==='gym'||loc.type==='trainer') && adv.cleared[loc.name];
    cells += `<div class="rm-cell ${i===adv.loc?'current':''} ${seen?'':'unseen'} rm-${loc.type}" style="grid-column:${gx}; grid-row:${gy}">
      <span>${seen?icon:'❔'}</span><small>${seen ? loc.name.replace(/^Route (\d+):.*/, 'Route $1') : '???'}${done?' ✓':''}</small></div>`;
    for(const l of loc.links){
      if(l.dir!=='right' && l.dir!=='down') continue;   // draw each link once
      cells += `<div class="rm-link ${l.dir==='right'?'h':'v'} ${l.gate&&!adv.cleared[loc.name]?'locked':''}" style="grid-column:${gx+(l.dir==='right'?1:0)}; grid-row:${gy+(l.dir==='down'?1:0)}"></div>`;
    }
  });
  el.innerHTML = `<div class="region-map" style="grid-template-columns:repeat(${cols}, minmax(0,1fr) 10px); grid-template-rows:repeat(${rows}, auto 10px)">${cells}</div>`;
}

function renderAdventure(){
  showAdvScreens();
  const loc = LOCATIONS[adv.loc];
  adv.visited[loc.name] = true;
  adv.party.concat(adv.box.filter(Boolean)).forEach(markOwned);   // saves from before the Pokédex
  // Saves from the old 10×6 maps (or a position on a door/exit) get put back at the entrance.
  if(adv.inside!=null && !getMap(loc).buildings[adv.inside]) adv.inside = null;
  const here = adv.pos ? tileAt(curMap(), adv.pos.x, adv.pos.y) : null;
  if(!here || !WALKABLE.has(here) || 'DEM'.includes(here)) spawnPlayer();
  renderOverworld();
  document.getElementById('advTitle').textContent = `${loc.type==='gym'?'🥊':loc.type==='trainer'?'🧑':loc.type==='route'?'🌿':'🏘️'} ${loc.name}`;
  document.getElementById('advDesc').innerHTML = `${loc.desc} <span class="pill">🧑 ${adv.playerName}</span> <span class="pill">🔴 x${adv.items.pokeball||0}</span>${adv.box.some(Boolean)?` <span class="pill">📦 Box: ${adv.box.filter(Boolean).length}</span>`:''}`;
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
      ? `<span class="note">${loc.type==='gym'?'Gym Leader':'Rival'} ${loc.leaderName} — defeated ✅</span>`
      : `<span class="note">${loc.type==='gym' ? `Walk into the Gym to challenge Leader ${loc.leaderName}.` : `Your rival ${loc.leaderName} is waiting up the path.`}</span>`;
  } else if(loc.endOfContent){
    actions += `<span class="note">End of current content — more of Vellorin is coming in a future build.</span>`;
  }
  if(adv.box.some(Boolean)){
    actions += `<button class="secondary" onclick="openBox()">📦 Box (${adv.box.filter(Boolean).length})</button>`;
  }
  actions += `<button class="secondary" onclick="resetAll()">Exit to menu</button>`;
  document.getElementById('advActions').innerHTML = actions;
}


// fixed = {names, level} for a scripted encounter (the opening); otherwise a wild pack from the area.
function startWildBattle(fixed){
  const loc = LOCATIONS[adv.loc];
  if(alive(adv.party).length===0){ showToast('Your whole party has fainted! Rest at the Pokémon Center.'); return; }
  // A wild pack: 1 up to half your (living) party, at most 4.
  const n = 1 + Math.floor(Math.random()*Math.min(4, Math.ceil(alive(adv.party).length/2)));
  const picks = fixed ? fixed.names : shuffle(areaPool(loc)).slice(0,n);
  let id=9000;
  const wild = picks.map(n=>makeMon(dexByName(n), id++, 'none', fixed ? fixed.level : wildLevel()));
  wild.forEach(markSeen);
  state = {sideA: adv.party, sideB: wild, log:[], mode:'story'};
  battleIntro(()=>{
  showAdvScreens();
  document.getElementById('battle').classList.remove('hidden');
  document.getElementById('adv').classList.add('hidden');
  addLog(`Wild ${wild.map(m=>`${m.name.toUpperCase()} (Lv${m.level})`).join(' and ')} appeared!`);
  startBattleUI();
  });
}

// A rival / Gym Leader battle, or (with npc) a route trainer's: their small team from the route,
// a level above the wild Pokémon there, and "CLASS NAME would like to battle!".
function startTrainerBattle(npc){
  const loc = LOCATIONS[adv.loc], rt = npc && npc.id;
  if(alive(adv.party).length===0){ showToast('Your whole party has fainted! Rest at the Pokémon Center.'); return; }
  let id=9000;
  const lv = rt ? Math.max(3, Math.min(advLevel(), Math.round(partyAvgLevel()) - 1)) : trainerLevel();
  // Rivals and Leaders field six: their signature team, filled out with type-fitting Pokémon.
  const names = rt ? npc.team.slice(0, Math.max(1, adv.party.length)) : loc.leaderTeam.slice();
  if(!rt){
    const theme = (GYM_STYLE[loc.leaderName] || {kind:''}).kind.replace('leader', '').toLowerCase();
    const extra = loc.type==='gym' && GYM_JUNIORS[theme] ? GYM_JUNIORS[theme].team : areaPool(loc);
    for(const n of extra) if(names.length<6 && !names.includes(n)) names.push(n);
    for(let i=0; names.length<6; i++) names.push(extra[i % extra.length]);
  }
  const team = names.map(n=>makeMon(dexByName(n), id++, rt ? 'none' : 'leftovers', lv));
  team.forEach(markSeen);
  state = {sideA: adv.party, sideB: team, log:[], mode:'story', trainerLoc: rt ? {type:'route', name:npc.id, leaderName:npc.title} : loc};
  battleIntro(()=>{
  showAdvScreens();
  document.getElementById('battle').classList.remove('hidden');
  document.getElementById('adv').classList.add('hidden');
  addLog(rt ? `${npc.title} would like to battle!` : `${loc.type==='gym'?'Gym Leader':'Rival'} ${loc.leaderName} challenges you with ${team.length} Pokémon! (Lv.${lv})`);
  startBattleUI();
  });
}

function continueStory(){
  document.getElementById('storyResult').classList.add('hidden');
  renderAdventure();
  if(adv.starterThanks) return starterThanks();
  dexRegisterNext();
}
// "Give a nickname to the caught X?" YES / NO, for each Pokémon caught in that battle.
let nickQueue = [];
function nickNext(){
  const m = nickQueue.shift();
  if(!m) return;
  owBusy = true;
  owPrompt(`Give a nickname to the caught ${m.name.toUpperCase()}?`);
  uiMenu(document.getElementById('owView'), ['YES', 'NO'], k=>{
    owPromptClose();
    if(k!==0) return nickNext();
    showConfirm(`${m.name.toUpperCase()}'s nickname?`, v=>{
      if(typeof v==='string' && v.trim()) m.nick = v.trim().slice(0,12);
      saveAdv(); renderAdventure(); nickNext();
    }, true, m.name);
  }, 'gm-yesno');
}
// After a first catch (Emerald): the new entry, headed "POKéDEX registration completed.", side bars
// blinking; A or B closes it (then the next one, if you caught more).
function dexRegisterNext(){
  const name = adv.dexNew && adv.dexNew.shift();
  if(!name) return nickNext();
  saveAdv();
  dexOpen();
  if(!dexScr) return;
  dexIdx = dexScr.list.findIndex(d=>d.name===name);
  dexScr.page = 'info'; dexScr.tab = 0; dexScr.reg = true;
  dexDraw();
}

function areaPool(loc){
  if(loc.pool) return loc.pool;
  for(const l of loc.links||[]) if(LOCATIONS[l.to].pool) return LOCATIONS[l.to].pool;
  return ['Rattata','Pidgey'];
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
  document.getElementById('continueBtn').textContent = hasSave ? 'Continue' : 'Start Adventure';
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
  startBattleUI();
}

// Every log line also records an HP/status snapshot of both sides, so the message box can
// replay a turn line by line with the HP bars draining in step, like the handheld games.
function addLog(t){
  state.log.push(t);
  (state.snaps = state.snaps || [])[state.log.length-1] = snapAll();
  const el=document.getElementById('log'); el.innerHTML = state.log.map(l=>`<div>${l}</div>`).join(''); el.scrollTop = el.scrollHeight;
}
function snapAll(){
  const s = m=>({hp:m.hp, maxhp:m.maxhp, fainted:!!m.fainted, caught:!!m.caught, status:m.status||null});
  return {A:state.sideA.map(s), B:state.sideB.map(s)};
}

// Paced message box: reveals a batch of lines one at a time with a tap/keypress-to-continue
// prompt, in the battle text box. Pass startIdx (the lines' index in state.log) to have
// the HP boxes follow each line's snapshot; the command menu stays hidden until it's done.
let msgQueue = [];
let msgQueueDone = null;
function showMsgBox(lines, onDone, startIdx){
  if(!lines || !lines.length){ if(onDone) onDone(); return; }
  msgQueue = lines.map((t,k)=>({t, snap: startIdx!=null && state && state.snaps ? state.snaps[startIdx+k] : null}));
  msgQueueDone = onDone || null;
  advanceMsgBox();
}
function advanceMsgBox(){
  const box = document.getElementById('msgBox');
  if(!box) return;
  const cmd = document.getElementById('battleCmd');
  if(msgQueue.length===0){
    box.classList.add('hidden');
    if(cmd) cmd.classList.remove('hidden');
    const cb = msgQueueDone; msgQueueDone = null;
    if(cb) cb();
    return;
  }
  const next = msgQueue.shift();
  document.getElementById('msgBoxText').innerHTML = next.t;
  if(next.snap) applySnap(next.snap);
  box.classList.remove('hidden');
  if(cmd) cmd.classList.add('hidden');
}
function alive(side){ return side.filter(m=>!m.fainted && !m.caught); }
// Once its battle is over, a caught Pokémon is just a party (or Box) member.
function clearCaught(){ if(adv) for(const m of [...adv.party, ...adv.box]) if(m) delete m.caught; }

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
  document.getElementById('boxList').innerHTML = adv.box.map((m,i)=> !m ? '' :
    `<div class="mon"><div class="mon-row">
       ${monSprite(m.dex)}
       <div class="mon-body">
         <div class="mon-top"><span>${dname(m)}<span class="lvbadge">Lv.${m.level}</span></span></div>
         <div class="types">${typeBadges(m.types)}</div>
         ${adv.party.length<MAX_PARTY
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
  if(adv.party.length < MAX_PARTY){
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

// ---------- Battle scene (GBA style) ----------
// Foe on a platform top-right with its HP box top-left; your side from behind bottom-left with
// HP boxes bottom-right. Sprites/boxes are keyed by side + index (A0, B2) since mon ids can repeat.
function hpColor(pct){ return pct>50?'var(--hp-g)':pct>20?'var(--hp-y)':'var(--hp-r)'; }
function spriteSize(n, player){ return (player ? [170,140,120,100,90,80] : [120,104,92,84,76,70])[Math.min(Math.max(n,1),6)-1]; }
function render(snap){
  if(!state) return;
  snap = snap || snapAll();
  for(const [key, player] of [['A',true],['B',false]]){
    const side = (key==='A'?state.sideA:state.sideB).slice(0, snap[key].length);
    const sz = spriteSize(side.length, player);
    document.getElementById('field'+key).innerHTML = side.map((m,i)=>
      `<div class="bmon" id="bs-${key}${i}" style="--sz:${sz}px"><img src="${spritePath(m.dex, player?'back':'front')}" alt="${dname(m)}" onerror="this.parentNode.classList.add('missing')"><span class="ph">❔</span></div>`).join('');
    const hud = document.getElementById('hud'+key);
    hud.classList.toggle('compact', side.length>3);
    hud.classList.toggle('dense', side.length>5);   // big sides: two columns of small HP boxes
    hud.innerHTML = side.map((m,i)=>`<div class="hpbox" id="hb-${key}${i}">
      <div class="hb-top"><span class="hb-name">${dname(m)}<span class="hb-st"></span></span><span class="hb-lv">Lv${m.level||50}</span></div>
      <div class="hb-bar"><b>HP</b><div class="hb-track"><div class="hb-fill"></div></div></div>
      ${player?'<div class="hb-num"></div>':''}
    </div>`).join('');
  }
  applySnap(snap, true);
}
// Point the HP boxes/sprites at a snapshot. Not instant -> bars drain via CSS transition and
// anything that just lost HP blinks.
function applySnap(snap, instant){
  for(const key of ['A','B']){
    snap[key].forEach((s,i)=>{
      const box = document.getElementById(`hb-${key}${i}`), spr = document.getElementById(`bs-${key}${i}`);
      if(!box || !spr) return;
      const fill = box.querySelector('.hb-fill');
      const pct = Math.max(0, 100*s.hp/s.maxhp);
      const prev = parseFloat(box.dataset.hp);
      if(!instant && s.hp < prev){ spr.classList.remove('hit'); void spr.offsetWidth; spr.classList.add('hit'); }
      box.dataset.hp = s.hp;
      if(instant) fill.style.transition = 'none';
      fill.style.width = pct+'%';
      fill.style.backgroundColor = hpColor(pct);
      if(instant){ void fill.offsetWidth; fill.style.transition = ''; }
      const num = box.querySelector('.hb-num');
      if(num) num.textContent = `${s.hp}/ ${s.maxhp}`;
      box.querySelector('.hb-st').innerHTML = statusTag(s);
      for(const el of [box, spr]){ el.classList.toggle('fainted', s.fainted); el.classList.toggle('caught', s.caught); }
    });
  }
}

// ---------- Command menu: FIGHT / BAG / POKéMON / RUN ----------
// Walks through each of your able Pokémon in turn; once the last one has an order, the turn runs.
let cmd = null;
function startBattleUI(){
  cmd = null;
  state.lastMove = {};
  document.getElementById('battleCmd').innerHTML = '';
  render();
  showMsgBox(state.log.slice(-1), beginCommand);
}
function beginCommand(){
  render();
  const queue = state.sideA.map((m,i)=>i).filter(i=>!state.sideA[i].fainted && !state.sideA[i].caught);
  cmd = {queue, pos:0, choices:{}, view:'main', pending:null};
  if(queue.length) renderCmd();
}
function cmdMon(){ return state.sideA[cmd.queue[cmd.pos]]; }
function enemyTargets(){ return state.sideB.map((m,i)=>i).filter(i=>!state.sideB[i].fainted && !state.sideB[i].caught); }
function renderCmd(focusIdx){
  const el = document.getElementById('battleCmd');
  const m = cmdMon(), ai = cmd.queue[cmd.pos];
  document.querySelectorAll('#battle .active').forEach(e=>e.classList.remove('active'));
  for(const id of [`bs-A${ai}`, `hb-A${ai}`]){ const e = document.getElementById(id); if(e) e.classList.add('active'); }
  const who = dname(m).toUpperCase();
  const step = cmd.queue.length>1 ? `<small>Pokémon ${cmd.pos+1} of ${cmd.queue.length}${cmd.pos>0?' · Esc: back':''}</small>` : '';
  const balls = adv && adv.items ? (adv.items.pokeball||0) : 0;
  let left, right;
  if(cmd.view==='fight'){
    left = `<div class="gba-box movegrid">${m.moves.map((mv,i)=>`<button onclick="cmdMove(${i})" onfocus="moveInfo(${i})">${mv.n}</button>`).join('')}
      <button onclick="cmdBack()">Cancel</button></div>`;
    right = `<div class="gba-box minfo" id="moveInfo"></div>`;
  } else if(cmd.view==='target'){
    const what = cmd.pending.move==='ball' ? 'Throw the Poké Ball at' : `Use ${m.moves[cmd.pending.move].n} on`;
    left = `<div class="gba-box tlist">${enemyTargets().map(i=>{ const t = state.sideB[i];
      return `<button onclick="cmdTarget(${i})">${dname(t)}<small>${Math.round(100*t.hp/t.maxhp)}%</small></button>`; }).join('')}
      <button onclick="cmdBack()">Cancel</button></div>`;
    right = `<div class="gba-box bdark">${what} which Pokémon?</div>`;
  } else if(cmd.view==='bag'){
    const wild = state.mode==='story' && !state.trainerLoc;
    left = `<div class="gba-box tlist" style="grid-template-columns:1fr">
      ${wild ? `<button onclick="cmdBall()" ${balls?'':'disabled'}>Poké Ball<small>×${balls}</small></button>` : ''}
      ${state.mode==='story' ? Object.keys(ITEM_INFO).filter(id=>isMedicine(id) && adv.items[id]>0).map(id=>`<button onclick="cmdItem('${id}')">${ITEM_INFO[id].name}<small>×${adv.items[id]}</small></button>`).join('') : ''}
      <button onclick="cmdBack()">Cancel</button></div>`;
    right = `<div class="gba-box bdark">${wild ? (balls ? 'Weaken it first for a better catch rate!' : 'Out of Poké Balls! Restock at a Pokémon Center.') : "There's nothing in the Bag you can use here."}</div>`;
  } else if(cmd.view==='party'){
    left = `<div class="gba-box plist">${state.sideA.map(p=>`<div class="pl ${p.fainted||p.caught?'out':''}"><span>${dname(p)} <span class="lvbadge">Lv${p.level||50}</span>${statusTag(p)}</span><span>${p.fainted?'FNT':`${p.hp}/${p.maxhp}`}</span></div>`).join('')}</div>`;
    right = `<div class="gba-box bmenu" style="grid-template-columns:1fr"><button onclick="cmdBack()">Cancel</button></div>`;
  } else {
    left = `<div class="gba-box bdark">What will<br>${who} do?${step}</div>`;
    right = `<div class="gba-box bmenu">
      <button onclick="cmdFight()">Fight</button><button onclick="cmdBag()">Bag</button>
      <button onclick="cmdParty()">Pokémon</button><button onclick="cmdRun()">Run</button></div>`;
  }
  el.innerHTML = left + right;
  const btns = el.querySelectorAll('button');
  btns.forEach(b=>b.addEventListener('mouseenter', ()=>{ if(!b.disabled) b.focus({preventScroll:true}); }));
  const f = btns[Math.min(focusIdx||0, btns.length-1)];
  if(f) f.focus({preventScroll:true});
}
function moveInfo(i){
  const el = document.getElementById('moveInfo');
  const mv = cmdMon().moves[i];
  if(!el || !mv) return;
  el.innerHTML = `<div class="row"><span>TYPE/</span>${typeBadge(mv.t)}</div>
    <div class="row"><span>POWER</span><span>${mv.p||'—'}</span></div>
    <div class="row"><span>ACCURACY</span><span>${mv.a}</span></div>
    <div class="row"><span>${mv.c==='phys'?'Physical':mv.c==='status'?'Status':'Special'}</span></div>`;
}
function cmdFight(){ cmd.view='fight'; renderCmd(state.lastMove[cmd.queue[cmd.pos]]||0); }
function cmdBag(){ cmd.view='bag'; renderCmd(); }
function cmdParty(){ cmd.view='party'; renderCmd(); }
function cmdMove(i){
  state.lastMove[cmd.queue[cmd.pos]] = i;
  pickTarget({move:i});
}
function cmdBall(){ pickTarget({move:'ball'}); }
function cmdItem(id){ cmdChoose({move:'item', item:id, target:enemyTargets()[0]}); }
function pickTarget(choice){
  const t = enemyTargets();
  if(t.length===1) return cmdChoose({...choice, target:t[0]});
  cmd.pending = choice; cmd.view = 'target'; renderCmd();
}
function cmdTarget(ti){ cmdChoose({...cmd.pending, target:ti}); }
function cmdChoose(choice){
  cmd.choices[cmd.queue[cmd.pos]] = choice;
  cmd.pos++; cmd.view = 'main'; cmd.pending = null;
  if(cmd.pos >= cmd.queue.length) return submitTurn();
  renderCmd();
}
function cmdBack(){
  if(cmd.view==='target' && cmd.pending.move!=='ball'){ cmd.view='fight'; renderCmd(cmd.pending.move); return; }
  if(cmd.view==='target'){ cmd.view='bag'; renderCmd(); return; }
  if(cmd.view!=='main'){ const from = cmd.view; cmd.view='main'; renderCmd({fight:0,bag:1,party:2}[from]); return; }
  if(cmd.pos>0){ cmd.pos--; delete cmd.choices[cmd.queue[cmd.pos]]; renderCmd(); }
}
function cmdRun(){
  if(state.mode==='free'){ showConfirm('Forfeit this battle?', ok=>{ if(ok){ cmd=null; resetAll(); } }); return; }
  if(state.trainerLoc){ showMsgBox(["No! There's no running from a Trainer battle!"], ()=>renderCmd(3)); return; }
  showMsgBox(['Got away safely!'], ()=>{
    cmd = null; state = null; clearCaught();
    document.getElementById('battle').classList.add('hidden');
    saveAdv(); renderAdventure();
  });
}
// Arrow keys / WASD move the ▶ cursor, Z or Enter confirms, X / Esc backs out.
function battleKey(e){
  if(!state || !cmd || typeof document.querySelectorAll!=='function') return false;
  if(document.getElementById('battle').classList.contains('hidden')) return false;
  if(!document.getElementById('confirmModal').classList.contains('hidden')) return false;
  const btns = Array.from(document.querySelectorAll('#battleCmd button')).filter(b=>!b.disabled);
  if(!btns.length) return false;
  const k = e.key.toLowerCase();
  if(k==='escape'||k==='x'||k==='backspace'){ e.preventDefault(); cmdBack(); return true; }
  let i = btns.indexOf(document.activeElement);
  const grid = getComputedStyle((btns[i]||btns[0]).parentNode).gridTemplateColumns;
  const cols = !grid || grid==='none' ? 1 : grid.split(' ').length;
  const dirs = {arrowleft:-1, arrowright:1, arrowup:-cols, arrowdown:cols, a:-1, d:1, w:-cols, s:cols};
  if(k in dirs){
    e.preventDefault();
    const j = i<0 ? 0 : i+dirs[k];
    if(j>=0 && j<btns.length) btns[j].focus({preventScroll:true});
    return true;
  }
  if(k==='z'){ e.preventDefault(); (btns[i]||btns[0]).click(); return true; }
  return k==='enter' || k===' ';
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
  let dmg = Math.floor((((2*(user.level||LEVEL)/5+2)*power*(atkStat/defStat))/50 + 2) * stab * e * rand);
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
  const pre = snapAll();
  const actions = [];
  for(const [ai, ch] of Object.entries(cmd ? cmd.choices : {})){
    const m = state.sideA[ai];
    const target = state.sideB[ch.target];
    if(!m || m.fainted || m.caught || !target || target.fainted || target.caught) continue;
    if(ch.move==='ball'){ actions.push({user:m, ball:true, target}); continue; }
    if(ch.move==='item'){ actions.push({user:m, item:ch.item, target:m}); continue; }
    actions.push({user:m, move:m.moves[ch.move], target});
  }
  cmd = null;
  for(const m of alive(state.sideB)){
    const opts = alive(state.sideA);
    if(opts.length===0) continue;
    const move = m.moves[Math.floor(Math.random()*m.moves.length)];
    const target = opts[Math.floor(Math.random()*opts.length)];
    actions.push({user:m, move, target});
  }
  actions.sort((a,b)=>(b.item?1:0) - (a.item?1:0) || effSpeed(b.user) - effSpeed(a.user));   // items go first

  for(const act of actions){
    if(act.user.fainted || act.user.caught || act.target.fainted || act.target.caught) continue;
    if(act.item){ const said = useItem(act.item, act.user); addLog(said ? `${adv.playerName} used a ${ITEM_INFO[act.item].name}! ${said}` : `It won't have any effect.`); continue; }
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
        nickQueue.push(act.target);
        if(markOwned(act.target)){ addLog(`${act.target.name.toUpperCase()}'s data was added to the POKéDEX.`); (adv.dexNew ||= []).push(act.target.name); }
        if(adv.party.length < MAX_PARTY) adv.party.push(act.target);
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

  render(pre);
  showMsgBox(state.log.slice(logStart), ()=>{ if(!checkEnd()) beginCommand(); }, logStart);
}

function checkEnd(){
  const a = alive(state.sideA).length, b = alive(state.sideB).length;
  if(a===0 || b===0){
    clearCaught();
    document.getElementById('battle').classList.add('hidden');
    if(state.mode==='story'){
      document.getElementById('storyResult').classList.remove('hidden');
      if(a>0){
        document.getElementById('storyResultText').textContent = '🏆 Victory!';
        const xpAmount = state.trainerLoc ? (state.trainerLoc.type==='gym'?60:35) : 18*state.sideB.length;
        const growStart = state.log.length;
        for(const m of adv.party) grantXp(m, xpAmount);
        const grew = state.log.slice(growStart);   // level-ups, new moves, evolutions
        const head = state.trainerLoc ? `You defeated ${state.trainerLoc.type==='gym'?'Gym Leader ':''}${state.trainerLoc.leaderName}! (+${xpAmount} XP)` : `The wild Pokémon retreated. (+${xpAmount} XP)`;
        if(state.trainerLoc){
          adv.cleared[state.trainerLoc.name] = true;
          const top = Math.max(...state.sideB.map(m=>m.level)), rate = state.trainerLoc.type==='gym' ? 100 : state.trainerLoc.type==='route' ? 20 : 60;
          adv.money = (adv.money ?? 3000) + top*rate;
          grew.unshift(`${adv.playerName} got ₽${top*rate} for winning!`);
        }
        document.getElementById('storyResultSub').innerHTML = [head, ...grew].join('<br>');
      } else {
        document.getElementById('storyResultText').textContent = '💀 Your party was defeated...';
        document.getElementById('storyResultSub').textContent = `${adv.playerName} whited out!`;
        healParty();
        sendToCenter();
      }
      saveAdv();
    } else {
      document.getElementById('result').classList.remove('hidden');
      document.getElementById('resultText').textContent = a>0 ? '🏆 Your side wins!' : (b>0 ? '💀 Enemy side wins.' : 'Double knockout — draw!');
    }
    return true;
  }
  return false;
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

// Installable (PWA): the service worker keeps a copy of the game for offline play.
if('serviceWorker' in navigator && location.protocol !== 'file:') navigator.serviceWorker.register('sw.js').catch(()=>{});
