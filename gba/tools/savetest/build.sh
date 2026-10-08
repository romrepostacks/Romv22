#!/bin/bash
# build.sh SRC_GBA_DIR INC OUTDIR : test build of a gba/ tree with test_setup() from INC called in main
set -e
src=$1; inc=$2; out=$3
rm -rf $out; mkdir -p $out
cp -r $src $out/gba; rm -rf $out/gba/build $out/gba/butano; ln -s $src/butano $out/gba/butano
ln -s $(dirname $src)/js $out/js; ln -s $(dirname $src)/sprites $out/sprites
python3 - $out/gba/src/main.cpp $inc <<'PY'
import sys
p,inc=sys.argv[1],sys.argv[2]; s=open(p).read()
s=s.replace('int main()','#include "%s"\nint main()'%inc,1).replace('pr::ui::set_faded(true);','pr::ui::set_faded(true);\n    test_setup();',1)
open(p,'w').write(s)
PY
export DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM PATH=/opt/devkitpro/devkitARM/bin:/opt/devkitpro/tools/bin:$PATH
make -C $out/gba -j8 > $out/build.log 2>&1 || { grep -E "error" $out/build.log | head; exit 1; }
