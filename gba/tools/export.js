// Exports the browser game's data for the GBA build: loads js/*.js (the same code the web game runs) in a
// sandbox with the browser stubbed out, then writes
//   <out>/data.json      areas (tiles, behaviours, exits, signs, people), species, moves, type chart, art
//   <out>/map_<i>.rgb    each area drawn at 1 GBA pixel per art pixel (16x16 per tile), raw RGB
// Map pictures are composed exactly like the web overworld: ground art, path/water shorelines, buildings,
// then trees (whose tops overhang the tile above) in row order.
// Run: node tools/export.js <out dir> <area index>...
const vm = require('vm'), fs = require('fs'), path = require('path');
const ROOT = path.join(__dirname, '..', '..');
const OUT = process.argv[2];
const AREAS = process.argv.slice(3).map(Number);
if(!OUT || !AREAS.length){ console.error('usage: node tools/export.js <out dir> <area index>...'); process.exit(1); }

// ---------- Load the game ----------
const stub = ()=>new Proxy(function(){}, {get:(t,k)=> k===Symbol.toPrimitive ? (()=>'') : k==='length' ? 0 : stub(), apply:()=>stub(), set:()=>true});
const ctx = {console, Math, JSON, Date, document:stub(), window:stub(), navigator:{}, location:{search:''},
  localStorage:{getItem:()=>null, setItem(){}, removeItem(){}}, getComputedStyle:()=>({}),
  setTimeout:()=>0, clearTimeout(){}, setInterval:()=>0, clearInterval(){}, requestAnimationFrame:()=>0, cancelAnimationFrame(){},
  performance:{now:()=>0}, Image:function(){}, Audio:function(){}, addEventListener(){}};
vm.createContext(ctx);
const src = ['js/dexdata.js', 'js/dexinfo.js', 'js/tileart.js', 'js/app.js'].map(f=>fs.readFileSync(path.join(ROOT, f), 'utf8')).join('\n;\n');
vm.runInContext(src + `
;this.G = {LOCATIONS, getMap, neighbours, TILE_ART, TILE_CLS, WALKABLE, LEDGE_DIR, EDGE_GROUPS, edgeOverlay, ROOF_SWAP, WALL_SWAP,
  HOUSE_ROOFS, HOUSE_ROOF_NAMES, tallGrassSvg, charSvg, CHARS, DEX, DEX_NUM, slug, MOVEDATA, CHART, TYPES, STRUGGLE, STARTER_TRIOS,
  EVOLUTIONS, GROUND, THING_TEXT: typeof THING_TEXT!=='undefined' ? THING_TEXT : {}, PROF};`, ctx);
const G = ctx.G;

// ---------- Pixels ----------
const hex = c=>[parseInt(c.substr(1,2),16), parseInt(c.substr(3,2),16), parseInt(c.substr(5,2),16)];
// An SVG of 1px rects (artSvg, charSvg, tallGrassSvg) back to a pixel grid: {w, h, px:[[r,g,b]|null]}.
function svgPixels(svg){
  const vb = /viewBox="0 0 (\d+) (\d+)"/.exec(svg), w = +vb[1], h = +vb[2];
  const px = Array.from({length:h}, ()=>Array(w).fill(null));
  for(const m of svg.matchAll(/<rect x="(\d+)" y="(\d+)" width="(\d+)" height="(\d+)" fill="(#[0-9a-f]{6})"\/>/gi))
    for(let y=+m[2]; y<+m[2]+ +m[4]; y++) for(let x=+m[1]; x<+m[1]+ +m[3]; x++) px[y][x] = hex(m[5]);
  if(/scaleX\(-1\)/.test(svg)) px.forEach(r=>r.reverse());
  return {w, h, px};
}
function artPixels(name, pal){
  const a = G.TILE_ART[name], p = pal || a.pal;
  return {w:a.w, h:a.h, px:a.rows.map(r=>[...r].map(ch=>ch==='.' ? null : hex(p[ch])))};
}
class Canvas {
  constructor(w, h, fill){ this.w = w; this.h = h; this.d = Buffer.alloc(w*h*3); if(fill) this.rect(0, 0, w, h, fill); }
  rect(x0, y0, w, h, c){ for(let y=y0; y<y0+h; y++) for(let x=x0; x<x0+w; x++) this.set(x, y, c); }
  set(x, y, c){ if(x<0||y<0||x>=this.w||y>=this.h) return; const i = (y*this.w+x)*3; this.d[i]=c[0]; this.d[i+1]=c[1]; this.d[i+2]=c[2]; }
  draw(img, x0, y0){ img.px.forEach((r,y)=>r.forEach((c,x)=>{ if(c) this.set(x0+x, y0+y, c); })); }
}

