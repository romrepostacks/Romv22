// Exports the browser game's data for the GBA build: loads js/*.js (the same code the web game runs) in a
// sandbox with the browser stubbed out, then writes
//   <out>/data.json       areas and rooms (tiles, behaviours, links, signs, people, trainers, items), species,
//                         moves, items, type chart, art
//   <out>/map_<i>.rgb     each area drawn at 1 GBA pixel per art pixel (16x16 per tile), raw RGB
//   <out>/room_<kind>.rgb each room kind (center, mart, house) drawn the same way
// Pictures are composed exactly like the web game's overworld and rooms (js/app.js tileHtml/buildingHtml and
// the room rules in css/style.css): ground art, shorelines, buildings, then tall pieces that overhang the
// tile above, in row order.
// Run: node tools/export.js <out dir> <area index>...
const vm = require('vm'), fs = require('fs'), path = require('path');
const ROOT = path.join(__dirname, '..', '..');
const OUT = process.argv[2];
const AREAS_ARG = process.argv.slice(3).map(Number);
if(!OUT){ console.error('usage: node tools/export.js <out dir> [area index...]'); process.exit(1); }

// ---------- Load the game ----------
const stub = ()=>new Proxy(function(){}, {get:(t,k)=> k===Symbol.toPrimitive ? (()=>'') : k==='length' ? 0 : stub(), apply:()=>stub(), set:()=>true});
const ctx = {console, Math, JSON, Date, document:stub(), window:stub(), navigator:{}, location:{search:''},
  localStorage:{getItem:()=>null, setItem(){}, removeItem(){}}, getComputedStyle:()=>({}),
  setTimeout:()=>0, clearTimeout(){}, setInterval:()=>0, clearInterval(){}, requestAnimationFrame:()=>0, cancelAnimationFrame(){},
  performance:{now:()=>0}, Image:function(){}, Audio:function(){}, addEventListener(){}};
ctx.adv = null;
vm.createContext(ctx);
const src = ['js/dexdata.js', 'js/moveextra.js', 'js/dexinfo.js', 'js/tileart.js', 'js/app.js', 'js/music.js'].map(f=>fs.readFileSync(path.join(ROOT, f), 'utf8')).join('\n;\n');
vm.runInContext(src + `
;this.G = {LOCATIONS, getMap, getInterior, neighbours, TILE_ART, TILE_CLS, WALKABLE, LEDGE_DIR, EDGE_GROUPS, edgeOverlay, ROOF_SWAP,
  WALL_SWAP, HOUSE_ROOFS, HOUSE_ROOF_NAMES, tallGrassSvg, charSvg, CHARS, DEX, DEX_NUM, slug, MOVEDATA, CHART, TYPES, STRUGGLE,
  STARTER_TRIOS, EVOLUTIONS, GROUND, THING_TEXT, PROF, INTRO_LINES, ITEM_INFO, TYPE_COLORS, BALL_SVG, GYM_STYLE, gymTheme, GYM_JUNIORS, PROF_CALLS,
  areaPool, waterPool, fishPool, ELITES, WALLPAPERS, WALL_NAMES, mfxScript, PIX, TYPE_COL, WEATHER, SURF_ROWS, TIME_TYPES, NIGHT_VISITORS, TEMPEST_POOL, ADMIN, ITEMS, PROC_ENTRY:null,
  setAdv:a=>{ adv = a; }, CURATED_DEX, leagueGates, ROOMS, DEXINFO:typeof DEXINFO!=='undefined' ? DEXINFO : {}, DEXDATA, MOVE_EXTRA, moveByIndex, GEN89:PROC_RAW_GEN89.map(r=>r[0]), TOWER_FLOORS, TOWER_INTROS, CHAMBER_THEMES, TOWER_ROOMS, SUNDERED_FORMS, SKYREACH_FORMS, MUSIC_TRACKS:typeof MUSIC_TRACKS!=='undefined' ? MUSIC_TRACKS : null};`, ctx);
const G = ctx.G;
// ---------- SPIRECREST TOWN, TRADEWIND VILLAGE and the SAFARI ZONE ----------
// The web game adds them (js/app.js, after every older area, so those keep their indices): SPIRECREST's big
// building is the CHALLENGE TOWER, TRADEWIND VILLAGE has its TRADER (the GBA places its own) and neither of the
// other two has trainers.
const TOWER_TOWN = G.LOCATIONS.find(l=>l.name==='Spirecrest Town');
const TRADE_TOWN = G.LOCATIONS.find(l=>l.name==='Tradewind Village');
const SAFARI = G.LOCATIONS.find(l=>l.name==='Safari Zone');
// The tower's rooms (js/app.js): five themed floors with one trainer each, the summit with the SUMMONING STONE,
// and a chamber per theme where the summoned legendary waits (the GBA picks the chamber by its type).
const {TOWER_FLOORS, TOWER_INTROS, CHAMBER_THEMES, TOWER_ROOMS} = G;
const AREAS = AREAS_ARG.length ? AREAS_ARG : G.LOCATIONS.map((l,i)=>i);

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
  get(x, y){ const i = (y*this.w+x)*3; return [this.d[i], this.d[i+1], this.d[i+2]]; }
  mix(x, y, c, a){ if(x<0||y<0||x>=this.w||y>=this.h) return; this.set(x, y, blend(this.get(x, y), c, a)); }
  wash(x0, y0, w, h, c, a){ for(let y=y0; y<y0+h; y++) for(let x=x0; x<x0+w; x++) this.mix(x, y, c, a); }
  filter(x0, y0, w, h, f){ for(let y=y0; y<y0+h; y++) for(let x=x0; x<x0+w; x++){ if(x<0||y<0||x>=this.w||y>=this.h) continue;
    this.set(x, y, f(this.get(x, y)).map(q=>Math.max(0, Math.min(255, Math.round(q))))); } }
}

// Pictures go out as PNG (node's zlib; no dependencies).
const zlib = require('zlib');
const CRC = (()=>{ const t = new Int32Array(256); for(let n=0;n<256;n++){ let c = n; for(let k=0;k<8;k++) c = c&1 ? 0xedb88320 ^ (c>>>1) : c>>>1; t[n] = c; } return t; })();
function crc32(buf){ let c = -1; for(const b of buf) c = CRC[(c ^ b) & 255] ^ (c >>> 8); return (c ^ -1) >>> 0; }
function writePng(file, w, h, rgb){
  const raw = Buffer.alloc((w*3+1)*h);
  for(let y=0; y<h; y++){ raw[y*(w*3+1)] = 0; rgb.copy(raw, y*(w*3+1)+1, y*w*3, (y+1)*w*3); }
  const chunk = (type, data)=>{ const len = Buffer.alloc(4); len.writeUInt32BE(data.length); const td = Buffer.concat([Buffer.from(type), data]);
    const crc = Buffer.alloc(4); crc.writeUInt32BE(crc32(td)); return Buffer.concat([len, td, crc]); };
  const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 2;
  fs.writeFileSync(file, Buffer.concat([Buffer.from([0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a]), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]));
}
function crop(c, x0, y0, w, h){ const out = new Canvas(w, h); for(let y=0; y<h; y++) c.d.copy(out.d, y*w*3, ((y0+y)*c.w + x0)*3, ((y0+y)*c.w + x0 + w)*3); return out; }
const savePng = (name, c)=>writePng(path.join(OUT, name), c.w, c.h, c.d);

