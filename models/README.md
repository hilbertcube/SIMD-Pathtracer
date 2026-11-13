# Models

The models are not in this repository: they are too large, and some may not be redistributed. Download the ones you need into this folder, under the names below. Scenes without models (1 to 8) work without any.

## Test models (scenes 9 to 14 and 16)

From Alec Jacobson's [common 3D test models](https://github.com/alecjacobson/common-3d-test-models) collection, which credits the original authors. Download them all from the repository root with:

```bash
for name in armadillo happy horse lucy max-planck nefertiti spot stanford-bunny suzanne teapot; do
    curl -L -o models/$name.obj \
        https://raw.githubusercontent.com/alecjacobson/common-3d-test-models/master/data/$name.obj
done
```

| File | Model | Original source | Scenes |
| --- | --- | --- | --- |
| `armadillo.obj` | Armadillo | [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/) | 13 |
| `happy.obj` | Happy Buddha | [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/) | 12 |
| `lucy.obj` | Lucy | [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/) | 12 |
| `stanford-bunny.obj` | Stanford Bunny | [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/) | 13, 14 |
| `horse.obj` | Horse | Cyberware | 14 |
| `max-planck.obj` | Max Planck bust | Max Planck Institute | 12 |
| `nefertiti.obj` | Nefertiti bust | [Thingiverse](https://www.thingiverse.com/thing:3974391) | 12 |
| `teapot.obj` | Utah teapot | Martin Newell | 9, 10, 16 |
| `suzanne.obj` | Suzanne | Blender | 10 |
| `spot.obj` | Spot | [Keenan Crane](https://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/#spot) | 10 |

## Christmas (scene 15)

The [Kenney Holiday Kit](https://kenney.nl/assets/holiday-kit) (CC0). Download it and copy the OBJ-format models and `colormap.png` into `models/christmas/`.

## Art Deco towers (scene 19)

Two free models from CGTrader (a free account is needed to download). After placing them as below, run `tools/prepare_models.sh` from the repository root (it needs Python 3 and ImageMagick). It splits the models into the files the scene loads, makes 4096-pixel copies of the textures, and removes the cherry tree's flower stalks, which are too thin to see.

- [Fantasy Art Deco Architecture Classic 01](https://www.cgtrader.com/free-3d-models/exterior/sci-fi-exterior/fantasy-art-deco-architecture-classic-01): put `FC1.obj` in `models/Building/OBJ/`, and unpack `textures.rar` into `models/Building/` so the images end up in `models/Building/textures/`.
- [Free Realistic Tree Aset Pack](https://www.cgtrader.com/free-3d-models/plant/leaf/free-realistic-tree-aset-pack): put `Realistic+Trees+.obj` in `models/Trees/`, and unpack `texture.zip` there so the images end up in `models/Trees/texture/`.

## Sources still to be added

These are used by scenes, but where they came from has not been recorded yet:

| File | Scenes |
| --- | --- |
| `dragon.obj` (a reduced Stanford Dragon) | 11 |
| `lionstatue.obj` | 12 |
| `demosthenes-low-res.obj` | 12 |
| `new+kitchen.obj` | 17 |
| `Mustang/` (`Mustang.obj` and its two stripe textures) | 18 |
