"""Draws the Sundered forms' sprites once (3.0.0): each is its original's front, back and shiny sprites from
sprites/pokemon/, recoloured toward the form's new type (SUNDERED_FORMS in js/app.js), saved under the form's
own number. Greys and outlines keep their colour; everything else is pulled to the form's hue.
Run by hand when the forms change; build_assets.py only reads the PNGs."""
import colorsys, os
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SPRITES = os.path.join(HERE, '..', '..', '..', 'sprites', 'pokemon')

# original's number -> (form's number, target hue in degrees, how far toward it 0..1, saturation scale)
FORMS = {
    27: (2001, 200, 0.75, 1.1),     # Sandshrew: sea-washed sand, blue-grey
    28: (2002, 200, 0.75, 1.1),
    322: (2003, 180, 0.8, 1.0),     # Numel: steam, teal
    323: (2004, 180, 0.8, 1.0),
    320: (2005, 30, 0.7, 0.9),      # Wailmer: island whale, sandy brown
    321: (2006, 30, 0.7, 0.9),
    341: (2007, 215, 0.85, 0.5),    # Corphish: ironclad, steel blue
    342: (2008, 215, 0.85, 0.5),
    318: (2009, 270, 0.8, 0.9),     # Carvanha: ghostly, violet
    319: (2010, 270, 0.8, 0.9),
    302: (2011, 45, 0.8, 1.2),      # Sableye: pirate gold
    324: (2012, 175, 0.8, 1.0),     # Torkoal: steam, teal
}


def recolour(src, dst, hue, pull, sat):
    im = Image.open(src).convert('RGBA')
    px = im.load()
    h_to = hue / 360
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
            if s < 0.18 or v < 0.15:
                continue                    # greys, whites and outlines stay
            d = (h_to - h + 0.5) % 1 - 0.5  # the short way round
            h = (h + d * pull) % 1
            r, g, b = colorsys.hsv_to_rgb(h, min(1, s * sat), v)
            px[x, y] = (int(r * 255), int(g * 255), int(b * 255), a)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    im.save(dst)


for orig, (num, hue, pull, sat) in FORMS.items():
    for sub in ('', 'back/', 'shiny/', 'back/shiny/'):
        src = os.path.join(SPRITES, sub + '%d.png' % orig)
        if os.path.exists(src):
            # Shinies take the form's hue from the other side of the wheel, so they still stand out.
            recolour(src, os.path.join(SPRITES, sub + '%d.png' % num), hue + (150 if 'shiny' in sub else 0), pull, sat)
