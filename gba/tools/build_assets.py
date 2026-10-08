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
import numpy as np
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
GBA = os.path.dirname(HERE)
ROOT = os.path.dirname(GBA)
MAP_BANKS = 14          # BG palette banks a map tileset may use; the 15th is the weather's, the 16th the UI's
MAX_TRAINERS = 256      # bits in game_state::beaten
MAX_ITEM_BALLS = 256    # bits in game_state::picked
ITEM_IDS = ['pokeball', 'potion', 'superpotion', 'antidote', 'parlyzheal', 'awakening', 'burnheal', 'hyperpotion',
            'revive', 'fullrestore', 'hm03', 'hm08', 'oldrod',      # ITEM_INFO order
            'greatball', 'ultraball', 'rarecandy', 'masterball',      # GBA only (GBA_ITEMS)
            'maxpotion', 'repel', 'superrepel', 'maxrepel', 'escaperope', 'goodrod', 'superrod',     # GBA 1.8
            'firestone', 'waterstone', 'thunderstone', 'leafstone', 'moonstone', 'sunstone', 'shinystone', 'duskstone',
            'dawnstone', 'icestone', 'ovalstone', 'razorclaw', 'razorfang', 'linkingcord']
            # ... then the TMs (add_extras), then GBA 1.9's held items (after the TMs, so older saves' bags stay a prefix)
HELD_ITEM_IDS = ['leftovers', 'lifeorb', 'choicescarf', 'focussash', 'sitrusberry']    # HELD's order
# GBA only: better POKé BALLS and the RARE CANDY (hidden items, the MART).
GBA_ITEMS = {'greatball': {'name': 'GREAT BALL', 'pocket': 1, 'desc': 'A good ball, with a higher catch rate than a POKé BALL.', 'price': 600},
             'ultraball': {'name': 'ULTRA BALL', 'pocket': 1, 'desc': 'A very good ball, with a higher catch rate than a GREAT BALL.', 'price': 1200},
             'masterball': {'name': 'MASTER BALL', 'pocket': 1, 'desc': 'The best ball there is. It catches any wild POKéMON without fail.', 'price': 0},
             'rarecandy': {'name': 'RARE CANDY', 'pocket': 0, 'desc': 'A candy packed with energy. It raises a POKéMON by one level.', 'price': 4800},
             # GBA 1.8
             'maxpotion': {'name': 'MAX POTION', 'pocket': 0, 'desc': 'A spray-type medicine. It fully restores the HP of one POKéMON.', 'price': 2500, 'heal': 999},
             'repel': {'name': 'REPEL', 'pocket': 0, 'desc': 'Keeps wild POKéMON away for 100 steps.', 'price': 350},
             'superrepel': {'name': 'SUPER REPEL', 'pocket': 0, 'desc': 'Keeps wild POKéMON away for 200 steps.', 'price': 500},
             'maxrepel': {'name': 'MAX REPEL', 'pocket': 0, 'desc': 'Keeps wild POKéMON away for 250 steps.', 'price': 700},
             'escaperope': {'name': 'ESCAPE ROPE', 'pocket': 0, 'desc': 'Use it in a cave or under the sea to return to the last POKéMON CENTER.', 'price': 550},
             'goodrod': {'name': 'GOOD ROD', 'pocket': 4, 'desc': 'A new, good-quality fishing rod. Bigger POKéMON bite.'},
             'superrod': {'name': 'SUPER ROD', 'pocket': 4, 'desc': 'An awesome, high-tech fishing rod. Rare POKéMON bite.'},
             'firestone': {'name': 'FIRE STONE', 'pocket': 0, 'desc': 'A peculiar stone that makes certain species of POKéMON evolve.', 'price': 2100},
             'waterstone': {'name': 'WATER STONE', 'pocket': 0, 'desc': 'A peculiar stone that makes certain species of POKéMON evolve.', 'price': 2100},
             'thunderstone': {'name': 'THUNDER STONE', 'pocket': 0, 'desc': 'A peculiar stone that makes certain species of POKéMON evolve.', 'price': 2100},
             'leafstone': {'name': 'LEAF STONE', 'pocket': 0, 'desc': 'A peculiar stone that makes certain species of POKéMON evolve.', 'price': 2100},
             'moonstone': {'name': 'MOON STONE', 'pocket': 0, 'desc': 'A peculiar stone that makes certain species of POKéMON evolve.', 'price': 2100},
             'sunstone': {'name': 'SUN STONE', 'pocket': 0, 'desc': 'A peculiar stone that makes certain species of POKéMON evolve.', 'price': 2100},
             'shinystone': {'name': 'SHINY STONE', 'pocket': 0, 'desc': 'A peculiar stone that makes certain species of POKéMON evolve.', 'price': 2100},
             'duskstone': {'name': 'DUSK STONE', 'pocket': 0, 'desc': 'A peculiar stone that makes certain species of POKéMON evolve.', 'price': 2100},
             'dawnstone': {'name': 'DAWN STONE', 'pocket': 0, 'desc': 'A peculiar stone that makes certain species of POKéMON evolve.', 'price': 2100},
             'icestone': {'name': 'ICE STONE', 'pocket': 0, 'desc': 'A peculiar stone that makes certain species of POKéMON evolve.', 'price': 2100},
             'ovalstone': {'name': 'OVAL STONE', 'pocket': 0, 'desc': 'A round stone. It makes a certain POKéMON evolve.', 'price': 3000},
             'razorclaw': {'name': 'RAZOR CLAW', 'pocket': 0, 'desc': 'A sharply hooked claw. It makes a certain POKéMON evolve.', 'price': 3000},
             'razorfang': {'name': 'RAZOR FANG', 'pocket': 0, 'desc': 'A sharp fang. It makes a certain POKéMON evolve.', 'price': 3000},
             # GBA 1.9: held items, given from the party screen or the BAG.
             'leftovers': {'name': 'LEFTOVERS', 'pocket': 0, 'desc': 'Held: restores a little HP at the end of every turn.', 'price': 4000},
             'lifeorb': {'name': 'LIFE ORB', 'pocket': 0, 'desc': 'Held: the holder\'s moves do 30% more damage.', 'price': 6000},
             'choicescarf': {'name': 'CHOICE SCARF', 'pocket': 0, 'desc': 'Held: raises the holder\'s SPEED by half.', 'price': 6000},
             'focussash': {'name': 'FOCUS SASH', 'pocket': 0, 'desc': 'Held: at full HP, survives one KO with 1 HP. Used up.', 'price': 4000},
             'sitrusberry': {'name': 'SITRUS BERRY', 'pocket': 3, 'desc': 'Held: restores HP once when it falls to half. Used up.', 'price': 1000},
             'linkingcord': {'name': 'LINKING CORD', 'pocket': 0, 'desc': 'A mysterious cord. POKéMON that evolve by trading evolve when it is used on them.', 'price': 3000}}
EVO_ITEMS = {'fire-stone': 'firestone', 'water-stone': 'waterstone', 'thunder-stone': 'thunderstone', 'leaf-stone': 'leafstone',
             'moon-stone': 'moonstone', 'sun-stone': 'sunstone', 'shiny-stone': 'shinystone', 'dusk-stone': 'duskstone',
             'dawn-stone': 'dawnstone', 'ice-stone': 'icestone', 'oval-stone': 'ovalstone', 'razor-claw': 'razorclaw',
             'razor-fang': 'razorfang', 'linking-cord': 'linkingcord'}
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
# then describe each picture as 16x16 metatiles of four cells. Every area has its own tileset, holding the
# area and the strips of its neighbours that can be seen from it; rooms have one per look. A tile is a pair
# of 8x8 blocks, its two animation frames (water and flowers change every half second); most are the same.

def pack_palettes(color_sets, max_banks, what, trials=120):
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
    for trial in range(trials):
        order = list(maximal)
        if trial:
            rnd.shuffle(order)
        for by_overlap in (False, True):
            banks = pack(order, by_overlap)
            if best is None or len(banks) < len(best):
                best = banks
        if len(best) <= max_banks and trial >= 20:
            break
    if len(best) > max_banks:
        raise SystemExit('%s needs %d palette banks (max %d)' % (what, len(best), max_banks))
    assign = [next(i for i, b in enumerate(best) if cs <= b) for cs in color_sets]
    return best, assign

def to555(img):
    """A picture as an array of rgb555 values (r | g << 5 | b << 10, as the GBA stores them)."""
    a = np.asarray(img.convert('RGB'), dtype=np.uint16)
    return (a[:, :, 0] >> 3) | ((a[:, :, 1] >> 3) << 5) | ((a[:, :, 2] >> 3) << 10)