// ---------- Areas ----------
const GRASS = hex(G.GROUND.base);
const tallGrass = svgPixels(G.tallGrassSvg());
// What the GBA needs to know about each tile.
const BEHAVIOUR = {walk:0, solid:1, tall:2, water:3, ledgeDown:4, ledgeRight:5, ledgeLeft:6, sign:7, door:8, item:9};
function behaviour(ch){
  if(ch==='"') return BEHAVIOUR.tall;
  if(ch==='~') return BEHAVIOUR.water;
  if(ch==='L') return BEHAVIOUR.ledgeDown;
  if(ch==='>') return BEHAVIOUR.ledgeRight;
  if(ch==='<') return BEHAVIOUR.ledgeLeft;
  if(ch==='S' || ch==='N') return BEHAVIOUR.sign;
  if(ch==='D') return BEHAVIOUR.door;
  if(ch==='I') return BEHAVIOUR.item;
  return G.WALKABLE.has(ch) ? BEHAVIOUR.walk : BEHAVIOUR.solid;
}
function drawArea(map){
  const c = new Canvas(map.w*16, map.h*16, GRASS);
  const at = (x,y)=>map.tiles[y][x];
  // Ground layer.
  for(let y=0; y<map.h; y++) for(let x=0; x<map.w; x++){
    const ch = at(x,y), cls = G.TILE_CLS[ch], X = x*16, Y = y*16;
    const v = ((x*73856093) ^ (y*19349663)) >>> 0;   // the web game's per-tile variation
    if(cls==='grass') c.draw(artPixels('grass_v'+(v%4)), X, Y);
    else if(cls==='tall') c.draw(tallGrass, X, Y);
    else if(cls==='path' || cls==='exit' || cls==='water'){
      const kind = cls==='water' ? 'water' : 'path';
      c.draw(artPixels(kind), X, Y);
      const ov = G.edgeOverlay(map, x, y, G.EDGE_GROUPS[kind], kind);
      for(const m of ov.matchAll(/--art-([a-z_]+)\)/g)) c.draw(artPixels(m[1]), X, Y);
    }
    else if(cls==='bush') c.draw(artPixels('bush'), X, Y);
    else if(cls==='ledge-e') c.draw(artPixels('ledge_e'), X, Y);
    else if(cls==='ledge-w') c.draw(artPixels('ledge_w'), X, Y);
    else if(cls!=='tree' && G.TILE_ART[cls]) c.draw(artPixels(cls), X, Y);   // flower, fence, sign, rsign, rock, item, ledge
  }
  // Buildings (as buildingHtml lays them out).
  for(const b of map.buildings){
    const roof = b.kind==='center' ? 'red' : b.kind==='mart' ? 'blue' : b.kind==='gym' || b.kind==='league' ? 'slate'
      : G.HOUSE_ROOF_NAMES[G.HOUSE_ROOFS.indexOf(b.roof)] || 'green';
    const wall = b.kind==='gym' || b.kind==='league' ? 'grey' : 'cream';
    const piece = name=>{ const a = G.TILE_ART[name], r = G.ROOF_SWAP[roof], w = G.WALL_SWAP[wall];
      return artPixels(name, {...a.pal, 1:r[0], 2:r[1], 3:r[2], 4:r[3], A:w[0], B:w[1], C:w[2]}); };
    const dc = b.door.x - b.x, cell = (name, col, row, tall)=>c.draw(piece(name), (b.x+col)*16, (b.y+row)*16 - (tall ? 8 : 0));
    for(let col=0; col<b.w; col++){
      const end = col===0 ? 'l' : col===b.w-1 ? 'r' : 'm';
      cell({l:'roof_tl', m:'roof_t', r:'roof_tr'}[end], col, 0, true); cell({l:'roof_bl', m:'roof_b', r:'roof_br'}[end], col, 1);
      for(let row=2; row<b.h; row++){
        const isDoor = row===b.h-1 && col===dc;
        cell(isDoor ? {house:'door_house', gym:'door_gym'}[b.kind] || 'door_glass' : 'wall_'+end, col, row);
        if(!isDoor && col>0) cell('window', col, row);
      }
    }
    if(b.kind==='house') cell('chimney', Math.min(2, b.w-1), 0, true);
  }
  // Trees last, row by row: each canopy rises 8 px into the tile above.
  for(let y=0; y<map.h; y++) for(let x=0; x<map.w; x++)
    if(at(x,y)==='T') c.draw(artPixels((x+y)%2 ? 'tree2' : 'tree'), x*16, y*16 - 8);
  return c;
}

