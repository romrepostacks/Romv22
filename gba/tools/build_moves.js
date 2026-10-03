// Builds gba/data/moves_extra.json: what the GBA battle engine knows about moves beyond js/dexdata.js's
// MOVEDATA (PP, who a move hits, draining and recoil, healing, stat changes, weather), the status moves it
// adds (stat raising and lowering, healing, weather), and the level-up learnset entries for those.
// Usage: node gba/tools/build_moves.js <folder with PokeAPI's CSVs>
// (https://github.com/PokeAPI/pokeapi/tree/master/data/v2/csv: moves, move_meta, move_meta_stat_changes,
// move_names, pokemon_moves.)
// MOVEDATA's indices stay as they are (saves hold them); the new moves come after them.
const fs = require('fs'), path = require('path');
const SRC = process.argv[2];
if(!SRC){ console.error('usage: node gba/tools/build_moves.js <csv folder>'); process.exit(1); }
const ROOT = path.join(__dirname, '..', '..');

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

const dex = fs.readFileSync(path.join(ROOT, 'js', 'dexdata.js'), 'utf8');
const MOVEDATA = JSON.parse(dex.match(/const MOVEDATA = (\[.*?\]);\n/s)[1]);
const DEXDATA = JSON.parse(dex.match(/const DEXDATA = (\{.*\});/s)[1]);
const app = fs.readFileSync(path.join(ROOT, 'js', 'app.js'), 'utf8');
const SLUG_ID = Object.fromEntries([...app.match(/const DEX_NUM = \{([\s\S]*?)\};/)[1].matchAll(/"([^"]+)":(\d+)/g)].map(m=>[m[1], +m[2]]));
const wanted = new Set(Object.values(SLUG_ID).map(String));

const TYPE_NAMES = [null,'Normal','Fighting','Flying','Poison','Ground','Rock','Bug','Ghost','Steel','Fire','Water','Grass','Electric','Psychic','Ice','Dragon','Dark','Fairy'];
const AILMENT = {1:'par', 2:'slp', 3:'frz', 4:'brn', 5:'psn'};
// PokeAPI stat ids: 2 attack .. 6 speed, 7 accuracy, 8 evasion -> 0..6
const STAT = {2:0, 3:1, 4:2, 5:3, 6:4, 7:5, 8:6};
const WEATHER = {'sunny-day':'sun', 'rain-dance':'rain', 'sandstorm':'sand', 'hail':'hail'};

const EN = '9';
const names = {}; for(const r of parseCsv('move_names.csv')) if(r.local_language_id===EN) names[r.move_id] = r.name;
const meta = {}; for(const r of parseCsv('move_meta.csv')) meta[r.move_id] = r;
const stats = {}; for(const r of parseCsv('move_meta_stat_changes.csv')) (stats[r.move_id] ||= []).push([STAT[+r.stat_id], +r.change]);
const moves = parseCsv('moves.csv');
const byName = {}; for(const r of moves) if(names[r.id]) byName[names[r.id]] = r;

// Who a move hits: one target, every foe, the user, or nobody in particular (weather).
function targetOf(r){
  const t = +r.target_id;
  if(t===9 || t===11) return 'foes';
  if(t===7 || t===13 || t===5) return 'self';
  if(t===12 || t===4 || t===6 || t===14 || t===15) return 'field';
  return 'one';
}

// The extra fields for a move row (null: the engine can't do anything useful with it).
function extra(r){
  const m = meta[r.id] || {}, cat = +m.meta_category_id;
  const e = {pp:+r.pp || 10, target:targetOf(r)};
  const drain = +m.drain || 0, heal = +m.healing || 0;
  if(drain) e.drain = drain;
  if(heal) e.heal = heal;
  const st = (stats[r.id] || []).filter(([s])=>s!==undefined);
  if(st.length && (cat===2 || cat===6 || cat===7)){
    e.stats = st;
    // Damaging moves: the chance, and whether the change is the user's (damage+raise: Close Combat,
    // Overheat, Power-Up Punch) or the target's (damage+lower).
    if(cat!==2){ e.stat_chance = +m.stat_chance || 100; e.stat_self = cat===7; }
    else e.stat_self = e.target==='self';
  }
  if(WEATHER[r.identifier]) e.weather = WEATHER[r.identifier];
  return e;
}

const existing = MOVEDATA.map(mv=>{
  const r = byName[mv.n];
  if(!r) throw new Error('no PokeAPI move named ' + mv.n);
  return extra(r);
});

// The new status moves: stat changes on the user or the foes, healing the user, and the four weathers.
const NO = new Set(['curse', 'acupressure', 'stuff-cheeks', 'clangorous-soul', 'no-retreat', 'tar-shot', 'fillet-away',
  'shed-tail', 'belly-drum', 'rest', 'swallow', 'purify', 'floral-healing', 'heal-pulse', 'life-dew', 'jungle-healing',
  'lunar-blessing', 'decorate', 'aromatic-mist', 'flower-shield', 'rototiller', 'gear-up', 'magnetic-flux', 'coaching',
  'howl', 'tickle', 'venom-drench', 'captivate', 'memento', 'parting-shot', 'strength-sap', 'spicy-extract', 'tidy-up',
  'victory-dance', 'geomancy', 'take-heart', 'shelter', 'max-guard', 'eerie-impulse', 'noble-roar', 'play-nice', 'confide']);
const added = [], addedIndex = {};
function newMove(r){
  if(r.id in addedIndex) return addedIndex[r.id];
  const e = extra(r), m = meta[r.id] || {}, cat = +m.meta_category_id;
  const mv = {n:names[r.id], t:TYPE_NAMES[+r.type_id], p:0, c:'status', a:r.accuracy ? +r.accuracy : 100, ...e};
  addedIndex[r.id] = MOVEDATA.length + added.length;
  added.push(mv);
  return addedIndex[r.id];
}
function wantNew(r){
  if(r.damage_class_id!=='1' || +r.generation_id > 7 || NO.has(r.identifier) || !names[r.id]) return false;
  const m = meta[r.id] || {}, cat = +m.meta_category_id, t = targetOf(r);
  if(cat===2) return !!(stats[r.id] || []).length && (t==='self' || t==='one' || t==='foes');
  if(cat===3) return t==='self' && +m.healing > 0;
  return !!WEATHER[r.identifier];
}

// Learnsets: the same game as js/dexdata.js (build-dexdata.js's VG_PREF), its new moves only.
const VG_PREF = ['18','17','16','15','14','11','10','9','8','7','6','5','4','3','2','1','19','20','23','25'];
const rowById = Object.fromEntries(moves.map(r=>[r.id, r]));
const learnRows = {};
for(const r of parseCsv('pokemon_moves.csv')){
  if(r.pokemon_move_method_id!=='1' || !wanted.has(r.pokemon_id)) continue;
  ((learnRows[r.pokemon_id] ||= {})[r.version_group_id] ||= []).push([Math.max(1, +r.level), +r.order || 0, r.move_id]);
}
const learn = {};
for(const [slug, id] of Object.entries(SLUG_ID)){
  if(!DEXDATA[slug]) continue;
  const vgs = learnRows[String(id)] || {}, vg = VG_PREF.find(v=>vgs[v]);
  if(!vg) continue;
  const rows = vgs[vg].sort((a,b)=>a[0]-b[0] || a[1]-b[1]), out = [];
  for(const [lv,,mid] of rows){
    const r = rowById[mid];
    if(!r || !wantNew(r)) continue;
    const ix = newMove(r);
    if(!out.some(([l,m])=>l===lv && m===ix)) out.push([lv, ix]);
  }
  if(out.length) learn[slug] = out;
}

const out = {existing, added, learn};
fs.writeFileSync(path.join(ROOT, 'gba', 'data', 'moves_extra.json'), JSON.stringify(out));
console.log(`moves_extra.json: ${existing.length} moves extended, ${added.length} new: ${added.map(m=>m.n).join(', ')}`);
console.log(`learnsets with new moves: ${Object.keys(learn).length}`);
