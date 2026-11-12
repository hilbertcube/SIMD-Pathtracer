#!/usr/bin/env bash
# Profile a scene with perf and draw a flame graph of where the render spends its time.
#
#   tools/profile.sh [scene] [--spp N] [other options for main]
#
# Builds an optimized binary with frame pointers and debug symbols in build-profile/, renders the
# scene under `perf record`, and writes profile/scene_<N>.svg (open it in a browser: click a frame
# to zoom). Defaults to scene 9 at 32 samples per pixel. Needs Linux perf, and Brendan Gregg's
# FlameGraph scripts in third_party/FlameGraph (tools/fetch_flamegraph.sh downloads them).
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

scene="${1:-9}"
shift || true
args=("$@")
if [[ ! " ${args[*]-} " =~ " --spp " ]]; then
    args+=(--spp 32)
fi

if ! command -v perf >/dev/null; then
    echo "perf not found: install it (e.g. linux-tools-\$(uname -r) or the perf package)" >&2
    exit 1
fi
flamegraph="third_party/FlameGraph"
if [[ ! -x "$flamegraph/flamegraph.pl" ]]; then
    echo "FlameGraph not found in $flamegraph: run tools/fetch_flamegraph.sh first" >&2
    exit 1
fi
if [[ "$(cat /proc/sys/kernel/perf_event_paranoid)" -gt 2 ]]; then
    echo "perf_event_paranoid is above 2, so perf can't record. Lower it with:" >&2
    echo "  sudo sysctl kernel.perf_event_paranoid=2" >&2
    exit 1
fi

cmake -S . -B build-profile -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-g -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer" >/dev/null
cmake --build build-profile -j"$(nproc)" --target main

mkdir -p profile
data="profile/scene_${scene}.perf.data"
svg="profile/scene_${scene}.svg"

perf record -F 499 -g -o "$data" ./build-profile/main "$scene" profile "${args[@]}"

samples="$(perf script -i "$data" -F comm 2>/dev/null | wc -l)"
perf script --no-inline -i "$data" 2>/dev/null | "$flamegraph/stackcollapse-perf.pl" |
    "$flamegraph/flamegraph.pl" --title "Scene ${scene} (${args[*]}), ${samples} samples" \
        --width 1600 >"$svg"

echo
echo "Top functions by self time:"
perf report -i "$data" --no-children --stdio --sort symbol 2>/dev/null |
    grep -E '^ +[0-9]' | head -10 | cut -c1-120
echo
echo "Flame graph: $svg"