// ---------- Tile behaviours (what the GBA needs to know about each tile) ----------
const GRASS = hex(G.GROUND.base);
const tallGrass = svgPixels(G.tallGrassSvg());
const BEHAVIOUR = {walk:0, solid:1, tall:2, water:3, ledgeDown:4, ledgeRight:5, ledgeLeft:6, sign:7, door:8, item:9,
  counter:10, pc:11, mat:12, statue:13, windUp:14, windDown:15, windLeft:16, windRight:17};
function behaviour(ch){
  if(ch==='"') return BEHAVIOUR.tall;
  if(ch==='~') return BEHAVIOUR.water;
  if(ch==='L') return BEHAVIOUR.ledgeDown;
  if(ch==='>') return BEHAVIOUR.ledgeRight;
  if(ch==='<') return BEHAVIOUR.ledgeLeft;
  if(ch==='S' || ch==='N') return BEHAVIOUR.sign;
  if(ch==='D') return BEHAVIOUR.door;
  if(ch==='I') return BEHAVIOUR.item;
  if(ch==='c' || ch==='C') return BEHAVIOUR.counter;
  if(ch==='P') return BEHAVIOUR.pc;
  if(ch==='M') return BEHAVIOUR.mat;
  if(ch==='u') return BEHAVIOUR.statue;
  if(ch==='8') return BEHAVIOUR.windUp;      // 4.0.0: the SKYREACH's wind currents
  if(ch==='2') return BEHAVIOUR.windDown;
  if(ch==='4') return BEHAVIOUR.windLeft;
  if(ch==='6') return BEHAVIOUR.windRight;
  return G.WALKABLE.has(ch) ? BEHAVIOUR.walk : BEHAVIOUR.solid;
}

