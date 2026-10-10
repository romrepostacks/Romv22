"""Draws a region's map for its plan (5.0.0 on): docs/<version>/<name>-map.svg from the built game's areas (their
region map grid, kinds and links, gba/generated/export/data.json), and a PNG of it with headless Chromium.
    python3 gba/tools/art/region_map.py REGION OUT.svg "TITLE" [SUBTITLE] [--png OUT.png]"""
import html, json, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, '..', '..', 'generated', 'export', 'data.json')
CHROME = '/opt/pw-browsers/chromium-1194/chrome-linux/chrome'
THEMES = {5: ('#dfe4ea', '#38b0a0'), 6: ('#efe6cf', '#a0743a'), 7: ('#d8d4e0', '#6a5a90'), 8: ('#e3f0d8', '#4f8f3a'),
          9: ('#f6ecd2', '#c8a040')}
FILL = {'town': '#f5d36b', 'gym': '#e8674f', 'route': '#ffffff', 'trainer': '#7a5bc4'}


def main():
    a = sys.argv[1:]
    png = a[a.index('--png') + 1] if '--png' in a else None
    a = [x for i, x in enumerate(a) if x != '--png' and (i == 0 or a[i - 1] != '--png')]
    region, out, title = int(a[0]), a[1], a[2]
    sub = a[3] if len(a) > 3 else ''
    areas = [x for x in json.load(open(DATA, encoding='utf8'))['areas'] if x['region'] == region and x['at'][0] < 100]
    bg, ink = THEMES.get(region, ('#eeeeee', '#555555'))
    cw, ch, ox, oy = 150, 110, 70, 130
    # Twins (another era, the reverse side; they come after their originals) get a second panel under the first.
    side = lambda x: 0 <= x['twin'] < x['index']
    two = any(side(x) for x in areas)
    xs, ys = [x['at'][0] for x in areas], [x['at'][1] for x in areas]
    x0, y0 = min(xs), min(ys)
    panel = (max(ys) - y0 + 1) * ch + 50
    W = max(1120, ox * 2 + (max(xs) - x0) * cw + 120)
    H = oy + (max(ys) - y0) * ch + 170 + (panel if two else 0)
    pos = {x['index']: (ox + (x['at'][0] - x0) * cw + 60, oy + (x['at'][1] - y0) * ch + 30 + (panel if side(x) else 0)) for x in areas}
    s = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d" font-family="DejaVu Sans, sans-serif">' % (W, H, W, H),
         '<rect width="100%%" height="100%%" fill="%s"/>' % bg,
         '<text x="%d" y="50" font-size="30" font-weight="bold" fill="%s">%s</text>' % (ox - 30, ink, html.escape(title)),
         '<text x="%d" y="82" font-size="16" fill="#444">%s</text>' % (ox - 30, html.escape(sub))]
    if two:
        s.append('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="%s" stroke-width="2" stroke-dasharray="3 6"/>' % (ox - 30, oy + panel - 40, W - 40, oy + panel - 40, ink))
    done = set()
    for x in areas:
        for l in x['links']:
            if l['to'] in pos and (l['to'], x['index']) not in done:
                done.add((x['index'], l['to']))
                (ax, ay), (bx, by) = pos[x['index']], pos[l['to']]
                dash = ' stroke-dasharray="7 5"' if l['badges'] or l['need'] or l['gate'] else ''
                s.append('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="%s" stroke-width="5"%s/>' % (ax, ay, bx, by, ink, dash))
    for x in areas:
        px, py = pos[x['index']]
        kind = 'town' if x['type'] == 'town' else x['type']
        r = 20 if kind in ('town', 'gym') else 13
        shape = '<rect x="%d" y="%d" width="%d" height="%d" rx="5"' % (px - r, py - r, 2 * r, 2 * r) if kind in ('town', 'gym') \
            else '<circle cx="%d" cy="%d" r="%d"' % (px, py, r)
        stroke = '#c03030' if x['legend'] else '#222'
        s.append('%s fill="%s" stroke="%s" stroke-width="%d"/>' % (shape, FILL.get(kind, '#fff'), stroke, 4 if x['legend'] else 2))
        label = x['name'] + (' (' + x['legend']['name'] + ')' if x['legend'] else '')
        s.append('<text x="%d" y="%d" font-size="13" text-anchor="middle" fill="#222">%s</text>' % (px, py + r + 16, html.escape(label)))
    ly = H - 50
    for i, (k, name) in enumerate((('town', 'town / League'), ('gym', 'gym town'), ('route', 'route'), ('trainer', 'story place'))):
        s.append('<rect x="%d" y="%d" width="18" height="18" fill="%s" stroke="#222"/>' % (ox - 30 + i * 170, ly - 14, FILL[k]))
        s.append('<text x="%d" y="%d" font-size="14" fill="#333">%s</text>' % (ox - 4 + i * 170, ly, name))
    s.append('<text x="%d" y="%d" font-size="14" fill="#333">dashed: needs badges or the story · red ring: a legendary%s</text>' % (
        ox - 30, ly + 26, ' · below the dotted line: the other side (reached through rifts)' if two else ''))
    s.append('</svg>')
    open(out, 'w').write('\n'.join(s))
    if png:
        subprocess.run([CHROME, '--headless', '--no-sandbox', '--disable-gpu', '--hide-scrollbars', '--screenshot=' + os.path.abspath(png),
                        '--window-size=%d,%d' % (W, H + 200), 'file://' + os.path.abspath(out)], check=True, capture_output=True)
        from PIL import Image
        Image.open(png).crop((0, 0, W, H)).save(png)


if __name__ == '__main__':
    main()
