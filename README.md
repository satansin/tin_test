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
./scripts/run_samples.sh
./build/release/tin_test generate --output-dir output_synthetic/example
./build/release/tin_test generate --format obj --seed 42 --num-objects 5 \
  --output-dir output_synthetic/example_obj
./build/release/tin_test normalize --input-dir output_synthetic/example \
  --output-dir output_synthetic/example_normalized
./build/release/tin_test ptsample -i output_synthetic/example_normalized \
  -o output_synthetic/example_samples -n 10000 --seed 42
./build/release/tin_test help
```

The sample-specific `sample_*` paths are held by
`scripts/run_samples.sh`; the CLI requires explicit output paths.

### Generate options

| Flag | Default | Description |
|------|---------|-------------|
| `--format FORMAT` | `ply` | `ply` or `obj` |
| `-o, --output-dir DIR` | (required) | Output directory |
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
| `-o, --output-dir DIR` | (required) | Output folder for normalized `.ply` files |
| `--max-objects N` | all | Normalize at most N meshes |

### Compress (PLY merge)

Concatenates per-mesh `.ply` files into bundle files (`.tinply`, default max **5000** meshes per bundle) plus `ply_merge_manifest.txt` with byte offsets for each original file. Intended for large normalized datasets (e.g. ShapeNetCore). Bundles store original PLY bytes unchanged.

| Flag | Default | Description |
|------|---------|-------------|
| `-i, --input-dir DIR` | (required) | Folder containing `.ply` files |
| `-o, --output-dir DIR` | (required) | Output folder for bundles + manifest |
| `--max-meshes-per-bundle N` | `5000` | Start a new bundle after N meshes |
| `--max-objects N` | all | Merge at most N meshes |

```bash
./build/release/tin_test compress -i ../tin_exp/real_First100_ShapeNetCore/norm \
  -o ../tin_exp/real_First100_ShapeNetCore/norm_pack
```

C++ API: `write_ply_merge()`, `load_ply_merge_manifest()`, `read_ply_from_merge()` in `include/tin_gen/ply_merge.hpp`.

Folder commands (`normalize`, `compress`, `kd`, `rs`, `ptsample`, `pairwise_distance`) list every matching mesh file in the input directory (`.ply` by default), sorted by numeric `name_NUMBER` suffix when present (`object_1`, `object_2`, …, `object_10`), otherwise lexicographic by filename. A `metadata.txt` file in the folder is ignored.

### Index build options (`kd` / `rs`)

Both commands share the same flags; defaults depend on the command.

| Flag | `kd` default | `rs` default | Description |
|------|--------------|--------------|-------------|
| `-i, --input-dir DIR` | (required) | (required) | Folder containing `.ply` files |
| `-o, --output-dir DIR` | (required) | (required) | Output directory |
| `--max-objects N` | all | all | Process at most N meshes |
| `--combined` | off | off | Write split bundles + manifest (5000 indexes per `merged_NNN` file) |

**`rs`** builds an **R*-tree** index (default spatial index for `distance` / `pairwise_distance`). Each `object_N.ply` → `object_N.rstree` (`TINRSV1`), or with `--combined`: `merged_000.tinrs`, … plus `rs_merge_manifest.txt` (5000 trees per bundle, same layout as PLY pack).

**`kd`** builds a **KD-tree** index. Each `object_N.ply` → `object_N.kdtree` (`TINKDV1`), or with `--combined`: `merged_000.tinkd`, … plus `kd_merge_manifest.txt`.

```bash
./build/release/tin_test rs --input-dir sample_normalized \
  --output-dir sample_rsvertices --combined
./build/release/tin_test kd --input-dir sample_normalized \
  --output-dir sample_kdvertices --combined
```

### Distance

Dissimilarity between two PLY meshes. Select the algorithm with `--algorithm` (default: `vertex`).

**`--algorithm vertex`** — symmetric mean RMS nearest-vertex distance (R*-tree by default):

- \(d_A = \sqrt{\frac{1}{|A|}\sum_{u \in A} \|u - \mathrm{NN}_B(u)\|^2}\)
- \(d_B = \sqrt{\frac{1}{|B|}\sum_{v \in B} \|v - \mathrm{NN}_A(v)\|^2}\)
- **distance** = \((d_A + d_B) / 2\)

```bash
./build/release/tin_test distance sample_normalized/object_1.ply sample_normalized/object_2.ply
./build/release/tin_test distance --algorithm vertex A.ply B.ply
```

Core API: `symmetric_vertex_distance()` in `include/tin_gen/vertex_distance.hpp`.

### Point sampling

`ptsample` samples points uniformly over the surfaces of all PLY meshes in an
input folder. The requested total number of points is written per mesh to the
output folder as point-cloud PLY files by default; `--format obj` is also
supported. Faces are selected proportionally to area and each selected face is
sampled uniformly using barycentric coordinates.

```bash
./build/release/tin_test ptsample \
  --input-dir sample_normalized \
  --output-dir sample_points \
  --num-points 10000 --seed 42