// ---------- Areas ----------
// An area at 1 GBA pixel per art pixel, composed like tileHtml/buildingHtml and the stylesheet: ground art
// (cave and underwater areas swap in their own floor and rock), shorelines, the weather on the ground
// (puddles, snow drifts, settled ash, caps on the trees), buildings, then tall pieces that overhang the
// tile above, in row order. Dive spots are darker water; light shafts under the sea are paler.
const blend = (c, o, a)=>[0,1,2].map(i=>Math.round(c[i]*(1-a) + o[i]*a));
function hexa(h){ const c = hex(h.slice(0,7)); return [c, h.length>7 ? parseInt(h.slice(7,9),16)/255 : 1]; }
function ellipse(c, cx, cy, rx, ry, col, a){
  for(let y=Math.floor(cy-ry); y<=cy+ry; y++) for(let x=Math.floor(cx-rx); x<=cx+rx; x++){
    const u = (x+0.5-cx)/rx, v = (y+0.5-cy)/ry;
    if(u*u+v*v<=1) c.mix(x, y, col, a);
  }
}
// `view` is the weather of the area you're standing in: the stylesheet hangs the tree caps and the colour
// filters on the view, so a neighbour seen across a connection takes them from where you are.
// opts: frameB draws water and flowers in their second animation frame; noItems leaves the item balls out
// (what's under one once it's picked up); noAsh leaves the settled ash off (where you've walked).
function drawArea(map, view, opts={}){
  const c = new Canvas(map.w*16, map.h*16, GRASS);
  const at = (x,y)=>{ const ch = map.tiles[y][x]; return opts.noItems && ch==='I' ? '.' : ch; };
  const cave = map.cave, deep = map.deep, wx = map.weather;
  const floorArt = v=>(cave ? 'cave_floor_v' : 'deep_floor_v') + (v%4===1 || v%4===3 ? 1 : 0);
  // Ground layer.
  for(let y=0; y<map.h; y++) for(let x=0; x<map.w; x++){
    const ch = at(x,y), cls = G.TILE_CLS[ch], X = x*16, Y = y*16;
    const v = ((x*73856093) ^ (y*19349663)) >>> 0;   // the web game's per-tile variation
    if((cave || deep) && ['grass','item','rsign','sign','rock','tablet','exit'].includes(cls)){ c.draw(artPixels(floorArt(v)), X, Y); continue; }
    if(cave && cls==='tree'){ c.draw(artPixels('cave_wall'), X, Y); if(y+1>=map.h || at(x,y+1)!=='T') c.draw(artPixels('cave_face'), X, Y); continue; }
    if(deep && cls==='tree'){ c.draw(artPixels('deep_rock'), X, Y); if(y+1>=map.h || at(x,y+1)!=='T') c.draw(artPixels('deep_face'), X, Y); continue; }
    if(deep && cls==='tall'){ c.draw(artPixels('deep_weed'), X, Y); continue; }
    if(cls==='grass' || cls==='tablet') c.draw(artPixels('grass_v'+(cls==='tablet' ? 0 : v%4)), X, Y);
    else if(cls==='wind') windTile(c, X, Y, ch);
    else if(cls==='tall') c.draw(tallGrass, X, Y);
    else if(cls==='path' || cls==='exit' || cls==='water'){
      const kind = cls==='water' ? 'water' : 'path';
      c.draw(artPixels(kind==='water' && opts.frameB ? 'water_b' : kind), X, Y);
      const ov = G.edgeOverlay(map, x, y, G.EDGE_GROUPS[kind], kind);
      for(const m of ov.matchAll(/--art-([a-z_]+)\)/g)) c.draw(artPixels(m[1]), X, Y);
      // Dive spots (.divespot): brightness .55, saturate 1.25.
      if(kind==='water' && map.diveSpots && map.diveSpots.has(x+','+y)) c.filter(X, Y, 16, 16, p=>saturate(p.map(q=>q*0.55), 1.25));
    }
    else if(cls==='bush') c.draw(artPixels('bush'), X, Y);
    else if(cls==='ledge-e') c.draw(artPixels('ledge_e'), X, Y);
    else if(cls==='ledge-w') c.draw(artPixels('ledge_w'), X, Y);
    else if(cls==='flower') c.draw(artPixels(opts.frameB ? 'flower_b' : 'flower'), X, Y);
    else if(cls!=='tree' && G.TILE_ART[cls]) c.draw(artPixels(cls), X, Y);   // fence, sign, rsign, rock, item, ledge
    // Light shafts under the sea (.shaft): a pale wash.
    if(map.shafts && map.shafts.has(x+','+y)) c.wash(X, Y, 16, 16, hex('#d8f8ff'), 0x50/255);
    // Weather on the ground (tileHtml): puddles in the rain, drifts in the snow, ash on the grass.
    const h = v % 11;
    if(wx==='rain' && cls==='grass' && h<2) ellipse(c, X+8, Y+8.25, 5, 2.25, hex('#6f9ed0'), 0xaa/255);
    if(wx==='snow' && cls==='grass' && h<4){ ellipse(c, X+4.8, Y+9.6, 4.5*0.7, 2.5*0.7, hex('#fbfdff'), 0xee/255); ellipse(c, X+11.5, Y+5.6, 3.5*0.7, 2*0.7, hex('#f4f8ff'), 0xdd/255); }
    if(wx==='ash' && !opts.noAsh && (cls==='grass' && h<6 || cls==='tall')){
      c.wash(X, Y, 16, 16, hex('#8a8a90'), 0x40/255);
      for(const [fx,fy,col] of [[0.2,0.3,'#6a6a70'],[0.6,0.7,'#707078'],[0.8,0.2,'#5c5c64'],[0.4,0.85,'#66666e']]){
        const px = X + Math.floor(fx*16), py = Y + Math.floor(fy*16);
        c.set(px, py, hex(col));
      }
    }
  }
  // Buildings (as buildingHtml lays them out).
  for(const b of map.buildings){
    const roof = b.kind==='center' ? 'red' : b.kind==='mart' ? 'blue' : b.kind==='gym' || b.kind==='league' || b.kind==='tower' ? 'slate'
      : G.HOUSE_ROOF_NAMES[G.HOUSE_ROOFS.indexOf(b.roof)] || 'green';
    const wall = b.kind==='gym' || b.kind==='league' || b.kind==='tower' ? 'grey' : 'cream';
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
    // Name plates on the lower roof row, centred (.plate: POKéMON on red, MART on blue, GYM / LEAGUE on gold).
    const plate = {center:['POKEMON', [0xe0,0x50,0x50], [0xf8,0xf8,0xf8]], mart:['MART', [0x38,0x68,0xc8], [0xf8,0xf8,0xf8]],
      gym:['GYM', [0xe8,0xb8,0x30], [0x30,0x28,0x30]], league:['LEAGUE', [0xe8,0xb8,0x30], [0x30,0x28,0x30]],
      tower:['TOWER', [0x90,0x60,0xd0], [0xf8,0xf8,0xf8]]}[b.kind];
    if(plate){
      const [text, bg, fg] = plate, w = text.length*4 + 3, hh = 9;
      const x0 = Math.round(b.x*16 + b.w*8 - w/2), y0 = b.y*16 + 24 - 4;
      c.rect(x0, y0, w, hh, [0x30,0x28,0x30]);
      c.rect(x0+1, y0+1, w-2, hh-2, bg);
      plateLetters(c, x0+2, y0+2, text, fg);
    }
  }
  // Tall pieces last, row by row: tree canopies and tablets rise 8 px into the tile above; snow and ash
  // settle on the treetops.
  for(let y=0; y<map.h; y++) for(let x=0; x<map.w; x++){
    if(at(x,y)==='T' && !cave && !deep){
      c.draw(artPixels((x+y)%2 ? 'tree2' : 'tree'), x*16, y*16 - 8);
      if(view==='snow'){ const X = x*16+1.5, Y = y*16-7.5; ellipse(c, X+6.5, Y+2.5, 6.5, 3, hex('#ffffff'), 1); ellipse(c, X+6.5, Y+3.2, 6, 2.2, hex('#e4eef8'), 0.6); }
      if(view==='ash') ellipse(c, x*16+2+6, y*16-7+2, 5, 2.2, hex('#9a9aa2'), 0xcc/255);
    }
    if(at(x,y)==='^') c.draw(artPixels('tablet'), x*16, y*16 - 8);
  }
  // Whole-area colour (the stylesheet's filters on #owTiles): rain is darker and richer, snow pale.
  if(view==='rain') c.filter(0, 0, c.w, c.h, p=>contrast(saturate(p.map(q=>q*0.9), 1.15), 1.05));
  if(view==='snow') c.filter(0, 0, c.w, c.h, p=>saturate(p, 0.3).map(q=>q*1.22));
  // 3.0.1: a volcano's slopes (MAGMA ISLE): greens burn to dark basalt and scorched scrub, earth goes red;
  // the sea stays blue.
  // 4.0.0: the SKYREACH: red mesas (greens dry to ochre scrub, earth goes red-orange) and cloud level (pale,
  // windswept grass, grey-white earth); water stays blue.
  if(map.tint==='mesa') c.filter(0, 0, c.w, c.h, ([r,g,b])=>{
    const L = 0.3*r + 0.59*g + 0.11*b;
    if(g > r && g >= b) return [L*0.95 + 40, L*0.72 + 18, L*0.36 + 8];
    if(r >= g && g >= b) return [Math.min(255, r*1.05), g*0.7, b*0.55];
    return [r, g, b];
  });
  if(map.tint==='cloud') c.filter(0, 0, c.w, c.h, ([r,g,b])=>{
    const L = 0.3*r + 0.59*g + 0.11*b;
    if(g > r && g >= b) return [L*0.55 + 110, L*0.6 + 112, L*0.55 + 128];
    if(r >= g && g >= b) return [L*0.6 + 90, L*0.6 + 92, L*0.6 + 100];
    return [r, g, b];
  });
  if(map.volcano) c.filter(0, 0, c.w, c.h, ([r,g,b])=>{
    const L = 0.3*r + 0.59*g + 0.11*b;
    if(g > r && g >= b) return [L*0.62 + 30, L*0.40 + 12, L*0.34 + 10];
    if(r >= g && g >= b) return [r*0.85, g*0.55, b*0.5];
    return [r, g, b];
  });
  return c;
}
// 4.0.0: a wind current: grass with pale streaks and a chevron pointing the way it blows.
function windTile(c, X, Y, ch){
  c.draw(artPixels('grass_v0'), X, Y);
  const t = ([x,y])=>ch==='6' ? [x,y] : ch==='4' ? [15-x,y] : ch==='2' ? [y,x] : [y,15-x];   // drawn blowing right
  const put = (x,y,col)=>{ const [a,b] = t([x,y]); c.set(X+a, Y+b, hex(col)); };
  for(const [y,x0,x1] of [[3,1,7],[8,3,10],[12,0,5]]) for(let x=x0; x<=x1; x++) put(x, y, x===x1 ? '#ffffff' : '#d8ecf8');
  for(let k=0; k<4; k++){ put(10+k, 4+k, '#ffffff'); put(10+k, 11-k, '#ffffff'); }
}
// CSS filter maths (filter-effects spec), on 0-255 rgb.
function saturate(p, s){
  const [r,g,b] = p;
  const m = [0.213+0.787*s, 0.715-0.715*s, 0.072-0.072*s, 0.213-0.213*s, 0.715+0.285*s, 0.072-0.072*s, 0.213-0.213*s, 0.715-0.715*s, 0.072+0.928*s];
  return [m[0]*r+m[1]*g+m[2]*b, m[3]*r+m[4]*g+m[5]*b, m[6]*r+m[7]*g+m[8]*b];
}
function contrast(p, k){ return p.map(q=>(q-127.5)*k+127.5); }
// A 3x5 pixel font for the building plates.
const PLATE_FONT = {P:['111','101','111','100','100'], O:['111','101','101','101','111'], K:['101','110','100','110','101'],
  E:['111','100','110','100','111'], M:['101','111','111','101','101'], N:['111','101','101','101','101'],
  A:['010','101','111','101','101'], R:['110','101','110','101','101'], T:['111','010','010','010','010'],
  G:['111','100','101','101','111'], Y:['101','101','010','010','010'], L:['100','100','100','100','111'], U:['101','101','101','101','111'],
  W:['101','101','111','111','101']};
