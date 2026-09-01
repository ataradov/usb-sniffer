#!/usr/bin/env bash
set -euo pipefail

# Run as root in WSL. The final executable is placed under the repository's
# ignored tools directory; source trees and intermediate objects stay on WSL's
# faster ext4 filesystem.
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
    build-essential cmake git ninja-build \
    libboost-all-dev libeigen3-dev libffi-dev \
    python3-dev python3-intervaltree

src=/opt/usb-sniffer-oss-src
prefix=/opt/usb-sniffer-oss
repo="${1:?repository path in WSL is required}"
mkdir -p "$src" "$prefix"
cd "$src"

if [[ ! -d prjtrellis/.git ]]; then
    git clone --depth 1 --recursive https://github.com/YosysHQ/prjtrellis.git
fi
cmake -S prjtrellis/libtrellis -B prjtrellis/build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix"
cmake --build prjtrellis/build --parallel "$(nproc)"
cmake --install prjtrellis/build

if [[ ! -d nextpnr/.git ]]; then
    git clone --depth 1 https://github.com/YosysHQ/nextpnr.git
fi
cmake -S nextpnr -B nextpnr/build-machxo2-2000 -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DARCH=machxo2 -DMACHXO2_DEVICES=2000 \
    -DTRELLIS_INSTALL_PREFIX="$prefix" \
    -DBUILD_GUI=OFF -DBUILD_PYTHON=OFF
cmake --build nextpnr/build-machxo2-2000 --parallel "$(nproc)"

dest="$repo/tools/nextpnr-machxo2-2000-wsl"
mkdir -p "$dest"
install -m 755 nextpnr/build-machxo2-2000/nextpnr-machxo2 \
    "$dest/nextpnr-machxo2-2000"
install -m 755 "$prefix/bin/ecppack" "$dest/ecppack"
install -m 755 "$prefix/lib/trellis/libtrellis.so" "$dest/libtrellis.so"
"$dest/nextpnr-machxo2-2000" --list-devices | grep LCMXO2-2000HC-5TG100C
