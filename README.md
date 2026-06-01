# tin_test

Synthetic 3D TIN (triangular irregular network) generator. Each mesh is the convex hull of a random point set with **exactly** the requested number of hull vertices.

**Dependencies:** [TriMesh2](https://github.com/Forceflow/trimesh2) and [Qhull](https://github.com/qhull/qhull) are downloaded into `third_party/` on first configure and built with the project. No system CGAL/Qhull/TriMesh2 install is required.

## Prerequisites

| Tool | Purpose |
|------|---------|
| CMake ≥ 3.21 | Configure and build |
| Ninja (recommended) | Fast builds |
| C++20 compiler | `g++` 10+, Clang 10+, or Apple Clang |
| Git | Fetch `third_party/trimesh2` and `third_party/qhull` once |

### macOS (Homebrew)

```bash
brew install cmake ninja git
xcode-select --install   # if needed
```

### Debian / Ubuntu

```bash
sudo apt update
sudo apt install cmake ninja-build g++ git
```

### CentOS / Rocky Linux / AlmaLinux

**Stream 8 / 9 / Rocky 9 / Alma 9** (recommended):

```bash
sudo dnf install cmake ninja-build gcc-c++ git
```

**CentOS 7** (end-of-life; needs a newer compiler for C++20):

```bash
sudo yum install centos-release-scl
sudo yum install devtoolset-11-gcc-c++ cmake3 ninja-build git
scl enable devtoolset-11 bash
# use cmake3 if `cmake` is still too old; CMake ≥ 3.21 is required
```

If `cmake` is older than 3.21, install a newer CMake from [Kitware](https://cmake.org/download/) or your module system, then use that binary.

## Build

First configure downloads vendored libraries (needs network):

```bash
cmake --preset release
cmake --build --preset release
```

Binary: `build/release/tin_test`

Without presets:

```bash
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j
```

### Offline / air-gapped

On a connected machine, run `cmake` once so `third_party/trimesh2` and `third_party/qhull` exist. Copy the whole repo (including those folders) to the offline host, then configure and build there.

### Clean reconfigure

If dependencies or CMake logic changed:

```bash
rm -rf build/release third_party/trimesh2 third_party/qhull
cmake --preset release && cmake --build --preset release
```

## Run

No arguments prints usage.

```bash
./build/release/tin_test
./build/release/tin_test generate
./build/release/tin_test generate --format obj --seed 42 --num-objects 5
./build/release/tin_test normalize --input-dir sample_gen --no-metadata
./build/release/tin_test help
```

### Generate options

| Flag | Default | Description |
|------|---------|-------------|
| `--format FORMAT` | `ply` | `ply` or `obj` |
| `-o, --output-dir DIR` | `sample_gen` | Output directory |
| `--num-objects N` | `10` | Number of meshes |
| `--num-vertices-per-object N` | `200` | Exact hull vertex count per mesh |
| `--scale VALUE` | `1.0` | Coordinate scale |
| `--seed N` | `0` | RNG seed (`0` = random) |
| `-q, --quiet` | off | Progress every 1000 meshes instead of per file |

Output: `<output-dir>/object_1.ply`, `object_2.ply`, … Meshes are written as they are generated (not all held in RAM). The command prints CPU and wall time.

**Large datasets (e.g. 100k meshes):** use a **Release** build and `--quiet`:

```bash
./build/release/tin_test generate --num-objects 10000 --num-vertices-per-object 200 \
  --output-dir output_synthetic/objects10000_vertices200 --seed 42 --quiet
```

The synthetic dataset script enables `--quiet` automatically.

### Normalize options

Translates each mesh so the vertex mean is at the origin (subtracts the per-mesh mean). **No scaling** is applied. Output is written as ASCII PLY.

| Flag | Default | Description |
|------|---------|-------------|
| `-i, --input-dir DIR` | (required) | Folder containing `.ply` files |
| `-o, --output-dir DIR` | `sample_normalized` | Output folder for normalized `.ply` files |
| `--metadata PATH` | `<input-dir>/metadata.txt` if present | CSV like `mesh_name,cat_name,no_v,no_f` (only listed meshes are processed; copied unchanged to the output folder) |
| `--no-metadata` | off | Ignore metadata even if `metadata.txt` exists |

Folder commands (`normalize`, `kdvertices`, `pairwise_distance`) use the same mesh ordering: **metadata row order** when `metadata.txt` is present (or `--metadata PATH`), otherwise numeric order for `name_NUMBER.ply` filenames (`object_1`, `object_2`, …, `object_10`), then lexicographic.

### Kdvertices options

Builds a **KD-tree index** over all mesh vertices.

| Flag | Default | Description |
|------|---------|-------------|
| `-i, --input-dir DIR` | (required) | Folder containing `.ply` files |
| `-o, --output-dir DIR` | `sample_kdvertices` | Output directory |
| `--max-objects N` | all | Process at most N meshes |
| `--combined` | off | Write one `combined.kdtree` bundle (`TINKDB1`) instead of per-mesh files |
| `--combined-file PATH` | `combined.kdtree` | Bundle filename with `--combined` |

Default: each `object_N.ply` → `object_N.kdtree` (`TINKDV1`). With `--combined`, all trees go into a single file with a table of contents (better for loading the last N objects in one read).

```bash
./build/release/tin_test kdvertices --input-dir sample_normalized
./build/release/tin_test kdvertices --input-dir sample_normalized --combined
```

### Distance

Dissimilarity between two PLY meshes. Select the algorithm with `--algorithm` (default: `vertex`).

**`--algorithm vertex`** — symmetric mean RMS nearest-vertex distance (KD-tree):

- \(d_A = \sqrt{\frac{1}{|A|}\sum_{u \in A} \|u - \mathrm{NN}_B(u)\|^2}\)
- \(d_B = \sqrt{\frac{1}{|B|}\sum_{v \in B} \|v - \mathrm{NN}_A(v)\|^2}\)
- **distance** = \((d_A + d_B) / 2\)

```bash
./build/release/tin_test distance sample_normalized/object_1.ply sample_normalized/object_2.ply
./build/release/tin_test distance --algorithm vertex A.ply B.ply
```

Core API: `symmetric_vertex_distance()` in `include/tin_gen/vertex_distance.hpp`.

### Pairwise_distance

Compute all pairwise dissimilarities for `.ply` files in a folder (same `--algorithm` as `distance`, default `vertex`). Mesh order matches `normalize` / `kdvertices` (see above). Matrix entry `(i, j)` is the distance between object `i` and object `j` (0-based indices).

Default output: `sample_pd/pairwise_distances_<algorithm>.txt` (e.g. `pairwise_distances_vertex.txt`) — comment header lists object order, then `n`, then `n` rows of `n` space-separated values.

```bash
./build/release/tin_test pairwise_distance --input-dir sample_normalized
./build/release/tin_test pairwise_distance -i sample_normalized -o sample_pd/pairwise_distances_vertex.txt
./build/release/tin_test kdvertices --input-dir sample_normalized --output-dir sample_kdvertices
./build/release/tin_test pd -i sample_normalized --kd-dir sample_kdvertices
```

With `--kd-dir`, KD-trees are loaded from that folder: if `combined.kdtree` exists (`kd --combined`), all trees are read from the bundle; otherwise each `object_N.kdtree` is loaded separately. Without `--kd-dir`, in-memory trees are built before the matrix; console output includes separate timing for **kd-tree build** / **kdtree load** and **matrix** computation.

Command aliases: `gen` (generate), `norm` (normalize), `kd` (kdvertices), `dist` (distance), `pd` (pairwise_distance).

## Synthetic datasets script

`scripts/generate_synthetic_datasets.sh` writes PLY meshes under `output_synthetic/`. It uses `build/release/tin_test` if present, otherwise `build/debug`.

```bash
./scripts/generate_synthetic_datasets.sh          # small preset (~15 MB)
./scripts/generate_synthetic_datasets.sh full     # large preset (~0.42 GB, slower)
```

On **csh/tcsh**, pick the binary with `setenv` (not `VAR=value command`):

```csh
setenv TIN_TEST_BIN build/release/tin_test
./scripts/generate_synthetic_datasets.sh
```

### Small preset (default)

| Folder | Objects | Vertices/object |
|--------|---------|-----------------|
| `output_synthetic/objects100_vertices200` | 100 | 200 |
| `output_synthetic/objects1000_vertices200` | 1000 | 200 |
| `output_synthetic/objects100_vertices500` | 100 | 500 |

### Full preset (`full`)

Six datasets: object counts **100, 1000, 10000** × hull vertices **200** and **500**.

| Objects | @ 200 v | @ 500 v | Subtotal |
|---------|---------|---------|----------|
| 100 | ~1.1 MB | ~2.7 MB | ~4 MB |
| 1,000 | ~11 MB | ~27 MB | ~38 MB |
| 10,000 | ~110 MB | ~270 MB | ~380 MB |
**Total ~0.42 GB** (ASCII PLY, ~11 KB/mesh @ 200 v, ~27 KB @ 500 v).

## Logging

Use these on **csh/tcsh** (and bash). Do **not** use `LOG_FILE=path ./script` or `2>&1 | tee` in csh — they fail with *Command not found* or *Ambiguous output redirect*.

**Dataset runs** — log file only (nothing on screen):

```csh
./scripts/generate_synthetic_datasets.sh --log-only output_synthetic/generation.log
./scripts/generate_synthetic_datasets.sh --log-only output_synthetic/generation_full.log full
```

Same, but also print progress to the terminal: use `--log` instead of `--log-only`.

**Single command** — csh merges stdout and stderr with `>&` (log only, no screen):

```csh
./build/release/tin_test generate --num-objects 10 --seed 42 >& run.log
cmake --build build/release -j >& build.log
```

Append instead of overwrite: `>>& logfile`.

## C++ API

```cpp
#include "tin_gen/generator.hpp"

auto meshes = tin_gen::generate_random_tin(
    num_objects, num_vertices_per_object, scale, seed);
tin_gen::save_objects_as_files(meshes, output_dir, tin_gen::MeshFormat::Ply);
```

Exact hull vertex counts use `src/convex_hull_vertices.cpp` (grow/prune point set, Qhull for hull geometry, TriMesh2 to orient and validate).

> Uniform random points in a 3D box yield only ~`N^(2/3)` hull vertices, so iterative adjustment is used for an exact count. Large `N` is slower than a single hull pass.

## Tests

```bash
ctest --preset debug -R tin_test_smoke
```

Only `tin_test` tests are registered (Qhull is built as two static libraries, not its full app/test suite).

## Project layout

```text
include/tin_gen/                 # public API
src/main.cpp                     # entry point
src/app.cpp                      # CLI commands
src/commands/generate.cpp
src/backends/generator.cpp       # TriMesh2 + Qhull generation
src/convex_hull_vertices.cpp     # exact vertex-count point set
src/convex_hull_3d.cpp           # Qhull hull mesh
cmake/fetch_trimesh2.cmake       # FetchContent → third_party/trimesh2
cmake/find_qhull.cmake           # fetch sources → third_party/qhull
cmake/qhull_vendor/              # build qhullstatic_r + qhullcpp only
cmake/trimesh2/                  # static TriMesh2 library target
scripts/generate_synthetic_datasets.sh
third_party/trimesh2/            # created at configure (gitignored)
third_party/qhull/               # created at configure (gitignored)
```

## Troubleshooting

| Problem | What to do |
|---------|------------|
| `Unrecognized "version" field` (presets) | Upgrade CMake to ≥ 3.21 |
| `Could not use disabled preset "debug"` on Linux | Pull latest `CMakePresets.json` (mac-only preset removed) |
| `TriMesh.h` / `libqhullcpp/...` not found | Run `cmake` from a clean build dir so fetch scripts run |
| Git fetch fails on server | Copy `third_party/trimesh2` and `third_party/qhull` from another machine |
| Very slow generation | Use **Release** build and `--quiet`; exact vertex counts need many Qhull passes per mesh — 500 vertices is much slower than 200 |
| `Nonrepresentable section on output` linking Qhull | Pull latest CMake: only Qhull **libraries** are built (not `user_eg3` / CLI tools) |
| `Ambiguous output redirect` with `tee` | Use csh `command >& file` or `./scripts/... --log file` (see [Logging](#logging)) |
| `LOG_FILE=...: Command not found` | csh/tcsh: use `./scripts/... --log file` or `setenv VAR value` then run the command |
