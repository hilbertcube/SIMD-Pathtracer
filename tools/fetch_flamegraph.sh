#!/usr/bin/env bash
# Download Brendan Gregg's FlameGraph scripts (https://github.com/brendangregg/FlameGraph) into
# third_party/FlameGraph, pinned to a known commit. Run it again to update to that commit.
#
#   tools/fetch_flamegraph.sh
#
# Then, from a perf recording (tools/profile.sh makes one in profile/):
#   perf script -i profile/scene_9.perf.data | third_party/FlameGraph/stackcollapse-perf.pl \
#       | third_party/FlameGraph/flamegraph.pl > profile/scene_9_gregg.svg
set -euo pipefail

repo="https://github.com/brendangregg/FlameGraph.git"
commit="41fee1f99f9276008b7cd112fca19dc3ea84ac32" # 2024-10-21

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dest="$root/third_party/FlameGraph"

if [[ ! -d "$dest/.git" ]]; then
    git clone --quiet "$repo" "$dest"
fi
git -C "$dest" fetch --quiet origin "$commit"
git -C "$dest" -c advice.detachedHead=false checkout --quiet "$commit"

echo "FlameGraph $(git -C "$dest" log -1 --format='%h (%cs)') is in third_party/FlameGraph"
