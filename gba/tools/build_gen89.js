// 2.0.0: every Pokémon up to Gen 9. Appends the species the game didn't have (the four Gen 7 gaps and #810-1025)
// after everything else, so the species and move indices that saves hold never move:
//   js/app.js       DEX_NUM entries and PROC_RAW_GEN89 (name, types, stats), concatenated last into DEX
//   js/dexdata.js   their DEXDATA (types, stats, ability, level-up learnset, level evolutions)
//   js/moveextra.js the moves they learn that the game didn't have, appended to MOVE_EXTRA.added
// Run after tools/build-dexdata.js and gba/tools/build_moves.js (it reads their output), then
// tools/build-dexinfo.js for the POKéDEX heights, weights and categories. Running it again changes nothing.
// Usage: node gba/tools/build_gen89.js <folder with PokeAPI's CSVs>
// (https://github.com/PokeAPI/pokeapi/tree/master/data/v2/csv: pokemon_species, pokemon_species_names,
// pokemon_stats, pokemon_types, pokemon_abilities, ability_names, ability_prose, pokemon_evolution, moves,
// move_names, move_meta, move_meta_stat_changes, pokemon_moves.)
const fs = require('fs'), path = require('path');
const SRC = process.argv[2];
if(!SRC){ console.error('usage: node gba/tools/build_gen89.js <csv folder>'); process.exit(1); }
const ROOT = path.join(__dirname, '..', '..');
const LAST = 1025;
const EN = '9';

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
const slug = name=>name.toLowerCase().normalize('NFD').replace(/[̀-ͯ]/g,'').replace(/[^a-z0-9]+/g,'-').replace(/^-+|-+$/g,'');

// ---- What the game has ----
const appPath = path.join(ROOT, 'js', 'app.js'), dexPath = path.join(ROOT, 'js', 'dexdata.js'), extraPath = path.join(ROOT, 'js', 'moveextra.js');
let app = fs.readFileSync(appPath, 'utf8');
const dexSrc = fs.readFileSync(dexPath, 'utf8');
const MOVEDATA = JSON.parse(dexSrc.match(/const MOVEDATA = (\[.*?\]);\n/s)[1]);
const DEXDATA = JSON.parse(dexSrc.match(/const DEXDATA = (\{.*\});/s)[1]);
const EXTRA = JSON.parse(fs.readFileSync(extraPath, 'utf8').match(/const MOVE_EXTRA = (\{.*\});/s)[1]);
const dexTable = app.match(/const DEX_NUM = \{([\s\S]*?)\};/)[1];
const SLUG_ID = Object.fromEntries([...dexTable.matchAll(/"([^"]+)":(\d+)/g)].map(m=>[m[1], +m[2]]));
const haveNum = new Set(Object.values(SLUG_ID));

// ---- PokeAPI ----
const TYPE_NAMES = [null,'Normal','Fighting','Flying','Poison','Ground','Rock','Bug','Ghost','Steel','Fire','Water','Grass','Electric','Psychic','Ice','Dragon','Dark','Fairy'];
const STAT_KEYS = [null,'hp','atk','def','spa','spd','spe'];
const AILMENT = {1:'par', 2:'slp', 3:'frz', 4:'brn', 5:'psn'};
const STAT = {2:0, 3:1, 4:2, 5:3, 6:4, 7:5, 8:6};
const WEATHER = {'sunny-day':'sun', 'rain-dance':'rain', 'sandstorm':'sand', 'hail':'hail', 'snowscape':'hail'};
const speciesName = {}; for(const r of parseCsv('pokemon_species_names.csv')) if(r.local_language_id===EN) speciesName[r.pokemon_species_id] = r.name;
const newNums = [];
for(let n=1; n<=LAST; n++) if(!haveNum.has(n)) newNums.push(n);
const NEW = newNums.map(n=>({num:n, name:speciesName[n], slug:slug(speciesName[n])}));
const wanted = new Set(newNums.map(String));   // the default form's pokemon id is its species id
const stats = {}; for(const r of parseCsv('pokemon_stats.csv')) if(wanted.has(r.pokemon_id)) (stats[r.pokemon_id] ||= {})[STAT_KEYS[+r.stat_id]] = +r.base_stat;
const types = {}; for(const r of parseCsv('pokemon_types.csv')) if(wanted.has(r.pokemon_id)) (types[r.pokemon_id] ||= [])[+r.slot-1] = TYPE_NAMES[+r.type_id];

// Abilities: as tools/build-dexdata.js.
const abilityName = {}; for(const r of parseCsv('ability_names.csv')) if(r.local_language_id===EN) abilityName[r.ability_id] = r.name;
const abilityText = {}; for(const r of parseCsv('ability_prose.csv')) if(r.local_language_id===EN) abilityText[r.ability_id] = r.short_effect.replace(/\[([^\]]*)\]\{[^}]*\}/g, (m,a)=>a || m.match(/:([^}]*)\}/)[1].replace(/-/g,' '));
const primaryAbility = {};
for(const r of parseCsv('pokemon_abilities.csv')) if(wanted.has(r.pokemon_id) && r.is_hidden==='0' && r.slot==='1') primaryAbility[r.pokemon_id] = r.ability_id;
const SIM = {
  overgrow:{type:'boost', boostType:'Grass'}, blaze:{type:'boost', boostType:'Fire'}, torrent:{type:'boost', boostType:'Water'}, swarm:{type:'boost', boostType:'Bug'},
  levitate:{type:'immune', immuneType:'Ground'}, 'volt-absorb':{type:'immune', immuneType:'Electric'}, 'lightning-rod':{type:'immune', immuneType:'Electric'},
  'motor-drive':{type:'immune', immuneType:'Electric'}, 'water-absorb':{type:'immune', immuneType:'Water'}, 'storm-drain':{type:'immune', immuneType:'Water'},
  'dry-skin':{type:'immune', immuneType:'Water'}, 'flash-fire':{type:'immune', immuneType:'Fire'}, 'sap-sipper':{type:'immune', immuneType:'Grass'},
  'well-baked-body':{type:'immune', immuneType:'Fire'}, 'earth-eater':{type:'immune', immuneType:'Ground'},
  'iron-fist':{type:'punch'}, disguise:{type:'disguise'}, merciless:{type:'merciless'}, corrosion:{type:'corrosion'}};