def blocks(arr):
    """(H, W) -> (H/8, W/8, 64) 8x8 blocks."""
    h, w = arr.shape
    return arr.reshape(h // 8, 8, w // 8, 8).transpose(0, 2, 1, 3).reshape(h // 8, w // 8, 64)

def reduce_colors(b, max_colors=15):
    """A 4bpp tile has 15 colours: merge the closest pair (keeping the more common) until it fits."""
    counts = {}
    for c in b:
        counts[c] = counts.get(c, 0) + 1
    if len(counts) <= max_colors:
        return b
    rgb = lambda c: (c & 31, (c >> 5) & 31, c >> 10)
    remap = {c: c for c in counts}
    while len(counts) > max_colors:
        cols = list(counts)
        best = None
        for i in range(len(cols)):
            for j in range(i + 1, len(cols)):
                d = sum((rgb(cols[i])[k] - rgb(cols[j])[k]) ** 2 for k in range(3))
                if best is None or d < best[0]:
                    best = (d, cols[i], cols[j])
        _, a, c2 = best
        keep, drop = (a, c2) if counts[a] >= counts[c2] else (c2, a)
        counts[keep] += counts.pop(drop)
        for k, v in remap.items():
            if v == drop:
                remap[k] = keep
    return tuple(remap[c] for c in b)

FLIPS = [np.arange(64).reshape(8, 8)[::1, ::1].ravel(), np.arange(64).reshape(8, 8)[:, ::-1].ravel(),
         np.arange(64).reshape(8, 8)[::-1, :].ravel(), np.arange(64).reshape(8, 8)[::-1, ::-1].ravel()]

class Tileset:
    def __init__(self, name, banks):
        self.name = name
        self.max_banks = banks
        self.uniq, self.index = [], {}     # tiles: 128-tuples (frame A then frame B)
        self.metas, self.meta_index = [], {}
        self.pending = []                  # pictures cut into tile refs, waiting for the palettes

    def ref(self, pair):
        a, b = pair[:64], pair[64:]
        for f, perm in enumerate(FLIPS):
            key = tuple(a[i] for i in perm) + tuple(b[i] for i in perm)
            if key in self.index:
                return self.index[key], f
        self.index[pair] = len(self.uniq)
        self.uniq.append(pair)
        return len(self.uniq) - 1, 0

    def add(self, img_a, img_b=None):
        """Adds a picture (w and h multiples of 16) in its two frames; returns a handle for metatiles_of."""
        A = blocks(to555(img_a))
        B = blocks(to555(img_b)) if img_b is not None else A
        h, w = A.shape[:2]
        refs = []
        cache = {}
        for y in range(h):
            row = []
            for x in range(w):
                pair = tuple(A[y, x].tolist()) + tuple(B[y, x].tolist())
                r = cache.get(pair)
                if r is None:
                    if len(set(pair)) > 15:
                        pair = self.reduce_pair(pair)
                    r = cache[pair] = self.ref(pair)
                row.append(r)
            refs.append(row)
        self.pending.append(refs)
        return len(self.pending) - 1

    @staticmethod
    def reduce_pair(pair):
        merged = reduce_colors(pair[:64] + pair[64:])
        return tuple(merged)

    def finish(self):
        if len(self.uniq) > 1020:
            raise SystemExit('tileset %s needs %d tiles (max 1020)' % (self.name, len(self.uniq)))
        # Too many colours for the banks: merge the closest pair of colours (the rarer one into the more
        # common one), a few at a time, until they pack.
        merged = 0
        while True:
            sets = [frozenset(t) for t in self.uniq]
            try:
                banks, self.assign = pack_palettes(sets, self.max_banks, 'tileset ' + self.name, 120 if not merged else 40)
                break
            except SystemExit:
                if merged > 400:
                    raise
            counts = {}
            for t in self.uniq:
                for c in set(t):
                    counts[c] = counts.get(c, 0) + 1
            cols = np.array(list(counts), dtype=np.int32)
            rgb = np.stack([cols & 31, (cols >> 5) & 31, cols >> 10], 1).astype(np.float64)
            weight = np.array([counts[c] for c in counts], dtype=np.float64)
            d = ((rgb[:, None, :] - rgb[None, :, :]) ** 2 * np.array([3, 4, 2])).sum(2)
            np.fill_diagonal(d, 1e9)
            remap = {}
            step = max(1, len(cols) // 25)
            for _ in range(step):
                i, j = np.unravel_index(np.argmin(d), d.shape)
                keep, drop = (i, j) if weight[i] >= weight[j] else (j, i)
                remap[int(cols[drop])] = int(cols[keep])
                d[drop, :] = 1e9
                d[:, drop] = 1e9
                weight[keep] += weight[drop]
            def follow(c):
                while c in remap:
                    c = remap[c]
                return c
            self.uniq = [tuple(follow(c) for c in t) for t in self.uniq]
            merged += step
        if merged:
            print('build_assets: tileset %s: merged %d colours to fit its palette banks' % (self.name, merged))
        self.bank_lists = [sorted(b) for b in banks]

    def cell(self, r):
        t, f = r
        return t | (f << 10) | (self.assign[t] << 12)

    def metatiles_of(self, pic):
        refs = self.pending[pic]
        out = []
        for y in range(len(refs) // 2):
            for x in range(len(refs[0]) // 2):
                key = tuple(self.cell(r) for r in (refs[2 * y][2 * x], refs[2 * y][2 * x + 1], refs[2 * y + 1][2 * x], refs[2 * y + 1][2 * x + 1]))
                if key not in self.meta_index:
                    self.meta_index[key] = len(self.metas)
                    self.metas.append(key)
                out.append(self.meta_index[key])
        return out

    def emit(self, L, fill_meta):
        p = 'ts_%s_' % self.name
        def words(t, lut):
            return tile_words(t, lut)
        luts = [{c: i + 1 for i, c in enumerate(self.bank_lists[self.assign[i]])} for i in range(len(self.uniq))]
        L.append('alignas(int) constexpr bn::tile %stiles[] = {\n    %s\n};' % (p, ',\n    '.join(
            words(t[:64], luts[i]) for i, t in enumerate(self.uniq))))
        anim = [i for i, t in enumerate(self.uniq) if t[:64] != t[64:]]
        L.append('constexpr uint16_t %sanim_index[] = {%s};' % (p, ', '.join(map(str, anim)) or '0'))
        L.append('alignas(int) constexpr bn::tile %sanim_tiles[] = {%s};' % (p, ',\n    '.join(
            words(self.uniq[i][64:], luts[i]) for i in anim) or '{}'))
        colors = []
        for bl in self.bank_lists:
            bank = [0] + bl
            colors += bank + [0] * (16 - len(bank))
        L.append('constexpr bn::color %scolors[] = {\n    %s\n};' % (p, ',\n    '.join(
            ', '.join('bn::color(%d, %d, %d)' % (c & 31, (c >> 5) & 31, c >> 10) for c in colors[i:i + 8]) for i in range(0, len(colors), 8))))
        L.append('constexpr uint16_t %smetatiles[][4] = {\n    %s\n};\n' % (p, ',\n    '.join('{%d, %d, %d, %d}' % m for m in self.metas)))
        return '{%stiles, %d, %scolors, %d, %smetatiles, %d, %sanim_index, %sanim_tiles, %d}' % (
            p, len(self.uniq), p, len(colors), p, fill_meta, p, p, len(anim))

def tile_words(b, lut):
    words = []
    for y in range(8):
        v = 0
        for x in range(8):
            v |= lut[b[y * 8 + x]] << (4 * x)
        words.append('0x%08x' % v)
    return '{' + ', '.join(words) + '}'

def png(exp, name):
    return Image.open(os.path.join(exp, name)).convert('RGB')

def solid(color):
    return Image.new('RGB', (16, 16), color)

def build_world(exp, data, out_inc):
    areas, rooms = data['areas'], data['rooms']
    by_index = {a['index']: ai for ai, a in enumerate(areas)}
    tilesets = []
    forest = art_image(data['art']['forest_fill'], (0x1e, 0x4a, 0x1c))

    # Every area: its picture, its neighbours' strips, the ground under its item balls (and theirs), and
    # in the ash, the ground swept clean.
    area_info = []
    for a in areas:
        i = a['index']
        ts = Tileset('area%d' % i, MAP_BANKS)
        info = {'ts': len(tilesets)}
        info['map'] = ts.add(png(exp, 'map_%d.png' % i), png(exp, 'map_%d_b.png' % i))
        if a['items']:
            info['items'] = ts.add(png(exp, 'map_%d_items.png' % i))
        if a['weather'] == 'ash':
            info['clean'] = ts.add(png(exp, 'map_%d_clean.png' % i), png(exp, 'map_%d_b.png' % i))
        info['strips'] = []
        for k, ln in enumerate(a['links']):
            st = {'pic': ts.add(png(exp, 'strip_%d_%d.png' % (i, k)), png(exp, 'strip_%d_%d_b.png' % (i, k)))}
            if ln['strip']['items']:
                st['items'] = ts.add(png(exp, 'strip_%d_%d_items.png' % (i, k)))
            info['strips'].append(st)
        info['fill'] = ts.add(forest)
        tilesets.append(ts)
        area_info.append(info)
    # Rooms: one tileset per look; past the walls is black.
    room_ts = {}
    for r in rooms:
        art = r['art']
        if art in room_ts:
            continue
        ts = Tileset('room_' + art, MAP_BANKS)
        info = {'ts': len(tilesets), 'map': ts.add(png(exp, 'room_%s.png' % art), png(exp, 'room_%s_b.png' % art)),
                'fill': ts.add(solid((0, 0, 0)))}
        if r['kind'] == 'league':
            info['gate'] = ts.add(png(exp, 'league_floor.png'))
        tilesets.append(ts)
        room_ts[art] = info
    for ts in tilesets:
        ts.finish()

    L = ['// Generated by tools/build_assets.py from the web game; do not edit.',
         '#ifndef PR_WORLD_DATA_H\n#define PR_WORLD_DATA_H\n',
         '#include "bn_tile.h"\n#include "bn_color.h"\n#include "pr_world_types.h"\n',
         'namespace pr::world_data\n{\n']
    # Metatiles first (they're numbered as pictures are described), then the tileset table.
    for a, info in zip(areas, area_info):
        ts = tilesets[info['ts']]
        info['meta'] = ts.metatiles_of(info['map'])
        info['fill_meta'] = ts.metatiles_of(info['fill'])[0]
        info['item_meta'] = ts.metatiles_of(info['items']) if 'items' in info else []
        info['clean_meta'] = ts.metatiles_of(info['clean']) if 'clean' in info else None
        for st in info['strips']:
            st['meta'] = ts.metatiles_of(st['pic'])
            st['item_meta'] = ts.metatiles_of(st['items']) if 'items' in st else []
    for art, info in room_ts.items():
        ts = tilesets[info['ts']]
        info['meta'] = ts.metatiles_of(info['map'])
        info['fill_meta'] = ts.metatiles_of(info['fill'])[0]
        info['gate_meta'] = ts.metatiles_of(info['gate'])[0] if 'gate' in info else -1
    fills = {}
    for info in area_info:
        fills[info['ts']] = info['fill_meta']
    for info in room_ts.values():
        fills[info['ts']] = info['fill_meta']
    ts_rows = [ts.emit(L, fills[i]) for i, ts in enumerate(tilesets)]
    L.append('constexpr tileset tilesets[] = {\n    %s\n};\n' % ',\n    '.join(ts_rows))

    def lines_array(name, lines):
        lines = [clean_text(l) for l in lines] or ['...']
        L.append('constexpr const char* %s[] = {%s};' % (name, ', '.join(c_string(l) for l in lines)))
        return len(lines)

    def people_rows(p, people):
        rows = []
        for i, n in enumerate(people):
            count = lines_array('%sperson%d_lines' % (p, i), n['lines'])
            role = {'nurse': 'NURSE', 'clerk': 'CLERK', 'mom': 'MOM', 'tower': 'TOWER', 'trader': 'TRADER', 'daycare': 'DAYCARE',
                    'tutor': 'TUTOR'}.get(n.get('role', ''), 'NONE')
            rows.append('{%d, %d, person_kind::%s, direction::%s, person_role::%s, %s, %sperson%d_lines, %d}' % (
                n['x'], n['y'], n['kind'], n['facing'].upper(), role, 'true' if n.get('wander') else 'false', p, i, count))
        L.append('constexpr person %speople[] = {%s};' % (p, nonempty(', '.join(rows),
            '{0, 0, person_kind::player, direction::DOWN, person_role::NONE, false, nullptr, 0}')))

    map_rows = []
    room_index = {}       # (area, building) -> map index
    for i, r in enumerate(rooms):
        room_index[(r['area'], r['building'])] = len(areas) + i

    # Trainer ids (bits in game_state::beaten), areas first then rooms; each area's gate keeper is its rival,
    # its gym's leader or the Champion.
    ids = {}
    for ai, a in enumerate(areas):
        for ti in range(len(a['trainers'])):
            ids[('area', ai, ti)] = len(ids)
    for ri, r in enumerate(rooms):
        for ti in range(len(r['trainers'])):
            ids[('room', ri, ti)] = len(ids)
    if len(ids) > MAX_TRAINERS:
        raise SystemExit('%d trainers (max %d)' % (len(ids), MAX_TRAINERS))
    def leader_of(ai):
        a = areas[ai]
        for ti, t in enumerate(a['trainers']):
            if t['role'] in ('rival', 'leader', 'champion'):
                return ids[('area', ai, ti)]
        for ri, r in enumerate(rooms):
            if r['area'] == a['index']:
                for ti, t in enumerate(r['trainers']):
                    if t['role'] in ('leader', 'champion'):
                        return ids[('room', ri, ti)]
        return -1
    leaders = [leader_of(ai) for ai in range(len(areas))]

    def trainer_rows(p, trainers, key, area):
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
                        '%safter, %d, %s, %d, %d, %d, %s}' % (
                t['x'], t['y'], t['kind'], t['facing'].upper(), t['role'].upper(), c_text(t['title']), q, len(t['team']),
                q, len(fill), q, intro, q, after, 'true' if t['vanish'] else 'false', ids[key + (ti,)], t.get('elite', -1), area,
                'true' if t.get('scene') else 'false'))
        L.append('constexpr trainer %strainers[] = {%s};' % (p, nonempty(', '.join(rows),
            '{0, 0, person_kind::player, direction::DOWN, trainer_role::ROUTE, "", nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, false, 0, -1, 0, false}')))

    def species_array(name, names):
        L.append('constexpr species_id %s[] = {%s};' % (name, ', '.join('species_id::%s' % enum_name(n) for n in names) or 'species_id::PIDGEY'))
        return len(names)

    def points(name, pts):
        L.append('constexpr uint8_t %s[] = {%s};' % (name, ', '.join('%d, %d' % (x, y) for x, y in pts) or '0'))
        return len(pts)

    item_count = 0
    item_ids = {}
    for a in areas:
        for it in a['items']:
            item_ids[(a['index'], it['x'], it['y'])] = item_count
            item_count += 1
    if item_count > MAX_ITEM_BALLS:
        raise SystemExit('%d item balls (max %d)' % (item_count, MAX_ITEM_BALLS))

    for ai, (a, info) in enumerate(zip(areas, area_info)):
        p = 'map%d_' % ai
        L.append('constexpr uint16_t %smap[] = {%s};' % (p, ', '.join(map(str, info['meta']))))
        L.append('constexpr uint8_t %sbehaviour[] = {%s};' % (p, ', '.join(str(v) for row in a['behaviour'] for v in row)))
        if info['clean_meta'] is not None:
            L.append('constexpr uint16_t %sclean[] = {%s};' % (p, ', '.join(map(str, info['clean_meta']))))
        # Town signs read their name and then the place's description (owInteract); route signs just point;
        # tablets have their own lines.
        desc = re.sub(r'\s*"[^"]*"\s*', ' ', a['desc']).strip()
        signs = []
        for si, sg in enumerate(a['signs']):
            text = sg['lines'] if sg['lines'] else [sg['text']] if sg['route'] else [sg['text'], desc]
            count = lines_array('%ssign%d_lines' % (p, si), text)
            signs.append('{%d, %d, %ssign%d_lines, %d}' % (sg['x'], sg['y'], p, si, count))
        L.append('constexpr sign %ssigns[] = {%s};' % (p, nonempty(', '.join(signs), '{0, 0, nullptr, 0}')))
        L.append('constexpr door %sdoors[] = {%s};' % (p, nonempty(', '.join(
            '{%d, %d, door_kind::%s, %d}' % (d['x'], d['y'], d['kind'].upper(), room_index[(a['index'], bi)])
            for bi, d in enumerate(a['doors'])), '{0, 0, door_kind::HOUSE, -1}')))
        people = a['people']
        tower_door = [d for d in a['doors'] if d['kind'] == 'tower']
        if tower_door and not any(n.get('role') == 'tower' for n in people):
            # GBA only (Phase 7): the CHALLENGE TOWER's guide beside its door in SPIRECREST TOWN.
            d = tower_door[0]
            people.append({'kind': 'gentleman', 'x': d['x'] + 2, 'y': d['y'] + 1, 'facing': 'left', 'role': 'tower', 'wander': False,
                           'lines': ['The CHALLENGE TOWER opens to CHAMPIONS.']})
        center_door = [d for d in a['doors'] if d['kind'] == 'center']
        if a.get('trade_town') and center_door and not any(n.get('role') == 'trader' for n in people):
            # GBA only: TRADEWIND VILLAGE's TRADER beside the POKéMON CENTER.
            d = center_door[0]
            people.append({'kind': 'gentleman', 'x': d['x'] + 2, 'y': d['y'] + 1, 'facing': 'down', 'role': 'trader', 'wander': False,
                           'lines': ['TRADER: "Any POKéMON for any POKéMON!"']})
        if a['name'] == DAYCARE_TOWN and center_door and not any(n.get('role') == 'daycare' for n in people):
            # GBA 1.8: the DAY CARE lady, on open ground near the POKéMON CENTER.
            d = center_door[0]
            x, y = free_spot(a, d['x'] + 3, d['y'] + 1)
            people.append({'kind': 'oldwoman', 'x': x, 'y': y, 'facing': 'down', 'role': 'daycare', 'wander': False,
                           'lines': ['DAY CARE: "I raise POKéMON for TRAINERS."']})
        people_rows(p, people)
        trainer_rows(p, a['trainers'], ('area', ai), ai)
        its = []
        for k, it in enumerate(a['items']):
            its.append('{%d, %d, item_id::%s, %d, %d}' % (it['x'], it['y'], it['id'].upper(), item_ids[(a['index'], it['x'], it['y'])],
                                                        info['item_meta'][k]))
        L.append('constexpr item_ball %sitems[] = {%s};' % (p, nonempty(', '.join(its), '{0, 0, item_id::POKEBALL, 0, 0}')))
        links = []
        for k, ln in enumerate(a['links']):
            st, sm = ln['strip'], info['strips'][k]
            q = '%slink%d_' % (p, k)
            L.append('constexpr uint16_t %sstrip[] = {%s};' % (q, ', '.join(map(str, sm['meta']))))
            target = by_index.get(ln['to'], -1)
            sits = []
            for j, (x, y) in enumerate(st['items']):
                sits.append('{%d, %d, %d}' % (x, y, sm['item_meta'][j]))
            L.append('constexpr strip_item %sitems[] = {%s};' % (q, nonempty(', '.join(sits), '{0, 0, 0}')))
            links.append('{%d, %d, %d, %d, %d, %s, %s, %d, %d, %d, %d, %d, %sstrip, %sitems, %d}' % (
                target, ln['ox'], ln['oy'], ln['w'], ln['h'], c_text(ln['name'].upper()), 'true' if ln['gate'] else 'false',
                ln['badges'], st['x'], st['y'], st['w'], st['h'], q, q, len(st['items'])))
        L.append('constexpr link %slinks[] = {%s};' % (p, nonempty(', '.join(links), '{-1, 0, 0, 0, 0, "", false, 0, 0, 0, 0, 0, nullptr, nullptr, 0}')))
        np_ = species_array(p + 'pool', a['area_pool'])
        nw = species_array(p + 'water', a['water'])
        nf = species_array(p + 'fish', a['fish'])
        nd = points(p + 'dive', a['dive_spots'])
        ns = points(p + 'shafts', a['shafts'])
        gate = {'gym': 'GYM', 'rival': 'RIVAL'}.get(a['gate_kind'], 'NONE')
        if a['champion']:
            gate = 'NONE'
        theme = {'plain': 'PLAIN', 'forest': 'FOREST', 'lake': 'LAKE', 'rocky': 'ROCKY', 'sea': 'SEA', 'deep': 'DEEP', 'cave': 'CAVE'}[a['theme']]
        weather = {'': 'NONE', 'rain': 'RAIN', 'snow': 'SNOW', 'ash': 'ASH', 'fog': 'FOG', 'deep': 'DEEP', 'cave': 'CAVE'}[a['weather']]
        kind = {'town': 'TOWN', 'route': 'ROUTE', 'gym': 'GYM', 'trainer': 'TRAINER'}[a['type']]
        flags = []
        if a['center']: flags.append('area_flag::CENTER')
        if a['league']: flags.append('area_flag::LEAGUE')
        if a['champion']: flags.append('area_flag::CHAMPION')
        if a['kind'] == 'boss': flags.append('area_flag::BOSS')
        if a.get('tower_town'): flags.append('area_flag::TOWER_TOWN')
        if a.get('safari'): flags.append('area_flag::SAFARI')
        if a.get('trade_town'): flags.append('area_flag::TRADE_TOWN')
        legend = a['legend']
        if a['own_pool']:
            flags.append('area_flag::OWN_POOL')
        L.append('constexpr area_info %sarea = {area_kind::%s, area_theme::%s, area_weather::%s, %s, %d, %d, %d, %d, %d, %s, '
                 '%sdive, %d, %sshafts, %d, %s, %d, %d, %d, %d, %s};' % (
            p, kind, theme, weather, ' | '.join(flags) or '0', a['tier'], a['at'][0], a['at'][1],
            by_index.get(a['dive'], -1) if a['dive'] >= 0 else -1, by_index.get(a['surface'], -1) if a['surface'] >= 0 else -1,
            c_text(a['scene']), p, nd, p, ns,
            'species_id::%s' % enum_name(legend['name']) if legend else 'species_id::PIDGEY', legend['x'] if legend else -1,
            legend['y'] if legend else -1, nw, nf, '%sclean' % p if info['clean_meta'] is not None else 'nullptr'))
        map_rows.append('{%s, %d, %d, %d, %smap, %sbehaviour, %ssigns, %d, %sdoors, %d, %speople, %d, %strainers, %d, %sitems, %d, '
                        'nullptr, 0, %slinks, %d, %spool, %d, %d, %d, %d, -1, 0, 0, %d, gate_kind::%s, %s, %s, &%sarea, %swater, %sfish, %s, nullptr}' % (
            c_text(a['name'].upper()), info['ts'], a['w'], a['h'], p, p, p, len(a['signs']), p, len(a['doors']), p, len(a['people']),
            p, len(a['trainers']), p, len(a['items']), p, len(a['links']), p, np_,
            a['spawn']['x'], a['spawn']['y'], min(50, 8 + a['tier'] * 4), leaders[ai], gate,
            c_text(a['name'].upper()), c_text(a['leader_name']), p, p, p, c_text(clean_text(a['desc']))))

    for ri, r in enumerate(rooms):
        p = 'room%d_' % ri
        info = room_ts[r['art']]
        L.append('constexpr uint8_t %sbehaviour[] = {%s};' % (p, ', '.join(str(v) for row in r['behaviour'] for v in row)))
        people = r['people']
        if r['kind'] == 'center' and not any(n.get('role') == 'tutor' for n in people):
            # GBA 1.8: the MOVE TUTOR in every POKéMON CENTER, by the right-hand wall.
            people.append({'kind': 'gentleman', 'x': 11, 'y': 6, 'facing': 'left', 'role': 'tutor', 'wander': False,
                           'lines': ['MOVE TUTOR: "I can help POKéMON remember moves."']})
        people_rows(p, people)
        area = by_index[r['area']]
        trainer_rows(p, r['trainers'], ('room', ri), area)
        things = []
        for ti, t in enumerate(r['things']):
            count = lines_array('%sthing%d_lines' % (p, ti), t['text'])
            things.append('{%d, %d, %sthing%d_lines, %d}' % (t['x'], t['y'], p, ti, count))
        L.append('constexpr thing %sthings[] = {%s};' % (p, nonempty(', '.join(things), '{0, 0, nullptr, 0}')))
        gates = ['{%d, %d, %d, %d}' % (g['y'], g['x0'], g['x1'], g['elite']) for g in r['gates']]
        L.append('constexpr league_gate %sgates[] = {%s};\n' % (p, nonempty(', '.join(gates), '{0, 0, 0, 0}')))
        name = {'center': 'POKéMON CENTER', 'mart': 'POKé MART', 'gym': 'POKéMON GYM', 'league': 'POKéMON LEAGUE',
                'tower': 'CHALLENGE TOWER', 'summit': 'TOWER SUMMIT', 'chamber': 'SUMMONING CHAMBER'}.get(r['kind'], 'HOUSE')
        gym = r.get('gym') or {}
        kind = {'center': 'CENTER', 'mart': 'MART', 'gym': 'GYM', 'league': 'LEAGUE', 'house': 'HOUSE', 'tower': 'TOWER',
                'summit': 'SUMMIT', 'chamber': 'CHAMBER'}[r['kind']]
        theme = (r.get('theme') or '').upper() or ('LEAGUE' if r['kind'] in ('summit', 'chamber') else 'NONE')
        L.append('constexpr room_info %sroom = {room_kind::%s, gym_theme::%s, %sgates, %d, %d, %s, %d};' % (
            p, kind, theme, p, len(r['gates']), info['gate_meta'], 'true' if r['home'] else 'false', r.get('floor', -1)))
        map_rows.append('{%s, %d, %d, %d, %s_map, %sbehaviour, nullptr, 0, nullptr, 0, %speople, %d, %strainers, %d, nullptr, 0, '
                        '%sthings, %d, nullptr, 0, nullptr, 0, %d, %d, %d, %d, %d, %d, %d, gate_kind::NONE, %s, %s, nullptr, nullptr, nullptr, "", &%sroom}' % (
            c_text(name), info['ts'], r['w'], r['h'], 'room_' + r['art'], p, p, len(r['people']), p, len(r['trainers']),
            p, len(r['things']), r['spawn']['x'], r['spawn']['y'], min(50, 8 + areas[area]['tier'] * 4), area,
            r['door']['x'], r['door']['y'], leaders[area] if r['kind'] in ('gym', 'league') else -1,
            c_text(gym.get('name', '')), c_text(gym.get('leader', '')), p))
    # Room metatile maps go before the map table (shared by every room of a look).
    room_maps = ['constexpr uint16_t room_%s_map[] = {%s};' % (k, ', '.join(map(str, v['meta']))) for k, v in sorted(room_ts.items())]
    L += room_maps
    L.append('\nconstexpr map_def maps[] = {\n    %s\n};\n' % ',\n    '.join(map_rows))
    L.append('constexpr int areas_count = %d;' % len(areas))
    hidden = hidden_items(areas)
    L.append('// GBA only: hidden items (press A facing the spot), by area.')
    L.append('constexpr hidden_item hidden_items[] = {%s};' % ', '.join(
        '{%d, %d, %d, item_id::%s, %d, %d}' % (h['area'], h['x'], h['y'], h['item'].upper(), h['count'], h['id']) for h in hidden))
    L.append('constexpr int hidden_items_count = %d;' % len(hidden))
    L.append('constexpr int maps_count = %d;' % (len(areas) + len(rooms)))
    L.append('constexpr int trainers_count = %d;' % len(ids))
    L.append('constexpr int items_count = %d;\n' % item_count)
    L.append('}\n\n#endif')
    write_if_changed(os.path.join(out_inc, 'pr_world_data.h'), '\n'.join(L))
    print('build_assets: %d tilesets, up to %d tiles and %d banks; %d trainers, %d item balls' % (
        len(tilesets), max(len(t.uniq) for t in tilesets), max(len(t.bank_lists) for t in tilesets), len(ids), item_count))

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
    import io
    buf = io.BytesIO()
    img.save(buf, format='BMP')
    data = buf.getvalue()
    WRITTEN.add(os.path.basename(path))
    WRITTEN.add(os.path.basename(path)[:-4] + '.json')
    # Unchanged pictures keep their timestamps, so Butano doesn't convert them again.
    if not (os.path.exists(path) and open(path, 'rb').read() == data):
        with open(path, 'wb') as f:
            f.write(data)
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
    # Surfing (and diving, the ride a little darker): the top 12 rows of the player sitting on the
    # Pokémon (surfSvg), bobbing a pixel every half second. 32x32 with the tile's top left at (8, 15).
    for name, dark in (('surf', False), ('dive', True)):
        frames = []
        for d in ('down', 'up', 'left'):
            ride = data['surf'][d]
            for bob in (0, 1):
                img = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
                px = img.load()
                for y, row in enumerate(ride):
                    for x, c in enumerate(row):
                        if c:
                            c = tuple(c)
                            if dark:
                                c = tuple(int(v * 0.8) for v in c)
                                g = sum(c) / 3
                                c = tuple(max(0, min(255, int(g + (v - g) * 1.2))) for v in c)
                            px[5 + x, 18 + y + bob] = c + (255,)
                body = person_frame(data['people']['player'][d][0]).crop((0, 0, 16, 12))
                img.alpha_composite(body, (8, 4 + bob))
                frames.append(img)
        save_sprite(gfx, name, frames, 32, 32)

def build_music(data, out_inc):
    """Music (js/music.js): 8th-note steps on four channels. Notes are MIDI numbers + 2 (0 rest, 1 hold);
    drums 0 none, 1 kick, 2 snare, 3 hi-hat."""
    NOTE = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
    def code(tok):
        if tok == '.':
            return 0
        if tok == '~':
            return 1
        m = re.match(r'^([A-G])([#b]?)(\d)$', tok)
        if not m:
            return 0
        semi = NOTE[m.group(1)] + (1 if m.group(2) == '#' else -1 if m.group(2) == 'b' else 0) + (int(m.group(3)) + 1) * 12
        return semi + 2
    L = ['// Generated by tools/build_assets.py from js/music.js; do not edit.', '#ifndef PR_MUSIC_DATA_H\n#define PR_MUSIC_DATA_H\n',
         '#include <cstdint>\n', 'namespace pr::music_data\n{\n',
         'struct track\n{\n    const char* name;\n    int bpm;\n    int length;\n    const uint8_t* lead;\n    const uint8_t* harm;\n'
         '    const uint8_t* bass;\n    const uint8_t* drum;\n    int lead_n, harm_n, bass_n, drum_n;\n};\n']
    rows = []
    for name, t in data['music'].items():
        for ch in ('lead', 'harm', 'bass'):
            L.append('constexpr uint8_t %s_%s[] = {%s};' % (name, ch, ', '.join(str(code(x)) for x in t[ch])))
        L.append('constexpr uint8_t %s_drum[] = {%s};' % (name, ', '.join(str({'k': 1, 's': 2, 'h': 3}.get(x, 0)) for x in t['drum'])))
        rows.append('{"%s", %d, %d, %s_lead, %s_harm, %s_bass, %s_drum, %d, %d, %d, %d}' % (
            name, t['bpm'], len(t['lead']), name, name, name, name, len(t['lead']), len(t['harm']), len(t['bass']), len(t['drum'])))
    L.append('\nconstexpr track tracks[] = {\n    %s\n};' % ',\n    '.join(rows))
    L.append('constexpr int tracks_count = %d;\n' % len(rows))
    L.append('}\n\n#endif')
    write_if_changed(os.path.join(out_inc, 'pr_music_data.h'), '\n'.join(L))

def mon_sprite(path, box_of=None):
    """A 96x96 sprite cropped to the 64x64 GBA frame: centred on its content, feet at the bottom. box_of: crop
    by another picture's content instead (a shiny sprite lines up with its normal one)."""
    img = Image.open(path).convert('RGBA')
    box = (Image.open(box_of).convert('RGBA') if box_of else img).getbbox() or (0, 0, 96, 96)
    w, h = box[2] - box[0], box[3] - box[1]
    if w > 64 or h > 64:
        s = 64 / max(w, h)
        img = img.crop(box).resize((max(1, int(w * s)), max(1, int(h * s))), Image.NEAREST)
        box = (0, 0) + img.size
        w, h = img.size
    out = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    out.alpha_composite(img.crop(box), ((64 - w) // 2, 64 - h - 2 if h < 62 else 0))
    return out

def shiny_palette(pal, idx, shiny, alpha_from=None):
    """The shiny colours for a sprite's palette: for each colour slot, the commonest colour the shiny sprite
    has where the normal one uses that slot (slots with nothing under them keep their colour)."""
    px = shiny.load()
    counts = [dict() for _ in range(len(pal) + 1)]
    for y, row in enumerate(idx):
        for x, k in enumerate(row):
            if not k:
                continue
            p = px[x, y]
            if len(p) == 4 and p[3] < 128:
                continue
            c = rgb555(p[:3])
            counts[k][c] = counts[k].get(c, 0) + 1
    out = []
    for k in range(1, len(pal) + 1):
        out.append(max(counts[k].items(), key=lambda kv: kv[1])[0] if counts[k] else pal[k - 1])
    return out

SHINY = {'front': [], 'back': []}

def build_mons(data, gfx):
    icons = []
    for s in data['species']:
        for side, sub in (('front', ''), ('back', 'back/')):
            path = os.path.join(ROOT, 'sprites', 'pokemon', sub + '%d.png' % s['num'])
            if not os.path.exists(path):
                path = os.path.join(ROOT, 'sprites', 'pokemon', '%d.png' % s['num'])
            img = mon_sprite(path)
            pal, idx = quantize_rgba(img, 15)
            save_indexed_bmp(os.path.join(gfx, 'mon_%s_%d.bmp' % (side, s['num'])), pal, idx,
                             {'type': 'sprite', 'bpp_mode': 'bpp_4'})
            # Its shiny colours (sprites/pokemon/[back/]shiny/, from PokeAPI), slot for slot.
            spath = os.path.join(ROOT, 'sprites', 'pokemon', sub + 'shiny', '%d.png' % s['num'])
            if not os.path.exists(spath):
                spath = os.path.join(ROOT, 'sprites', 'pokemon', 'shiny', '%d.png' % s['num'])
            shiny = mon_sprite(spath, box_of=path) if os.path.exists(spath) else img
            SHINY[side].append(shiny_palette(pal, idx, shiny))
            if side == 'front':
                # A 32x32 icon (the party screen, the PC's boxes): the front sprite at half size.
                small = img.resize((32, 32), Image.LANCZOS)
                a = small.split()[3].point(lambda v: 255 if v >= 110 else 0)
                small = small.convert('RGB').convert('RGBA')
                small.putalpha(a)
                icons.append((s['num'], small))
    icons.append(('egg', egg_icon()))      # GBA 1.9: the party screen's EGGS
    # The icons share ICON_PALETTES palettes (Emerald uses 3): a box of 30 different species must fit the
    # 16 sprite palettes with the text's. Shiny icons keep the normal colours, as in Emerald.
    for num, pal, idx in icon_palettes(icons):
        save_indexed_bmp(os.path.join(gfx, 'mon_icon_%s.bmp' % num), pal, idx, {'type': 'sprite', 'bpp_mode': 'bpp_4'})

ICON_PALETTES = 6

def egg_icon():
    """A 32x32 EGG icon, drawn (no sprite for it in sprites/): a cream egg with green spots, outlined."""
    big = Image.new('RGBA', (128, 128), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.ellipse((30, 22, 98, 118), fill=(40, 40, 48, 255))                       # outline
    d.ellipse((36, 28, 92, 112), fill=(248, 240, 208, 255))                   # shell
    for box in ((44, 48, 60, 64), (70, 40, 84, 54), (66, 74, 84, 92), (42, 86, 56, 100)):
        d.ellipse(box, fill=(96, 176, 80, 255))                               # spots
    d.ellipse((46, 34, 56, 46), fill=(255, 255, 255, 255))                    # shine
    small = big.resize((32, 32), Image.LANCZOS)
    a = small.split()[3].point(lambda v: 255 if v >= 110 else 0)
    small = small.convert('RGB').convert('RGBA')
    small.putalpha(a)
    return small

def icon_palettes(icons):
    """Groups the icons by colour into ICON_PALETTES shared 15-colour palettes (k-means style: a palette
    per group from its icons' pixels, then each icon moves to the palette that draws it best)."""
    pix = []
    for _, im in icons:
        a = np.asarray(im, dtype=np.int32).reshape(-1, 4)
        pix.append(a[a[:, 3] >= 128][:, :3] >> 3)          # opaque pixels, rgb555
    def hue(p):
        r, g, b = (p.mean(axis=0) if len(p) else np.zeros(3)) / 31
        import colorsys
        return colorsys.rgb_to_hsv(r, g, b)[0]
    order = sorted(range(len(icons)), key=lambda i: hue(pix[i]))
    group = [0] * len(icons)
    for k, i in enumerate(order):
        group[i] = k * ICON_PALETTES // len(icons)
    def palette_of(members):
        allp = np.concatenate([pix[i] for i in members]) if members else np.zeros((1, 3), np.int32)
        strip = Image.fromarray((allp[None, :, :] << 3).astype(np.uint8), 'RGB')
        q = strip.quantize(colors=15, method=Image.Quantize.MEDIANCUT, kmeans=2)
        flat = q.getpalette()[:len(set(q.getdata())) * 3]
        return sorted(set((flat[j] >> 3, flat[j + 1] >> 3, flat[j + 2] >> 3) for j in range(0, len(flat), 3)))
    def cost(p, pal):
        if not len(p):
            return 0
        d = ((p[:, None, :] - np.array(pal)[None, :, :]) ** 2).sum(axis=2)
        return int(d.min(axis=1).sum())
    for _ in range(4):
        pals = [palette_of([i for i in range(len(icons)) if group[i] == g]) for g in range(ICON_PALETTES)]
        group = [min(range(ICON_PALETTES), key=lambda g: cost(pix[i], pals[g])) for i in range(len(icons))]
    pals = [palette_of([i for i in range(len(icons)) if group[i] == g]) for g in range(ICON_PALETTES)]
    out = []
    for i, (num, im) in enumerate(icons):
        pal, idx = quantize_rgba(im, 15, fixed=pals[group[i]])
        out.append((num, pal, idx))
    return out

def write_shiny(out_inc):
    L = ['// Generated by tools/build_assets.py from sprites/pokemon/shiny; do not edit.',
         '#ifndef PR_SHINY_DATA_H\n#define PR_SHINY_DATA_H\n', '#include "bn_color.h"\n',
         'namespace pr::shiny_data\n{\n',
         '// Per species (species_list order): the shiny palette of its front and back sprites (slot 0 unused).']
    for side in ('front', 'back'):
        rows = []
        for pal in SHINY[side]:
            cols = ['bn::color(31, 0, 31)'] + [c_color(c) for c in pal] + ['bn::color(0, 0, 0)'] * (15 - len(pal))
            rows.append('{' + ', '.join(cols) + '}')
        L.append('constexpr bn::color %s[][16] = {\n    %s\n};\n' % (side, ',\n    '.join(rows)))
    L.append('}\n\n#endif\n')
    write_if_changed(os.path.join(out_inc, 'pr_shiny_data.h'), '\n'.join(L))

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
    # The party screen's empty slots (.pb-empty, dimmed) and the ones past the party cap (.pb-locked).
    ('EMPTY',    '#305870', '#1c3444', None,      4, 2),
    ('LOCKED',   '#1c3444', '#183050', None,      4, 2),
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
WRITTEN = set()      # graphics files this run produced

HIDDEN_FIRST_ID = 128     # hidden items' bits in game_state::picked (item balls use the low ones)


def hidden_items(areas):
    """GBA only: hidden items along the way, a fixed few per area on reachable open ground (a nook or a dead end
    when there is one), out of the way of doors, signs and people. Rare Candies and better balls."""
    import random
    out = []
    for ai, a in enumerate(areas):
        if a['at'][0] >= 100:
            continue        # under the sea
        beh = a['behaviour']
        h, w = len(beh), len(beh[0])
        sx, sy = a['spawn']['x'], a['spawn']['y']
        open_ = lambda x, y: 0 <= x < w and 0 <= y < h and beh[y][x] in (0, 2)
        if not open_(sx, sy):
            continue
        seen = {(sx, sy)}
        todo = [(sx, sy)]
        while todo:
            x, y = todo.pop()
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                q = (x + dx, y + dy)
                if q not in seen and open_(*q):
                    seen.add(q)
                    todo.append(q)
        busy = {(n['x'], n['y']) for n in a['people'] + a['trainers']} | {(d['x'], d['y'] + 1) for d in a['doors']} | \
               {(sg['x'], sg['y'] + 1) for sg in a['signs']} | {(it['x'], it['y']) for it in a['items']} | {(sx, sy)}
        def walls(x, y):
            return sum(not open_(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
        cands = [(x, y) for (x, y) in seen if beh[y][x] == 0 and 3 <= x < w - 3 and 3 <= y < h - 3 and (x, y) not in busy]
        if not cands:
            continue
        rnd = random.Random(1000 + ai)
        nooks = sorted(c for c in cands if walls(*c) >= 2)
        pool = nooks if len(nooks) >= 3 else sorted(cands)
        n = 3 if a.get('safari') else 1 if a['type'] == 'town' else 2
        picks = []
        for _ in range(n):
            free = [c for c in pool if all(abs(c[0] - p[0]) + abs(c[1] - p[1]) > 6 for p in picks)]
            if not free:
                break
            picks.append(rnd.choice(free))
        late = a['tier'] >= 12
        for k, (x, y) in enumerate(picks):
            item, count = ('rarecandy', 1) if (k + ai) % 2 == 0 else ('ultraball' if late else 'greatball', 3)
            out.append({'area': ai, 'x': x, 'y': y, 'item': item, 'count': count, 'id': HIDDEN_FIRST_ID + len(out)})
    if len(out) > MAX_ITEM_BALLS - HIDDEN_FIRST_ID:
        raise SystemExit('%d hidden items (max %d)' % (len(out), MAX_ITEM_BALLS - HIDDEN_FIRST_ID))
    return out


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
    # Frame 0 the POKé BALL; 1 a GREAT BALL (blue top) and 2 an ULTRA BALL (dark top, yellow bands): the red
    # half recoloured.
    frames = [b]
    # (3: a MASTER BALL, purple with pink bands.)
    for top, band in (((0x38, 0x78, 0xe0), (0xf0, 0x48, 0x48)), ((0x40, 0x40, 0x48), (0xf8, 0xd8, 0x30)),
                      ((0x80, 0x40, 0xb8), (0xf0, 0x80, 0xc0))):
        f = b.copy()
        fp = f.load()
        for y in range(16):
            for x in range(16):
                r, g, bl, al = fp[x, y]
                if al and r > 150 and g < 120 and bl < 120:
                    fp[x, y] = (band if y in (5, 6) and x in (4, 5, 10, 11) else top) + (255,)
        frames.append(f)
    save_sprite(gfx, 'ball', frames, 16, 16)
    # A hidden item's glint: a small four-point star, two sizes.
    frames = []
    for big in (False, True):
        f = Image.new('RGBA', (8, 8), (0, 0, 0, 0))
        d = ImageDraw.Draw(f)
        r = 3 if big else 2
        d.line((4 - r, 4, 4 + r, 4), fill=(0xff, 0xff, 0xff, 255))
        d.line((4, 4 - r, 4, 4 + r), fill=(0xff, 0xff, 0xff, 255))
        d.point((4, 4), fill=(0xf8, 0xe8, 0x80, 255))
        frames.append(f)
    save_sprite(gfx, 'glint', frames, 8, 8)
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

def build_plank(gfx):
    # Emerald's area name plank (.ow-popup): wood stripes, a dark frame with no top, a light inner edge and
    # a shadowed bottom, rounded at the bottom corners. Three 32x32 pieces: left, middle, right; 24 px tall.
    H = 24
    frames = []
    for piece in ('l', 'm', 'r'):
        img = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
        px = img.load()
        for y in range(H):
            for x in range(32):
                c = (0xe8, 0xb0, 0x70) if y % 3 != 2 else (0xd8, 0x9c, 0x5c)
                left, right = piece == 'l' and x < 2, piece == 'r' and x >= 30
                inner_l, inner_r = piece == 'l' and x == 2, piece == 'r' and x == 29
                if y >= H - 2 or left or right:
                    c = (0x60, 0x38, 0x18)
                elif y >= H - 4:
                    c = (0xb8, 0x78, 0x38)
                elif inner_l or inner_r or y == 0:
                    c = (0xf8, 0xd0, 0x98)
                # Rounded bottom corners.
                cut = False
                if piece == 'l' and y >= H - 5:
                    dx, dy = 5 - x, y - (H - 6)
                    cut = x < 5 and dx * dx + dy * dy > 26
                if piece == 'r' and y >= H - 5:
                    dx, dy = x - 26, y - (H - 6)
                    cut = x > 26 and dx * dx + dy * dy > 26
                if not cut:
                    px[x, y] = c + (255,)
        frames.append(img)
    save_sprite(gfx, 'plank', frames, 32, 32)

def build_screens(data, gfx):
    W = 256
    def hexc(h):
        return (int(h[1:3], 16), int(h[3:5], 16), int(h[5:7], 16), 255)
    # POKéDEX list (pokeemerald, halved): green stripes, the big ball on the left, the viewer and the yellow list
    # whose middle row (the selected one) is always the white one.
    img = Image.new('RGBA', (W, W))
    d = ImageDraw.Draw(img)
    for y in range(0, W, 4):
        d.rectangle((0, y, W - 1, y + 1), fill=hexc('#31d54a'))
        d.rectangle((0, y + 2, W - 1, y + 3), fill=hexc('#188320'))
    d.ellipse((-48, 32, 48, 128), fill=hexc('#101828'))
    d.ellipse((-45, 35, 45, 125), fill=hexc('#3a4a80'))
    d.ellipse((-40, 40, 40, 120), fill=hexc('#d8e0f0'))
    d.pieslice((-40, 40, 40, 120), 180, 360, fill=hexc('#29396a'))
    d.rectangle((-40, 76, 40, 83), fill=hexc('#101828'))
    d.rounded_rectangle((62, 22, 131, 136), radius=4, fill=hexc('#293941'))
    for x in range(64, 130):
        t = (x - 64) / 66
        c = tuple(int(0xf8 + (0xc8 - 0xf8) * t) for _ in range(2)) + (int(0xf8 + (0xd0 - 0xf8) * t),)
        d.line((x, 24, x, 134), fill=c + (255,))
    d.rectangle((134, 0, 237, 159), fill=hexc('#293941'))
    d.rectangle((136, 0, 235, 159), fill=hexc('#eef639'))
    for r in range(11):
        y = -8 + r * 16
        d.line((136, y + 15, 235, y + 15), fill=hexc('#d8e020'))
    d.rectangle((136, -8 + 5 * 16, 235, -8 + 6 * 16 - 1), fill=hexc('#293941'))
    d.rectangle((138, -8 + 5 * 16 + 2, 233, -8 + 6 * 16 - 3), fill=hexc('#ffffff'))
    save_bg(img, gfx, 'dex_bg', 'bpp_8')
    # An entry: stripes, the tab bar, the card and the description box with its yellow sides.
    img = Image.new('RGBA', (W, W))
    d = ImageDraw.Draw(img)
    for y in range(0, W, 4):
        d.rectangle((0, y, W - 1, y + 1), fill=hexc('#31d54a'))
        d.rectangle((0, y + 2, W - 1, y + 3), fill=hexc('#188320'))
    d.rectangle((0, 0, 239, 17), fill=hexc('#29396a'))
    d.rounded_rectangle((6, 22, 233, 96), radius=4, fill=hexc('#293941'))
    d.rounded_rectangle((8, 24, 231, 94), radius=3, fill=hexc('#f8f8f8'))
    d.rounded_rectangle((6, 102, 233, 153), radius=4, fill=hexc('#293941'))
    d.rectangle((8, 104, 231, 151), fill=hexc('#f8c030'))
    d.rectangle((15, 104, 224, 151), fill=hexc('#f8f8f8'))
    save_bg(img, gfx, 'dex_entry_bg', 'bpp_8')
    # The PC's box screen: the left panel (blue gradient), the title bar and the box frame on the right, and the
    # message line along the bottom.
    img = Image.new('RGBA', (W, W), hexc('#c8d0e8'))
    d = ImageDraw.Draw(img)
    for y in range(160):
        t = y / 159
        c = tuple(int(a + (b - a) * t) for a, b in zip((0x88, 0x98, 0xc8), (0x68, 0x78, 0xa8)))
        d.line((0, y, 85, y), fill=c + (255,))
    d.rectangle((86, 0, 87, 159), fill=hexc('#404858'))
    d.rectangle((88, 140, 239, 159), fill=hexc('#f8f8f8'))
    d.rectangle((88, 139, 239, 140), fill=hexc('#404858'))
    save_bg(img, gfx, 'pc_bg', 'bpp_8')
    # TRAINER CARD: the green card on a dark ground, with the badge case.
    img = Image.new('RGBA', (W, W), hexc('#303848'))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((10, 10, 229, 149), radius=7, fill=hexc('#285830'))
    for y in range(12, 148):
        t = (y - 12) / 136
        c = tuple(int(a + (b - a) * t) for a, b in zip((0xa8, 0xe0, 0x90), (0x78, 0xc0, 0x60)))
        d.line((12, y, 227, y), fill=c + (255,))
    d.line((16, 26, 223, 26), fill=hexc('#285830'), width=2)
    save_bg(img, gfx, 'card_bg', 'bpp_8')
    # Badges: empty and shining.
    frames = []
    for on in (False, True):
        b = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
        bd = ImageDraw.Draw(b)
        if on:
            bd.ellipse((1, 1, 14, 14), fill=hexc('#a07010'))
            bd.ellipse((1, 1, 12, 12), fill=hexc('#e8b020'))
            bd.ellipse((3, 3, 7, 7), fill=hexc('#fff8c0'))
        else:
            bd.ellipse((1, 1, 14, 14), fill=hexc('#1c4020'))
        frames.append(b)
    save_sprite(gfx, 'badge', frames, 16, 16)
    # The PC's hand: open (pointing) and closed (holding).
    frames = []
    for closed in (False, True):
        h = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
        hd = ImageDraw.Draw(h)
        if closed:
            hd.rounded_rectangle((2, 4, 13, 13), radius=4, fill=hexc('#303038'))
            hd.rounded_rectangle((3, 5, 12, 12), radius=3, fill=hexc('#f8f8f8'))
        else:
            hd.rounded_rectangle((2, 0, 13, 9), radius=3, fill=hexc('#303038'))
            hd.rounded_rectangle((3, 1, 12, 8), radius=2, fill=hexc('#f8f8f8'))
            hd.rectangle((5, 9, 10, 15), fill=hexc('#303038'))
            hd.rectangle((6, 9, 9, 14), fill=hexc('#f8f8f8'))
        frames.append(h)
    save_sprite(gfx, 'hand', frames, 16, 16)
    # Region map cells (.rm-cell): route, gym, town, trainer, unseen, and the blinking frame of where you are;
    # then the POKéDEX AREA's cells (.da-cell): land, a town, and a habitat lit up.
    frames = []
    for fill, border, rnd in (('#e4f4dc', '#98a8b8', 4), ('#fff0d8', '#98a8b8', 4), ('#ffffff', '#98a8b8', 4), ('#f8f8f8', '#98a8b8', 4),
                              ('#e0e4e8', '#98a8b8', 4), (None, '#f85838', 4), ('#98d070', '#285080', 2), ('#f0e8d0', '#285080', 7),
                              ('#f8f070', '#e05030', 4)):
        c = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
        cd = ImageDraw.Draw(c)
        if fill:
            cd.rounded_rectangle((1, 2, 14, 13), radius=rnd, fill=hexc(border))
            cd.rounded_rectangle((2, 3, 13, 12), radius=max(0, rnd - 1), fill=hexc(fill))
        else:
            cd.rounded_rectangle((0, 1, 15, 14), radius=rnd, outline=hexc(border), width=2)
        frames.append(c)
    save_sprite(gfx, 'cell', frames, 16, 16)
    # Links between cells: across and down, plain and locked (dashed), then the POKéDEX AREA's.
    frames = []
    for col, dashed, horiz in (('#b8a068', False, True), ('#b8a068', True, True), ('#b8a068', False, False), ('#b8a068', True, False),
                               ('#285080', False, True), ('#285080', False, False)):
        l = Image.new('RGBA', (8, 8), (0, 0, 0, 0))
        ld = ImageDraw.Draw(l)
        for k in range(8):
            if dashed and k % 4 >= 2:
                continue
            if horiz:
                ld.rectangle((k, 3, k, 4), fill=hexc(col))
            else:
                ld.rectangle((3, k, 4, k), fill=hexc(col))
        frames.append(l)
    save_sprite(gfx, 'link', frames, 8, 8)
    # The BAG (.bag-sprite), one colour per pocket: ITEMS orange, POKé BALLS red/white, TMs blue, BERRIES red,
    # KEY ITEMS purple.
    frames = []
    for top, body in (('#f8a040', '#e87830'), ('#f85050', '#f8f8f8'), ('#58a0f0', '#3878d0'), ('#f86060', '#d84040'), ('#b890f0', '#9068d0')):
        b = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
        bd = ImageDraw.Draw(b)
        bd.rounded_rectangle((8, 8, 55, 55), radius=13, fill=hexc('#503820'))
        bd.rounded_rectangle((10, 10, 53, 53), radius=11, fill=hexc(body))
        bd.rectangle((10, 10, 53, 22), fill=hexc(top))
        bd.rectangle((10, 23, 53, 24), fill=hexc('#503820'))
        frames.append(b)
    save_sprite(gfx, 'bag', frames, 64, 64)
    # Ledge dust (dust_1..3).
    frames = [art_image(data['art']['dust_%d' % k]) for k in (1, 2, 3)]
    sheet = []
    for f in frames:
        img = Image.new('RGBA', (16, 8), (0, 0, 0, 0))
        img.alpha_composite(f.convert('RGBA'), (0, 0))
        sheet.append(img)
    save_sprite(gfx, 'dust', sheet, 16, 8)
    # A box's wallpaper (boxWall): 45-degree stripes in two colours (palette entries 1 and 2), 6 px each; the
    # game recolours it per box.
    img = Image.new('RGBA', (W, W))
    px = img.load()
    for y in range(W):
        for x in range(W):
            px[x, y] = (0xf8, 0x00, 0x00, 255) if ((x + y) // 6) % 2 == 0 else (0x00, 0x00, 0xf8, 255)
    save_bg(img, gfx, 'wallpaper_bg')

# ---------------------------------------------------------------------------------------------------
# Move animations (the web game's atkFx): every move's MOVE_FX script, parsed into steps, and the pixel
# shapes (PIX) as 16x16 sprite frames in one palette: 1 outline, 2 white, 3 and 4 the step's two colours
# (filled in at run time), plus a ring and a bar segment.
FX_KINDS = ['rush', 'jaws', 'slash', 'impact', 'projectile', 'stream', 'beam', 'gather', 'rain', 'rise', 'burst', 'vortex',
            'rings', 'ringin', 'notes', 'bind', 'whip', 'bolt', 'quake', 'drain', 'powder', 'thrash', 'flip', 'vanish', 'self',
            'flash']

def fx_color(h):
    h = h.strip()
    if len(h) == 4:
        h = '#' + ''.join(c * 2 for c in h[1:])
    return h

def build_fx(data, gfx, out_inc):
    pix = data['pix']
    shapes = list(pix.keys()) + ['ring', 'dot', 'up', 'down', 'fade']
    frames = []
    for name in shapes:
        idx = [[0] * 16 for _ in range(16)]
        if name in pix:
            P = pix[name]
            h, w = len(P), len(P[0])
            ox, oy = (16 - w) // 2, (16 - h) // 2
            for y, row in enumerate(P):
                for x, ch in enumerate(row):
                    v = {'K': 1, 'W': 2, 'A': 3, 'B': 4}.get(ch, 0)
                    if v:
                        idx[oy + y][ox + x] = v
        elif name == 'ring':
            for y in range(16):
                for x in range(16):
                    d = ((x - 7.5) ** 2 + (y - 7.5) ** 2) ** 0.5
                    if 5.6 <= d <= 7.6:
                        idx[y][x] = 3
        elif name == 'dot':
            for y in range(5, 11):
                for x in range(5, 11):
                    idx[y][x] = 4 if 6 < y < 9 else 3
        frames.append(idx)
    sheet = [row for f in frames for row in f]
    placeholder = [rgb555((32, 32, 32)), rgb555((255, 255, 255)), rgb555((248, 0, 0)), rgb555((0, 248, 0))]
    save_indexed_bmp(os.path.join(gfx, 'fx.bmp'), placeholder, sheet, {'type': 'sprite', 'height': 16, 'bpp_mode': 'bpp_4'})

    type_col = data['type_col']
    steps = []
    firsts = []
    pat = re.compile(r'^(\w+)(?::(\w+))?(?:\*(\d+))?([\^x!~@]*)$')
    for m in data['moves']:
        firsts.append((len(steps), 0))
        n0 = len(steps)
        for step in m['fx'].split('+'):
            parts = step.strip().split()
            if not parts:
                continue
            mm = pat.match(parts[0])
            if not mm or mm.group(1) not in FX_KINDS:
                continue
            kind, shape, count, flags = mm.group(1), mm.group(2), int(mm.group(3) or 1), mm.group(4)
            cols = parts[1].split('/') if len(parts) > 1 else type_col.get(m['t'], type_col['Normal'])
            c1, c2 = fx_color(cols[0]), fx_color(cols[1] if len(cols) > 1 else cols[0])
            sh = shapes.index(shape) if shape in shapes else -1
            fl = ('1' if '^' in flags else '0', '1' if 'x' in flags else '0', '1' if '!' in flags else '0',
                  '1' if '~' in flags else '0', '1' if '@' in flags else '0')
            steps.append('{fx_kind::%s, %d, %d, %s, %s, %s, %s, %s, %s, %s, %s}' % (
                kind.upper(), sh, count, fl[0], fl[1], fl[2], fl[3], fl[4], 'true' if len(parts) > 1 else 'false',
                c_color(tuple(v >> 3 for v in (int(c1[1:3], 16), int(c1[3:5], 16), int(c1[5:7], 16)))),
                c_color(tuple(v >> 3 for v in (int(c2[1:3], 16), int(c2[3:5], 16), int(c2[5:7], 16))))))
        firsts[-1] = (n0, len(steps) - n0)
    L = ['// Generated by tools/build_assets.py from the web game; do not edit.',
         '#ifndef PR_FX_DATA_H\n#define PR_FX_DATA_H\n', '#include "bn_color.h"', '#include "bn_sprite_items_fx.h"\n',
         'namespace pr::fx_data\n{\n',
         'enum class fx_kind : uint8_t\n{\n' + ''.join('    %s,\n' % k.upper() for k in FX_KINDS) + '};\n',
         'namespace fx_shape\n{\n' + ''.join('    constexpr int %s = %d;\n' % (n.upper(), i) for i, n in enumerate(shapes)) + '}\n',
         'constexpr uint8_t shape_w[] = {%s};  // drawn width in px\n' % ', '.join(
             str(len(pix[n][0]) if n in pix else 16 if n == 'ring' else 6) for n in shapes),
         'struct fx_step\n{\n    fx_kind kind;\n    int8_t shape;       // -1: the kind\'s own\n    uint8_t n;\n'
         '    bool above, cross, big, spin, arc, custom;\n    bn::color c1, c2;\n};\n',
         'constexpr fx_step steps[] = {\n    ' + ',\n    '.join(steps) + '\n};\n',
         '// Per move: its first step and how many.',
         'constexpr uint16_t move_first[] = {%s};' % ', '.join(str(a) for a, _ in firsts),
         'constexpr uint8_t move_count[] = {%s};\n' % ', '.join(str(b) for _, b in firsts),
         '}\n\n#endif\n']
    write_if_changed(os.path.join(out_inc, 'pr_fx_data.h'), '\n'.join(L))

# ---------------------------------------------------------------------------------------------------
# The condensed font (tools/narrow_font.py): 8x8 cells from '!' on, then é, drawn in colours 14 (letter) and
# 12 (shadow) like Butano's own fonts, so the text palettes apply. A glyph's width is its pixels plus the
# shadow column.
def build_narrow_font(gfx, out_inc):
    import importlib.util
    spec = importlib.util.spec_from_file_location('narrow_font', os.path.join(HERE, 'narrow_font.py'))
    nf = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(nf)
    chars = [chr(c) for c in range(33, 127)] + ['é', 'É']
    rows = []
    widths = [nf.SPACE_WIDTH]
    for ch in chars:
        g = nf.GLYPHS.get(ch, ['###', '#.#', '#.#', '#.#', '###'])
        w = max(len(r) for r in g)
        cell = [[0] * 8 for _ in range(8)]
        for y, row in enumerate(g):
            for x, c in enumerate(row):
                if c == '#':
                    cell[y + 1][x] = 14
        for y in range(8):
            for x in range(8):
                if cell[y][x] == 14:
                    for dx, dy in ((1, 0), (0, 1), (1, 1)):
                        if x + dx < 8 and y + dy < 8 and cell[y + dy][x + dx] == 0:
                            cell[y + dy][x + dx] = 12
        rows.extend(cell)
        widths.append(w + 1)
    palette = [rgb555((0, 0, 0))] * 15
    save_indexed_bmp(os.path.join(gfx, 'narrow_font.bmp'), palette, rows, {'type': 'sprite', 'height': 8, 'bpp_mode': 'bpp_4'})
    L = ['// Generated by tools/build_assets.py from tools/narrow_font.py; do not edit.',
         '#ifndef PR_NARROW_FONT_H\n#define PR_NARROW_FONT_H\n',
         '#include "bn_sprite_font.h"', '#include "bn_utf8_characters_map.h"', '#include "bn_sprite_items_narrow_font.h"\n',
         'namespace pr\n{\n',
         'constexpr bn::utf8_character narrow_font_utf8_characters[] = { "é", "É" };',
         'constexpr int8_t narrow_font_widths[] = {%s};' % ', '.join(map(str, widths)),
         'constexpr bn::span<const bn::utf8_character> narrow_font_utf8_characters_span(narrow_font_utf8_characters);',
         'constexpr auto narrow_font_utf8_characters_map = bn::utf8_characters_map<narrow_font_utf8_characters_span>();',
         'constexpr bn::sprite_font narrow_sprite_font(bn::sprite_items::narrow_font, narrow_font_utf8_characters_map.reference(),',
         '                                             narrow_font_widths);\n',
         '}\n\n#endif\n']
    write_if_changed(os.path.join(out_inc, 'pr_narrow_font.h'), '\n'.join(L))

def build_weather(gfx):
    """The weather overlays (the stylesheet's .wx-* layers): particles on a transparent 256x256 layer that the
    game scrolls and blends over the field. Three densities for rain, snow, ash and bubbles (OPTION > WEATHER
    LOW / MED / HIGH); the fog's drifting banks; the cave's darkness at the edges; the Ghost gym's circle of sight."""
    import random
    W = 256
    def layer(seed, count, draw_one):
        img = Image.new('RGBA', (W, W), (0, 0, 0, 0))
        d = ImageDraw.Draw(img)
        r = random.Random(seed)
        for _ in range(count):
            draw_one(d, r.randrange(W), r.randrange(W), r)
        return img
    def wrap_draw(fn):
        # Draw at the point and its wrapped copies so the layer tiles seamlessly.
        def draw(d, x, y, r):
            for ox in (0, -W, W):
                for oy in (0, -W, W):
                    fn(d, x + ox, y + oy, r)
        return draw
    def rain(d, x, y, r):
        n = r.choice((3, 4, 5))
        for k in range(n):
            d.point((x - k // 3, y + k), fill=(0xd8, 0xe8, 0xff, 255))
    def snow(d, x, y, r):
        s = r.choice((0, 1, 1))
        d.ellipse((x, y, x + s, y + s), fill=(0xff, 0xff, 0xff, 255))
    def ash(d, x, y, r):
        c = r.choice(((0x5a, 0x5a, 0x62), (0x70, 0x70, 0x78), (0x4a, 0x4a, 0x52), (0x60, 0x60, 0x68)))
        s = r.choice((0, 1, 1))
        d.rectangle((x, y, x + s, y + (1 if s else 0)), fill=c + (255,))
    def bubble(d, x, y, r):
        s = r.choice((1, 2, 2, 3))
        if s >= 2:
            d.ellipse((x, y, x + s, y + s), outline=(0xe8, 0xfb, 0xff, 255))
        else:
            d.point((x, y), fill=(0xe8, 0xfb, 0xff, 255))
    for name, fn, base in (('rain', rain, 90), ('snow', snow, 70), ('ash', ash, 70), ('deep', bubble, 30)):
        for level in (1, 2, 3):
            img = layer(hash(name) % 1000 + level, base * level, wrap_draw(fn))
            save_bg(img, gfx, 'wx_%s_%d' % (name, level))
    # Fog: soft white banks, dithered so the layer has edges to blend.
    img = Image.new('RGBA', (W, W), (0, 0, 0, 0))
    px = img.load()
    for y in range(W):
        for x in range(W):
            v = 0
            for cx, cy, rx, ry in ((70, 80, 80, 30), (190, 170, 100, 35), (200, 40, 60, 22)):
                for ox in (0, -W, W):
                    u = ((x - cx - ox) / rx) ** 2 + ((y - cy) / ry) ** 2
                    v = max(v, 1 - u)
            bayer = ((x % 4) * 4 + (y % 4) * 7 + (x // 4 % 2) * 2) % 16 / 16
            if v > bayer * 0.9 + 0.05:
                px[x, y] = (0xff, 0xff, 0xff, 255)
    save_bg(img, gfx, 'wx_fog')
    # Cave: darkness creeping in from the edges (radial, dithered).
    img = Image.new('RGBA', (W, W), (0, 0, 0, 0))
    px = img.load()
    for y in range(160):
        for x in range(240):
            dist = (((x - 120) / 120) ** 2 + ((y - 80) / 120) ** 2) ** 0.5 * 1.25
            t = max(0, min(1, (dist - 0.38) / 0.52))
            bayer = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]][y % 4][x % 4] / 16
            if t > bayer:
                px[x, y] = (0, 0, 0, 255)
    save_bg(img, gfx, 'wx_cave')
    # Ghost gym: dark except a circle around you that grows with every junior beaten (70, 115, 160 css px).
    for k, radius in ((1, 35), (2, 57), (3, 80)):
        img = Image.new('RGBA', (W, W), (0x08, 0x00, 0x10, 255))
        px = img.load()
        for y in range(160):
            for x in range(240):
                dist = ((x - 120) ** 2 + (y - 75) ** 2) ** 0.5
                if dist < radius - 10:
                    px[x, y] = (0, 0, 0, 0)
                elif dist < radius:
                    bayer = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]][y % 4][x % 4] / 16
                    if (dist - (radius - 10)) / 10 < bayer:
                        px[x, y] = (0, 0, 0, 0)
        save_bg(img, gfx, 'wx_ghost_%d' % k)

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
    town = Image.open(os.path.join(EXP, 'map_0.png')).convert('RGB')
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
ABILITY_KIND = {'flavor': 'FLAVOR', 'boost': 'BOOST', 'immune': 'IMMUNE', 'punch': 'PUNCH', 'merciless': 'MERCILESS',
                'corrosion': 'CORROSION', 'disguise': 'DISGUISE'}
HELD = ['none', 'leftovers', 'lifeorb', 'scarf', 'sash', 'sitrus']

DAYCARE_TOWN = 'Cindergate Town'

def free_spot(a, x, y):
    """The nearest open path tile to (x, y) in an area: nobody on it, and not right below a door."""
    beh = a['behaviour']
    taken = {(n['x'], n['y']) for n in a['people']} | {(t['x'], t['y']) for t in a['trainers']} | \
            {(sg['x'], sg['y']) for sg in a['signs']} | {(it['x'], it['y']) for it in a['items']} | \
            {(d['x'], d['y'] + k) for d in a['doors'] for k in (0, 1)}
    for r in range(0, 12):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                tx, ty = x + dx, y + dy
                if max(abs(dx), abs(dy)) == r and 0 <= ty < len(beh) and 0 <= tx < len(beh[0]) and beh[ty][tx] == 0 \
                        and (tx, ty) not in taken:
                    return tx, ty
    raise SystemExit('no free spot near (%d, %d) in %s' % (x, y, a['name']))

def add_extras(data):
    """GBA 1.8 (data/extras.json, from tools/build_extras.py): evolutions by item (stones, the LINKING CORD) and the
    level-up ones the data had no level for (friendship, a known move, a place: a stand-in level), egg groups and
    base forms for the DAY CARE, and the TMs that teach moves this game has."""
    ex = json.load(open(os.path.join(HERE, '..', 'data', 'extras.json'), encoding='utf8'))
    sp = data['species']
    names = [s['name'] for s in sp]
    by_num = {s['num']: i for i, s in enumerate(sp) if s['num'] < 10000}
    into = {}           # species index -> the level its pre-evolution evolves into it at
    for s in sp:
        if s['evo'] and s['evo']['to'] in names:
            into[names.index(s['evo']['to'])] = s['evo']['level']
    evo_items = set()
    for e in ex['evolutions']:
        a, b = by_num.get(e['from']), by_num.get(e['to'])
        if a is None or b is None:
            continue
        if 'item' in e:
            evo_items.add((a, b, EVO_ITEMS[e['item']]))
        elif not sp[a]['evo']:
            sp[a]['evo'] = {'to': names[b], 'level': e['level'] or min(60, max(20, into.get(a, 0) + 10))}
            into[b] = sp[a]['evo']['level']
    data['evo_items'] = sorted(evo_items)
    # Egg groups (two nibbles; 15 = can't breed) and the base form an EGG hatches into.
    data['egg_groups'], data['babies'] = [], []
    for i, s in enumerate(sp):
        g = ex['egg_groups'].get(str(s['num']), [15]) if s['num'] < 10000 else [15]
        data['egg_groups'].append(g[0] | ((g[1] if len(g) > 1 else g[0]) << 4))
        n = s['num']
        while s['num'] < 10000 and ex['evolves_from'].get(str(n)) in by_num:
            n = ex['evolves_from'][str(n)]
        data['babies'].append(by_num.get(n, i))
    # TMs (Ultra Sun / Moon's numbers), reusable; sold in the MART from a badge count that rises with power.
    norm = lambda t: re.sub(r'[^a-z0-9]+', '-', t.lower()).strip('-')
    # Only 1.x moves get TMs: a new TM would shift every held item's id, and saves hold item ids.
    move_index = {norm(m['n']): i for i, m in enumerate(data['moves'][:data.get('moves_v1', len(data['moves']))])}
    data['tms'] = []
    for t in ex['tms']:
        if t['move'] not in move_index:
            continue
        mi = move_index[t['move']]
        m = data['moves'][mi]
        key = 'tm%02d' % t['tm']
        status = m['c'] == 'status' or not m['p']
        price, badges = (1000, 0) if status or m['p'] <= 60 else (2000, 2) if m['p'] <= 80 else (3000, 4) if m['p'] <= 95 else (5000, 6)
        ITEM_IDS.append(key)
        what = 'A %s move%s.' % (m['t'].upper(), '' if status else ', power %d' % m['p'])
        GBA_ITEMS[key] = {'name': 'TM%02d %s' % (t['tm'], m['n'].upper()), 'pocket': 2, 'price': price,
                          'desc': what + ' Teach it again and again.'}
        data['tms'].append({'move': mi, 'item': key, 'badges': badges, 'slug': t['move']})
    ITEM_IDS.extend(HELD_ITEM_IDS)
    if len(ITEM_IDS) > 255:
        raise SystemExit('%d kinds of items (max 255)' % len(ITEM_IDS))
    data['tm_compat'] = []
    for s in sp:
        can = set(ex['tm_compat'].get(str(s['num']), []))
        bits = [0] * ((len(data['tms']) + 7) // 8)
        for k, t in enumerate(data['tms']):
            if t['slug'] in can:
                bits[k >> 3] |= 1 << (k & 7)
        data['tm_compat'].append(bits)

def build_game_data(data, out_inc):
    types = data['types']
    L = ['// Generated by tools/build_assets.py from the web game; do not edit.',
         '#ifndef PR_GAME_DATA_H\n#define PR_GAME_DATA_H\n', '#include "pr_game_types.h"']
    for s in data['species']:
        L.append('#include "bn_sprite_items_mon_front_%d.h"\n#include "bn_sprite_items_mon_back_%d.h"\n#include "bn_sprite_items_mon_icon_%d.h"' % (s['num'], s['num'], s['num']))
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
        st = (m.get('stats') or [])[:3]
        st_c = ', '.join('{%d, %d}' % (a, b) for a, b in st + [(0, 0)] * (3 - len(st)))
        mv.append('{%s, %d, %d, %s, %d, %s, %s, %d, %s, %d, move_target::%s, %d, %d, {%s}, %d, %d, %s, battle_weather::%s}' % (
            c_text(m['n'].upper()), types.index(m['t']), m['p'], cat[m['c']], m['a'],
            STATUS[m.get('status')], STATUS[sec.get('status')], sec.get('chance', 0), 'true' if m.get('punch') else 'false',
            m.get('pp', 10), (m.get('target') or 'one').upper(), m.get('drain', 0), m.get('heal', 0), st_c, len(st),
            m.get('stat_chance', 100), 'true' if m.get('stat_self') else 'false', (m.get('weather') or 'none').upper()))
    L.append('constexpr move moves[] = {\n    ' + ',\n    '.join(mv) + '\n};\n')
    # Legendary and mythical Pokémon (Phase 7: the CHALLENGE TOWER's prizes), by species index.
    legend_nums = {144, 145, 146, 150, 151, 243, 244, 245, 249, 250, 251, 377, 378, 379, 380, 381, 382, 383, 384, 385,
                   386, 480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 638, 639, 640, 641, 642,
                   643, 644, 645, 646, 647, 648, 649, 716, 717, 718, 719, 720, 721, 785, 786, 787, 788, 789, 790, 791, 792,
                   800, 801, 802, 807, 808, 809, 888, 889, 890, 891, 892, 893, 894, 895, 896, 897, 898, 905, 1001, 1002,
                   1003, 1004, 1007, 1008, 1014, 1015, 1016, 1017, 1024, 1025}
    legends = [i for i, sp in enumerate(data['species']) if sp['num'] in legend_nums]
    L.append('constexpr uint16_t legendaries[] = {%s};' % ', '.join(map(str, legends)))
    L.append('constexpr int legendaries_count = %d;\n' % len(legends))
    names = [s['name'] for s in data['species']]
    # Abilities, shared by name.
    abil_index = {}
    abil_rows = []
    for s in data['species']:
        a = s['ability']
        key = (a['n'], a['desc'], a['type'], a['of'])
        if key not in abil_index:
            abil_index[key] = len(abil_rows)
            desc = a['desc'].replace('(No battle effect yet.)', '').strip()
            abil_rows.append('{%s, %s, ability_kind::%s, %d}' % (c_text(a['n'].upper()), c_text(desc), ABILITY_KIND.get(a['type'], 'FLAVOR'),
                                                              types.index(a['of']) if a['of'] in types else -1))
    L.append('constexpr ability abilities[] = {\n    %s\n};\n' % ',\n    '.join(abil_rows))
    for i, s in enumerate(data['species']):
        L.append('constexpr learn_entry learn_%d[] = {%s};' % (i, ', '.join('{%d, %d}' % (lv, mi) for lv, mi in s['learn']) or '{1, 0}'))
        L.append('constexpr uint16_t fixed_%d[] = {%s};' % (i, ', '.join(map(str, s['moves'])) or '0'))
    L.append('\nconstexpr species species_list[] = {')
    for i, s in enumerate(data['species']):
        b = s['base']
        evo = s['evo']
        evo_to = names.index(evo['to']) if evo and evo['to'] in names else -1
        t2 = types.index(s['types'][1]) if len(s['types']) > 1 else -1
        a = s['ability']
        L.append('    {%s, %d, %d, %d, {%d, %d, %d, %d, %d, %d}, learn_%d, %d, fixed_%d, %d, %d, %d, %d, abilities[%d], %d, %d, %s, '
                 'bn::sprite_items::mon_front_%d, bn::sprite_items::mon_back_%d, bn::sprite_items::mon_icon_%d},' % (
            c_text(s['name'].upper()), s['num'], types.index(s['types'][0]), t2,
            b['hp'], b['atk'], b['def'], b['spa'], b['spd'], b['spe'], i, len(s['learn']), i, len(s['moves']),
            evo_to, evo['level'] if evo_to >= 0 else 0, s['capture'], abil_index[(a['n'], a['desc'], a['type'], a['of'])],
            s['height'] or 0, s['weight'] or 0, c_text(s['genus'] or ''), s['num'], s['num'], s['num']))
    L.append('};\n')
    # Items (ITEM_INFO): name, pocket, description, price, and what they do.
    its = []
    for i in ITEM_IDS:
        it = data['items'][i] if i in data['items'] else GBA_ITEMS[i]
        its.append('{%s, %d, %s, %d, %d, %s, %s, %s}' % (
            c_text(it['name']), it['pocket'], c_text(it['desc']), it.get('price', 0), it.get('heal', 0),
            STATUS[it.get('cure')], 'true' if it.get('revive') else 'false', 'true' if it.get('full') else 'false'))
    L.append('constexpr item_info items[] = {\n    %s\n};' % ',\n    '.join(its))
    # GBA 1.8: evolution items, egg groups and base forms, TMs.
    L.append('constexpr evo_item evo_items[] = {%s};' % ', '.join(
        '{%d, %d, item_id::%s}' % (a, b, it.upper()) for a, b, it in data['evo_items']))
    L.append('constexpr int evo_items_count = %d;' % len(data['evo_items']))
    L.append('constexpr uint8_t egg_groups[] = {%s};' % ', '.join(map(str, data['egg_groups'])))
    L.append('constexpr uint16_t babies[] = {%s};' % ', '.join(map(str, data['babies'])))
    L.append('constexpr tm_info tms[] = {%s};' % ', '.join(
        '{%d, item_id::%s, %d}' % (t['move'], t['item'].upper(), t['badges']) for t in data['tms']))
    L.append('constexpr int tms_count = %d;' % len(data['tms']))
    L.append('constexpr uint8_t tm_compat[][%d] = {%s};' % (len(data['tm_compat'][0]), ', '.join(
        '{%s}' % ', '.join(map(str, b)) for b in data['tm_compat'])))
    held = data['held_items']
    L.append('constexpr held_item_info held_items[] = {%s};' % ', '.join(
        '{%s, %s}' % (c_text(held[h]['n'].upper()), c_text(held[h].get('desc', ''))) for h in HELD))
    L.append('constexpr species_id starter_trios[][3] = {%s};' % ', '.join(
        '{%s}' % ', '.join('species_id::%s' % enum_name(n) for n in trio) for trio in data['starter_trios']))
    L.append('constexpr const char* prof_name = %s;' % c_text(data['prof']))
    L.append('constexpr const char* intro_lines[] = {%s};' % ', '.join(c_text(l) for l in data['intro']))
    # The professor's calls (PROF_CALLS): after 1, 2 and 4 badges.
    for k, call in enumerate(data['prof_calls']):
        L.append('constexpr const char* prof_call%d[] = {%s};' % (k, ', '.join(c_text(data['prof'] + ': ' + l) for l in call['lines'])))
    L.append('constexpr const char* const* prof_calls[] = {%s};' % ', '.join('prof_call%d' % k for k in range(len(data['prof_calls']))))
    L.append('constexpr int prof_call_lines[] = {%s};' % ', '.join(str(len(c['lines'])) for c in data['prof_calls']))
    L.append('constexpr int prof_call_badges[] = {%s};' % ', '.join(str(c['badges']) for c in data['prof_calls']))
    # Day and night (TIME_TYPES, NIGHT_VISITORS).
    tod = data['time_types']
    for k in ('morning', 'day', 'evening', 'night'):
        L.append('constexpr int8_t time_types_%s[] = {%s, -1};' % (k, ', '.join(str(types.index(t)) for t in tod[k])))
    L.append('constexpr const int8_t* time_types[] = {time_types_morning, time_types_day, time_types_evening, time_types_night};')
    L.append('constexpr species_id night_visitors[] = {%s};' % ', '.join('species_id::%s' % enum_name(n) for n in data['night_visitors']))
    L.append('constexpr species_id tempest_pool[] = {%s};' % ', '.join('species_id::%s' % enum_name(n) for n in data['tempest_pool']))
    # The POKéDEX's order (DEX_LIST): national numbers, the regional forms left out.
    order = sorted([i for i, sp in enumerate(data['species']) if 0 < sp['num'] < 10000], key=lambda i: data['species'][i]['num'])
    L.append('constexpr uint16_t dex_order[] = {%s};' % ', '.join(map(str, order)))
    L.append('constexpr int dex_order_count = %d;' % len(order))
    # The PC's wallpapers (WALLPAPERS, WALL_NAMES).
    L.append('constexpr uint16_t wallpapers[][2] = {%s};' % ', '.join(
        '{%d, %d}' % tuple((int(c[1:3], 16) >> 3) | ((int(c[3:5], 16) >> 3) << 5) | ((int(c[5:7], 16) >> 3) << 10) for c in w)
        for w in data['wallpapers']))
    L.append('constexpr const char* wall_groups[] = {%s};' % ', '.join(c_text(g[0]) for g in data['wall_names']))
    L.append('constexpr const char* wall_names[] = {%s};' % ', '.join(c_text(n) for g in data['wall_names'] for n in g[1]))
    L.append('}\n\n#endif')
    write_if_changed(os.path.join(out_inc, 'pr_game_data.h'), '\n'.join(L))

    # Enums shared by the data headers.
    kinds = list(data['people'])
    E = ['// Generated by tools/build_assets.py; do not edit.', '#ifndef PR_IDS_H\n#define PR_IDS_H\n',
         '#include <cstdint>\n', 'namespace pr\n{\n', 'enum class species_id : uint16_t\n{']
    seen_names = set()
    for n in names:
        e = enum_name(n)
        assert e not in seen_names, 'duplicate species enum ' + e
        seen_names.add(e)
        E.append('    %s,' % e)
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

def build_news(out_inc):
    """WHAT'S NEW: the message of the day and the changelog (news_gba.json), as of the build."""
    news = json.load(open(os.path.join(HERE, '..', 'news_gba.json'), encoding='utf8'))
    L = ['// Generated by tools/build_assets.py from news_gba.json; do not edit.', '#ifndef PR_NEWS_DATA_H\n#define PR_NEWS_DATA_H\n',
         'namespace pr::news_data\n{\n', 'constexpr const char* motd = %s;' % c_text(news.get('motd', ''))]
    lines = []
    for e in news.get('changelog', []):
        v = e['version']
        lines.append('%s%s  %s' % ('' if v.startswith('GBA') else 'v', v, e['date']))
        lines.append(e['title'])
        for c in e.get('changes', []):
            lines.append('- ' + c)
        lines.append('')
    L.append('constexpr const char* lines[] = {%s};' % ', '.join(c_text(l) for l in lines))
    L.append('constexpr int lines_count = %d;' % len(lines))
    L.append('}\n\n#endif')
    write_if_changed(os.path.join(out_inc, 'pr_news_data.h'), '\n'.join(L))

def inputs_hash():
    h = hashlib.sha256()
    files = [os.path.join(ROOT, 'js', f) for f in ('app.js', 'dexdata.js', 'dexinfo.js', 'tileart.js', 'music.js')] + \
            [os.path.join(HERE, f) for f in ('export.js', 'build_assets.py', 'narrow_font.py')]
    files.append(os.path.join(ROOT, 'js', 'moveextra.js'))
    files.append(os.path.join(HERE, '..', 'news_gba.json'))
    files.append(os.path.join(HERE, '..', 'data', 'extras.json'))
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
    # Graphics from older builds would still be picked up by Butano: whatever this run doesn't write goes.
    before = set(os.listdir(gfx))
    for f in ('pr_species_id.h',):
        if os.path.exists(os.path.join(inc, f)):
            os.remove(os.path.join(inc, f))
    subprocess.run(['node', os.path.join(HERE, 'export.js'), EXP], check=True)
    data = json.load(open(os.path.join(EXP, 'data.json'), encoding='utf8'))
    add_extras(data)
    build_world(EXP, data, inc)
    build_game_data(data, inc)
    build_music(data, inc)
    build_news(inc)
    build_ui_tiles(inc)
    build_people(data, gfx)
    build_mons(data, gfx)
    write_shiny(inc)
    BALL.extend(data['ball'])
    build_small_sprites(gfx)
    build_plank(gfx)
    build_screens(data, gfx)
    build_weather(gfx)
    build_fx(data, gfx, inc)
    build_narrow_font(gfx, inc)
    build_grass_front(data, gfx)
    build_backgrounds(data, gfx)
    written = set(WRITTEN)
    for f in before:
        if f not in written:
            os.remove(os.path.join(gfx, f))
    open(stamp, 'w').write(digest)

if __name__ == '__main__':
    main()