function plateLetters(c, x, y, text, col=[0xf8,0xf8,0xf8]){
  [...text].forEach((ch,i)=>PLATE_FONT[ch].forEach((row,yy)=>[...row].forEach((b,xx)=>{ if(b==='1') c.set(x+i*4+xx, y+yy, col); })));
}

// ---------- Rooms (getInterior + the room rules in css/style.css) ----------
const ROOM_ART = {house:{floor:'floor_house', wall:'wall_house', counter:'counter', cend:'counter_end'},
  center:{floor:'floor_center', wall:'wall_center', counter:'counter', cend:'counter_end'},
  mart:{floor:'floor_mart', wall:'wall_mart', counter:'counter_mart', cend:'counter_end_mart'}};
const DECO = {n:'deco_poster', m:'deco_map', w:'deco_window', K:'deco_clock'};
const TALL_PIECES = new Set(['bookshelf', 'plant', 'shelf', 'healer', 'pc']);
// Gyms (css: .room-gym, .gym-<theme>): the theme's floor art under everything, maze walls and statues on
// it, plain painted walls, a rug and the exit mat.
const GYM_WALL = {fire:['#e08858','#b05830'], water:['#a8d8f8','#6098d0'], ground:['#c8a060','#987038'], ghost:['#786098','#503870'],
  electric:['#f8e890','#b09020'], grass:['#a8d880','#488830'], ice:['#e8f8ff','#88c0e0'], dragon:['#8870a8','#4c3868'],
  flying:['#d0e8f8','#7898c0'], normal:['#e8d8b8','#a88858'], rock:['#b0a090','#706050'],
  bug:['#c8d870','#788820'], steel:['#d0d8e0','#808898'], fairy:['#f8d8e8','#d088a8'], poison:['#c0a0d0','#704888'],
  fighting:['#e8c8a0','#a07040'], psychic:['#f8c0d8','#c05888']};
const GYM_RUG = {fire:'#f8c048', water:'#f8f8f8', ground:'#98b050', ghost:'#9870c8', electric:'#3868d0', grass:'#f0e0a0', ice:'#4890c8', dragon:'#e0a030',
  flying:'#f8f8f8', normal:'#d84848', rock:'#c04040', bug:'#e0c030', steel:'#e08030', fairy:'#f8f8f8', poison:'#a0e048',
  fighting:'#d83830', psychic:'#f8e8a0', league:'#9080c0'};
const GYM_FLOOR = {electric:['#f0d860','#c8a830'], grass:['#78b858','#58983c'], ice:['#d8f0f8','#a8d8f0'], dragon:['#685088','#4c3868'],
  flying:['#a8d0f0','#7898c0'], normal:['#d8b880','#a88858'], rock:['#a89888','#786858'],
  // 3.0.0: the Sundered Isles' gyms.
  bug:['#b0c060','#88982c'], steel:['#b8c0c8','#888f98'], fairy:['#f8e0ec','#e8b8d0'], poison:['#a080b8','#785090'],
  fighting:['#d8b888','#b08858'], psychic:['#f0b8d0','#d888b0']};
// The four newer gyms have no floor or wall art, only the stylesheet's patterns (.gym-electric etc.).
function gymFloorPattern(theme){
  const [f] = GYM_FLOOR[theme], c = new Canvas(16, 16, hex(f));
  if(theme==='electric'){ c.rect(0, 0, 16, 1, hex('#303038')); c.rect(0, 0, 1, 16, hex('#303038')); }
  if(theme==='grass'){ for(const [x,y] of [[0,0],[8,0],[0,8],[8,8]]) ellipse(c, x+0.5, y+0.5, 1.2, 1.2, hex('#98d070'), 1);
    for(const [x,y] of [[4,5.5],[15,5.5],[4,16.5],[15,16.5]]) ellipse(c, x, y, 0.9, 0.9, hex('#4c8c34'), 1); }
  if(theme==='ice') for(let y=0; y<16; y++) for(let x=0; x<16; x++) if(((x+y)%12+12)%12===5 || ((x+y)%12+12)%12===6) c.set(x, y, hex('#ffffff'));
  if(theme==='flying') for(const [x,y] of [[3,4],[11,12]]){ c.rect(x, y, 4, 1, hex('#f8f8f8')); c.rect(x+1, y-1, 2, 1, hex('#f8f8f8')); }
  if(theme==='normal') for(let y=0; y<16; y+=4) c.rect(0, y, 16, 1, hex('#b89868'));
  if(theme==='rock') for(const [x,y] of [[2,3],[10,6],[5,11],[13,13]]) c.rect(x, y, 2, 2, hex('#786858'));
  if(theme==='bug') for(const [x,y] of [[2,2],[10,10]]){ c.rect(x, y, 4, 3, hex('#d8d878')); c.rect(x+1, y+1, 2, 1, hex('#88982c')); }
  if(theme==='steel'){ c.rect(0, 0, 16, 1, hex('#788088')); c.rect(0, 0, 1, 16, hex('#788088')); for(const [x,y] of [[2,2],[13,2],[2,13],[13,13]]) c.set(x, y, hex('#e8eef4')); }
  if(theme==='fairy') for(const [x,y] of [[4,4],[12,12]]){ c.rect(x-1, y, 3, 1, hex('#ffffff')); c.rect(x, y-1, 1, 3, hex('#ffffff')); }
  if(theme==='poison') for(const [x,y] of [[3,4],[11,9],[6,13]]) ellipse(c, x, y, 1.6, 1.6, hex('#b8e050'), 1);
  if(theme==='fighting'){ for(let y=0; y<16; y+=8) c.rect(0, y, 16, 1, hex('#a07848')); c.rect(8, 0, 1, 8, hex('#a07848')); c.rect(0, 8, 1, 8, hex('#a07848')); }
  if(theme==='psychic') for(let y=0; y<16; y++) for(let x=0; x<16; x++) if(Math.abs(Math.hypot(x-7.5, y-7.5)-5) < 0.6) c.set(x, y, hex('#f8e0f0'));
  if(theme==='dragon'){ c.rect(0, 0, 16, 1, hex('#4c3868')); c.rect(0, 8, 16, 1, hex('#4c3868')); c.rect(0, 0, 1, 16, hex('#4c3868')); c.rect(8, 0, 1, 16, hex('#4c3868')); }
  return {w:16, h:16, px:Array.from({length:16}, (_,y)=>Array.from({length:16}, (_,x)=>c.get(x, y)))};
}
// A maze wall drawn by .t-gymwall::before: a rounded block inset 1 px with a vertical gradient, a dark
// bottom edge and a light top edge (balls for the grass gym).
function gymWallPattern(bg, top, bottom, shade, light, round){
  const c = new Canvas(16, 16, hex(bg));
  for(let y=1; y<15; y++) for(let x=1; x<15; x++){
    if(round){ const u = (x+0.5-8)/7, v = (y+0.5-8)/7; if(u*u+v*v>1) continue; }
    else if((x<3 || x>12) && (y<3 || y>12) && Math.hypot(x<3 ? 3-x-0.5 : x+0.5-13, y<3 ? 3-y-0.5 : y+0.5-13) > 2.5) continue;
    const t = (y-1)/13;
    let col = blend(hex(top), hex(bottom), t);
    if(y>=12) col = hex(shade); else if(light && y<=2) col = hex(light);
    c.set(x, y, col);
  }
  return {w:16, h:16, px:Array.from({length:16}, (_,y)=>Array.from({length:16}, (_,x)=>c.get(x, y)))};
}
const GYMWALL_CSS = {electric:['#c8a830','#f8e048','#c89818','#806010','#fff8b0',false], grass:['#58983c','#88c860','#285a1c','#1c4012',null,true],
  ice:['#a8d8f0','#f0fbff','#98d0f0','#5898c8','#fff',false], dragon:['#4c3868','#a04838','#602820','#401810','#e07860',false],
  flying:['#7898c0','#e8f4ff','#a8c8e8','#587898','#fff',true], normal:['#a88858','#e8c890','#c8a068','#785830','#f8e8c0',false],
  rock:['#786858','#b8a898','#988878','#504038','#d8c8b8',false],
  bug:['#88982c','#d8e078','#a8b840','#586018','#f0f8b0',true], steel:['#888f98','#e8eef4','#a8b0b8','#585f68','#fff',false],
  fairy:['#e8b8d0','#fff0f8','#f0c8dc','#b07898','#fff',true], poison:['#785090','#c8a0e0','#9068a8','#482860','#e0c8f0',true],
  fighting:['#b08858','#f0d0a0','#c8a070','#705030','#f8e8c8',false], psychic:['#d888b0','#ffe0f0','#e8a8c8','#a05080','#fff',true],
  league:['#8878b0','#e0d0f8','#9080c0','#605088','#fff',false]};
