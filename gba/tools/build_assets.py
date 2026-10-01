#!/usr/bin/env python3
"""Builds Party Royale's GBA data from the web game.

1. tools/export.js loads the web game's js/*.js and writes the areas (drawn as pictures), people art,
   species and moves.
2. This script turns that into GBA data in <out>/:
   - include/pr_world_data.h  overworld tiles, palettes, 16x16 metatiles, per-area maps, behaviours,
                              signs, people and connections
   - include/pr_game_data.h   species, moves, the type chart
   - graphics/*.bmp + .json   sprites and full-screen backgrounds for Butano's asset tool
Needs Node.js and Pillow. Called by the Makefile (EXTTOOL) before every build; it skips work when its
inputs haven't changed.
"""
import argparse, hashlib, json, os, re, subprocess, sys
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
GBA = os.path.dirname(HERE)
ROOT = os.path.dirname(GBA)
AREAS = [0, 1]          # Duskmere Hollow, Route 1 (the proof-of-concept slice)
MAP_BANKS = 13          # BG palette banks the overworld may use; the rest are for text boxes

# ---------------------------------------------------------------------------------------------------
def rgb555(c):
    return (c[0] >> 3, c[1] >> 3, c[2] >> 3)

def c_color(c):
    return 'bn::color(%d, %d, %d)' % c

def enum_name(s):
    return re.sub(r'[^A-Za-z0-9]', '_', s).upper()

def nonempty(body, dummy):
    """C++ has no empty arrays: a placeholder element (the count stays 0)."""
    return body or dummy

