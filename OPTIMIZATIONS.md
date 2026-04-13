# Optimization notes

A record of the performance work on this ray tracer, in the order it was done: what changed, why, and what it bought. Each step is its own commit. The hashes are listed so `git show <hash>` gives the full change.

## How things were measured

- **Benchmark:** every scene at 300 px wide and 64 samples per pixel, built with `-O3 -march=native`, run on all 32 threads of an i9-14900HX laptop. Times are medians of 3–5 runs.
- **Interleaving:** later comparisons alternate the old and new binaries run by run. The laptop's clock speed drifts as it heats up, so back-to-back batches can be 20–30% apart on the same code.
- **Profiling:** `perf record`, to find where time goes before changing anything.
- **Correctness:**
  - BVH changes are checked against brute force: every ray is tested against every triangle of the teapot, Suzanne, spot and dragon meshes. The rays include ones starting inside the mesh and axis-aligned ones.
  - Rendering changes are checked by comparing images. Mean brightness is not enough on its own: it hid fireflies once. Changes are also checked for noise (difference from a blurred copy) and speckle counts, and against a 4096-sample reference where it matters.

## Summary

| Final scene (300 px, 64 spp) | Time |
| --- | --- |
| Start of this work | 5378 ms |
| After removing CPU waste (shared_ptr, BVH, top-level BVH, Russian roulette) | 506 ms |
| Now | 445 ms |

From start to now, the final scene renders about 12× faster. Light sampling with MIS (below) sits between the last two rows. It makes each sample more expensive, because every diffuse bounce adds a shadow ray, but each sample is much less noisy. Raw times before and after it are not directly comparable.

## 1. Cleanup first (no speed change, less code)

| Commit | Change |
| --- | --- |
| `7a692c5` | Removed the unused thread pool, and the Benchmark classes apart from `benchmark::Timer`. |
| `7d1faf6` | Removed the unused `fmt` dependency and all MSVC support (GCC/Clang only). |
| `62a0585` | Replaced `VecN` with a small `Vector<T, N>`. Removed the SSE `vec3` (see "Tried and dropped"). |

## 2. Removing CPU waste

### shared_ptr reference counting on the hot path (`d754ad0`)
`perf` showed about **50% of render time** in `shared_ptr` reference counting. Each copy of a `shared_ptr` is an atomic increment and decrement on a counter that all 32 threads share, so the cache line holding it bounced between cores. The two copies that caused it:
- every recorded hit copied the material pointer;
- every bounce heap-allocated a pdf.

Changes:
- `hit_record` holds a plain `const material*`. The objects own their materials, so a non-owning pointer is safe.
- The scatter pdf is stored inside `scatter_record`, so a bounce no longer allocates.

| Scene | Before | After |
| --- | --- | --- |
| Spheres | 475 ms | 190 ms |
| Cornell box | 1547 ms | 324 ms |
| Final scene | 5378 ms | 1515 ms |
| Three models | 2069 ms | 813 ms |

### Flat BVH built with binned SAH (`d26e0df`)
The original BVH was a tree of heap nodes reached through `shared_ptr`, split at the median along the longest axis after sorting the objects. It is now:
- one flat array of nodes, with the two children next to each other;
- split with the surface area heuristic over 12 bins per axis, with up to 4 primitives per leaf;
- traversed with a loop and an explicit stack instead of recursion, nearer child first;
- given the inverse ray direction once per ray.

The dragon's tree (250k triangles) builds in about 165 ms.

| Scene | Before | After |
| --- | --- | --- |
| Spheres | 190 ms | 117 ms |
| Final scene | 1515 ms | 1148 ms |
| Three models | 813 ms | 574 ms |

### BVH over the top level of the scenes (`60868ce`)
`translate` and `rotate_y` transform the ray before any bounding-box test, so every ray paid for every quad of both Cornell boxes. Wrapping each scene's top-level objects in a BVH lets rays skip whatever they can't reach.

