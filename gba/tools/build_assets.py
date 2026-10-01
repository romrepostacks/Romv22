#!/usr/bin/env python3
"""Builds Party Royale's GBA data from the web game.

1. tools/export.js loads the web game's js/*.js and writes the areas and rooms (drawn as pictures), people
   art, species, moves and items.
2. This script turns that into GBA data in <out>/:
   - include/pr_world_data.h  tilesets (outdoors, and one per kind of room): 8x8 tiles, 4bpp palette banks
                              and 16x16 metatiles; every map's metatiles, behaviours, signs, doors, people,
                              trainers, item balls and connections
   - include/pr_game_data.h   species, moves, items, the type chart, intro text
   - include/pr_ids.h         species, item and people enums
   - include/pr_ui_data.h     the window frame tiles (9-slice styles sharing one palette bank)
   - graphics/*.bmp + .json   sprites and full-screen backgrounds for Butano's asset tool
Needs Node.js and Pillow. Called by the Makefile (EXTTOOL) before every build; it skips work when its
inputs haven't changed.
"""
import argparse, hashlib, json, os, re, subprocess
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
GBA = os.path.dirname(HERE)
ROOT = os.path.dirname(GBA)
AREAS = [0, 1, 2, 3]    # Duskmere Hollow, Route 1, Fernway Overlook, Cindergate Town
MAP_BANKS = 15          # BG palette banks a map tileset may use; the 16th is the UI's
ITEM_IDS = ['pokeball', 'potion', 'superpotion', 'hyperpotion', 'antidote', 'parlyzheal', 'awakening', 'burnheal',
            'revive', 'fullrestore']
MART_STOCK = ['pokeball', 'potion', 'antidote', 'parlyzheal', 'awakening']    # martOpen() before 2 badges
STATUS = {None: 'status::NONE', 'psn': 'status::POISON', 'brn': 'status::BURN', 'par': 'status::PARALYSIS',
          'slp': 'status::SLEEP', 'frz': 'status::FREEZE'}

# ---------------------------------------------------------------------------------------------------
def rgb555(c):
    return (c[0] >> 3, c[1] >> 3, c[2] >> 3)

def hex555(h):
    return rgb555((int(h[1:3], 16), int(h[3:5], 16), int(h[5:7], 16)))

def c_color(c):
    return 'bn::color(%d, %d, %d)' % c

def enum_name(s):
    return re.sub(r'[^A-Za-z0-9]', '_', s).upper()

def nonempty(body, dummy):
    """C++ has no empty arrays: a placeholder element (the count stays 0)."""
    return body or dummy