```

The input directory, output directory, and `--num-points` are required.
Use `--pack` to write sampled PLY meshes directly into `.tinply` bundles and
`ply_merge_manifest.txt`, without creating one output PLY file per mesh:

```bash
./build/release/tin_test ptsample \
  --input-dir sample_normalized \
  --output-dir sample_points_pack \
  --num-points 10000 --seed 42 --pack
```

Packed output is PLY-only. The default maximum is 5000 meshes per bundle; use
`--max-meshes-per-bundle N` to change it.

### Pairwise_distance

Compute all pairwise dissimilarities for `.ply` files in a folder (same `--algorithm` as `distance`, default `vertex`). Mesh order matches `normalize` / `rs` (see above). Matrix entry `(i, j)` is the distance between object `i` and object `j` (0-based indices).

The output path is required. A sample default is
`sample_pd/pairwise_distances_<algorithm>.txt` (e.g.
`pairwise_distances_vertex.txt`), supplied by `scripts/run_samples.sh`.
The comment header lists object order, then `n`, then `n` rows of `n`
space-separated values.

```bash
./build/release/tin_test pairwise_distance -i sample_normalized -o sample_pd/pairwise_distances_vertex.txt
./build/release/tin_test rs --input-dir sample_normalized --output-dir sample_rsvertices --combined
./build/release/tin_test pd -i sample_normalized --rs-dir sample_rsvertices \
  -o sample_pd/pairwise_distances_vertex.txt
