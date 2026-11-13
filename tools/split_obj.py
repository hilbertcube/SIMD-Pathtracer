#!/usr/bin/env python3
"""Copy chosen objects of an OBJ into smaller OBJ files of their own, renumbering their indices.

    tools/split_obj.py SOURCE.obj OUT_DIR name=object[,object...] [name=...]

Each name=... writes OUT_DIR/name.obj holding those objects ("o" lines). Object names may contain
spaces (quote the argument). The source is read once; each object's v / vt / vn lines must be in
its own block, as Blender writes them.
"""
import os
import sys

source, out_dir = sys.argv[1], sys.argv[2]
groups = {}  # object -> output name
for spec in sys.argv[3:]:
    name, objects = spec.split("=", 1)
    for obj in objects.split(","):
        groups[obj] = name
outputs = {name: [] for name in set(groups.values())}

counts = {"v": 0, "vt": 0, "vn": 0}  # running totals in the source
current = None  # (object, counts at its start, its lines) while inside a chosen object


def finish_object():
    if current is not None:
        outputs[groups[current[0]]].append(current)


with open(source, encoding="utf-8", errors="replace") as f:
    for line in f:
        tag = line.split(" ", 1)[0]
        if tag == "o":
            finish_object()
            name    = line[2:].strip()
            current = (name, dict(counts), []) if name in groups else None
            continue
        if tag in counts:
            counts[tag] += 1
        if current is not None and tag in ("v", "vt", "vn", "f", "usemtl", "s"):
            current[2].append(line)
    finish_object()

missing = set(groups) - {obj for parts in outputs.values() for obj, _, _ in parts}
if missing:
    sys.exit(f"objects not found in {source}: {', '.join(sorted(missing))}")

os.makedirs(out_dir, exist_ok=True)
for name, objects in outputs.items():
    path    = os.path.join(out_dir, name + ".obj")
    written = {"v": 0, "vt": 0, "vn": 0}
    faces   = 0
    with open(path, "w") as out:
        out.write(f"# Extracted from {os.path.basename(source)}\n")
        for obj, start, lines in objects:
            out.write(f"o {obj}\n")
            local = {"v": 0, "vt": 0, "vn": 0}
            for line in lines:
                tag = line.split(" ", 1)[0]
                if tag in local:
                    local[tag] += 1
                    out.write(line)
                elif tag == "f":
                    corners = []
                    for corner in line.split()[1:]:
                        fixed = []
                        for kind, value in zip(("v", "vt", "vn"), corner.split("/")):
                            if value == "":
                                fixed.append("")
                                continue
                            index = int(value) - start[kind]  # 1-based within the object
                            if not 1 <= index <= local[kind]:
                                sys.exit(f"{obj}: a face refers outside the object")
                            fixed.append(str(index + written[kind]))
                        corners.append("/".join(fixed))
                    out.write("f " + " ".join(corners) + "\n")
                    faces += 1
                else:
                    out.write(line)
            for kind in written:
                written[kind] += local[kind]
    print(f"{path}: {faces} faces")