function drawGym(room, opts={}){
  const league = room.interior==='league';
  const theme = league ? 'league' : room.theme || 'fire', c = new Canvas(room.w*16, room.h*16, [0,0,0]);
  const at = (x,y)=>room.tiles[y] ? room.tiles[y][x] : '#';
  const floor = league ? null : G.TILE_ART['gymfloor_' + theme] ? artPixels('gymfloor_' + theme) : gymFloorPattern(theme);
  const wallArt = G.TILE_ART['gymwall_' + theme] ? artPixels('gymwall_' + theme) : GYMWALL_CSS[theme] ? gymWallPattern(...GYMWALL_CSS[theme]) : null;
  const statues = [], stones = [];
  for(let y=0; y<room.h; y++) for(let x=0; x<room.w; x++){
    const ch = at(x,y), X = x*16, Y = y*16;
    if(ch==='#'){
      if('#M'.includes(at(x,y+1)) || y+1>=room.h){ c.rect(X, Y, 16, 16, hex('#383040')); c.rect(X, Y+15, 16, 1, hex('#504858')); }
      else if(!league){ const [w1,w2] = GYM_WALL[theme]; const base = theme==='ghost' ? '#281838' : '#584848', cut = theme==='ghost' ? 11 : 12;
        c.rect(X, Y, 16, cut, hex(w1)); c.rect(X, Y+cut-1, 16, 13-cut+1, hex(w2)); c.rect(X, Y+13, 16, 3, hex(base)); }
      continue;   // the League's wall faces have no colour of their own: black
    }
    if(league){ if(ch==='_' || ch==='x' || ch==='^' || ch==='o' || (ch==='u' && room.tower)) c.rect(X, Y, 16, 16, hex('#c0b8d8')); }
    else c.draw(floor, X, Y);
    if(ch==='x' && wallArt) c.draw(wallArt, X, Y);
    else if(ch==='x') c.rect(X, Y, 16, 16, hex(GYM_FLOOR[theme] ? GYM_FLOOR[theme][1] : '#a8a8b8'));
    else if(ch==='~'){ c.draw(artPixels(opts.frameB ? 'water_b' : 'water'), X, Y); }
    else if(ch==='o') c.rect(X, Y, 16, 16, hex(GYM_RUG[theme]));
    else if(ch==='M'){ for(let xx=0; xx<16; xx++) c.rect(X+xx, Y, 1, 16, hex(Math.floor(xx/2)%2 ? '#a03030' : '#c04848')); c.rect(X, Y, 16, 1, hex('#702020')); c.rect(X, Y+15, 16, 1, hex('#702020')); }
    else if(ch==='u') statues.push([X, Y]);
    if(ch==='^') stones.push([X, Y]);
  }
  for(const [X, Y] of statues) c.draw(artPixels('statue'), X, Y - 8);
  // The CHALLENGE TOWER's SUMMONING STONE (a TIDEWARDEN tablet).
  for(const [X, Y] of stones) c.draw(artPixels('tablet'), X, Y - 8);
  return c;
}
// A League gate tile, open: the floor.
function leagueFloor(){ const c = new Canvas(16, 16, hex('#c0b8d8')); return c; }
function drawRoom(room, opts={}){
  if(room.interior==='gym' || room.interior==='league') return drawGym(room, opts);
  const art = ROOM_ART[room.interior], c = new Canvas(room.w*16, room.h*16, [0,0,0]);
  const at = (x,y)=>room.tiles[y] ? room.tiles[y][x] : '#';
  const pieces = [];
  for(let y=0; y<room.h; y++) for(let x=0; x<room.w; x++){
    const ch = at(x,y), X = x*16, Y = y*16;
    let cls = (G.TILE_CLS[ch] || '').split(' ')[0];
    c.draw(artPixels(art.floor), X, Y);
    if(ch==='#'){ c.draw(artPixels('#nmwKM'.includes(at(x,y+1)) || y+1>=room.h ? 'walltop' : art.wall), X, Y); continue; }
    if(DECO[ch]){ c.draw(artPixels(art.wall), X, Y); c.draw(artPixels(DECO[ch]), X, Y); continue; }
    let name = null;
    if(cls==='table' || cls==='shelf'){ let n = 0; while(x-n-1>=0 && at(x-n-1,y)===ch) n++; name = cls + (n%2 ? '_right' : '_left'); }
    else if(cls==='bed') name = y===0 || at(x,y-1)!=='e' ? 'bed_top' : 'bed_bottom';
    else if(ch==='c') name = art.counter;
    else if(ch==='C') name = art.cend;
    else if(ch==='q') name = 'seat';
    else if(ch==='Q') name = 'seat_yellow';
    else if(['bookshelf','tv','plant','healer','pc','glasstable','mat','rug'].includes(cls)) name = cls;
    if(name) pieces.push({name, x:X, y:Y, tall:TALL_PIECES.has(cls) || name==='bed_top'});
  }
  for(const p of pieces){
    const a = artPixels(p.name);
    // Tall pieces are 16x24 and rise 8 px into the tile above; the rest fill their tile.
    c.draw(a, p.x, p.y - (a.h > 16 ? 8 : 0));
  }
  return c;
}