const abilityIdent = id=>(abilityName[id] || '').toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'');

// Level evolutions of the new species (the rest come from data/extras.json in build_assets.py; regional forms'
// evolutions, like Galarian Corsola's, are left out with them).
const evoFrom = {}; for(const r of parseCsv('pokemon_species.csv')) if(r.evolves_from_species_id) evoFrom[r.id] = r.evolves_from_species_id;
const allNum = new Set([...haveNum, ...newNums]);
const numSlug = Object.fromEntries([...Object.entries(SLUG_ID).map(([s,n])=>[n, s]), ...NEW.map(s=>[s.num, s.slug])]);
const evo = {};
for(const r of parseCsv('pokemon_evolution.csv')){
  if(r.evolution_trigger_id!=='1' || !r.minimum_level || r.evolved_pokemon_form_id || r.region_id) continue;
  const to = +r.evolved_species_id, from = +evoFrom[to];
  if(!from || !allNum.has(from) || !allNum.has(to) || evo[from]) continue;
  evo[from] = {to:numSlug[to], level:+r.minimum_level};
}

// ---- Moves ----
const names = {}; for(const r of parseCsv('move_names.csv')) if(r.local_language_id===EN) names[r.move_id] = r.name;
const meta = {}; for(const r of parseCsv('move_meta.csv')) meta[r.move_id] = r;
const statChanges = {}; for(const r of parseCsv('move_meta_stat_changes.csv')) (statChanges[r.move_id] ||= []).push([STAT[+r.stat_id], +r.change]);
const moveRows = Object.fromEntries(parseCsv('moves.csv').map(r=>[r.id, r]));
const NO_SIM = new Set(['selfdestruct','explosion','misty-explosion','mind-blown','final-gambit','struggle']);
// Status moves the engine would get wrong (gba/tools/build_moves.js's NO list).
const NO = new Set(['curse', 'acupressure', 'stuff-cheeks', 'clangorous-soul', 'no-retreat', 'tar-shot', 'fillet-away',
  'shed-tail', 'belly-drum', 'rest', 'swallow', 'purify', 'floral-healing', 'heal-pulse', 'life-dew', 'jungle-healing',
  'lunar-blessing', 'decorate', 'aromatic-mist', 'flower-shield', 'rototiller', 'gear-up', 'magnetic-flux', 'coaching',
  'howl', 'tickle', 'venom-drench', 'captivate', 'memento', 'parting-shot', 'strength-sap', 'spicy-extract', 'tidy-up',
  'victory-dance', 'geomancy', 'take-heart', 'shelter', 'max-guard', 'eerie-impulse', 'noble-roar', 'play-nice', 'confide']);
