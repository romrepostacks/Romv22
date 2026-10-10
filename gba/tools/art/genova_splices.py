"""Draws GENOVA's experiments' sprites once (5.0.0): each is the top of one Pokémon spliced onto the bottom of
another (front, back, shiny and back shiny, from sprites/pokemon/), recoloured toward a lab hue, saved under the
experiment's number (GENOVA_FORMS in js/app.js). Run by hand when they change; build_assets.py only reads the PNGs."""
import colorsys, os
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SPRITES = os.path.join(HERE, '..', '..', '..', 'sprites', 'pokemon')

# number -> (top's number, bottom's number, where the top ends 0..1 of its height, hue, pull 0..1)
SPLICES = {
    2021: (19, 23, 0.55, 290, 0.5),     # Chimurr: a RATTATA's head on an EKANS' coils, poison violet
    2022: (20, 24, 0.5, 290, 0.5),      # Chimaul: RATICATE on ARBOK
    2023: (170, 60, 0.5, 55, 0.45),     # Voltadpole: CHINCHOU's lures on a POLIWAG
    2024: (171, 61, 0.5, 55, 0.45),     # Voltoad: LANTURN on a POLIWHIRL
    2025: (597, 43, 0.6, 140, 0.4),     # Graftling: a FERROSEED grafted onto an ODDISH
    2026: (598, 44, 0.55, 140, 0.4),    # Graftree: FERROTHORN on a GLOOM
    2027: (104, 353, 0.5, 265, 0.6),    # Specterra: a CUBONE's skull over a SHUPPET
    2028: (137, 132, 0.55, 175, 0.5),   # Mimicore: PORYGON on a DITTO
    2029: (151, 148, 0.5, 320, 0.35),   # Helixeon: the lab's try at a MEW, on a DRAGONAIR's body
}


def bbox(im):
    return im.getchannel('A').getbbox() or (0, 0, im.width, im.height)


def splice(top_path, bottom_path, cut):
    a, b = Image.open(top_path).convert('RGBA'), Image.open(bottom_path).convert('RGBA')
    ax0, ay0, ax1, ay1 = bbox(a)
    bx0, by0, bx1, by1 = bbox(b)
    ac = ay0 + int((ay1 - ay0) * cut)           # the top's cut row
    bc = by0 + int((by1 - by0) * cut)           # ...and the bottom's
    out = Image.new('RGBA', a.size)
    # The bottom half first, raised so its cut meets the top's, centred under it.
    shift_x = (ax0 + ax1) // 2 - (bx0 + bx1) // 2
    for y in range(bc, b.height):
        for x in range(b.width):
            px = b.getpixel((x, y))
            ty, tx = y - bc + ac, x + shift_x
            if px[3] and 0 <= tx < out.width and 0 <= ty < out.height:
                out.putpixel((tx, ty), px)
    for y in range(0, ac + 1):
        for x in range(a.width):
            px = a.getpixel((x, y))
            if px[3]:
                out.putpixel((x, y), px)
    # Then it stands where the bottom one stood (or as low as fits).
    ox0, oy0, ox1, oy1 = bbox(out)
    moved = Image.new('RGBA', out.size)
    moved.alpha_composite(out.crop((0, oy0, out.width, oy1)), (0, max(0, min(by1, out.height) - (oy1 - oy0))))
    return moved


def recolour(im, hue, pull):
    px = im.load()
    h_to = hue / 360
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = px[x, y]
            if not a:
                continue
            h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
            if s < 0.18 or v < 0.15:
                continue
            d = (h_to - h + 0.5) % 1 - 0.5
            r, g, b = colorsys.hsv_to_rgb((h + d * pull) % 1, s, v)
            px[x, y] = (int(r * 255), int(g * 255), int(b * 255), a)
    return im


for num, (top, bottom, cut, hue, pull) in SPLICES.items():
    for sub in ('', 'back/', 'shiny/', 'back/shiny/'):
        t, b = os.path.join(SPRITES, sub + '%d.png' % top), os.path.join(SPRITES, sub + '%d.png' % bottom)
        if os.path.exists(t) and os.path.exists(b):
            im = recolour(splice(t, b, cut), hue + (150 if 'shiny' in sub else 0), pull)
            im.save(os.path.join(SPRITES, sub + '%d.png' % num))