// Trainers as the GBA sees them. Route trainers and gym juniors have their own team (role 'route' / 'junior');
// a gym leader or rival fields the place's signature team (role 'leader' / 'rival'; startTrainerBattle).
function trainerOut(n, loc){
  // Elite Four: their own six, at your average level plus their place in line (startTrainerBattle).
  if(n.elite!=null) return {kind:n.kind, x:n.x, y:n.y, facing:n.facing, role:'elite', elite:n.elite, title:n.title, team:n.team,
    intro:[`${n.title}: "${n.intro}"`], after:[`${n.title}: "${n.after}"`], vanish:false};
  if(n.id) return {kind:n.kind, x:n.x, y:n.y, facing:n.facing, role:/#gym\d/.test(n.id) ? 'junior' : 'route', title:n.title, team:n.team,
    intro:[`${n.title}: "${n.intro}"`], after:[`${n.title}: "${n.after}"`], vanish:false, grunt:n.kind==='grunt'};
  const leader = loc.type==='gym', name = loc.leaderName;
  const quote = n.champion ? loc.championQuote || "So you made it. I always knew it'd be you. One last battle, for real this time!"
    : leader ? `So, a new challenger has come to the ${loc.name.split(' ')[0]} Gym. Show me what your Pokémon can do!`
    : (loc.desc.match(/"([^"]+)"/)||[])[1] || "Let's battle!";
  const theme = leader ? G.gymTheme(loc.leaderName) : '';
  return {kind:n.kind, x:n.x, y:n.y, facing:n.facing, role:n.champion ? 'champion' : leader ? 'leader' : 'rival',
    title:(leader ? 'Gym Leader ' : n.champion && loc.championKind ? 'Champion ' : 'Rival ') + name,
    team:loc.leaderTeam, fill:leader && G.GYM_JUNIORS[theme] ? G.GYM_JUNIORS[theme].team : G.areaPool(loc),
    intro:[`${name}: "${quote}"`],
    after:leader || n.champion ? [`${name}: "You've already beaten me. The road ahead is waiting for you!"`] : (loc.rivalAfter || []).map(l=>`${name.toUpperCase()}: ${l}`),
    vanish:!!n.vanish};
}

// The CHALLENGE TOWER's rooms (all left by the mat to the tower's door). Floors and chambers are drawn like
// gyms in their theme, the summit (and the plain chamber) like the League.
function towerRoom(li, building, kind, theme, rows, b, npcs, things){
  const tiles = rows.map(r=>r.split('')), h = tiles.length, w = tiles[0].length, matX = rows[h-1].indexOf('M');
  const league = theme==='league';
  const room = {w, h, tiles, buildings:[], npcs, exits:[], signs:[], interior:league ? 'league' : 'gym', theme:league ? null : theme,
    spawn:{x:matX, y:h-2}, mat:{x:matX, y:h-1}, tower:true};
  const art = kind + '_' + theme;
  if(!roomArts.has(art)) roomArts.set(art, {area:li, building, room});
  for(const n of npcs) peopleKinds.add(n.kind);
  const trainers = npcs.filter(n=>n.trainer).map(n=>({kind:n.kind, x:n.x, y:n.y, facing:n.facing, role:'tower', elite:n.floor,
    title:n.title, team:n.team, intro:[`${n.title}: "${n.intro}"`], after:[`${n.title}: "${n.after}"`], vanish:false}));
  for(const t of trainers) t.team.forEach(s=>trainerSpecies.add(s));
  rooms.push({area:li, building, kind, art, theme:league ? null : theme, home:false, w, h, tiles:rows,
    trainers, gym:null, behaviour:tiles.map(r=>r.map(behaviour)), spawn:room.spawn, door:{x:b.door.x, y:b.door.y},
    people:[], things, gates:[], floor:npcs.length ? npcs[0].floor : -1});
  return rooms.length - 1;
}
function towerRooms(li, bi, b){
  let first = -1;
  TOWER_FLOORS.forEach((f, k)=>{
    const npc = {kind:f.kind, x:7, y:2, facing:'down', trainer:true, floor:k, title:f.title, team:f.team, intro:TOWER_INTROS[k],
      after:k===4 ? "The summit is yours. Go, and see what answers the stone." : "Up you go. The next floor won't be so kind."};
    const r = towerRoom(li, k ? `${bi}/floor${k}` : bi, 'tower', f.theme, TOWER_ROOMS.floor, b, [npc], []);
    if(!k) first = r;
  });
  towerRoom(li, `${bi}/summit`, 'summit', 'league', TOWER_ROOMS.summit, b, [],
    [{x:7, y:5, text:['A SUMMONING STONE. It hums faintly.']}]);
  for(const t of CHAMBER_THEMES) towerRoom(li, `${bi}/chamber_${t}`, 'chamber', t, TOWER_ROOMS.chamber, b, [], []);
  return {x:b.door.x, y:b.door.y, kind:'tower', room:first};
}