const PUNCH = /punch|^meteor-mash$|^hammer-arm$|^sky-uppercut$|^ice-hammer$|^jet-punch$|^rage-fist$|^headlong-rush$/;
function targetOf(r){
  const t = +r.target_id;
  if(t===9 || t===11) return 'foes';
  if(t===7 || t===13 || t===5) return 'self';
  if(t===12 || t===4 || t===6 || t===14 || t===15) return 'field';
  return 'one';
}
// gba/tools/build_moves.js's extra fields.
function extra(r){
  const m = meta[r.id] || {}, cat = +m.meta_category_id;
  const e = {pp:+r.pp || 10, target:targetOf(r)};
  const drain = +m.drain || 0, heal = +m.healing || 0;
  if(drain) e.drain = drain;
  if(heal) e.heal = heal;
  const st = (statChanges[r.id] || []).filter(([s])=>s!==undefined);
  if(st.length && (cat===2 || cat===6 || cat===7)){
    e.stats = st;
    if(cat!==2){ e.stat_chance = +m.stat_chance || 100; e.stat_self = cat===7; }
    else e.stat_self = e.target==='self';
  }
  if(WEATHER[r.identifier]) e.weather = WEATHER[r.identifier];
  return e;
}
// A move the battle engine can run: any damaging move (its unusual effects left out: it hits for its power),
// a status move that only inflicts a status, changes stats, heals the user or sets the weather.
function engineMove(r){
  const cls = r.damage_class_id, power = +r.power || 0, t = TYPE_NAMES[+r.type_id], m = meta[r.id] || {}, cat = +m.meta_category_id;
  if(!t || !names[r.id] || NO_SIM.has(r.identifier) || r.identifier.startsWith('max-') || r.identifier.startsWith('g-max-')) return null;
  const acc = r.accuracy ? +r.accuracy : 100;
  if((cls==='2' || cls==='3') && power>0){
    const mv = {n:names[r.id], t, p:power, c:cls==='2' ? 'phys' : 'spec', a:acc};
    const st = AILMENT[m.meta_ailment_id], ch = +m.ailment_chance || 0;
    if(st && ch>0) mv.sec = {status:st, chance:ch};
    if(PUNCH.test(r.identifier)) mv.punch = true;
    return {...mv, ...extra(r)};
  }
  if(cls!=='1' || NO.has(r.identifier)) return null;
  if(cat===1 && AILMENT[m.meta_ailment_id]) return {n:names[r.id], t, p:0, c:'status', a:acc, status:AILMENT[m.meta_ailment_id], ...extra(r)};
  const tg = targetOf(r);
  if((cat===2 && (statChanges[r.id] || []).length && (tg==='self' || tg==='one' || tg==='foes')) || (cat===3 && tg==='self' && +m.healing > 0) ||
     WEATHER[r.identifier])
    return {n:names[r.id], t, p:0, c:'status', a:acc, ...extra(r)};
  return null;
}
// Global move index: MOVEDATA's, then MOVE_EXTRA.added's (MOVEDATA.length + k). New ones join `added`.
const indexOfName = {};
MOVEDATA.forEach((m, i)=>{ indexOfName[m.n] = i; });
EXTRA.added.forEach((m, k)=>{ if(!(m.n in indexOfName)) indexOfName[m.n] = MOVEDATA.length + k; });
let addedMoves = 0;
function moveIndex(id){
  const n = names[id];
  if(n in indexOfName) return indexOfName[n];
  const mv = engineMove(moveRows[id]);
  if(!mv) return -1;
  indexOfName[n] = MOVEDATA.length + EXTRA.added.length;
  EXTRA.added.push(mv);
  ++addedMoves;
  return indexOfName[n];
}

