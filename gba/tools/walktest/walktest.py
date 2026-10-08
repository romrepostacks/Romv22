#!/usr/bin/env python3
"""Walk test: drops a level-100 party at every map's spawn point and walks it around at random (pressing A
now and then, so battles and text go on), in mGBA, watching for a Butano assert (the palette crashes of 1.6
were one). Prints each map and the ones that crashed; the last frame of each is left in OUT/m<map>.ppm.

Needs what `make` needs, plus mGBA's library (apt-get install libmgba-dev).
    python3 tools/walktest/walktest.py [OUT] [--frames 8000] [--maps 0-128] [--seed 0]
"""
import argparse, os, random, shutil, struct, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
GBA = os.path.dirname(os.path.dirname(HERE))

def build(work):
    # A copy of gba/ whose main() runs test_setup() from test_walk.inc (js/ and sprites/ next to it).
    gba = os.path.join(work, 'gba')
    shutil.copytree(GBA, gba, symlinks=True, ignore=shutil.ignore_patterns('build', 'roms', 'butano', '*.gba', '*.elf'))
    os.symlink(os.path.join(GBA, 'butano'), os.path.join(gba, 'butano'))
    for d in ('js', 'sprites'):
        os.symlink(os.path.join(os.path.dirname(GBA), d), os.path.join(work, d))
    main = os.path.join(gba, 'src', 'main.cpp')
    s = open(main).read()
    s = s.replace('int main()', '#include "%s"\nint main()' % os.path.join(HERE, 'test_walk.inc'), 1)
    s = s.replace('pr::ui::set_faded(true);', 'pr::ui::set_faded(true);\n    test_setup();', 1)
    open(main, 'w').write(s)
    subprocess.run(['make', '-j8'], cwd=gba, check=True, stdout=subprocess.DEVNULL)
    harness = os.path.join(work, 'harness')
    subprocess.run(['gcc', '-O2', os.path.join(HERE, 'harness.c'), '-o', harness, '-lmgba'], check=True)
    return os.path.join(gba, 'party-royale.gba'), harness

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out', nargs='?', default='walktest-out')
    ap.add_argument('--frames', type=int, default=8000)
    ap.add_argument('--maps', default='')
    ap.add_argument('--seed', type=int, default=0)
    a = ap.parse_args()
    out = os.path.abspath(a.out)
    os.makedirs(out, exist_ok=True)
    work = tempfile.mkdtemp(prefix='walktest-')
    rom, harness = build(work)
    base = os.path.join(out, 'base.sav')
    if os.path.exists(base):
        os.remove(base)
    subprocess.run([harness, rom, base], input='w 200\n', text=True, check=True)     # writes the test save
    maps_count = int(next(l for l in open(os.path.join(work, 'gba', 'generated', 'include', 'pr_world_data.h'))
                          if 'constexpr int maps_count' in l).split('=')[1].strip(' ;\n'))
    lo, hi = map(int, a.maps.split('-')) if a.maps else (0, maps_count)
    bad = []
    for m in range(lo, hi):
        rnd = random.Random(m + a.seed)
        sav = os.path.join(out, 'm%d.sav' % m)
        data = bytearray(open(base, 'rb').read())
        data[0x7E00:0x7E02] = struct.pack('<h', m + 1)      # test_setup's request: go to map m
        open(sav, 'wb').write(data)
        lines = ['w 300', 'k A', 'w 60', 'k A', 'w 200']   # title, CONTINUE
        t = 0
        while t < a.frames:
            r = rnd.random()
            if r < 0.7:
                n = rnd.randint(6, 30)
                lines.append('k %s %d' % (rnd.choice('UDLR'), n))
                t += n + 4
            else:
                lines.append('k %s 4' % ('A' if r < 0.95 else 'B'))
                t += 8
        lines.append('p %s' % os.path.join(out, 'm%d.ppm' % m))
        subprocess.run([harness, rom, sav], input='\n'.join(lines) + '\n', text=True, check=True)
        crashed = open(sav, 'rb').read()[0x7E10:0x7E14] == b'CRSH'
        print(m, 'CRASH' if crashed else 'ok', flush=True)
        if crashed:
            bad.append(m)
        os.remove(sav)
    shutil.rmtree(work)
    print('crashed:', bad or 'none')
    sys.exit(1 if bad else 0)

if __name__ == '__main__':
    main()