fs.mkdirSync(OUT, {recursive:true});
G.setAdv({cleared:{}, picked:{}, story:{}, items:{}, party:[], box:[]});   // the League's gates start shut
const areas = [], rooms = [], peopleKinds = new Set(['player', 'prof', 'nurse', 'clerk', 'mom', 'rival', 'grunt', 'admin', 'captain', 'pilot']);
const roomArts = new Map(), trainerSpecies = new Set();
const lines = n=>n.lines || [];
const STRIP_X = 10, STRIP_Y = 8;   // how far into a neighbour the camera can see (and the BG draws)
const locIndex = new Map(G.LOCATIONS.map((l,i)=>[l, i]));
for(const li of AREAS){
  const loc = G.LOCATIONS[li], map = G.getMap(loc);
  const V = map.weather;
  const itemsHere = [];
  map.tiles.forEach((r,y)=>r.forEach((ch,x)=>{ if(ch==='I') itemsHere.push([x, y]); }));
  // The area in both animation frames, what's under its item balls, and (ash) the ground swept clean.
  const pictureSet = (m, view, rect, itemList, base)=>{
    const a = drawArea(m, view), b = drawArea(m, view, {frameB:true}), r = rect || {x:0, y:0, w:m.w, h:m.h};
    savePng(base + '.png', crop(a, r.x*16, r.y*16, r.w*16, r.h*16));
    savePng(base + '_b.png', crop(b, r.x*16, r.y*16, r.w*16, r.h*16));
    if(itemList.length){
      const picked = drawArea(m, view, {noItems:true}), strip = new Canvas(itemList.length*16, 16);
      itemList.forEach(([x,y],k)=>{ for(let yy=0; yy<16; yy++) picked.d.copy(strip.d, (yy*strip.w + k*16)*3, ((y*16+yy)*picked.w + x*16)*3, ((y*16+yy)*picked.w + x*16 + 16)*3); });
      savePng(base + '_items.png', strip);
    }
  };
  pictureSet(map, V, null, itemsHere, `map_${li}`);
  if(V==='ash') savePng(`map_${li}_clean.png`, drawArea(map, V, {noAsh:true}));
  const nb = G.neighbours(li);
  const people = map.npcs.filter(n=>!n.trainer && !n.legend), trainers = map.npcs.filter(n=>n.trainer && !n.legend);
  for(const n of map.npcs) if(!n.legend) peopleKinds.add(n.kind);
  for(const t of trainers.map(n=>trainerOut(n, loc))) [...t.team, ...(t.fill || [])].forEach(s=>trainerSpecies.add(s));
  // What can be seen of each neighbour from here, drawn with this area's weather.
  const strips = nb.map((n, k)=>{
    const m = n.map, d = n.exit.dir;
    const r = d==='right' ? {x:0, y:0, w:Math.min(STRIP_X, m.w), h:m.h} : d==='left' ? {x:Math.max(0, m.w-STRIP_X), y:0, w:Math.min(STRIP_X, m.w), h:m.h}
      : d==='down' ? {x:0, y:0, w:m.w, h:Math.min(STRIP_Y, m.h)} : {x:0, y:Math.max(0, m.h-STRIP_Y), w:m.w, h:Math.min(STRIP_Y, m.h)};
    const its = [];
    m.tiles.forEach((row,y)=>row.forEach((ch,x)=>{ if(ch==='I' && x>=r.x && y>=r.y && x<r.x+r.w && y<r.y+r.h) its.push([x, y]); }));
    pictureSet(m, V, r, its, `strip_${li}_${k}`);
    return {...r, items:its};
  });
  const doors = map.buildings.map((b, bi)=>{
    if(b.kind==='tower') return towerRooms(li, bi, b);
    const room = G.getInterior(loc, bi);
    const art = room.interior==='gym' ? 'gym_' + room.theme : room.interior;
    if(!roomArts.has(art)) roomArts.set(art, {area:li, building:bi});
    for(const n of room.npcs) peopleKinds.add(n.kind);
    const roomTrainers = room.npcs.filter(n=>n.trainer).map(n=>trainerOut(n, loc));
    for(const t of roomTrainers) [...t.team, ...(t.fill || [])].forEach(s=>trainerSpecies.add(s));
    rooms.push({area:li, building:bi, kind:room.interior, art, theme:room.theme, home:!!b.home, w:room.w, h:room.h, tiles:room.tiles.map(r=>r.join('')),
      trainers:roomTrainers, gym:room.interior==='gym' ? {name:loc.name.toUpperCase(), leader:loc.leaderName} : null,
      behaviour:room.tiles.map(r=>r.map(behaviour)), spawn:room.spawn, door:{x:b.door.x, y:b.door.y},
      people:room.npcs.filter(n=>!n.trainer).map(n=>({kind:n.kind, x:n.x, y:n.y, facing:n.facing, role:n.role || '', wander:!!n.wander, lines:lines(n)})),
      things:room.tiles.flatMap((r,y)=>r.map((ch,x)=>({x, y, text:G.THING_TEXT[(G.TILE_CLS[ch]||'').split(' ')[0]]})).filter(t=>t.text)),
      // League: the gate rows that open as each Elite Four trainer is beaten (leagueGates).
      gates:room.interior==='league' ? [17,13,9,5].map((row,k)=>({y:row, x0:6, x1:8, elite:k})) : []});
    return {x:b.door.x, y:b.door.y, kind:b.kind, room:rooms.length-1};
  });
  // Item balls the generator placed without a type are POKé BALLS (as in owInteract); in picture order.
  const items = itemsHere.map(([x,y])=>({x, y, id:(map.itemTypes && map.itemTypes[`${x},${y}`]) || 'pokeball'}));
  const legend = map.npcs.find(n=>n.legend);
  const keys = set=>[...(set || [])].map(k=>k.split(',').map(Number));
  areas.push({index:li, region:loc.region || 1, name:loc.name, type:loc.type, kind:loc.kind || '', w:map.w, h:map.h, desc:loc.desc, at:loc.at,
    tiles:map.tiles.map(r=>r.join('')), behaviour:map.tiles.map(r=>r.map(behaviour)),
    links:nb.map((n,k)=>({to:n.exit.to, dir:n.exit.dir, ox:n.ox, oy:n.oy, w:n.map.w, h:n.map.h, name:G.LOCATIONS[n.exit.to].name, gate:!!n.exit.gate,
      badges:G.LOCATIONS[n.exit.to].badges || 0, need:n.exit.need || '', strip:strips[k]})),
    gate_kind:loc.type==='gym' ? 'gym' : loc.type==='trainer' ? 'rival' : '', leader_name:loc.leaderName || '',
    signs:map.signs.map(s=>({x:s.x, y:s.y, text:s.text || '', route:!!s.route, lines:s.lines || null})),
    doors, items,
    people:people.filter(n=>n.role!=='trader' && n.role!=='tower').map(n=>({kind:n.kind, x:n.x, y:n.y, facing:n.facing, wander:!!n.wander, lines:lines(n)})),
    trainers:(loc===TRADE_TOWN || loc===SAFARI ? [] : trainers).map(n=>trainerOut(n, loc)).concat(loc.scene==='portmere' ? [{kind:'grunt', x:0, y:0, facing:'down', role:'route',
      title:'TEMPEST GRUNT', team:['Poochyena','Carvanha','Zubat'], scene:true, vanish:false,
      intro:['TEMPEST GRUNT: "The Admin said nobody gets past. That means you!"'],
      after:['TEMPEST GRUNT: "Go ahead, then. You\'ll never reach the shrine without a way to dive."']}] : []),
    spawn:map.spawn, pool:loc.pool || [], area_pool:G.areaPool(loc), water:G.waterPool(loc), fish:G.fishPool(loc), tier:loc.tier ?? li,
    theme:loc.theme || 'plain', weather:map.weather || '', volcano:!!map.volcano, tint:map.tint || '', cave:!!map.cave, deep:!!map.deep, center:!!loc.center,
    dive:loc.dive ?? -1, surface:loc.surface ?? -1, dive_spots:keys(map.diveSpots), shafts:keys(map.shafts),
    scene:loc.scene || '', legend:legend ? {name:legend.legend, x:legend.x, y:legend.y} : null, league:!!loc.league, champion:!!loc.champion, tower_town:loc===TOWER_TOWN, trade_town:loc===TRADE_TOWN, safari:loc===SAFARI, shrine:!!loc.shrine, roam:loc.roam || [],
    leader_team:loc.leaderTeam || [], rival_after:loc.rivalAfter || [], own_pool:!!loc.pool});
  if(legend) trainerSpecies.add(legend.legend);
}
for(const [art, {area, building, room:own}] of roomArts){
  const room = own || G.getInterior(G.LOCATIONS[area], building);
  savePng(`room_${art}.png`, drawRoom(room));
  savePng(`room_${art}_b.png`, drawRoom(room, {frameB:true}));
}
// League gates, open: the floor tile.
savePng('league_floor.png', leagueFloor());

// ---------- People art: 16x21 frames as the web game draws them (charSvg) ----------
// frames[dir] = [stand, step1, step2]; dir is down/up/left (right = left mirrored). `dy` is how far the frame
// sits below the standing frame (the Gen 3 step dip).
const people = {};
for(const kind of peopleKinds){
  people[kind] = {};
  for(const dir of ['down', 'up', 'left']){
    people[kind][dir] = [0, 1, 2].map(f=>{
      const svg = G.charSvg(kind, dir, f, false), p = svgPixels(svg);
      return {dy: /margin:-8px/.test(svg) ? 1 : 0, px:p.px.map(r=>r.map(c=>c ? c : 0))};
    });
  }
}