| Scene | Before | After |
| --- | --- | --- |
| Cornell box | 321 ms | 250 ms |
| Final scene | 1150 ms | 984 ms |
| Three models | 551 ms | 435 ms |

### Russian roulette (`4a4e42e`)
After 3 bounces, a path continues with probability p and survivors are scaled by 1/p, so the estimate stays unbiased. p is the brightest channel of the surface's attenuation, capped at 0.95. Dark surfaces stop wasting bounces. At 4096 spp the Cornell box mean matches the render without roulette to within 0.1%.

| Scene | Before | After |
| --- | --- | --- |
| Final scene | 984 ms | 506 ms |
| Cornell box | 250 ms | 225 ms |
| Three models | 435 ms | 388 ms |

`29c02a9` (non-virtual quad helpers) was tidy-up and changed nothing measurable.

## 3. Build and threading

- **Release by default** (`d4c6cb5`): with no `CMAKE_BUILD_TYPE`, CMake added no optimization flags at all. A Debug build renders the Cornell box in **15.1 s instead of 1.3 s**.
- **Rows instead of tiles** (`a248ba9`): OpenMP `parallel for schedule(dynamic)` hands out one image row per task. The tile renderer ran at the same speed, within noise, and needed a tile struct, a tile list and an atomic counter, so it was removed.
- **Thread scaling is limited by the laptop's power budget, not the code:** about 5.1 GHz with 1 thread busy, about 2.4 GHz with 16 or more.

## 4. Light sampling with multiple importance sampling (`5525f20`)

This was an image-quality fix first, but it is also the biggest gain in *time to a clean image*.

**The problem:** Book 3's mixture pdf sends half the bounces toward the lights but has no shadow ray. Its density assumes the light is visible. Where it isn't (fog inside the glass sphere, gaps between the floor boxes), the other half of the samples get up to 2× weight per bounce. That compounds into the blue and green fireflies in the final scene.

**The fix:** every diffuse bounce traces a shadow ray toward a sample point on a light, and also continues along a material sample. The two are combined with the power heuristic. A light counts only if the shadow ray actually reaches it.

**Results:**
- **Fireflies:** final scene (400 px, 500 spp) went from 62–78 blue and 18–27 green speckles to 0.
- **Same answer:** the Cornell box at 4096 spp matches the old method to within 0.01%.
- **Less noise in the same time:** each sample costs one more ray, but at equal render time noise improves:

  | Scene | Less noise at equal time |
  |---|---|
  | Cornell box | 2.9× |
  | Three models | 3.8× |
  | Simple light | 2.3× |
  | Final scene | 1.2× |

**Scene seed** (`22e18c1`, correctness rather than speed): the scenes generated with random numbers (sphere layouts, box heights, Perlin noise) are now built from a fixed seed, so they are the same on every run. Rendering is still seeded from the clock, so each run's noise is independent. This was also needed to compare noise between builds at all.

## 5. 4-wide BVH with SSE (`6b81bd5`)

After the steps above, `perf` showed BVH traversal at about 40% of the mesh scenes.

**How it works:** the binary SAH tree is still built as before, then collapsed so each node holds up to four children. Collapsing works by repeatedly opening the child with the largest surface area. The node stores its children's bounds structure-of-arrays, so one SSE load fetches one slab for all four. One slab test checks the ray against all four boxes, and the children it hits are visited nearest first.

**Details that matter:**
- The near and far slab are picked once per ray from the sign of the direction, which saves a min/max per axis in every node.
- `_mm_max_ps(t, t_enter)` returns its second operand when either is NaN. NaN comes from 0 × ∞, when a ray runs exactly along a slab. Passing the running interval second keeps the scalar version's behavior.
- Empty child slots get an inverted box that no ray can hit, and are also skipped explicitly.

| Scene | Before | After | Speedup |
| --- | --- | --- | --- |
| Spheres | 107 ms | 78 ms | 1.38× |
| Cornell box | 567 ms | 513 ms | 1.11× |
| Final scene | 895 ms | 762 ms | 1.17× |
| 3D model | 763 ms | 607 ms | 1.26× |
| Multiple models | 896 ms | 735 ms | 1.22× |

