#!/usr/bin/env python3
"""Copy an OBJ without the faces of some materials, keeping only the v / vt / vn still used.

    tools/slim_obj.py IN.obj OUT.obj material [material ...]
"""
import sys

source, target, dropped = sys.argv[1], sys.argv[2], set(sys.argv[3:])
lines = {"v": [], "vt": [], "vn": []}
faces = []  # (material, corners as (v, vt, vn) index tuples, 0 when absent)
material = ""
with open(source, encoding="utf-8", errors="replace") as f:
    for line in f:
        tag = line.split(" ", 1)[0]
        if tag in lines:
            lines[tag].append(line)
        elif tag == "usemtl":
            material = line.split()[1]
        elif tag == "f" and material not in dropped:
            corners = [tuple(int(i) if i else 0 for i in c.split("/")) for c in line.split()[1:]]
            faces.append((material, corners))

remap = [{}, {}, {}]  # old index -> new index, for v, vt, vn
for _, corners in faces:
    for corner in corners:
        for k, index in enumerate(corner):
            if index and index not in remap[k]:
                remap[k][index] = len(remap[k]) + 1

with open(target, "w") as out:
    out.write(f"# {source} without the faces of: {', '.join(sorted(dropped))}\n")
    for k, tag in enumerate(("v", "vt", "vn")):
        for old in sorted(remap[k], key=remap[k].get):
            out.write(lines[tag][old - 1])
    current = None
    for material, corners in faces:
        if material != current:
            out.write(f"usemtl {material}\n")
            current = material
        out.write("f " + " ".join("/".join(str(remap[k][i]) if i else "" for k, i in enumerate(c))
                                  for c in corners) + "\n")
print(f"{target}: {len(faces)} faces")