```

With `--rs-dir` (or `-rs`), R*-trees are loaded from that folder: if `rs_merge_manifest.txt` exists (`rs --combined`), trees are read from split `merged_*.tinrs` bundles; otherwise each `object_N.rstree` is loaded separately. Without `--rs-dir`, in-memory R*-trees are built before the matrix; console output includes separate timing for **rs build** / **rs load** and **matrix** computation. Use `--kd-dir` (or `-kd`) to load KD-trees instead (`--rs-dir` and `--kd-dir` are mutually exclusive).

Command aliases: `gen` (generate), `norm` (normalize), `kd` (kdvertices), `rs`, `dist` (distance), `pd` (pairwise_distance).

## Experiment dataset scripts (`../tin_exp/`)

Requires a sibling `tin_exp` directory. Uses `build/release/tin_test` when present, else `build/debug` (override with `TIN_TEST_BIN`).

### Real dataset statistics

Counts and formats for the four real-world datasets used in experiments. Raw sources live under `datasets_raw/`; processed outputs live under per-dataset folders (e.g. `real_First100_ModelNet40/norm/`).

**Original dataset** (upstream sources, before this repo):

| Dataset | Format | Meshes | Notes |
|---------|--------|--------|-------|
| ModelNet40 | OFF | 12,311 | Count from CSV |
| ModelNet40_auto_aligned | OFF | 12,311 | |
| ModelNet40_manually_aligned | OFF | 12,311 | |
| ShapeNetCore | OBJ | 52,472 | Count from original files |

**Raw datasets** (`../tin_exp/datasets_raw/<name>/`):

| Dataset | Format | Meshes | Notes |
|---------|--------|--------|-------|
| ModelNet40 | PLY | 9,449 | `metadata.txt`: filename, category, #v, #f, … |
| ModelNet40_auto_aligned | PLY | 12,311 | `metadata.txt`: filename, category, #v, #f, … |
| ModelNet40_manually_aligned | PLY | 12,311 | `metadata.txt`: filename, category, #v, #f, … |
| ShapeNetCore | PLY | 51,209 | 52,472 entries in `metadata.txt` (filename, category, #v, #f, …) |

**Per-dataset layout** (`../tin_exp/<dataset>/`):

| Subfolder | Contents |
|-----------|----------|
| `norm/` | Normalized `.ply` (zero-mean translation only) |
| `norm_pack/` | `merged_*.tinply` + `ply_merge_manifest.txt` |
| `norm_rs/` | `merged_*.tinrs` + `rs_merge_manifest.txt` |
| `norm_kd/` | KD indexes (optional; not used by PD today) |
| `norm_ptsample_<N>/` | Sampled point clouds as `merged_*.tinply` + manifest |
| `norm_pd/` | `pairwise_distances_vertex.txt` |

**Dataset folder names** (small = local default, full = server):

| Small (`real_First100_*`, 100 meshes) | Full (`real_*`, all meshes) |
|---------------------------------------|-----------------------------|
| `real_First100_ModelNet40` | `real_ModelNet40` |
| `real_First100_ModelNet40_auto_aligned` | `real_ModelNet40_auto_aligned` |
| `real_First100_ModelNet40_manually_aligned` | `real_ModelNet40_manually_aligned` |
| `real_First100_ShapeNetCore` | `real_ShapeNetCore` |

Synthetic presets use `synthetic_<preset>/` (e.g. `synthetic_objects100_vertices200/`). Lists are in `scripts/datasets_common.sh`.

| Script | Input | Output |
|--------|--------|--------|
| `scripts/normalize_datasets.sh` | `output_synthetic/`, `datasets_raw/` | `<dataset>/norm/` |
| `scripts/compress_datasets.sh` | `<dataset>/norm/` | `<dataset>/norm_pack/` |
| `scripts/build_rs_datasets.sh` | `<dataset>/norm_pack/` | `<dataset>/norm_rs/` |
| `scripts/build_kd_datasets.sh` | `<dataset>/norm_pack/` | `<dataset>/norm_kd/` (optional) |
| `scripts/ptsample_datasets.sh` | `<dataset>/norm/` | `<dataset>/norm_ptsample_<N>/` |
| `scripts/compute_pd_datasets.sh` | `<dataset>/norm_pack/`, `<dataset>/norm_rs/` | `<dataset>/norm_pd/pairwise_distances_vertex.txt` |

```bash
./scripts/normalize_datasets.sh          # small (default)
./scripts/normalize_datasets.sh full
./scripts/compress_datasets.sh           # after normalize
./scripts/compress_datasets.sh full
./scripts/build_rs_datasets.sh           # after normalize (default index)
./scripts/build_rs_datasets.sh full
./scripts/build_kd_datasets.sh           # optional KD-tree indexes
./scripts/build_kd_datasets.sh full
./scripts/ptsample_datasets.sh           # 512, 1024, and 2048 points per mesh
./scripts/ptsample_datasets.sh full
./scripts/compute_pd_datasets.sh         # after build_rs_datasets
./scripts/compute_pd_datasets.sh full
```

`compress_datasets.sh` runs `tin_test compress` on each `<dataset>/norm/` and writes bundles under `<dataset>/norm_pack/` (max 5000 meshes per `.tinply` bundle).

`build_rs_datasets.sh` runs `tin_test rs --combined` on each `<dataset>/norm_pack/` and writes split bundles under `<dataset>/norm_rs/`. Run `compress_datasets.sh` first.

`build_kd_datasets.sh` runs `tin_test kd --combined` into `<dataset>/norm_kd/` (optional; not required for PD). Run `compress_datasets.sh` first.

`ptsample_datasets.sh` runs `tin_test ptsample --pack` on each dataset's
normalized per-mesh PLY files for 512, 1024, and 2048 points per mesh. It
writes `<dataset>/norm_ptsample_512/`,
`<dataset>/norm_ptsample_1024/`, and `<dataset>/norm_ptsample_2048/`.

`compute_pd_datasets.sh` runs `tin_test pd --algorithm vertex --rs-dir <dataset>/norm_rs` on each packed dataset. **Small** mode: synthetic presets + `real_First100_*` datasets; **full** mode: all presets + `real_*` datasets. Run `compress_datasets.sh` and `build_rs_datasets.sh` first.

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

Long runs (`generate`, `normalize`, `build_kd`, etc.) are often started from **bash** or **tcsh**. Syntax differs; using bash-style redirects in tcsh causes errors such as `Ambiguous output redirect` or `Command not found`.

Create the log directory first if needed, e.g. `mkdir -p output_synthetic`.

### bash / sh

| Goal | How |
|------|-----|
| **Log only** (nothing on screen) | `command > logfile 2>&1` or `command &> logfile` |
| **Log + screen** | `command 2>&1 \| tee logfile` |

**`generate_synthetic_datasets.sh`** (built-in flags; preferred for that script):

```bash
# log only
./scripts/generate_synthetic_datasets.sh --log-only output_synthetic/generation.log
./scripts/generate_synthetic_datasets.sh --log-only output_synthetic/generation_full.log full

