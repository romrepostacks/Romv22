#!/usr/bin/env python3
"""Play test (3.0.0): builds the game with test_play.inc, then plays scripted scenarios (see test_play.inc's
test_setup) in mGBA, saving a screenshot at each `p NAME` line of a scenario's script as OUT/<scenario>-NAME.png.
    python3 tools/walktest/playtest.py OUT SCRIPT.txt
SCRIPT.txt: `= N` starts scenario N; then harness lines (k KEYS FRAMES, w FRAMES) and `p NAME`."""
import os, shutil, subprocess, sys, tempfile
from PIL import Image
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import walktest

def main():
    out, script = os.path.abspath(sys.argv[1]), sys.argv[2]
    os.makedirs(out, exist_ok=True)
    work = tempfile.mkdtemp(prefix='playtest-')
    rom, harness = walktest.build(work, 'test_play.inc')
    scenarios, cur = [], None
    for line in open(script):
        line = line.strip()
        if line.startswith('='):
            cur = [int(line[1:]), ['w 300', 'k A', 'w 60', 'k A', 'w 200']]
            scenarios.append(cur)
        elif line and cur:
            if line.startswith('p '):
                line = 'p %s' % os.path.join(work, '%d-%s.ppm' % (cur[0], line[2:].strip()))
            cur[1].append(line)
    bad = []
    for n, lines in scenarios:
        sav = os.path.join(work, 's%d.sav' % n)
        data = bytearray(b'\xff' * 0x8000)
        data[0x7E00:0x7E04] = bytes([0, 0, n & 0xff, n >> 8])
        open(sav, 'wb').write(data)
        subprocess.run([harness, rom, sav], input='\n'.join(lines) + '\n', text=True, check=True)
        crashed = open(sav, 'rb').read()[0x7E10:0x7E14] == b'CRSH'
        print(n, 'CRASH' if crashed else 'ok', flush=True)
        bad += [n] * crashed
    for f in sorted(os.listdir(work)):
        if f.endswith('.ppm'):
            Image.open(os.path.join(work, f)).resize((480, 320), Image.NEAREST).save(os.path.join(out, f[:-4] + '.png'))
    shutil.rmtree(work)
    print('crashed:', bad or 'none')

if __name__ == '__main__':
    main()