def c_string(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'

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

# ---------------------------------------------------------------------------------------------------
# Overworld: cut every area picture into 8x8 tiles, share them (with flips) across areas, pack their
# colours into 4bpp palette banks, then describe each area as 16x16 metatiles of four cells.

def pack_palettes(color_sets, max_banks):
    """Greedy bin packing of tile colour sets into banks of 15 colours (index 0 is transparent)."""
    banks = []
    order = sorted(range(len(color_sets)), key=lambda i: -len(color_sets[i]))
    assign = [None] * len(color_sets)
    for i in order:
        cs = color_sets[i]
        best, best_add = None, None
        for b, bank in enumerate(banks):
            add = len(cs - bank)
            if len(bank) + add <= 15 and (best is None or add < best_add):
                best, best_add = b, add
        if best is None:
            banks.append(set(cs))
            best = len(banks) - 1
        else:
            banks[best] |= cs
        assign[i] = best
    if len(banks) > max_banks:
        raise SystemExit('overworld needs %d palette banks (max %d)' % (len(banks), max_banks))
    return banks, assign

def build_world(exp, data, out_inc):
    areas = data['areas']
    # Every 8x8 block of every area (plus the forest that fills the world past the maps' edges).
    pics = []
    for a in areas:
        raw = open(os.path.join(exp, 'map_%d.rgb' % a['index']), 'rb').read()
        pics.append(Image.frombytes('RGB', (a['w'] * 16, a['h'] * 16), raw))
    forest = art_image(data['art']['forest_fill'], (0x1e, 0x4a, 0x1c))
    blocks = []      # list of 64-tuples of rgb555
    def cut(img, cx, cy):
        px = img.load()
        return tuple(rgb555(px[cx * 8 + x, cy * 8 + y]) for y in range(8) for x in range(8))
    area_blocks = []
    for img in pics:
        w, h = img.size[0] // 8, img.size[1] // 8
        area_blocks.append([[cut(img, x, y) for x in range(w)] for y in range(h)])
    forest_blocks = [[cut(forest, x, y) for x in range(2)] for y in range(2)]
    grass = art_image(data['art']['grass_v0'], (0x48, 0xd8, 0x90))      # drawn over picked-up items
    grass_blocks = [[cut(grass, x, y) for x in range(2)] for y in range(2)]

    # Unique blocks up to flips.
    def flips(b):
        rows = [b[y * 8:(y + 1) * 8] for y in range(8)]
        h = tuple(p for r in rows for p in reversed(r))
        v = tuple(p for r in reversed(rows) for p in r)
        hv = tuple(p for r in reversed(rows) for p in reversed(r))
        return [(b, 0), (h, 1), (v, 2), (hv, 3)]
    uniq, index = [], {}
    def ref(b):
        for fb, f in flips(b):
            if fb in index:
                return index[fb], f
        index[b] = len(uniq)
        uniq.append(b)
        return len(uniq) - 1, 0
    area_refs = [[[ref(b) for b in row] for row in ab] for ab in area_blocks]
    forest_refs = [[ref(b) for b in row] for row in forest_blocks]
    grass_refs = [[ref(b) for b in row] for row in grass_blocks]

    sets = [frozenset(b) for b in uniq]
    banks, assign = pack_palettes(sets, MAP_BANKS)
    bank_lists = [sorted(b) for b in banks]
    if len(uniq) > 1008:
        raise SystemExit('overworld needs %d tiles (max 1008)' % len(uniq))

    def tile_words(b, bank):
        lut = {c: i + 1 for i, c in enumerate(bank_lists[bank])}
        words = []
        for y in range(8):
            v = 0
            for x in range(8):
                v |= lut[b[y * 8 + x]] << (4 * x)
            words.append('0x%08x' % v)
        return '{' + ', '.join(words) + '}'

    def cell(r):
        t, f = r
        return t | (f << 10) | (assign[t] << 12)

    # Metatiles: four cells (TL, TR, BL, BR), shared by every area.
    metas, meta_index = [], {}
    def meta(refs4):
        key = tuple(cell(r) for r in refs4)
        if key not in meta_index:
            meta_index[key] = len(metas)
            metas.append(key)
        return meta_index[key]
    forest_meta = meta([forest_refs[0][0], forest_refs[0][1], forest_refs[1][0], forest_refs[1][1]])
    grass_meta = meta([grass_refs[0][0], grass_refs[0][1], grass_refs[1][0], grass_refs[1][1]])
    area_meta = []
    for a, refs in zip(areas, area_refs):
        area_meta.append([meta([refs[2 * y][2 * x], refs[2 * y][2 * x + 1], refs[2 * y + 1][2 * x], refs[2 * y + 1][2 * x + 1]])
                          for y in range(a['h']) for x in range(a['w'])])

    L = []
    L.append('// Generated by tools/build_assets.py from the web game; do not edit.')
    L.append('#ifndef PR_WORLD_DATA_H\n#define PR_WORLD_DATA_H\n')
    L.append('#include "bn_tile.h"\n#include "bn_color.h"\n#include "pr_world_types.h"\n')
    L.append('namespace pr::world_data\n{\n')
    L.append('constexpr int tiles_count = %d;' % len(uniq))
    L.append('constexpr int palette_banks = %d;' % len(banks))
    L.append('constexpr int forest_metatile = %d;' % forest_meta)
    L.append('constexpr int grass_metatile = %d;\n' % grass_meta)
    L.append('alignas(int) constexpr bn::tile tiles[] = {\n    ' +
             ',\n    '.join(tile_words(b, assign[i]) for i, b in enumerate(uniq)) + '\n};\n')
    colors = []
    for bl in bank_lists:
        bank = [(0, 0, 0)] + bl
        bank += [(0, 0, 0)] * (16 - len(bank))
        colors += bank
    L.append('constexpr bn::color colors[] = {\n    ' + ',\n    '.join(', '.join(c_color(c) for c in colors[i:i + 8])
                                                     for i in range(0, len(colors), 8)) + '\n};\n')
    L.append('constexpr uint16_t metatiles[][4] = {\n    ' +
             ',\n    '.join('{%d, %d, %d, %d}' % m for m in metas) + '\n};\n')

    for ai, (a, mm) in enumerate(zip(areas, area_meta)):
        p = 'area%d_' % ai
        L.append('constexpr uint16_t %smap[] = {%s};' % (p, ', '.join(map(str, mm))))
        L.append('constexpr uint8_t %sbehaviour[] = {%s};' % (p, ', '.join(str(v) for row in a['behaviour'] for v in row)))
        L.append('constexpr sign %ssigns[] = {%s};' % (p, nonempty(', '.join(
            '{%d, %d, %s}' % (s['x'], s['y'], c_string(clean_text(s['text']))) for s in a['signs']), '{0, 0, ""}')))
        L.append('constexpr door %sdoors[] = {%s};' % (p, nonempty(', '.join(
            '{%d, %d, door_kind::%s}' % (d['x'], d['y'], d['kind'].upper()) for d in a['doors']), '{0, 0, door_kind::HOUSE}')))
        ppl = []
        for pi, n in enumerate(a['people']):
            lines = [clean_text(l) for l in n['lines']] or ['...']
            L.append('constexpr const char* %sperson%d_lines[] = {%s};' % (p, pi, ', '.join(c_string(l) for l in lines)))
            ppl.append('{%d, %d, person_kind::%s, direction::%s, %sperson%d_lines, %d}' % (
                n['x'], n['y'], n['kind'], n['facing'].upper(), p, pi, len(lines)))
        L.append('constexpr person %speople[] = {%s};' % (p, nonempty(', '.join(ppl), '{0, 0, person_kind::player, direction::DOWN, nullptr, 0}')))
        links = []
        for ln in a['links']:
            target = AREAS.index(ln['to']) if ln['to'] in AREAS else -1
            links.append('{%d, %d, %d, %d, %d, %s}' % (target, ln['ox'], ln['oy'], ln['w'], ln['h'], c_string(clean_text(ln['name'].upper()))))
        L.append('constexpr link %slinks[] = {%s};' % (p, nonempty(', '.join(links), '{-1, 0, 0, 0, 0, ""}')))
        pool = ', '.join('species_id::%s' % enum_name(s) for s in a['pool'])
        L.append('constexpr species_id %spool[] = {%s};' % (p, pool or 'species_id::PIDGEY'))
        L.append('')
    L.append('constexpr area areas[] = {')
    for ai, a in enumerate(areas):
        p = 'area%d_' % ai
        L.append('    {%s, %d, %d, %smap, %sbehaviour, %ssigns, %d, %sdoors, %d, %speople, %d, %slinks, %d, %spool, %d, %d, %d, %d},' % (
            c_string(clean_text(a['name'].upper())), a['w'], a['h'], p, p, p, len(a['signs']), p, len(a['doors']),
            p, len(a['people']), p, len(a['links']), p, len(a['pool']) if a['pool'] else 0,
            a['spawn']['x'], a['spawn']['y'], min(50, 8 + a['tier'] * 4)))
    L.append('};\n')
    L.append('}\n\n#endif')
    write_if_changed(os.path.join(out_inc, 'pr_world_data.h'), '\n'.join(L))
    print('build_assets: overworld %d tiles, %d palette banks, %d metatiles' % (len(uniq), len(banks), len(metas)))

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

def quantize_rgba(img, max_colors):
    """RGBA -> list of opaque rgb555 colours (<= max_colors) and an index map (0 = transparent)."""
    px = img.load()
    w, h = img.size
    cols = {}
    for y in range(h):
        for x in range(w):
            p = px[x, y]
            if p[3] >= 128:
                c = rgb555(p)
                cols[c] = cols.get(c, 0) + 1
    palette = list(cols)
    remap = {c: c for c in palette}
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

def sprite_sheet(frames, fw, fh, max_colors=15):
    """Frames (RGBA images fw x fh) stacked vertically, sharing one palette."""
    sheet = Image.new('RGBA', (fw, fh * len(frames)), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        sheet.alpha_composite(f, (0, i * fh))
    return quantize_rgba(sheet, max_colors)

def build_people(data, gfx):
    # 16x32 sprites: the 16x21 frame sits 10 px down (11 on step frames), so the feet land on the tile.
    names = []
    for kind, dirs in data['people'].items():
        frames = []
        for d in ('down', 'up', 'left'):
            for f in dirs[d]:
                img = Image.new('RGBA', (16, 32), (0, 0, 0, 0))
                px = img.load()
                for y, row in enumerate(f['px']):
                    for x, c in enumerate(row):
                        if c:
                            px[x, y + 10 + f['dy']] = tuple(c) + (255,)
                frames.append(img)
        pal, idx = sprite_sheet(frames, 16, 32)
        save_indexed_bmp(os.path.join(gfx, 'person_%s.bmp' % kind), pal, idx, {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
        names.append(kind)
    return names

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
# Full-screen backgrounds (256x256 so Butano can use them as regular BGs; the screen shows 240x160).
BOX_DARK = (40, 56, 96)       # frame colours, after the web game's text box
BOX_MID = (104, 136, 200)
BOX_FILL = (248, 248, 248)

def rounded_box(draw, x0, y0, x1, y1, fill=BOX_FILL):
    draw.rounded_rectangle((x0, y0, x1, y1), radius=4, fill=BOX_DARK)
    draw.rounded_rectangle((x0 + 1, y0 + 1, x1 - 1, y1 - 1), radius=3, fill=BOX_MID)
    draw.rounded_rectangle((x0 + 3, y0 + 3, x1 - 3, y1 - 3), radius=2, fill=fill)

def save_bg(img, gfx, name, bpp='bpp_4'):
    pal, idx = quantize_rgba(img, 15 if bpp == 'bpp_4' else 255)
    save_indexed_bmp(os.path.join(gfx, name + '.bmp'), pal, idx, {'type': 'regular_bg', 'bpp_mode': bpp})

def build_ui(data, gfx):
    # Message box across the bottom (overworld and battle).
    img = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
    rounded_box(ImageDraw.Draw(img), 2, 114, 237, 159)
    save_bg(img, gfx, 'ui_textbox')
    # Small menu box on the right, above the message box (YES/NO, battle commands, start menu).
    img = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
    rounded_box(ImageDraw.Draw(img), 152, 50, 237, 113)
    save_bg(img, gfx, 'ui_menu')
    # Tall menu box on the right (start menu, move list).
    img = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
    rounded_box(ImageDraw.Draw(img), 136, 2, 237, 113)
    save_bg(img, gfx, 'ui_menu_tall')

def build_battle_bg(data, gfx):
    a = data['art']
    img = Image.new('RGBA', (256, 256), (0xd4, 0xf2, 0xc4, 255))
    bg = art_image(a['battle_bg'])
    for x in range(0, 256, bg.size[0]):
        img.alpha_composite(bg, (x, 0))
    def platform(l, m, r, x, y, mids):
        L, M, R = art_image(a[l]), art_image(a[m]), art_image(a[r])
        img.alpha_composite(L, (x, y))
        cx = x + L.size[0]
        for _ in range(mids):
            img.alpha_composite(M, (cx, y))
            cx += M.size[0]
        img.alpha_composite(R, (cx, y))
    platform('plat_enemy_l', 'plat_enemy_m', 'plat_enemy_r', 140, 52, 5)     # 24 + 40 + 24 = 88 wide
    platform('plat_player_l', 'plat_player_m', 'plat_player_r', 4, 86, 4)    # 32 + 32 + 32 = 96 wide
    save_bg(img.copy(), gfx, 'battle_bg_plain', 'bpp_8')     # intro and starter choice
    d = ImageDraw.Draw(img)
    # HP panels: the foe's top left, yours above the message box on the right.
    for (x0, y0, x1, y1) in ((4, 6, 112, 38), (126, 68, 236, 110)):
        d.rounded_rectangle((x0, y0, x1, y1), radius=5, fill=(40, 48, 56))
        d.rounded_rectangle((x0 + 1, y0 + 1, x1 - 1, y1 - 1), radius=4, fill=(248, 248, 216))
    for (x, y) in ((36, 28), (164, 88)):     # HP bar troughs ("HP" label + 48 px bar)
        d.rectangle((x - 1, y - 1, x + 48, y + 5), fill=(64, 72, 80))
        d.rectangle((x, y, x + 48, y + 4), fill=(56, 64, 72))
    save_bg(img, gfx, 'battle_bg', 'bpp_8')

def build_hpbar(gfx):
    # 8x8 bar segments: fill 0-8 px, in green, yellow and red (27 frames).
    frames = []
    for col in ((88, 208, 128), (248, 216, 56), (240, 72, 56)):
        for n in range(9):
            f = Image.new('RGBA', (8, 8), (0, 0, 0, 0))
            d = ImageDraw.Draw(f)
            d.rectangle((0, 0, 7, 4), fill=(56, 64, 72, 255))
            if n:
                d.rectangle((0, 0, n - 1, 4), fill=col + (255,))
                d.rectangle((0, 0, n - 1, 0), fill=tuple(min(255, v + 60) for v in col) + (255,))
            frames.append(f)
    pal, idx = sprite_sheet(frames, 8, 8)
    save_indexed_bmp(os.path.join(gfx, 'hpbar.bmp'), pal, idx, {'type': 'sprite', 'height': 8, 'bpp_mode': 'bpp_4'})

def build_grass_front(data, gfx):
    # The bottom half of a tall-grass tile, drawn over whoever stands in it (the web game's .tg-front).
    img = Image.new('RGBA', (16, 8), (0, 0, 0, 0))
    px = img.load()
    for y in range(8):
        for x in range(16):
            c = data['tall_grass'][y + 8][x]
            if c:
                px[x, y] = tuple(c) + (255,)
    pal, idx = quantize_rgba(img, 15)
    save_indexed_bmp(os.path.join(gfx, 'grass_front.bmp'), pal, idx, {'type': 'sprite', 'height': 8, 'bpp_mode': 'bpp_4'})

def build_title(data, gfx):
    # Title backdrop: a slice of the home town.
    a = data['areas'][0]
    img = Image.frombytes('RGB', (a['w'] * 16, a['h'] * 16), open(os.path.join(EXP, 'map_%d.rgb' % a['index']), 'rb').read())
    crop = img.crop((96, 120, 96 + 256, 120 + 256)).convert('RGBA')
    d = ImageDraw.Draw(crop)
    rounded_box(d, 24, 18, 215, 62)
    save_bg(crop, gfx, 'title_bg', 'bpp_8')

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
        row = []
        for dt in types:
            m = chart.get(at, {}).get(dt, 1)
            row.append(str(int(m * 2)))      # x2 so 0.5 fits an integer
        rows.append('{' + ', '.join(row) + '}')
    L.append('// Damage multiplier x2: [attacking type][defending type].')
    L.append('constexpr uint8_t type_chart[%d][%d] = {\n    %s\n};\n' % (len(types), len(types), ',\n    '.join(rows)))
    cat = {'phys': 'move_category::PHYSICAL', 'spec': 'move_category::SPECIAL', 'status': 'move_category::STATUS'}
    st = {None: 'status::NONE', 'psn': 'status::POISON', 'brn': 'status::BURN', 'par': 'status::PARALYSIS',
          'slp': 'status::SLEEP', 'frz': 'status::FREEZE'}
    mv = []
    for m in data['moves']:
        sec = m.get('sec') or {}
        mv.append('{%s, %d, %d, %s, %d, %s, %s, %d}' % (
            c_string(clean_text(m['n'].upper())), types.index(m['t']), m['p'], cat[m['c']], m['a'],
            st[m.get('status')], st[sec.get('status')], sec.get('chance', 0)))
    L.append('constexpr move moves[] = {\n    ' + ',\n    '.join(mv) + '\n};\n')
    names = [s['name'] for s in data['species']]
    for s in data['species']:
        L.append('constexpr learn_entry learn_%d[] = {%s};' % (s['num'], ', '.join('{%d, %d}' % (lv, mi) for lv, mi in s['learn']) or '{1, 0}'))
    L.append('\nconstexpr species species_list[] = {')
    for s in data['species']:
        b = s['base']
        evo = s['evo']
        evo_to = -1
        if evo:
            tgt = [i for i, n in enumerate(names) if n == evo['to']]
            evo_to = tgt[0] if tgt else -1
        t2 = types.index(s['types'][1]) if len(s['types']) > 1 else -1
        L.append('    {%s, %d, %d, %d, {%d, %d, %d, %d, %d, %d}, learn_%d, %d, %d, %d, bn::sprite_items::mon_front_%d, bn::sprite_items::mon_back_%d},' % (
            c_string(clean_text(s['name'].upper())), s['num'], types.index(s['types'][0]), t2,
            b['hp'], b['atk'], b['def'], b['spa'], b['spd'], b['spe'], s['num'], max(1, len(s['learn'])),
            evo_to, evo['level'] if evo and evo.get('level') else 0, s['num'], s['num']))
    L.append('};\n')
    L.append('constexpr species_id starters[] = {%s};' % ', '.join('species_id::%s' % enum_name(n) for n in data['starters']))
    L.append('constexpr const char* prof_name = %s;\n' % c_string(clean_text(data['prof'])))
    L.append('}\n\n#endif')
    write_if_changed(os.path.join(out_inc, 'pr_game_data.h'), '\n'.join(L))
    # The species enum is needed by both headers, so it lives on its own.
    E = ['// Generated by tools/build_assets.py; do not edit.', '#ifndef PR_SPECIES_ID_H\n#define PR_SPECIES_ID_H\n',
         '#include <cstdint>\n', 'namespace pr\n{\n', 'enum class species_id : uint8_t\n{']
    E += ['    %s,' % enum_name(n) for n in names]
    E += ['};\n', 'constexpr int species_count = %d;' % len(names), 'constexpr int types_count = %d;\n' % len(types)]
    kinds = list(data['people'])
    E += ['enum class person_kind : uint8_t\n{'] + ['    %s,' % k for k in kinds] + ['};\n', '}\n', '#endif']
    write_if_changed(os.path.join(out_inc, 'pr_species_id.h'), '\n'.join(E))
    P = ['// Generated by tools/build_assets.py; do not edit.', '#ifndef PR_PEOPLE_SPRITES_H\n#define PR_PEOPLE_SPRITES_H\n']
    P += ['#include "bn_sprite_items_person_%s.h"' % k for k in kinds]
    P += ['\nnamespace pr\n{\n', 'constexpr const bn::sprite_item* person_sprites[] = {%s};\n' %
          ', '.join('&bn::sprite_items::person_%s' % k for k in kinds), '}\n', '#endif']
    write_if_changed(os.path.join(out_inc, 'pr_people_sprites.h'), '\n'.join(P))

def write_if_changed(path, text):
    if os.path.exists(path) and open(path, encoding='utf8').read() == text:
        return
    with open(path, 'w', encoding='utf8') as f:
        f.write(text)

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
    subprocess.run(['node', os.path.join(HERE, 'export.js'), EXP] + [str(a) for a in AREAS], check=True)
    data = json.load(open(os.path.join(EXP, 'data.json'), encoding='utf8'))
    build_world(EXP, data, inc)
    build_game_data(data, inc)
    build_people(data, gfx)
    build_mons(data, gfx)
    build_ui(data, gfx)
    build_battle_bg(data, gfx)
    build_hpbar(gfx)
    build_grass_front(data, gfx)
    build_title(data, gfx)
    open(stamp, 'w').write(digest)

if __name__ == '__main__':
    main()
