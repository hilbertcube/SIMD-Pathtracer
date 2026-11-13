# Models

The models are not in this repo: they are too large, and some may not be redistributed. Download the ones you need into this folder, under the names below. Scenes without models (1 to 8) work without any.

## Test models (scenes 9 to 14 and 16)

From Alec Jacobson's [common 3D test models](https://github.com/alecjacobson/common-3d-test-models) collection, which credits the original authors. Download them all from the repository root with:

```bash
for name in armadillo happy horse lucy max-planck nefertiti spot stanford-bunny suzanne teapot; do
    curl -L -o models/$name.obj \
        https://raw.githubusercontent.com/alecjacobson/common-3d-test-models/master/data/$name.obj
done
curl -L -o models/dragon.obj \
    https://raw.githubusercontent.com/alecjacobson/common-3d-test-models/master/data/xyzrgb_dragon.obj
```

| File | Model | Original source | Scenes |
| --- | --- | --- | --- |
| `armadillo.obj` | Armadillo | [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/) | 13 |
| `happy.obj` | Happy Buddha | [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/) | 12 |
| `lucy.obj` | Lucy | [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/) | 12 |
| `stanford-bunny.obj` | Stanford Bunny | [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/) | 13, 14 |
| `dragon.obj` | XYZ RGB Dragon (reduced; `xyzrgb_dragon.obj` in the collection) | [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/), scanned by XYZ RGB Inc. | 11 |
| `horse.obj` | Horse | Cyberware | 14 |
| `max-planck.obj` | Max Planck bust | Max Planck Institute | 12 |
| `nefertiti.obj` | Nefertiti bust | [Thingiverse](https://www.thingiverse.com/thing:3974391) | 12 |
| `teapot.obj` | Utah teapot | Martin Newell | 9, 10, 16 |
| `suzanne.obj` | Suzanne | Blender | 10 |
| `spot.obj` | Spot | [Keenan Crane](https://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/#spot) | 10 |

## Gallery statues (scene 12)

From Oded Stein's [meshes](https://github.com/odedstein/meshes) collection. Download them from the repository root with:

```bash
curl -L -o models/lionstatue.obj \
    https://raw.githubusercontent.com/odedstein/meshes/master/objects/lionstatue/lionstatue.obj
curl -L -o models/demosthenes-low-res.obj \
    https://raw.githubusercontent.com/odedstein/meshes/master/objects/demosthenes/demosthenes-low-res.obj
```

| File | Model | Original source and license |
| --- | --- | --- |
| `lionstatue.obj` | Lion statue | by tomburtonwood, [Thingiverse](https://www.thingiverse.com/thing:25832) ([CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/)), modified by Oded Stein |
| `demosthenes-low-res.obj` | Bust of Demosthenes (remeshed, low resolution) | [Louvre Demosthenes photoscan](https://sketchfab.com/3d-models/louvre-demosthenes-photoscan-cf5d9dd9bf4a457c95dc76e17848727b) by ryanfb ([CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)) |

## Christmas (scene 15)

The [Kenney Holiday Kit](https://kenney.nl/assets/holiday-kit) (CC0). Download it and copy the OBJ-format models and `colormap.png` into `models/christmas/`.

## Kitchen (scene 17)

A free [modern kitchen on CGTrader](https://www.cgtrader.com/free-3d-models/interior/kitchen/modern-kitchen-c40ea1b8-6c81-4792-b3fe-08fdc5888c3b) (a free account is needed to download). Save its OBJ as `models/new+kitchen.obj`.

## Mustang (scene 18)

A free [Mustang model on CGTrader](https://www.cgtrader.com/items/864398/download-page) (a free account is needed to download). Put `Mustang.obj` and its two body-stripe textures, `MustangBodyStripe1.png` and `MustangBodyStripe2.png`, in `models/Mustang/`.

## Art Deco towers (scene 19)

Two free models from CGTrader (a free account is needed to download). After placing them as below, run `tools/prepare_models.sh` from the repository root (it needs Python 3 and ImageMagick). It splits the models into the files the scene loads, makes 4096-pixel copies of the textures, and removes the cherry tree's flower stalks, which are too thin to see.

- [Fantasy Art Deco Architecture Classic 01](https://www.cgtrader.com/free-3d-models/exterior/sci-fi-exterior/fantasy-art-deco-architecture-classic-01): put `FC1.obj` in `models/Building/OBJ/`, and unpack `textures.rar` into `models/Building/` so the images end up in `models/Building/textures/`.
- [Free Realistic Tree Aset Pack](https://www.cgtrader.com/free-3d-models/plant/leaf/free-realistic-tree-aset-pack): put `Realistic+Trees+.obj` in `models/Trees/`, and unpack `texture.zip` there so the images end up in `models/Trees/texture/`.