fs.mkdirSync(OUT, {recursive:true});
const areas = [], peopleKinds = new Set();
for(const li of AREAS){
  const loc = G.LOCATIONS[li], map = G.getMap(loc);
  fs.writeFileSync(path.join(OUT, `map_${li}.rgb`), drawArea(map).d);
  const nb = G.neighbours(li);
  for(const n of map.npcs) peopleKinds.add(n.kind);
  areas.push({index:li, name:loc.name, type:loc.type, w:map.w, h:map.h,
    tiles:map.tiles.map(r=>r.join('')), behaviour:map.tiles.map(r=>r.map(behaviour)),
    links:nb.map(n=>({to:n.exit.to, dir:n.exit.dir, ox:n.ox, oy:n.oy, w:n.map.w, h:n.map.h, name:G.LOCATIONS[n.exit.to].name})),
    signs:map.signs.map(s=>({x:s.x, y:s.y, text:s.text})),
    doors:map.buildings.map(b=>({x:b.door.x, y:b.door.y, kind:b.kind})),
    people:map.npcs.filter(n=>!n.trainer).map(n=>({kind:n.kind, x:n.x, y:n.y, facing:n.facing, lines:n.lines || []})),
    spawn:map.spawn, pool:loc.pool || [], tier:loc.tier ?? li});
}

// ---------- People art: 16x21 frames as the web game draws them (charSvg) ----------
// frames[dir] = [stand, step1, step2]; dir is down/up/left (right = left mirrored). `dy` is how far the frame
// sits below the standing frame (the Gen 3 step dip).
const people = {};
for(const kind of ['player', ...peopleKinds]){
  people[kind] = {};
  for(const dir of ['down', 'up', 'left']){
    people[kind][dir] = [0, 1, 2].map(f=>{
      const svg = G.charSvg(kind, dir, f, false), p = svgPixels(svg);
      return {dy: /margin:-8px/.test(svg) ? 1 : 0, px:p.px.map(r=>r.map(c=>c ? c : 0))};
    });
  }
}

// ---------- Species and moves ----------
const usedSpecies = new Set();
for(const a of areas) a.pool.forEach(n=>usedSpecies.add(n));
for(const trio of G.STARTER_TRIOS.slice(2, 3)) trio.forEach(n=>usedSpecies.add(n));   // Hoenn trio for the test build
// Evolutions as the web game's tryEvolve finds them: level-up data first, then its hand-written table.
const evoOf = d=>d.evo ? {to:(G.DEX.find(e=>G.slug(e.name)===d.evo.to) || {}).name, level:d.evo.level}
  : G.EVOLUTIONS[d.name] ? {to:G.EVOLUTIONS[d.name].to, level:G.EVOLUTIONS[d.name].level} : null;
for(const name of [...usedSpecies]){
  let d = G.DEX.find(e=>e.name===name), e;
  while(d && (e = evoOf(d)) && e.to && e.level && !usedSpecies.has(e.to)){ usedSpecies.add(e.to); d = G.DEX.find(x=>x.name===e.to); }
}
const moveIdx = new Map(), moves = [];
const addMove = m=>{ const k = m.n; if(!moveIdx.has(k)){ moveIdx.set(k, moves.length); moves.push(m); } return moveIdx.get(k); };
addMove(G.STRUGGLE);
const species = [...usedSpecies].map(name=>{
  const d = G.DEX.find(e=>e.name===name);
  if(!d) throw new Error('no species ' + name);
  const num = G.DEX_NUM[G.slug(d.name)];
  const evo = evoOf(d);
  return {name:d.name, num, types:d.types, base:d.base, evo:evo && evo.to && evo.level && usedSpecies.has(evo.to) ? evo : null,
    learn:(d.learn || []).filter(([lv])=>lv<=100).map(([lv, mi])=>[lv, addMove(G.MOVEDATA[mi])])};
});

fs.writeFileSync(path.join(OUT, 'data.json'), JSON.stringify({
  areas, people, species, moves, tall_grass:tallGrass.px, types:G.TYPES, chart:G.CHART, starters:G.STARTER_TRIOS[2], prof:G.PROF,
  art:Object.fromEntries(['battle_bg', 'plat_enemy_l', 'plat_enemy_m', 'plat_enemy_r', 'plat_player_l', 'plat_player_m', 'plat_player_r', 'forest_fill', 'grass_v0']
    .map(n=>[n, G.TILE_ART[n]])),
}));
console.log(`export: ${areas.length} areas, ${species.length} species, ${moves.length} moves, ${Object.keys(people).length} people`);
