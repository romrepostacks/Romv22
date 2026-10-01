#!/bin/sh
# Installs what `make` needs on Linux x86_64: the Wonderful GBA toolchain (GCC for ARM), BlocksDS's
# grit/mmutil (Butano's asset tools) and Pillow. Node.js and Python 3 must already be installed.
# Afterwards: export WONDERFUL_TOOLCHAIN=/opt/wonderful PATH=/opt/wonderful/bin:$PATH
set -e
if [ ! -x /opt/wonderful/bin/wf-pacman ]; then
  mkdir -p /opt/wonderful
  curl -sSL https://wonderful.asie.pl/bootstrap/wf-bootstrap-x86_64.tar.gz | tar xz -C /opt/wonderful
fi
export PATH=/opt/wonderful/bin:$PATH
wf-pacman -Syu --noconfirm
wf-pacman -S --noconfirm --needed target-gba toolchain-gcc-arm-none-eabi-libstdcxx-picolibc
wf-config repo enable blocksds
wf-pacman -Syu --noconfirm
wf-pacman -S --noconfirm --needed blocksds-toolchain
python3 -m pip install --quiet pillow
echo "Toolchain ready. Run: export WONDERFUL_TOOLCHAIN=/opt/wonderful PATH=/opt/wonderful/bin:\$PATH"
