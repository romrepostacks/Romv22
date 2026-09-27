// Builds js/dexdata.js from PokeAPI's CSV dump (https://github.com/PokeAPI/pokeapi/tree/master/data/v2/csv).
// Usage: node tools/build-dexdata.js <folder with the CSVs>
// Output: real types, base stats, primary ability, level-up learnset (Ultra Sun/Moon, falling back
// to the nearest game that has one) and level-based evolutions for every species in the game,
// plus a shared move table. Only moves the battle engine can actually simulate are kept.
const fs = require('fs'), path = require('path');
const SRC = process.argv[2];
if(!SRC){ console.error('usage: node tools/build-dexdata.js <csv folder>'); process.exit(1); }
const ROOT = path.join(__dirname, '..');

function parseCsv(file){
  const text = fs.readFileSync(path.join(SRC, file), 'utf8').replace(/^﻿/, '');
  const rows = []; let row = [], field = '', q = false;
  for(let i=0;i<text.length;i++){
    const c = text[i];
    if(q){ if(c==='"'){ if(text[i+1]==='"'){ field+='"'; i++; } else q=false; } else field+=c; }
    else if(c==='"') q = true;
    else if(c===','){ row.push(field); field=''; }
    else if(c==='\n'){ row.push(field.replace(/\r$/,'')); rows.push(row); row=[]; field=''; }
    else field += c;
  }
  if(field || row.length){ row.push(field); rows.push(row); }
  const head = rows.shift();
  return rows.filter(r=>r.length>1).map(r=>Object.fromEntries(head.map((h,i)=>[h, r[i]])));
}
const EN = '9';

// The game's species list: the slug -> PokeAPI id table in js/app.js.
const app = fs.readFileSync(path.join(ROOT, 'js', 'app.js'), 'utf8');
const table = app.match(/const DEX_NUM = \{([\s\S]*?)\};/)[1];
const SLUG_ID = Object.fromEntries([...table.matchAll(/"([^"]+)":(\d+)/g)].map(m=>[m[1], +m[2]]));
const ID_SLUG = Object.fromEntries(Object.entries(SLUG_ID).map(([s,i])=>[i,s]));
const wanted = new Set(Object.values(SLUG_ID).map(String));

const TYPE_NAMES = [null,'Normal','Fighting','Flying','Poison','Ground','Rock','Bug','Ghost','Steel','Fire','Water','Grass','Electric','Psychic','Ice','Dragon','Dark','Fairy'];
const STAT_KEYS = [null,'hp','atk','def','spa','spd','spe'];
const AILMENT = {1:'par', 2:'slp', 3:'frz', 4:'brn', 5:'psn'};

// ---- Moves ----
const moveNames = {}; for(const r of parseCsv('move_names.csv')) if(r.local_language_id===EN) moveNames[r.move_id] = r.name;
const meta = {}; for(const r of parseCsv('move_meta.csv')) meta[r.move_id] = r;
const NO_SIM = new Set(['selfdestruct','explosion','misty-explosion','mind-blown','final-gambit','struggle']);
const PUNCH = /punch|^meteor-mash$|^hammer-arm$|^sky-uppercut$|^drain-punch$|^bullet-punch$|^shadow-punch$|^power-up-punch$|^ice-hammer$/;
const moveInfo = {};   // move id -> engine move, or null if the engine can't simulate it
for(const r of parseCsv('moves.csv')){
  const m = meta[r.id] || {}, cls = r.damage_class_id, power = +r.power || 0;
  const acc = r.accuracy ? +r.accuracy : 100, t = TYPE_NAMES[+r.type_id];
  if(!t || !moveNames[r.id] || NO_SIM.has(r.identifier)) { moveInfo[r.id] = null; continue; }
  let mv = null;
  if((cls==='2'||cls==='3') && power>0){
    mv = {n:moveNames[r.id], t, p:power, c:cls==='2'?'phys':'spec', a:acc};
    const st = AILMENT[m.meta_ailment_id], ch = +m.ailment_chance || 0;
    if(st && ch>0) mv.sec = {status:st, chance:ch};
    if(PUNCH.test(r.identifier)) mv.punch = true;
  } else if(cls==='1' && m.meta_category_id==='1' && AILMENT[m.meta_ailment_id]){
    mv = {n:moveNames[r.id], t, p:0, c:'status', a:acc, status:AILMENT[m.meta_ailment_id]};
  }
  moveInfo[r.id] = mv;
}

// ---- Learnsets: level-up moves from the newest Gen 7-or-earlier game that has them ----
const VG_PREF = ['18','17','16','15','14','11','10','9','8','7','6','5','4','3','2','1','19','20','23','25'];
const learnRows = {};  // pokemon id -> vg -> [[level, order, move]]
for(const r of parseCsv('pokemon_moves.csv')){
  if(r.pokemon_move_method_id!=='1' || !wanted.has(r.pokemon_id)) continue;
  ((learnRows[r.pokemon_id] ||= {})[r.version_group_id] ||= []).push([Math.max(1, +r.level), +r.order || 0, r.move_id]);
}
const MOVES = [], moveIndex = {};
function useMove(id){
  if(!(id in moveIndex)){ moveIndex[id] = MOVES.length; MOVES.push(moveInfo[id]); }
  return moveIndex[id];
}

// ---- Species data ----
const stats = {}; for(const r of parseCsv('pokemon_stats.csv')) if(wanted.has(r.pokemon_id)) (stats[r.pokemon_id] ||= {})[STAT_KEYS[+r.stat_id]] = +r.base_stat;
const types = {}; for(const r of parseCsv('pokemon_types.csv')) if(wanted.has(r.pokemon_id)) (types[r.pokemon_id] ||= [])[+r.slot-1] = TYPE_NAMES[+r.type_id];
const abilityName = {}; for(const r of parseCsv('ability_names.csv')) if(r.local_language_id===EN) abilityName[r.ability_id] = r.name;
const abilityText = {}; for(const r of parseCsv('ability_prose.csv')) if(r.local_language_id===EN) abilityText[r.ability_id] = r.short_effect.replace(/\[([^\]]*)\]\{[^}]*\}/g, (m,a)=>a || m.match(/:([^}]*)\}/)[1].replace(/-/g,' '));
const primaryAbility = {};
for(const r of parseCsv('pokemon_abilities.csv')) if(wanted.has(r.pokemon_id) && r.is_hidden==='0' && r.slot==='1') primaryAbility[r.pokemon_id] = r.ability_id;
// Abilities the engine can actually simulate; everything else is shown but has no battle effect.
const SIM = {
  overgrow:{type:'boost', boostType:'Grass'}, blaze:{type:'boost', boostType:'Fire'}, torrent:{type:'boost', boostType:'Water'}, swarm:{type:'boost', boostType:'Bug'},
  levitate:{type:'immune', immuneType:'Ground'}, 'volt-absorb':{type:'immune', immuneType:'Electric'}, 'lightning-rod':{type:'immune', immuneType:'Electric'},
  'motor-drive':{type:'immune', immuneType:'Electric'}, 'water-absorb':{type:'immune', immuneType:'Water'}, 'storm-drain':{type:'immune', immuneType:'Water'},
  'dry-skin':{type:'immune', immuneType:'Water'}, 'flash-fire':{type:'immune', immuneType:'Fire'}, 'sap-sipper':{type:'immune', immuneType:'Grass'},
  'iron-fist':{type:'punch'}, disguise:{type:'disguise'}, merciless:{type:'merciless'}, corrosion:{type:'corrosion'}};
