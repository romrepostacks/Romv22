#!/usr/bin/env python3
"""Timing (3.1.0): builds the game twice with test_play.inc, with the Pokémon sprites stored plain and
LZ77-compressed (PR_MON_COMPRESSION), and compares them in mGBA:
- for each scripted moment (boot, a battle starting, the party screen, sailing, walking into the next map), how
  many frames later the compressed build shows each new screen than the plain one;
- the CPU time to make every Pokémon's front, back and icon sprite (test_play.inc scenario 99);
- the ROM sizes.
    python3 tools/walktest/timing.py OUT.md
"""
import os, shutil, struct, subprocess, sys, tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import walktest

OPEN = ['w 300', 'k A', 'w 60', 'k A', 'w 200']     # title, CONTINUE
# (name, scenario, script): `h N` lines hash N frames.
MOMENTS = [
    ('Boot to the title screen', 4, ['h 400']),
    ('Continue to the map (gym)', 4, ['w 300', 'k A 1', 'h 200']),
    ('Gym leader battle starting', 4, OPEN + ['k A 1', 'h 700']),
    ('Party screen opening', 4, OPEN + ['k S 1', 'w 30', 'k A 1', 'h 120']),
    ('Pokémon summary opening', 4, OPEN + ['k S 1', 'w 30', 'k A 1', 'w 60', 'k A 1', 'w 20', 'k A 1', 'h 120']),
    ('Sailing to Port Keel (map load)', 2, OPEN + ['k A 1', 'w 90', 'k A 1', 'w 30', 'k A 1', 'h 400']),
    ('Walking up into Victory Road (map change)', 10, OPEN + ['K U', 'h 160', 'K -']),
]

def sav_for(work, scen):
    sav = os.path.join(work, 's.sav')
    data = bytearray(b'\xff' * 0x8000)
    data[0x7E02:0x7E04] = struct.pack('<h', scen)
    open(sav, 'wb').write(data)
    return sav

def run(rom, harness, work, scen, lines):
    sav = sav_for(work, scen)
    hfile = os.path.join(work, 'h.txt')
    if os.path.exists(hfile):
        os.remove(hfile)
    lines = [l + ' ' + hfile if l.startswith('h ') else l for l in lines]
    subprocess.run([harness, rom, sav], input='\n'.join(lines) + '\n', text=True, check=True)
    return (open(hfile).read().split() if os.path.exists(hfile) else []), open(sav, 'rb').read()

def lag(a, b):
    """Each screen's first frame in b minus its first frame in a (screens both show)."""
    fa, fb = {}, {}
    for i, h in enumerate(a):
        fa.setdefault(h, i)
    for i, h in enumerate(b):
        fb.setdefault(h, i)
    common = sorted((fa[h], fb[h] - fa[h]) for h in fa if h in fb)
    return [d for _, d in common], len(common)

def main():
    out = sys.argv[1]
    builds = {}
    for comp in ('none', 'lz77'):
        if len(sys.argv) > 2:       # reuse earlier builds: timing.py OUT.md NONE_DIR LZ77_DIR
            work = sys.argv[2 if comp == 'none' else 3]
            builds[comp] = (os.path.join(work, 'gba', 'party-royale.gba'), os.path.join(work, 'harness'), work)
            continue
        work = tempfile.mkdtemp(prefix='timing-%s-' % comp)
        os.environ['PR_MON_COMPRESSION'] = comp
        rom, harness = walktest.build(work, 'test_play.inc')
        builds[comp] = (rom, harness, work)
    rows = []
    for name, scen, lines in MOMENTS:
        seqs = {c: run(b[0], b[1], b[2], scen, lines)[0] for c, b in builds.items()}
        d, n = lag(seqs['none'], seqs['lz77'])
        rows.append((name, n, max(d) if d else 0, sum(1 for x in d if x)))
    micro = {}
    for c, b in builds.items():
        _, sav = run(b[0], b[1], b[2], 99, ['w 400'])
        micro[c] = struct.unpack('<3i', sav[0x7E20:0x7E2C])
    sizes = {c: os.path.getsize(b[0]) for c, b in builds.items()}
    L = ['| Moment | Screens compared | Most frames later with compression | Screens that came later |', '|---|---|---|---|']
    L += ['| %s | %d | %d | %d |' % r for r in rows]
    L += ['', '| Make every Pokémon sprite (%d species) | Plain | LZ77 |' % 1037, '|---|---|---|']
    for i, k in enumerate(('front', 'back', 'icon')):
        a, b = micro['none'][i] * 64 / 16.78e6 * 1000, micro['lz77'][i] * 64 / 16.78e6 * 1000
        L.append('| %s, all | %.0f ms (%.3f ms each) | %.0f ms (%.3f ms each) |' % (k, a, a / 1037, b, b / 1037))
    L += ['', 'ROM: plain %.2f MB, LZ77 %.2f MB' % (sizes['none'] / 1e6, sizes['lz77'] / 1e6)]
    open(out, 'w').write('\n'.join(L) + '\n')
    print('\n'.join(L))
    for b in builds.values():
        shutil.rmtree(b[2])

if __name__ == '__main__':
    main()