def c_string(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'

def c_text(s):
    return c_string(clean_text(s))

# The Butano font has ASCII plus a few accented letters (é among them, for POKéMON).
TEXT_SWAP = {'→': '>', '←': '<', '↑': '^', '↓': 'v', '…': '...', '“': '"', '”': '"', '‘': "'", '’': "'", '—': '-', '–': '-',
             '₽': '$', '♂': 'M', '♀': 'F', '×': 'x'}
FONT_EXTRA = set('ÁÉÍÓÚÜÑáéíóúüñ¡¿')

def clean_text(s):
    out = ''
    for ch in s:
        ch = TEXT_SWAP.get(ch, ch)
        out += ''.join(c if (32 <= ord(c) < 127 or c in FONT_EXTRA or c == '\n') else '?' for c in ch)
    return out

def write_if_changed(path, text):
    if os.path.exists(path) and open(path, encoding='utf8').read() == text:
        return
    with open(path, 'w', encoding='utf8') as f:
        f.write(text)

# ---------------------------------------------------------------------------------------------------
# Tilesets: cut pictures into 8x8 tiles, share them (with flips), pack their colours into 4bpp banks,
# then describe each picture as 16x16 metatiles of four cells.

def pack_palettes(color_sets, max_banks, what):
    """Packs tile colour sets into banks of 15 colours (index 0 is transparent). Only the sets that aren't
    contained in another need packing; it tries many seeded orders and keeps the fewest banks."""
    import random
    uniq = sorted(set(color_sets), key=lambda s: (-len(s), sorted(s)))
    maximal = [s for s in uniq if not any(s < t for t in uniq)]
    def pack(order, by_overlap):
        banks = []
        for cs in order:
            best = None
            for i, b in enumerate(banks):
                if len(b | cs) <= 15:
                    score = -len(cs & b) if by_overlap else len(cs - b)
                    if best is None or score < best[0]:
                        best = (score, i)
            if best is None:
                banks.append(set(cs))
            else:
                banks[best[1]] |= cs
        return banks
    rnd = random.Random(1234)
    best = None
    for trial in range(400):
        order = list(maximal)
        if trial:
            rnd.shuffle(order)
        for by_overlap in (False, True):
            banks = pack(order, by_overlap)
            if best is None or len(banks) < len(best):
                best = banks
    if len(best) > max_banks:
        raise SystemExit('%s needs %d palette banks (max %d)' % (what, len(best), max_banks))
    assign = [next(i for i, b in enumerate(best) if cs <= b) for cs in color_sets]
    return best, assign

def cut(img, cx, cy):
    px = img.load()
    return tuple(rgb555(px[cx * 8 + x, cy * 8 + y]) for y in range(8) for x in range(8))

def reduce_block(b, max_colors=15):
    """A 4bpp tile has 15 colours: merge the closest pair (keeping the more common) until it fits."""
    counts = {}
    for c in b:
        counts[c] = counts.get(c, 0) + 1
    if len(counts) <= max_colors:
        return b
    remap = {c: c for c in counts}
    while len(counts) > max_colors:
        cols = list(counts)
        best = None
        for i in range(len(cols)):
            for j in range(i + 1, len(cols)):
                d = sum((cols[i][k] - cols[j][k]) ** 2 for k in range(3))
                if best is None or d < best[0]:
                    best = (d, cols[i], cols[j])
        _, a, c2 = best
        keep, drop = (a, c2) if counts[a] >= counts[c2] else (c2, a)
        counts[keep] += counts.pop(drop)
        for k, v in remap.items():
            if v == drop:
                remap[k] = keep
    return tuple(remap[c] for c in b)

def flips(b):
    rows = [b[y * 8:(y + 1) * 8] for y in range(8)]
    h = tuple(p for r in rows for p in reversed(r))
    v = tuple(p for r in reversed(rows) for p in r)
    hv = tuple(p for r in reversed(rows) for p in reversed(r))
    return [(b, 0), (h, 1), (v, 2), (hv, 3)]

def tile_words(b, lut):
    words = []
    for y in range(8):
        v = 0
        for x in range(8):
            v |= lut[b[y * 8 + x]] << (4 * x)
        words.append('0x%08x' % v)
    return '{' + ', '.join(words) + '}'

class Tileset:
    def __init__(self, name):
        self.name = name
        self.uniq, self.index = [], {}
        self.pictures = []       # (w, h, refs)
        self.metas, self.meta_index = [], {}

    def ref(self, b):
        for fb, f in flips(b):
            if fb in self.index:
                return self.index[fb], f
        self.index[b] = len(self.uniq)
        self.uniq.append(b)
        return len(self.uniq) - 1, 0

    def add(self, img):
        """Adds a picture (w and h multiples of 16); returns its index."""
        w, h = img.size[0] // 8, img.size[1] // 8
        refs = [[self.ref(reduce_block(cut(img, x, y))) for x in range(w)] for y in range(h)]
        self.pictures.append(refs)
        return len(self.pictures) - 1

    def finish(self):
        sets = [frozenset(b) for b in self.uniq]
        banks, self.assign = pack_palettes(sets, MAP_BANKS, 'tileset ' + self.name)
        self.bank_lists = [sorted(b) for b in banks]
        assert all(len(b) <= 15 for b in self.bank_lists), 'a palette bank has more than 15 colours'
        if len(self.uniq) > 1000:
            raise SystemExit('tileset %s needs %d tiles (max 1000)' % (self.name, len(self.uniq)))

    def cell(self, r):
        t, f = r
        return t | (f << 10) | (self.assign[t] << 12)

    def metatiles_of(self, pic):
        refs = self.pictures[pic]
        out = []
        for y in range(len(refs) // 2):
            for x in range(len(refs[0]) // 2):
                key = tuple(self.cell(r) for r in (refs[2 * y][2 * x], refs[2 * y][2 * x + 1], refs[2 * y + 1][2 * x], refs[2 * y + 1][2 * x + 1]))
                if key not in self.meta_index:
                    self.meta_index[key] = len(self.metas)
                    self.metas.append(key)
                out.append(self.meta_index[key])
        return out

    def emit(self, L, fill_meta, grass_meta):
        p = 'ts_%s_' % self.name
        L.append('alignas(int) constexpr bn::tile %stiles[] = {\n    %s\n};' % (p, ',\n    '.join(
            tile_words(b, {c: i + 1 for i, c in enumerate(self.bank_lists[self.assign[i]])}) for i, b in enumerate(self.uniq))))
        colors = []
        for bl in self.bank_lists:
            bank = [(0, 0, 0)] + bl
            colors += bank + [(0, 0, 0)] * (16 - len(bank))
        L.append('constexpr bn::color %scolors[] = {\n    %s\n};' % (p, ',\n    '.join(
            ', '.join(c_color(c) for c in colors[i:i + 8]) for i in range(0, len(colors), 8))))
        L.append('constexpr uint16_t %smetatiles[][4] = {\n    %s\n};\n' % (p, ',\n    '.join('{%d, %d, %d, %d}' % m for m in self.metas)))
        return '{%stiles, %d, %scolors, %d, %smetatiles, %d, %d}' % (p, len(self.uniq), p, len(colors), p, fill_meta, grass_meta)

def picture(exp, name, w, h):
    return Image.frombytes('RGB', (w * 16, h * 16), open(os.path.join(exp, name), 'rb').read())

def solid(color):
    return Image.new('RGB', (16, 16), color)

def build_world(exp, data, out_inc):
    areas, rooms = data['areas'], data['rooms']
    trainer_count = item_count = 0

    # Outdoors: every area, the forest past the edges and plain grass (drawn over picked-up items).
    outdoor = Tileset('outdoor')
    area_pics = [outdoor.add(picture(exp, 'map_%d.rgb' % a['index'], a['w'], a['h'])) for a in areas]
    forest_pic = outdoor.add(art_image(data['art']['forest_fill'], (0x1e, 0x4a, 0x1c)))
    grass_pic = outdoor.add(art_image(data['art']['grass_v0'], (0x48, 0xd8, 0x90)))
    outdoor.finish()
    tilesets = [outdoor]
    fills = {}
    area_meta = [outdoor.metatiles_of(p) for p in area_pics]
    fills['outdoor'] = (outdoor.metatiles_of(forest_pic)[0], outdoor.metatiles_of(grass_pic)[0])

    # Rooms: one tileset per kind; past the walls is black.
    room_tileset = {}
    room_meta = {}
    for kind in sorted({r['kind'] for r in rooms}):
        r = next(r for r in rooms if r['kind'] == kind)
        ts = Tileset(kind)
        pic = ts.add(picture(exp, 'room_%s.rgb' % kind, r['w'], r['h']))
        black = ts.add(solid((0, 0, 0)))
        ts.finish()
        room_meta[kind] = ts.metatiles_of(pic)
        fills[kind] = (ts.metatiles_of(black)[0],) * 2
        room_tileset[kind] = len(tilesets)
        tilesets.append(ts)

    L = ['// Generated by tools/build_assets.py from the web game; do not edit.',
         '#ifndef PR_WORLD_DATA_H\n#define PR_WORLD_DATA_H\n',
         '#include "bn_tile.h"\n#include "bn_color.h"\n#include "pr_world_types.h"\n',
         'namespace pr::world_data\n{\n']
    ts_rows = [ts.emit(L, *fills[ts.name]) for ts in tilesets]
    L.append('constexpr tileset tilesets[] = {\n    %s\n};\n' % ',\n    '.join(ts_rows))

    def lines_array(name, lines):
        lines = [clean_text(l) for l in lines] or ['...']
        L.append('constexpr const char* %s[] = {%s};' % (name, ', '.join(c_string(l) for l in lines)))
        return len(lines)

    def people_rows(p, people):
        rows = []
        for i, n in enumerate(people):
            count = lines_array('%sperson%d_lines' % (p, i), n['lines'])
            role = {'nurse': 'NURSE', 'clerk': 'CLERK', 'mom': 'MOM'}.get(n.get('role', ''), 'NONE')
            rows.append('{%d, %d, person_kind::%s, direction::%s, person_role::%s, %s, %sperson%d_lines, %d}' % (
                n['x'], n['y'], n['kind'], n['facing'].upper(), role, 'true' if n.get('wander') else 'false', p, i, count))
        L.append('constexpr person %speople[] = {%s};' % (p, nonempty(', '.join(rows),
            '{0, 0, person_kind::player, direction::DOWN, person_role::NONE, false, nullptr, 0}')))

    map_rows = []
    room_index = {}       # (area, building) -> map index
    for i, r in enumerate(rooms):
        room_index[(r['area'], r['building'])] = len(areas) + i

    # Trainer ids (bits in game_state::beaten), areas first then rooms; each area's gate keeper is its rival
    # or its gym's leader.
    ids = {}
    for ai, a in enumerate(areas):
        for ti in range(len(a['trainers'])):
            ids[('area', ai, ti)] = len(ids)
    for ri, r in enumerate(rooms):
        for ti in range(len(r['trainers'])):
            ids[('room', ri, ti)] = len(ids)
    if len(ids) > 64:
        raise SystemExit('%d trainers (max 64)' % len(ids))
    def leader_of(ai):
        a = areas[ai]
        for ti, t in enumerate(a['trainers']):
            if t['role'] in ('rival', 'leader'):
                return ids[('area', ai, ti)]
        for ri, r in enumerate(rooms):
            if r['area'] == a['index']:
                for ti, t in enumerate(r['trainers']):
                    if t['role'] == 'leader':
                        return ids[('room', ri, ti)]
        return -1

    def trainer_rows(p, trainers, key):
        rows = []
        for ti, t in enumerate(trainers):
            q = '%strainer%d_' % (p, ti)
            L.append('constexpr species_id %steam[] = {%s};' % (q, ', '.join('species_id::%s' % enum_name(n) for n in t['team'])))
            fill = t.get('fill') or []
            L.append('constexpr species_id %sfill[] = {%s};' % (q, ', '.join('species_id::%s' % enum_name(n) for n in fill) or 'species_id::PIDGEY'))
            intro = lines_array(q + 'intro', t['intro'])
            after = lines_array(q + 'after', t['after']) if t['after'] else 0
            if not t['after']:
                L.append('constexpr const char* %safter[] = {""};' % q)
            rows.append('{%d, %d, person_kind::%s, direction::%s, trainer_role::%s, %s, %steam, %d, %sfill, %d, %sintro, %d, '
                        '%safter, %d, %s, %d}' % (
                t['x'], t['y'], t['kind'], t['facing'].upper(), t['role'].upper(), c_text(t['title']), q, len(t['team']),
                q, len(fill), q, intro, q, after, 'true' if t['vanish'] else 'false', ids[key + (ti,)]))
        L.append('constexpr trainer %strainers[] = {%s};' % (p, nonempty(', '.join(rows),
            '{0, 0, person_kind::player, direction::DOWN, trainer_role::ROUTE, "", nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, false, 0}')))

    for ai, (a, mm) in enumerate(zip(areas, area_meta)):
        p = 'map%d_' % ai
        L.append('constexpr uint16_t %smap[] = {%s};' % (p, ', '.join(map(str, mm))))
        L.append('constexpr uint8_t %sbehaviour[] = {%s};' % (p, ', '.join(str(v) for row in a['behaviour'] for v in row)))
        # Town signs read their name and then the place's description (owInteract); route signs just point.
        desc = re.sub(r'\s*"[^"]*"\s*', ' ', a['desc']).strip()
        signs = []
        for si, sg in enumerate(a['signs']):
            count = lines_array('%ssign%d_lines' % (p, si), [sg['text']] if sg['route'] else [sg['text'], desc])
            signs.append('{%d, %d, %ssign%d_lines, %d}' % (sg['x'], sg['y'], p, si, count))
        L.append('constexpr sign %ssigns[] = {%s};' % (p, nonempty(', '.join(signs), '{0, 0, nullptr, 0}')))
        L.append('constexpr door %sdoors[] = {%s};' % (p, nonempty(', '.join(
            '{%d, %d, door_kind::%s, %d}' % (d['x'], d['y'], d['kind'].upper(), room_index[(a['index'], bi)])
            for bi, d in enumerate(a['doors'])), '{0, 0, door_kind::HOUSE, -1}')))
        people_rows(p, a['people'])
        trainer_rows(p, a['trainers'], ('area', ai))
        its = []
        for it in a['items']:
            its.append('{%d, %d, item_id::%s, %d}' % (it['x'], it['y'], it['id'].upper(), item_count))
            item_count += 1
        L.append('constexpr item_ball %sitems[] = {%s};' % (p, nonempty(', '.join(its), '{0, 0, item_id::POKEBALL, 0}')))
        links = []
        for ln in a['links']:
            target = AREAS.index(ln['to']) if ln['to'] in AREAS else -1
            links.append('{%d, %d, %d, %d, %d, %s, %s}' % (target, ln['ox'], ln['oy'], ln['w'], ln['h'], c_text(ln['name'].upper()),
                                                        'true' if ln['gate'] else 'false'))
        L.append('constexpr link %slinks[] = {%s};' % (p, nonempty(', '.join(links), '{-1, 0, 0, 0, 0, "", false}')))
        pool = ', '.join('species_id::%s' % enum_name(n) for n in a['pool'])
        L.append('constexpr species_id %spool[] = {%s};\n' % (p, pool or 'species_id::PIDGEY'))
        gate = {'gym': 'GYM', 'rival': 'RIVAL'}.get(a['gate_kind'], 'NONE')
        map_rows.append('{%s, 0, %d, %d, %smap, %sbehaviour, %ssigns, %d, %sdoors, %d, %speople, %d, %strainers, %d, %sitems, %d, '
                        'nullptr, 0, %slinks, %d, %spool, %d, %d, %d, %d, -1, 0, 0, %d, gate_kind::%s, %s, %s}' % (
            c_text(a['name'].upper()), a['w'], a['h'], p, p, p, len(a['signs']), p, len(a['doors']), p, len(a['people']),
            p, len(a['trainers']), p, len(a['items']), p, len(a['links']), p, len(a['pool']),
            a['spawn']['x'], a['spawn']['y'], min(50, 8 + a['tier'] * 4), leader_of(ai), gate,
            c_text(a['name'].upper()), c_text(a['leader_name'])))

    for ri, r in enumerate(rooms):
        p = 'room%d_' % ri
        L.append('constexpr uint8_t %sbehaviour[] = {%s};' % (p, ', '.join(str(v) for row in r['behaviour'] for v in row)))
        people_rows(p, r['people'])
        trainer_rows(p, r['trainers'], ('room', ri))
        things = []
        for ti, t in enumerate(r['things']):
            count = lines_array('%sthing%d_lines' % (p, ti), t['text'])
            things.append('{%d, %d, %sthing%d_lines, %d}' % (t['x'], t['y'], p, ti, count))
        L.append('constexpr thing %sthings[] = {%s};\n' % (p, nonempty(', '.join(things), '{0, 0, nullptr, 0}')))
        area = AREAS.index(r['area'])
        name = {'center': 'POKéMON CENTER', 'mart': 'POKé MART', 'gym': 'POKéMON GYM'}.get(r['kind'], 'HOUSE')
        gym = r.get('gym') or {}
        map_rows.append('{%s, %d, %d, %d, ts_%s_map, %sbehaviour, nullptr, 0, nullptr, 0, %speople, %d, %strainers, %d, nullptr, 0, '
                        '%sthings, %d, nullptr, 0, nullptr, 0, %d, %d, %d, %d, %d, %d, %d, gate_kind::NONE, %s, %s}' % (
            c_text(name), room_tileset[r['kind']], r['w'], r['h'], r['kind'], p, p, len(r['people']), p, len(r['trainers']),
            p, len(r['things']), r['spawn']['x'], r['spawn']['y'], min(50, 8 + areas[area]['tier'] * 4), area,
            r['door']['x'], r['door']['y'], leader_of(area) if r['kind'] == 'gym' else -1,
            c_text(gym.get('name', '')), c_text(gym.get('leader', ''))))
    # Room metatile maps go before the map table (shared by every room of a kind).
    room_maps = ['constexpr uint16_t ts_%s_map[] = {%s};' % (k, ', '.join(map(str, v))) for k, v in sorted(room_meta.items())]
    L += room_maps
    L.append('\nconstexpr map_def maps[] = {\n    %s\n};\n' % ',\n    '.join(map_rows))
    L.append('constexpr int areas_count = %d;' % len(areas))
    L.append('constexpr int trainers_count = %d;' % len(ids))
    L.append('constexpr int items_count = %d;\n' % item_count)
    L.append('}\n\n#endif')
    write_if_changed(os.path.join(out_inc, 'pr_world_data.h'), '\n'.join(L))
    print('build_assets: ' + ', '.join('%s %d tiles/%d banks' % (t.name, len(t.uniq), len(t.bank_lists)) for t in tilesets))

# ---------------------------------------------------------------------------------------------------
def art_image(a, bg=None):
    img = Image.new('RGBA', (a['w'], a['h']), (0, 0, 0, 0))
    px = img.load()
    for y, row in enumerate(a['rows']):
        for x, ch in enumerate(row):
            if ch != '.':
                c = a['pal'][ch]
                px[x, y] = (int(c[1:3], 16), int(c[3:5], 16), int(c[5:7], 16), 255)
    if bg is not None:
        base = Image.new('RGBA', img.size, bg + (255,))
        base.alpha_composite(img)
        return base.convert('RGB')
    return img

def quantize_rgba(img, max_colors, fixed=None):
    """RGBA -> list of opaque rgb555 colours (<= max_colors) and an index map (0 = transparent).
    `fixed` (a palette list) maps colours to existing slots instead."""
    px = img.load()
    w, h = img.size
    cols = {}
    for y in range(h):
        for x in range(w):
            p = px[x, y]
            if p[3] >= 128:
                c = rgb555(p)
                cols[c] = cols.get(c, 0) + 1
    palette = list(fixed) if fixed else list(cols)
    remap = {c: c for c in palette}
    if fixed:
        for c in cols:
            if c not in remap:
                remap[c] = min(palette, key=lambda q: sum((q[k] - c[k]) ** 2 for k in range(3)))
    # Too many colours: merge the closest pair (keeping the more common one) until it fits.
    while len(palette) > max_colors:
        best = None
        for i in range(len(palette)):
            for j in range(i + 1, len(palette)):
                a, b = palette[i], palette[j]
                d = sum((a[k] - b[k]) ** 2 for k in range(3))
                if best is None or d < best[0]:
                    best = (d, i, j)
        _, i, j = best
        keep, drop = (palette[i], palette[j]) if cols[palette[i]] >= cols[palette[j]] else (palette[j], palette[i])
        cols[keep] += cols.pop(drop)
        palette.remove(drop)
        for k, v in remap.items():
            if v == drop:
                remap[k] = keep
    lut = {c: i + 1 for i, c in enumerate(palette)}
    idx = [[0] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            p = px[x, y]
            if p[3] >= 128:
                idx[y][x] = lut[remap[rgb555(p)]]
    return palette, idx

def save_indexed_bmp(path, palette555, idx, json_obj, transparent=(255, 0, 255)):
    """An 8-bit indexed BMP for Butano: colour 0 is transparent, then the palette, padded to 16."""
    h, w = len(idx), len(idx[0])
    img = Image.new('P', (w, h))
    pal = [transparent] + [(c[0] << 3, c[1] << 3, c[2] << 3) for c in palette555]
    n = 16 if len(pal) <= 16 else 256
    pal += [(0, 0, 0)] * (n - len(pal))
    img.putpalette([v for c in pal for v in c])
    img.putdata([v for row in idx for v in row])
    img.save(path)
    write_if_changed(path[:-4] + '.json', json.dumps(json_obj))

def save_sprite(gfx, name, frames, fw, fh):
    sheet = Image.new('RGBA', (fw, fh * len(frames)), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        sheet.alpha_composite(f, (0, i * fh))
    pal, idx = quantize_rgba(sheet, 15)
    save_indexed_bmp(os.path.join(gfx, name + '.bmp'), pal, idx, {'type': 'sprite', 'height': fh, 'bpp_mode': 'bpp_4'})

def save_bg(img, gfx, name, bpp='bpp_4'):
    pal, idx = quantize_rgba(img, 15 if bpp == 'bpp_4' else 255)
    save_indexed_bmp(os.path.join(gfx, name + '.bmp'), pal, idx, {'type': 'regular_bg', 'bpp_mode': bpp})

def person_frame(f, scale=1):
    img = Image.new('RGBA', (16, 21), (0, 0, 0, 0))
    px = img.load()
    for y, row in enumerate(f['px']):
        for x, c in enumerate(row):
            if c:
                px[x, y] = tuple(c) + (255,)
    return img.resize((16 * scale, 21 * scale), Image.NEAREST) if scale > 1 else img

def build_people(data, gfx):
    # 16x32 sprites: the 16x21 frame sits 10 px down (11 on step frames), so the feet land on the tile.
    for kind, dirs in data['people'].items():
        frames = []
        for d in ('down', 'up', 'left'):
            for f in dirs[d]:
                img = Image.new('RGBA', (16, 32), (0, 0, 0, 0))
                img.alpha_composite(person_frame(f), (0, 10 + f['dy']))
                frames.append(img)
        save_sprite(gfx, 'person_' + kind.lower(), frames, 16, 32)     # Butano wants lowercase names
    # The professor in the intro, at twice the size (the web game draws him scaled up on a dark stage).
    big = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    big.alpha_composite(person_frame(data['people']['prof']['down'][0], 2), (16, 64 - 42))
    save_sprite(gfx, 'prof_big', [big], 64, 64)

def mon_sprite(path):
    """A 96x96 sprite cropped to the 64x64 GBA frame: centred on its content, feet at the bottom."""
    img = Image.open(path).convert('RGBA')
    box = img.getbbox() or (0, 0, 96, 96)
    w, h = box[2] - box[0], box[3] - box[1]
    if w > 64 or h > 64:
        s = 64 / max(w, h)
        img = img.crop(box).resize((max(1, int(w * s)), max(1, int(h * s))), Image.NEAREST)
        box = (0, 0) + img.size
        w, h = img.size
    out = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    out.alpha_composite(img.crop(box), ((64 - w) // 2, 64 - h - 2 if h < 62 else 0))
    return out

def build_mons(data, gfx):
    for s in data['species']:
        for side, sub in (('front', ''), ('back', 'back/')):
            path = os.path.join(ROOT, 'sprites', 'pokemon', sub + '%d.png' % s['num'])
            if not os.path.exists(path):
                path = os.path.join(ROOT, 'sprites', 'pokemon', '%d.png' % s['num'])
            pal, idx = quantize_rgba(mon_sprite(path), 15)
            save_indexed_bmp(os.path.join(gfx, 'mon_%s_%d.bmp' % (side, s['num'])), pal, idx,
                             {'type': 'sprite', 'bpp_mode': 'bpp_4'})

# ---------------------------------------------------------------------------------------------------
# UI: 9-slice window styles (8x8 corners, edges and fill), all in one 15-colour palette bank, after the
# web game's stylesheet: .gba-box (white window), .bdark (battle message), .hpbox, the party screen's
# slots, the bag's and the summary's boxes.
UI_STYLES = [
    # name,      fill,      border,    inner line, radius, border width
    ('WINDOW',   '#f8f8f8', '#3c4858', '#98b8d0', 3, 2),
    ('DARK',     '#305870', '#e0b060', '#1c3444', 3, 2),
    ('HUD',      '#f8f8d8', '#404858', None,      3, 2),
    ('SLOT',     '#4c98d4', '#183050', None,      4, 2),
    ('SLOT_ON',  '#f09060', '#f8f0a0', None,      4, 2),
    ('SLOT_FNT', '#b46868', '#183050', None,      4, 2),
    ('BAG',      '#f8f8f8', '#806040', None,      3, 2),
    ('PAGE',     '#f8f8f8', '#a89878', None,      3, 2),
]

def style_box(fill, border, inner, radius, bw):
    img = Image.new('RGBA', (24, 24), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((0, 0, 23, 23), radius=radius, fill=border)
    if inner:
        d.rounded_rectangle((bw, bw, 23 - bw, 23 - bw), radius=max(0, radius - bw), fill=inner)
        d.rectangle((bw + 1, bw + 1, 22 - bw, 22 - bw), fill=fill)
    else:
        d.rounded_rectangle((bw, bw, 23 - bw, 23 - bw), radius=max(0, radius - bw), fill=fill)
    return img

def build_ui_tiles(out_inc):
    colors = []
    for _, fill, border, inner, _, _ in UI_STYLES:
        for h in (fill, border, inner):
            if h and hex555(h) not in colors:
                colors.append(hex555(h))
    if len(colors) > 15:
        raise SystemExit('UI styles need %d colours (max 15)' % len(colors))
    lut = {c: i + 1 for i, c in enumerate(colors)}
    tiles = [(0,) * 64]          # tile 0: empty (transparent)
    for _, fill, border, inner, radius, bw in UI_STYLES:
        img = style_box(fill, border, inner, radius, bw)
        px = img.load()
        for ty in range(3):
            for tx in range(3):
                t = []
                for y in range(8):
                    for x in range(8):
                        p = px[tx * 8 + x, ty * 8 + y]
                        t.append(lut[rgb555(p)] if p[3] >= 128 else 0)
                tiles.append(tuple(t))
    def words(t):
        out = []
        for y in range(8):
            v = 0
            for x in range(8):
                v |= t[y * 8 + x] << (4 * x)
            out.append('0x%08x' % v)
        return '{' + ', '.join(out) + '}'
    pal = [(0, 0, 0)] + colors + [(0, 0, 0)] * (15 - len(colors))
    L = ['// Generated by tools/build_assets.py; do not edit.', '#ifndef PR_UI_DATA_H\n#define PR_UI_DATA_H\n',
         '#include "bn_tile.h"\n#include "bn_color.h"\n', 'namespace pr::ui_data\n{\n',
         '// Window styles: 9 tiles each (top-left, top, top-right, left, fill, right, bottom-left, bottom, bottom-right).',
         'enum class style : uint8_t\n{\n' + ''.join('    %s,\n' % s[0] for s in UI_STYLES) + '};\n',
         'alignas(int) constexpr bn::tile tiles[] = {\n    %s\n};' % ',\n    '.join(words(t) for t in tiles),
         'constexpr bn::color colors[] = {%s};\n' % ', '.join(c_color(c) for c in pal), '}\n\n#endif']
    write_if_changed(os.path.join(out_inc, 'pr_ui_data.h'), '\n'.join(L))

BALL = []

def build_small_sprites(gfx):
    # Menu cursor: the web game's ▶ in ink (#383840) with its light shadow.
    cur = Image.new('RGBA', (8, 8), (0, 0, 0, 0))
    d = ImageDraw.Draw(cur)
    d.polygon([(2, 1), (6, 4), (2, 7)], fill=(0xd0, 0xd0, 0xc8, 255))
    d.polygon([(1, 0), (5, 3), (1, 6)], fill=(0x38, 0x38, 0x40, 255))
    save_sprite(gfx, 'cursor', [cur], 8, 8)
    # "More text" arrow: the web game's red ▼ (#f85838), two bounce frames.
    frames = []
    for dy in (0, 1):
        f = Image.new('RGBA', (8, 8), (0, 0, 0, 0))
        d = ImageDraw.Draw(f)
        d.polygon([(0, 1 + dy), (6, 1 + dy), (3, 5 + dy)], fill=(0x88, 0x28, 0x18, 255))
        d.polygon([(0, 1 + dy), (5, 1 + dy), (3, 4 + dy)], fill=(0xf8, 0x58, 0x38, 255))
        frames.append(f)
    save_sprite(gfx, 'next_arrow', frames, 8, 8)
    # Trainer's "!" (the web game's .ow-bang: white box, dark border, red mark).
    b = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
    d = ImageDraw.Draw(b)
    d.rounded_rectangle((3, 0, 12, 11), radius=2, fill=(0x30, 0x28, 0x30, 255))
    d.rounded_rectangle((4, 1, 11, 10), radius=1, fill=(0xf8, 0xf8, 0xf8, 255))
    d.rectangle((7, 2, 8, 6), fill=(0xe0, 0x30, 0x30, 255))
    d.rectangle((7, 8, 8, 9), fill=(0xe0, 0x30, 0x30, 255))
    d.polygon([(6, 11), (9, 11), (7, 14)], fill=(0x30, 0x28, 0x30, 255))
    save_sprite(gfx, 'bang', [b], 16, 16)
    # The web game's POKé BALL (BALL_SVG, 12x12) in a 16x16 sprite.
    b = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
    px = b.load()
    for y, row in enumerate(BALL):
        for x, c in enumerate(row):
            if c:
                px[x + 2, y + 2] = tuple(c) + (255,)
    save_sprite(gfx, 'ball', [b], 16, 16)
    # HP bar segments: 8x8, fill 0-8 px, in green, yellow and red (27 frames), on the web game's trough.
    frames = []
    for col in ((88, 208, 128), (248, 216, 56), (240, 72, 56)):
        for n in range(9):
            f = Image.new('RGBA', (8, 8), (0, 0, 0, 0))
            d = ImageDraw.Draw(f)
            d.rectangle((0, 0, 7, 3), fill=(0x58, 0x60, 0x68, 255))
            if n:
                d.rectangle((0, 0, n - 1, 3), fill=col + (255,))
                d.rectangle((0, 0, n - 1, 0), fill=tuple(min(255, v + 60) for v in col) + (255,))
            frames.append(f)
    save_sprite(gfx, 'hpbar', frames, 8, 8)

def build_grass_front(data, gfx):
    # The bottom half of a tall-grass tile, drawn over whoever stands in it (the web game's .tg-front).
    img = Image.new('RGBA', (16, 8), (0, 0, 0, 0))
    px = img.load()
    for y in range(8):
        for x in range(16):
            c = data['tall_grass'][y + 8][x]
            if c:
                px[x, y] = tuple(c) + (255,)
    save_sprite(gfx, 'grass_front', [img], 16, 8)

# ---------------------------------------------------------------------------------------------------
# Full-screen backgrounds (256x256 so Butano can use them as regular BGs; the screen shows 240x160).
def build_backgrounds(data, gfx):
    a = data['art']
    # Battle: the sky/hedge backdrop repeated sideways over its fill, and the two platforms.
    img = Image.new('RGBA', (256, 256), (0xd4, 0xf2, 0xc4, 255))
    bg = art_image(a['battle_bg'])
    for x in range(0, 256, bg.size[0]):
        img.alpha_composite(bg, (x, 0))
    def platform(l, m, r, x, y, mids):
        Lp, M, R = art_image(a[l]), art_image(a[m]), art_image(a[r])
        img.alpha_composite(Lp, (x, y))
        cx = x + Lp.size[0]
        for _ in range(mids):
            img.alpha_composite(M, (cx, y))
            cx += M.size[0]
        img.alpha_composite(R, (cx, y))
    platform('plat_enemy_l', 'plat_enemy_m', 'plat_enemy_r', 132, 50, 6)     # 24 + 48 + 24 = 96 wide
    platform('plat_player_l', 'plat_player_m', 'plat_player_r', 0, 84, 5)    # 32 + 40 + 32 = 104 wide
    save_bg(img, gfx, 'battle_bg', 'bpp_8')

    # Intro stage: the web game's dark radial spotlight (.intro-stage).
    img = Image.new('RGBA', (256, 256), (0x10, 0x18, 0x20, 255))
    px = img.load()
    for y in range(160):
        for x in range(240):
            dx, dy = (x - 120) / 150.0, (y - 72) / 100.0
            t = min(1.0, max(0.0, ((dx * dx + dy * dy) ** 0.5 - 0.3) / 0.4))
            c0, c1 = (0x38, 0x48, 0x68), (0x10, 0x18, 0x20)
            px[x, y] = tuple(int(c0[k] + (c1[k] - c0[k]) * t) for k in range(3)) + (255,)
    save_bg(img, gfx, 'intro_bg', 'bpp_8')

    # Title: a slice of the home town.
    t = data['areas'][0]
    town = Image.frombytes('RGB', (t['w'] * 16, t['h'] * 16), open(os.path.join(EXP, 'map_%d.rgb' % t['index']), 'rb').read())
    save_bg(town.crop((96, 120, 96 + 256, 120 + 256)).convert('RGBA'), gfx, 'title_bg', 'bpp_8')

    # Party screen: the web game's diagonal blue stripes (.pty), at GBA scale.
    img = Image.new('RGBA', (256, 256))
    px = img.load()
    for y in range(256):
        for x in range(256):
            px[x, y] = (0x40, 0x80, 0xb0, 255) if ((x + y) // 4) % 2 == 0 else (0x38, 0x78, 0xa8, 255)
    save_bg(img, gfx, 'party_bg')
    # Bag: vertical tan stripes (.bag).
    img = Image.new('RGBA', (256, 256))
    px = img.load()
    for y in range(256):
        for x in range(256):
            px[x, y] = (0xf8, 0xd8, 0x90, 255) if (x // 4) % 2 == 0 else (0xf0, 0xc8, 0x78, 255)
    save_bg(img, gfx, 'bag_bg')
    # Summary: cream page, blue header, gold left panel (.sm).
    img = Image.new('RGBA', (256, 256), (0xf8, 0xf0, 0xe0, 255))
    d = ImageDraw.Draw(img)
    d.rectangle((0, 18, 87, 159), fill=(0xf0, 0xd0, 0x78, 255))
    d.rectangle((0, 0, 239, 17), fill=(0x38, 0x68, 0xb8, 255))
    save_bg(img, gfx, 'summary_bg')

# ---------------------------------------------------------------------------------------------------
def build_game_data(data, out_inc):
    types = data['types']
    L = ['// Generated by tools/build_assets.py from the web game; do not edit.',
         '#ifndef PR_GAME_DATA_H\n#define PR_GAME_DATA_H\n', '#include "pr_game_types.h"']
    for s in data['species']:
        L.append('#include "bn_sprite_items_mon_front_%d.h"\n#include "bn_sprite_items_mon_back_%d.h"' % (s['num'], s['num']))
    L.append('\nnamespace pr::game_data\n{\n')
    L.append('constexpr const char* type_names[] = {%s};' % ', '.join(c_string(t.upper()) for t in types))
    chart = data['chart']
    rows = []
    for at in types:
        rows.append('{' + ', '.join(str(int(chart.get(at, {}).get(dt, 1) * 2)) for dt in types) + '}')
    L.append('// Damage multiplier x2: [attacking type][defending type].')
    L.append('constexpr uint8_t type_chart[%d][%d] = {\n    %s\n};\n' % (len(types), len(types), ',\n    '.join(rows)))
    cat = {'phys': 'move_category::PHYSICAL', 'spec': 'move_category::SPECIAL', 'status': 'move_category::STATUS'}
    mv = []
    for m in data['moves']:
        sec = m.get('sec') or {}
        mv.append('{%s, %d, %d, %s, %d, %s, %s, %d}' % (
            c_text(m['n'].upper()), types.index(m['t']), m['p'], cat[m['c']], m['a'],
            STATUS[m.get('status')], STATUS[sec.get('status')], sec.get('chance', 0)))
    L.append('constexpr move moves[] = {\n    ' + ',\n    '.join(mv) + '\n};\n')
    names = [s['name'] for s in data['species']]
    for s in data['species']:
        L.append('constexpr learn_entry learn_%d[] = {%s};' % (s['num'], ', '.join('{%d, %d}' % (lv, mi) for lv, mi in s['learn']) or '{1, 0}'))
    L.append('\nconstexpr species species_list[] = {')
    for s in data['species']:
        b = s['base']
        evo = s['evo']
        evo_to = names.index(evo['to']) if evo and evo['to'] in names else -1
        t2 = types.index(s['types'][1]) if len(s['types']) > 1 else -1
        L.append('    {%s, %d, %d, %d, {%d, %d, %d, %d, %d, %d}, learn_%d, %d, %d, %d, %d, bn::sprite_items::mon_front_%d, bn::sprite_items::mon_back_%d},' % (
            c_text(s['name'].upper()), s['num'], types.index(s['types'][0]), t2,
            b['hp'], b['atk'], b['def'], b['spa'], b['spd'], b['spe'], s['num'], max(1, len(s['learn'])),
            evo_to, evo['level'] if evo_to >= 0 else 0, s['capture'], s['num'], s['num']))
    L.append('};\n')
    # Items (ITEM_INFO): name, pocket, description, price, and what they do.
    its = []
    for i in ITEM_IDS:
        it = data['items'][i]
        its.append('{%s, %d, %s, %d, %d, %s, %s, %s}' % (
            c_text(it['name']), it['pocket'], c_text(it['desc']), it.get('price', 0), it.get('heal', 0),
            STATUS[it.get('cure')], 'true' if it.get('revive') else 'false', 'true' if it.get('full') else 'false'))
    L.append('constexpr item_info items[] = {\n    %s\n};' % ',\n    '.join(its))
    L.append('constexpr item_id mart_stock[] = {%s};\n' % ', '.join('item_id::%s' % i.upper() for i in MART_STOCK))
    L.append('constexpr species_id starters[] = {%s};' % ', '.join('species_id::%s' % enum_name(n) for n in data['starters']))
    L.append('constexpr const char* prof_name = %s;' % c_text(data['prof']))
    L.append('constexpr const char* intro_lines[] = {%s};' % ', '.join(c_text(l) for l in data['intro']))
    call = data['prof_calls'][0]
    L.append('// The professor\'s call after the first badge (PROF_CALLS).')
    L.append('constexpr const char* first_badge_call[] = {%s};\n' % ', '.join(c_text(data['prof'] + ': ' + l) for l in call['lines']))
    L.append('}\n\n#endif')
    write_if_changed(os.path.join(out_inc, 'pr_game_data.h'), '\n'.join(L))

    # Enums shared by the data headers.
    kinds = list(data['people'])
    E = ['// Generated by tools/build_assets.py; do not edit.', '#ifndef PR_IDS_H\n#define PR_IDS_H\n',
         '#include <cstdint>\n', 'namespace pr\n{\n', 'enum class species_id : uint8_t\n{']
    E += ['    %s,' % enum_name(n) for n in names]
    E += ['};\n', 'constexpr int species_count = %d;' % len(names), 'constexpr int types_count = %d;\n' % len(types)]
    E += ['enum class item_id : uint8_t\n{'] + ['    %s,' % i.upper() for i in ITEM_IDS] + ['};\n',
          'constexpr int items_count = %d;\n' % len(ITEM_IDS)]
    E += ['enum class person_kind : uint8_t\n{'] + ['    %s,' % k for k in kinds] + ['};\n', '}\n', '#endif']
    write_if_changed(os.path.join(out_inc, 'pr_ids.h'), '\n'.join(E))
    P = ['// Generated by tools/build_assets.py; do not edit.', '#ifndef PR_PEOPLE_SPRITES_H\n#define PR_PEOPLE_SPRITES_H\n']
    P += ['#include "bn_sprite_items_person_%s.h"' % k.lower() for k in kinds]
    P += ['\nnamespace pr\n{\n', 'constexpr const bn::sprite_item* person_sprites[] = {%s};\n' %
          ', '.join('&bn::sprite_items::person_%s' % k.lower() for k in kinds), '}\n', '#endif']
    write_if_changed(os.path.join(out_inc, 'pr_people_sprites.h'), '\n'.join(P))

def inputs_hash():
    h = hashlib.sha256()
    files = [os.path.join(ROOT, 'js', f) for f in ('app.js', 'dexdata.js', 'dexinfo.js', 'tileart.js')] + \
            [os.path.join(HERE, f) for f in ('export.js', 'build_assets.py')]
    for f in files:
        h.update(open(f, 'rb').read())
    return h.hexdigest()

def main():
    global EXP
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default='generated')
    args = ap.parse_args()
    out = os.path.join(GBA, args.out)
    inc, gfx = os.path.join(out, 'include'), os.path.join(out, 'graphics')
    EXP = os.path.join(out, 'export')
    for d in (inc, gfx, EXP):
        os.makedirs(d, exist_ok=True)
    stamp = os.path.join(out, 'inputs.sha256')
    digest = inputs_hash()
    if os.path.exists(stamp) and open(stamp).read() == digest:
        return
    # Graphics from older builds would still be picked up by Butano, so start clean.
    for f in os.listdir(gfx):
        os.remove(os.path.join(gfx, f))
    for f in ('pr_species_id.h',):
        if os.path.exists(os.path.join(inc, f)):
            os.remove(os.path.join(inc, f))
    subprocess.run(['node', os.path.join(HERE, 'export.js'), EXP] + [str(a) for a in AREAS], check=True)
    data = json.load(open(os.path.join(EXP, 'data.json'), encoding='utf8'))
    build_world(EXP, data, inc)
    build_game_data(data, inc)
    build_ui_tiles(inc)
    build_people(data, gfx)
    build_mons(data, gfx)
    BALL.extend(data['ball'])
    build_small_sprites(gfx)
    build_grass_front(data, gfx)
    build_backgrounds(data, gfx)
    open(stamp, 'w').write(digest)

if __name__ == '__main__':
    main()
