#!/bin/sh
# Save converter test: builds the game at an older git ref (default 1.9.1 = save version 10) with old_setup.inc,
# which writes a save full of known values and its digest, then this tree with new_setup.inc, which loads
# (converts) that save, digests it, saves it as the current version, reloads and digests again. All three
# digests must match. Needs the toolchain (README) and libmgba-dev. For a version 9 (1.8.x) save use
# ITEMS=111 REF=29aa376.
#   sh tools/savetest/run.sh [WORKDIR]
set -e
HERE=$(cd "$(dirname "$0")" && pwd); GBA=$(dirname "$(dirname "$HERE")")
W=${1:-/tmp/savetest}; REF=${REF:-0d26ba3}; ITEMS=${ITEMS:-116}
rm -rf "$W"; mkdir -p "$W"
git -C "$GBA" worktree add -f "$W/old" "$REF" >/dev/null
git -C "$W/old" submodule update --init gba/butano >/dev/null
printf '#define ITEMS %s\n#include "%s/old_setup.inc"\n' "$ITEMS" "$HERE" > "$W/old.inc"
printf '#define ITEMS %s\n#include "%s/new_setup.inc"\n' "$ITEMS" "$HERE" > "$W/new.inc"
bash "$HERE/build.sh" "$W/old/gba" "$W/old.inc" "$W/o"
bash "$HERE/build.sh" "$GBA" "$W/new.inc" "$W/n"
gcc -O2 "$GBA/tools/walktest/harness.c" -o "$W/harness" -lmgba
echo "w 120" | "$W/harness" "$W/o/gba/party-royale.gba" "$W/t.sav"
echo "w 120" | "$W/harness" "$W/n/gba/party-royale.gba" "$W/t.sav"
git -C "$GBA" worktree remove --force "$W/old"
python3 - "$W/t.sav" <<'PY'
import struct, sys
d = open(sys.argv[1], 'rb').read()
a, b, c = struct.unpack('<III', d[0x7E20:0x7E2C])
print('version', struct.unpack('<i', d[8:12])[0], 'digests', hex(a), hex(b), hex(c))
sys.exit(0 if a == b == c and a > 2 else 1)
PY
echo PASS