// Learnsets: Scarlet/Violet first, then Sword/Shield, Legends: Arceus, BDSP, then the older games.
const VG_PREF = ['25','20','24','23','18','17','16','15','14','11','10','9','8','7','6','5','4','3','2','1'];
const learnRows = {};
for(const r of parseCsv('pokemon_moves.csv')){
  if(r.pokemon_move_method_id!=='1' || !wanted.has(r.pokemon_id)) continue;
  ((learnRows[r.pokemon_id] ||= {})[r.version_group_id] ||= []).push([Math.max(1, +r.level), +r.order || 0, r.move_id]);
}

// ---- Append ----
const procRows = [], addedSlugs = [];
let added = 0;
for(const s of NEW){
  const pid = String(s.num);
  if(!s.name || !stats[pid] || !types[pid]) throw new Error('no PokeAPI data for #' + s.num);
  if(s.slug in SLUG_ID) continue;
  const vgs = learnRows[pid] || {}, vg = VG_PREF.find(v=>vgs[v]);
  const learn = [];
  for(const [lv,,mid] of (vg ? vgs[vg] : []).sort((a,b)=>a[0]-b[0] || a[1]-b[1])){
    const ix = moveIndex(mid);
    if(ix >= 0 && !learn.some(([l,m])=>l===lv && m===ix)) learn.push([lv, ix]);
  }
  const aid = primaryAbility[pid], ab = {n:abilityName[aid] || '—', desc:abilityText[aid] || '', ...(SIM[abilityIdent(aid)] || {type:'flavor'})};
  if(ab.type==='flavor') ab.desc += ' (No battle effect yet.)';
  const t = types[pid].filter(Boolean), b = stats[pid];
  DEXDATA[s.slug] = {types:t, base:b, ability:ab, learn};
  if(evo[s.num]) DEXDATA[s.slug].evo = evo[s.num];
  procRows.push(JSON.stringify([s.name, t[0], t[1] || null, b.hp, b.atk, b.def, b.spa, b.spd, b.spe]));
  SLUG_ID[s.slug] = s.num;
  addedSlugs.push(`"${s.slug}":${s.num}`);
  ++added;
}
if(added){
  app = app.replace(/(const DEX_NUM = \{[\s\S]*?)\n\};/, (m, a)=>`${a},\n // 2.0.0 (gba/tools/build_gen89.js): the four Gen 7 gaps and Gen 8-9\n ${addedSlugs.join(',')}\n};`);
  app = app.replace(/\nconst DEX = CURATED_DEX\.concat\(([^\n]*)\);/, (m, a)=>
    `\n// 2.0.0: the four Gen 7 gaps and Gen 8-9 (gba/tools/build_gen89.js, from PokeAPI), after everything older so\n` +
    `// species indices in saves stay put.\nconst PROC_RAW_GEN89 = [\n${procRows.join(',\n')}\n];\n` +
    `const DEX = CURATED_DEX.concat(${a}).concat(PROC_RAW_GEN89.map(procEntry));`);
  fs.writeFileSync(appPath, app);
  fs.writeFileSync(dexPath, dexSrc.replace(/const DEXDATA = \{.*\};/s, 'const DEXDATA = ' + JSON.stringify(DEXDATA) + ';'));
  const ex = fs.readFileSync(extraPath, 'utf8');
  fs.writeFileSync(extraPath, ex.replace(/const MOVE_EXTRA = \{.*\};/s, ()=>'const MOVE_EXTRA = ' + JSON.stringify(EXTRA) + ';'));
}
console.log(`build_gen89: ${added} species added, ${addedMoves} moves added (${MOVEDATA.length + EXTRA.added.length} in all)`);