# log + screen
./scripts/generate_synthetic_datasets.sh --log output_synthetic/generation.log
./scripts/generate_synthetic_datasets.sh -l output_synthetic/generation_full.log full
```

Same as `--log`: `LOG_FILE=path ./scripts/generate_synthetic_datasets.sh` (bash/sh only).

**Other bash scripts** (`normalize_datasets.sh`, `build_kd_datasets.sh`) — no `--log` flags; use redirects:

```bash
./scripts/normalize_datasets.sh full > ../tin_exp/normalize.log 2>&1
./scripts/build_kd_datasets.sh full 2>&1 | tee ../tin_exp/build_kd.log
```

**Single `tin_test` command:**

```bash
./build/release/tin_test kd -i sample_normalized -o /tmp/kd_out 2>&1 | tee kd.log
./build/release/tin_test gen --num-objects 10 --seed 42 > run.log 2>&1
```

Append instead of overwrite: `>> logfile 2>&1` or `2>&1 | tee -a logfile`.

### tcsh / csh

Do **not** use `VAR=value command`, `2>&1`, or `| tee` in tcsh for these jobs.

| Goal | How |
|------|-----|
| **Log only** | `command >& logfile` |
| **Log + screen** | Use a script’s `--log` flag (see below), or run one bash line: `bash -c 'command 2>&1 \| tee logfile'` |

**`generate_synthetic_datasets.sh`** — `--log` / `--log-only` work from tcsh (the script is bash; it handles logging internally):

```csh
# log only
./scripts/generate_synthetic_datasets.sh --log-only output_synthetic/generation.log
./scripts/generate_synthetic_datasets.sh --log-only output_synthetic/generation_full.log full

# log + screen
./scripts/generate_synthetic_datasets.sh --log output_synthetic/generation.log
```

**Other scripts** (`normalize_datasets.sh`, `build_kd_datasets.sh`) — log only via `>&`:

```csh
./scripts/normalize_datasets.sh full >& ../tin_exp/normalize.log
./scripts/build_kd_datasets.sh full >& ../tin_exp/build_kd.log
```

For **log + screen** on those scripts, either use bash:

```csh
bash -c './scripts/build_kd_datasets.sh full 2>&1 | tee ../tin_exp/build_kd.log'
```

or log only with `>&` and watch with `tail -f ../tin_exp/build_kd.log` in another window.

**Single `tin_test` command:**

```csh
./build/release/tin_test kd -i sample_normalized -o /tmp/kd_out >& kd.log
```

**Environment variables** — set before the command, not inline:

```csh
setenv TIN_TEST_BIN /path/to/tin_test/build/release/tin_test
./scripts/build_kd_datasets.sh full >& build_kd.log
```

Append instead of overwrite: `command >>& logfile`.

## C++ API

```cpp
#include "tin_gen/generator.hpp"

auto meshes = tin_gen::generate_random_tin(
    num_objects, num_vertices_per_object, scale, seed);
tin_gen::save_objects_as_files(meshes, output_dir, tin_gen::MeshFormat::Ply);
```

Surface samples can be generated uniformly across a TIN's faces:

```cpp
#include "tin_gen/face_sampling.hpp"

auto samples = tin_gen::sample_points_on_faces(mesh, num_points, seed);
```

The requested total number of points is returned. Faces are selected
proportionally to area, and each selected face is sampled uniformly using
barycentric coordinates. A seed of `0` uses non-deterministic seeding.

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
src/face_sampling.cpp            # uniform surface sampling on TIN faces
src/convex_hull_vertices.cpp     # exact vertex-count point set
src/convex_hull_3d.cpp           # Qhull hull mesh
cmake/fetch_trimesh2.cmake       # FetchContent → third_party/trimesh2
cmake/find_qhull.cmake           # fetch sources → third_party/qhull
cmake/qhull_vendor/              # build qhullstatic_r + qhullcpp only
cmake/trimesh2/                  # static TriMesh2 library target
scripts/generate_synthetic_datasets.sh
scripts/ptsample_datasets.sh
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
| Qhull `QhullLinkedList` / `template-id not allowed for destructor` on Linux GCC | Qhull 8.0.2 + C++20; pull latest (auto-patches Qhull headers) or re-run `cmake` after `rm -rf third_party/qhull build/release` |
| `Ambiguous output redirect` with `tee` | You are in tcsh: use `>&` or `--log` / `--log-only`; see [Logging](#logging) |
| `LOG_FILE=...: Command not found` | tcsh: use `--log PATH` on `generate_synthetic_datasets.sh`, or `setenv LOG_FILE PATH` then run it from bash |
