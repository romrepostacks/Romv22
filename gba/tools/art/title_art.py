"""Draws the title screen art once into gba/data/: the dusk sky with the three beasts on the ridge (title_sky.png),
the logo (title_logo.png), a twinkle (title_spark.png, 4 frames of 16x16) and HO-OH's silhouette (title_hooh.png).
Run by hand when the art changes (it needs the Caladea font); build_assets.py only reads the PNGs."""
import math, os, random, sys
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', '..', '..')
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', '..', 'data')
FONT = '/usr/share/fonts/truetype/crosextra/Caladea-Bold.ttf'
W, H = 240, 160            # the screen; the 256x256 backgrounds hold it at (8, 48)
NIGHT = (24, 10, 36)

def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))

def ramp(stops, t):
    for (t0, c0), (t1, c1) in zip(stops, stops[1:]):
        if t <= t1:
            return lerp(c0, c1, (t - t0) / (t1 - t0))
    return stops[-1][1]

def silhouette(num, size, col):
    im = Image.open(os.path.join(ROOT, 'sprites', 'pokemon', '%d.png' % num)).convert('RGBA')
    im = im.crop(im.getbbox())
    s = size / max(im.size)
    im = im.resize((max(1, int(im.size[0] * s)), max(1, int(im.size[1] * s))), Image.NEAREST)
    out = Image.new('RGBA', im.size, (0, 0, 0, 0))
    a, o = im.load(), out.load()
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            if a[x, y][3] > 128:
                o[x, y] = col + (255,)
    return out

def on_bg(img):
    bg = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
    bg.alpha_composite(img, (8, 48))
    return bg

def sky():
    img = Image.new('RGBA', (W, H))
    px = img.load()
    # Dusk in 4-px bands: deep violet overhead, magenta, a burning orange horizon.
    stops = [(0, (24, 16, 56)), (0.35, (88, 32, 104)), (0.62, (200, 72, 96)), (0.8, (248, 152, 72)), (1, (255, 216, 120))]
    for y in range(H):
        c = ramp(stops, min(1.0, (y // 4 * 4) / 128.0)) + (255,)
        for x in range(W):
            px[x, y] = c
    rnd = random.Random(9)
    for _ in range(50):
        x, y = rnd.randrange(W), rnd.randrange(52)
        px[x, y] = (200, 184, 255, 255) if rnd.random() < 0.5 else (136, 120, 200, 255)
    d = ImageDraw.Draw(img)
    for r, c in ((40, (255, 176, 88)), (30, (255, 200, 104)), (22, (255, 232, 160)), (16, (255, 248, 216))):
        d.ellipse((120 - r, 112 - r, 120 + r, 112 + r), fill=c)
    def ridge(base, amp, f, phase, col):
        pts = [(0, H)]
        for x in range(0, W + 1, 2):
            a = 2 * math.pi * x / W
            y = base - amp * (0.6 * math.sin(f * a + phase) + 0.3 * math.sin(2 * f * a + phase * 2) + 0.15 * abs(math.sin(5 * f * a)))
            pts.append((x, int(y)))
        pts.append((W, H))
        d.polygon(pts, fill=col)
    ridge(126, 22, 2, 0.5, (120, 48, 96))
    ridge(138, 16, 3, 2.1, (72, 32, 80))
    # The front ridge: three crags the beasts stand on (Raikou left, Suicune centre, Entei right).
    crags = ((36, 132, 243), (120, 150, 245), (204, 132, 244))
    pts = [(0, H), (0, 146)]
    for cx, top, _ in crags:
        pts += [(cx - 26, 148), (cx - 12, top), (cx + 12, top), (cx + 26, 148)]
    pts += [(W, 146), (W, H)]
    d.polygon(pts, fill=NIGHT)
    for cx, top, num in crags:
        s = silhouette(num, 34, NIGHT)
        img.alpha_composite(s, (cx - s.size[0] // 2, top - s.size[1] + 2))
    return on_bg(img)

def logo():
    big, small = ImageFont.truetype(FONT, 32), ImageFont.truetype(FONT, 14)
    mask = Image.new('L', (W, 60))
    d = ImageDraw.Draw(mask)
    d.fontmode = '1'      # no anti-aliasing: crisp pixels
    for text, font, y in (('POKé LEGENDS', small, 2), ('LANDS OF NINE', big, 20)):
        d.text(((W - d.textlength(text, font=font)) // 2, y), text, font=font, fill=255)
    m = mask.load()
    on = lambda x, y: 0 <= x < W and 0 <= y < 60 and m[x, y]
    img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    p = img.load()
    gold = [(0, (255, 248, 200)), (0.35, (255, 216, 96)), (0.55, (240, 160, 40)), (0.8, (200, 96, 24)), (1, (152, 56, 16))]
    for y in range(60):
        for x in range(W):
            if on(x, y):
                t = (y - 4) / 14.0 if y < 19 else (y - 22) / 28.0
                p[x, y + 14] = ramp(gold, max(0.0, min(1.0, t))) + (255,)
            elif any(on(x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1)):
                p[x, y + 14] = (56, 16, 32, 255)
            elif on(x - 2, y - 2) or on(x - 1, y - 2) or on(x - 2, y - 1):
                p[x, y + 14] = NIGHT + (255,)
    return on_bg(img)

def spark():
    # A four-point twinkle growing and fading: 4 frames, 16x16 each, stacked.
    sheet = Image.new('RGBA', (16, 64), (0, 0, 0, 0))
    d = ImageDraw.Draw(sheet)
    for f, r in enumerate((2, 4, 7, 4)):
        cx, cy = 8, f * 16 + 8
        d.line((cx - r, cy, cx + r, cy), fill=(255, 240, 160, 255))
        d.line((cx, cy - r, cx, cy + r), fill=(255, 240, 160, 255))
        if r > 3:
            d.line((cx - 1, cy - 1, cx + 1, cy + 1), fill=(255, 255, 255, 255))
            d.line((cx - 1, cy + 1, cx + 1, cy - 1), fill=(255, 255, 255, 255))
        sheet.putpixel((cx, cy), (255, 255, 255, 255))
    return sheet

def hooh():
    out = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    s = silhouette(250, 56, (56, 20, 48))
    out.alpha_composite(s, ((64 - s.size[0]) // 2, (64 - s.size[1]) // 2))
    return out

if __name__ == '__main__':
    os.makedirs(OUT, exist_ok=True)
    sky().save(os.path.join(OUT, 'title_sky.png'))
    logo().save(os.path.join(OUT, 'title_logo.png'))
    spark().save(os.path.join(OUT, 'title_spark.png'))
    hooh().save(os.path.join(OUT, 'title_hooh.png'))