The brute-force check found no mismatches over about 92,000 rays. **Trade-off:** the project now builds only for x86-64. On anything else `bvh_node.h` stops with an `#error`.

## 6. Code that did unnecessary work

A read-through of the hot paths, with each finding checked against the profile.

| Commit | Problem | Fix | Gain |
| --- | --- | --- | --- |
| `dd8044b` | `sphere::hit` called `acos` and `atan2` for texture coordinates on every hit. That included hits later replaced by a closer one, both boundary hits of a fog volume, and the light-pdf hit test. About 21% of the final scene. | Materials and textures report `needs_uv()` (true by default, so new ones stay correct). Only image textures need them. A sphere checks this once when it is built. | 1.31× final scene |
| `a011c00` | Perlin `interpolate` summed into a `double` (`auto sum = 0.0`), and `noise_texture` called C's `double` `sin`. | Kept the math in `float`. | 1.08× Perlin scene |
| `09b0269` | `quad::hit` took two cross products per hit for the plane coordinates. | By the scalar triple product they are two dot products against axes precomputed in the constructor. | 1.08× Cornell box |
| `8d77a59` | Each diffuse bounce normalized the same vectors several times. The `ONB` built a second unit vector, and the cosine densities built a whole normalized vector to take one dot product. | Branchless ONB (Duff et al., JCGT 2017) with one square root. Densities divide the dot product by the length. Checked orthonormal to about 4e-7 over a million directions. | 1.02–1.16× |

Combined, against the 4-wide BVH build:

| Scene | Before | After | Speedup |
| --- | --- | --- | --- |
| 1 spheres | 77 ms | 67 ms | 1.16× |
| 3 earth | 77 ms | 71 ms | 1.07× |
| 4 Perlin | 65 ms | 52 ms | 1.25× |
| 6 simple light | 64 ms | 52 ms | 1.23× |
| 7 Cornell box | 504 ms | 435 ms | 1.16× |
| 8 final scene | 590 ms | 445 ms | 1.33× |
| 9 3D model | 603 ms | 547 ms | 1.10× |
| 10 multiple models | 745 ms | 666 ms | 1.12× |

Noise and mean brightness are unchanged in every scene.

## Tried and dropped

- **SSE `vec3`:** with `-O3 -march=native` it was within ±15% of plain C++, faster on some scenes and slower on others. A 3-float vector uses only 3 of the 4 SSE lanes, and packing and unpacking it around scalar code costs about as much as it saves. SIMD paid off only in the BVH, where 4 child boxes fill all 4 lanes.
- **Precomputing mesh triangle edges and normal:** 0.99× and 1.00× on the mesh scenes. The subtractions it saved are cheap, the normal's square root only ran on actual hits, and it made every triangle bigger. Reverted.
- **Tile renderer:** same speed as rows, more code (see section 3).
- **`-ffast-math`:** not used. It lets the compiler assume no NaN or infinity, which breaks the checks a path tracer relies on (a zero-density sample, the 0 × ∞ slab case above).

## Ideas not done yet

Rough expectations from other renderers, not measured here:

- **Denoising** (for example Intel Open Image Denoise): render at 16–64 spp and denoise. This is the biggest step toward interactive speed.
- **Low-discrepancy or blue-noise sampling** instead of plain random numbers: less noise for the same sample count.
- **Adaptive sampling:** stop sampling a pixel once its variance is low.
- **Profile-guided optimization** (`-fprofile-generate` / `-fprofile-use`): typically 5–15%, with no code change.
- **Shorter shadow rays for quad lights:** stop at the light instead of searching to infinity. Sphere lights would need `random()` to return a vector that ends on the light first.
- **A triangle array stored in the BVH** instead of one heap object and virtual call per triangle.
- **Small leftovers:** the per-thread RNG's hidden setup check (about 1–2%), `std::pow(x, 5)` in the glass reflectance, the record copy in `hittable_list::hit`, and a few `double` literals in float code.
