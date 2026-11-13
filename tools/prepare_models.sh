#!/usr/bin/env bash
# Turn the two CGTrader downloads (see models/README.md) into the files the towers scene loads:
#
#   models/Building/OBJ/FC1.obj + models/Building/textures/*.jpg
#       -> models/Building/OBJ/tower_{a,b,c}.obj and models/Building/textures/4k/
#   models/Trees/Realistic+Trees+.obj + models/Trees/texture/
#       -> models/Trees/OBJ/{tree_a,tree_b,tree_c,tree_d,cherry}.obj and texture/oak_leaf.png
#
# Needs Python 3 and ImageMagick (convert). Steps whose inputs are missing are skipped.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

building=models/Building
if [[ -f $building/OBJ/FC1.obj ]]; then
    echo "Building: splitting the three towers"
    python3 tools/split_obj.py $building/OBJ/FC1.obj $building/OBJ \
        "tower_a=FC01 A" "tower_b=FC01 B" "tower_c=FC01 C,FC08.001"

    echo "Building: 4096-pixel copies of the textures"
    mkdir -p $building/textures/4k
    for material in FC01 FC08; do
        for map in Base_color Metallic Roughness; do
            convert "$building/textures/MAT ${material}_${map}.jpg" -filter Lanczos \
                -resize 4096x4096 -quality 92 "$building/textures/4k/MAT ${material}_${map}.jpg"
        done
    done
else
    echo "Building: $building/OBJ/FC1.obj not found, skipped"
fi

trees=models/Trees
if [[ -f "$trees/Realistic+Trees+.obj" ]]; then
    echo "Trees: copying out each tree"
    python3 tools/split_obj.py "$trees/Realistic+Trees+.obj" $trees/OBJ \
        tree_a=Bark_9.001,Leaf_6.001 tree_b=Bark_9.002,Leaf_6.002 tree_c=tree,leaves \
        tree_d=tree.002,leaves.002 cherry_full=v176CherryTree01.Shape

    # The cherry's 1.2M flower-stalk faces are too thin to see; without them it is 70 MB, not 290
    python3 tools/slim_obj.py $trees/OBJ/cherry_full.obj $trees/OBJ/cherry.obj \
        v176CherryFlowerStalk.001
    rm $trees/OBJ/cherry_full.obj

    oak="$trees/texture/13f7b9ab1f960cf463fefed194dd34f2-removebg-preview (1).png"
    if [[ -f "$oak" ]]; then
        mv "$oak" $trees/texture/oak_leaf.png
    fi
else
    echo "Trees: $trees/Realistic+Trees+.obj not found, skipped"
fi
