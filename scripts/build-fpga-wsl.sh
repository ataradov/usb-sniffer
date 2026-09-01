#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"
diamond_root="${1:-$repo_root/tools/lattice-diamond-3.14/install}"

export LSC_DIAMOND=true
export FOUNDRY="$diamond_root/ispfpga"
export TCL_LIBRARY="$diamond_root/tcltk/lib/tcl8.5"
export fpgabindir="$diamond_root/ispfpga/bin/lin64"
export PATH="$diamond_root/bin/lin64:$PATH"
export LD_LIBRARY_PATH="$diamond_root/bin/lin64:$diamond_root/ispfpga/bin/lin64"

if [[ -f "$diamond_root/license/license.dat" ]]; then
  export LATTICE_LICENSE_FILE="$diamond_root/license/license.dat"
fi

cd "$repo_root/fpga"
"$diamond_root/bin/lin64/pnmainc" build.tcl

jed_path="$repo_root/fpga/impl/usb_sniffer_impl.jed"
if [[ ! -f "$jed_path" ]]; then
  echo "Diamond completed without producing $jed_path" >&2
  exit 1
fi

cp -f "$jed_path" "$repo_root/bin/usb_sniffer_impl.jed"
sha256sum "$repo_root/bin/usb_sniffer_impl.jed"