// Ability identifier from its English name ("Lightning Rod" -> "lightning-rod").
const abilityIdentById = {};
for(const [id,n] of Object.entries(abilityName)) abilityIdentById[id] = n.toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'');

// ---- Evolutions (level-up with a minimum level only) ----
const evoFrom = {}; for(const r of parseCsv('pokemon_species.csv')) if(r.evolves_from_species_id) evoFrom[r.id] = r.evolves_from_species_id;
const evo = {};
for(const r of parseCsv('pokemon_evolution.csv')){
  if(r.evolution_trigger_id!=='1' || !r.minimum_level) continue;
  const to = r.evolved_species_id, from = evoFrom[to];
  if(!from || !ID_SLUG[from] || !ID_SLUG[to] || evo[ID_SLUG[from]]) continue;
  evo[ID_SLUG[from]] = {to: ID_SLUG[to], level: +r.minimum_level};
}

const out = {};
let missing = [];
for(const [slug, id] of Object.entries(SLUG_ID)){
  const pid = String(id);
  if(!stats[pid] || !types[pid]){ missing.push(slug); continue; }
  const vgs = learnRows[pid] || {};
  const vg = VG_PREF.find(v=>vgs[v]);
  const learn = (vg ? vgs[vg] : []).sort((a,b)=>a[0]-b[0] || a[1]-b[1])
    .filter(([,,m])=>moveInfo[m]).map(([lv,,m])=>[lv, useMove(m)])
    .filter(([lv,m],i,arr)=>arr.findIndex(([l2,m2])=>l2===lv && m2===m)===i);
  const aid = primaryAbility[pid];
  const ident = abilityIdentById[aid];
  const ab = {n: abilityName[aid] || '—', desc: abilityText[aid] || '', ...(SIM[ident] || {type:'flavor'})};
  if(ab.type==='flavor') ab.desc += ' (No battle effect yet.)';
  out[slug] = {types: types[pid].filter(Boolean), base: stats[pid], ability: ab, learn};
  if(evo[slug]) out[slug].evo = evo[slug];
}
if(missing.length) console.warn('missing data for:', missing.join(', '));
const js = `// Generated by tools/build-dexdata.js from PokeAPI data (https://pokeapi.co) — do not edit by hand.\n` +
  `// DEXDATA[slug] = {types, base, ability, learn:[[level, moveIndex]...], evo?:{to, level}}\n` +
  `const MOVEDATA = ${JSON.stringify(MOVES)};\n` +
  `const DEXDATA = ${JSON.stringify(out)};\n`;
fs.writeFileSync(path.join(ROOT, 'js', 'dexdata.js'), js);
console.log(`wrote js/dexdata.js: ${Object.keys(out).length} species, ${MOVES.length} moves, ${Object.keys(evo).length} evolutions, ${(js.length/1024).toFixed(0)} KB`);