// The player also has a running pose (charSvg run), and rides a surfing Pokémon (surfSvg's SURF_ROWS).
people.player_run = {};
for(const dir of ['down', 'up', 'left']){
  people.player_run[dir] = [0, 1, 2].map(f=>{
    const svg = G.charSvg('player', dir, f, true), p = svgPixels(svg);
    return {dy: /margin:-8px/.test(svg) ? 1 : 0, px:p.px.map(r=>r.map(c=>c ? c : 0))};
  });
}
const SURF_COLORS = {K:'#202838', B:'#5090e8', b:'#3468c0', W:'#ffffff', L:'#a8d8ff', F:'#88c0f8', w:'#e8f8ff'};
const surf = Object.fromEntries(['down', 'up', 'left'].map(d=>[d, G.SURF_ROWS[d].map(r=>[...r].map(ch=>SURF_COLORS[ch] ? hex(SURF_COLORS[ch]) : 0))]));

// ---------- Species and moves ----------
// Every species in the web game's dex: FREE BATTLE drafts from all of them.
const usedSpecies = new Set(G.DEX.filter(d=>G.DEX_NUM[G.slug(d.name)]).map(d=>d.name));
// Evolutions as the web game's tryEvolve finds them: level-up data first, then its hand-written table.
const evoOf = d=>d.evo ? {to:(G.DEX.find(e=>G.slug(e.name)===d.evo.to) || {}).name, level:d.evo.level}
  : G.EVOLUTIONS[d.name] ? {to:G.EVOLUTIONS[d.name].to, level:G.EVOLUTIONS[d.name].level} : null;
// Catch rates (PokeAPI pokemon_species.csv, vendored in data/capture_rates.json): the web game has none, so
// the GBA uses Emerald's formula with each species' real rate.
const CAPTURE = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'data', 'capture_rates.json'), 'utf8'));
const moveIdx = new Map(), moves = [];
const addMove = m=>{ const k = m.n; if(!moveIdx.has(k)){ moveIdx.set(k, moves.length); moves.push(m); } return moveIdx.get(k); };
addMove(G.STRUGGLE);
const species = [...usedSpecies].map(name=>{
  const d = G.DEX.find(e=>e.name===name);
  const num = G.DEX_NUM[G.slug(d.name)];
  const evo = evoOf(d);
  const rate = CAPTURE[G.slug(d.name)] ?? CAPTURE[G.slug(d.name).replace(/-.*/, '')] ?? 45;
  const info = G.DEXINFO[num] || {h:0, w:0, g:''};
  const ab = d.ability || {n:'', desc:'', type:'flavor'};
  return {name:d.name, num, types:d.types, base:d.base, capture:rate, evo:evo && evo.to && evo.level && usedSpecies.has(evo.to) ? evo : null,
    learn:d.learn || [],
    moves:d.learn ? [] : d.moves.map(addMove),   // hand-made entries without a learnset know these four
    ability:{n:ab.n || '', desc:ab.desc || '', type:ab.type || 'flavor', of:ab.boostType || ab.immuneType || ''},
    height:info.h, weight:info.w, genus:info.g, curated:G.CURATED_DEX.includes(d)};
});
for(const n of trainerSpecies) if(!usedSpecies.has(n)) throw new Error('no species ' + n);

// The GBA's extra move data (tools/build_moves.js: PP, targets, draining, healing, stat changes, weather) and
// the status moves it adds, which join the learnsets after every existing move has its index (saves hold them).
// 2.0's species (PROC_RAW_GEN89) and the moves only they learn come last, so every 1.x move keeps its index.
const EXTRA = G.MOVE_EXTRA, gen89 = new Set(G.GEN89);
Object.assign(G.STRUGGLE, {pp:1, target:'one', drain:-25});
const learnOf = (sp, rows)=>rows.filter(([lv])=>lv<=100).map(([lv, mi])=>[lv, addMove(G.moveByIndex(mi))]);
const addLearn = sp=>{
  const extra = EXTRA.learn[G.slug(sp.name)];
  if(extra && sp.learn.length) sp.learn.push(...learnOf(sp, extra));
  sp.learn.sort((a, b)=>a[0]-b[0]);
};
for(const sp of species) if(!gen89.has(sp.name)) sp.learn = learnOf(sp, sp.learn);
for(const sp of species) if(!gen89.has(sp.name)) addLearn(sp);
const moves_v1 = moves.length;
for(const sp of species) if(gen89.has(sp.name)){ sp.learn = learnOf(sp, sp.learn); addLearn(sp); }

// Music (js/music.js): 8th-note steps per channel.
const music = G.MUSIC_TRACKS ? Object.fromEntries(Object.entries(G.MUSIC_TRACKS).map(([k, t])=>[k, {bpm:t.bpm,
  lead:t.lead, harm:t.harm, bass:t.bass, drum:t.drum}])) : {};

// Move animations (atkFx): each move's script, resolved as the web game does it (MOVE_FX, else MFX_RULES, else
// a rush or an orb; '+EL' becomes its type's flourish).
for(const m of moves) m.fx = G.mfxScript(m);
fs.writeFileSync(path.join(OUT, 'data.json'), JSON.stringify({
  pix:G.PIX, type_col:G.TYPE_COL,
  areas, rooms, people, surf, species, species_v1:species.filter(sp=>!gen89.has(sp.name)).length,
  species_v2:species.length - G.SUNDERED_FORMS.length - G.SKYREACH_FORMS.length, species_v3:species.length - G.SKYREACH_FORMS.length, moves, moves_v1, tall_grass:tallGrass.px, types:G.TYPES, chart:G.CHART, type_colors:G.TYPE_COLORS,
  starter_trios:G.STARTER_TRIOS, prof:G.PROF, intro:G.INTRO_LINES, items:G.ITEM_INFO, held_items:G.ITEMS, prof_calls:G.PROF_CALLS,
  elites:G.ELITES, time_types:G.TIME_TYPES, night_visitors:G.NIGHT_VISITORS, tempest_pool:G.TEMPEST_POOL, admin:G.ADMIN,
  music, weather:G.WEATHER, wallpapers:G.WALLPAPERS, wall_names:G.WALL_NAMES,
  art:Object.fromEntries(['battle_bg', 'plat_enemy_l', 'plat_enemy_m', 'plat_enemy_r', 'plat_player_l', 'plat_player_m', 'plat_player_r',
    'forest_fill', 'grass_v0', 'floor_house', 'dust_1', 'dust_2', 'dust_3', 'water', 'water_b', 'flower', 'flower_b'].map(n=>[n, G.TILE_ART[n]])),
  ball:svgPixels(G.BALL_SVG).px,
}));
console.log(`export: ${areas.length} areas, ${rooms.length} rooms, ${species.length} species, ${moves.length} moves, ${Object.keys(people).length} people`);
