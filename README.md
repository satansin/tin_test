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
./build/release/tin_test help
```

### Generate options

| Flag | Default | Description |
|------|---------|-------------|
| `--format FORMAT` | `ply` | `ply` or `obj` |
| `-o, --output-dir DIR` | `output` | Output directory |
| `--num-objects N` | `10` | Number of meshes |
| `--num-vertices-per-object N` | `200` | Exact hull vertex count per mesh |
| `--scale VALUE` | `1.0` | Coordinate scale |
| `--seed N` | `0` | RNG seed (`0` = random) |

Output: `<output-dir>/object_1.ply`, `object_2.ply`, … The command prints CPU time, wall time, and mesh stats.

### Save output to a log file

Use `2>&1` so stderr is included. Use `tee` to see output on the terminal **and** write a file.

**Single generate run** (print + save):

```bash
./build/release/tin_test generate --num-objects 10 --seed 42 2>&1 | tee run.log
```

**Single generate run** (file only):

```bash
./build/release/tin_test generate --num-objects 10 --seed 42 > run.log 2>&1
```

**Dataset script** (small preset):

```bash
mkdir -p output_synthetic
./scripts/generate_synthetic_datasets.sh 2>&1 | tee output_synthetic/generation.log
```

**Dataset script** (full preset, long run):

```bash
mkdir -p output_synthetic
./scripts/generate_synthetic_datasets.sh full 2>&1 | tee output_synthetic/generation_full.log
```

**Timestamped log** (handy on servers):

```bash
mkdir -p output_synthetic
LOG="output_synthetic/run_$(date +%Y%m%d_%H%M%S).log"
./scripts/generate_synthetic_datasets.sh 2>&1 | tee "$LOG"
```

**Build log:**

```bash
cmake --build build/release -j 2>&1 | tee build.log
```

| Syntax | Meaning |
|--------|---------|
| `> file.txt` | Write stdout to file (overwrite) |
| `2>&1` | Include stderr in the same stream |
| `>> file.txt` | Append instead of overwrite |
| `tee file.txt` | Show on screen and save to file |

## C++ API

```cpp
#include "tin_gen/generator.hpp"

auto meshes = tin_gen::generate_random_tin(
    num_objects, num_vertices_per_object, scale, seed);
tin_gen::save_objects_as_files(meshes, output_dir, tin_gen::MeshFormat::Ply);
```

Exact hull vertex counts use `src/convex_hull_vertices.cpp` (grow/prune point set, Qhull for hull geometry, TriMesh2 to orient and validate).

> Uniform random points in a 3D box yield only ~`N^(2/3)` hull vertices, so iterative adjustment is used for an exact count. Large `N` is slower than a single hull pass.

## Synthetic datasets script

```bash
./scripts/generate_synthetic_datasets.sh          # small preset (~15 MB)
./scripts/generate_synthetic_datasets.sh full     # large preset (~4.2 GB, hours)
```

Uses `build/release/tin_test` if present, otherwise `build/debug`. Override with:

```bash
TIN_TEST_BIN=build/release/tin_test ./scripts/generate_synthetic_datasets.sh
```

To save a log while generating, see [Save output to a log file](#save-output-to-a-log-file) above.

### Small preset (default)

| Folder | Objects | Vertices/object |
|--------|---------|-----------------|
| `output_synthetic/objects100_vertices200` | 100 | 200 |
| `output_synthetic/objects1000_vertices200` | 1000 | 200 |
| `output_synthetic/objects100_vertices500` | 100 | 500 |

### Full preset (`full`)

Eight datasets: object counts **100, 1000, 10000, 100000** × hull vertices **200** and **500**.

| Objects | @ 200 v | @ 500 v | Subtotal |
|---------|---------|---------|----------|
| 100 | ~1.1 MB | ~2.7 MB | ~4 MB |
| 1,000 | ~11 MB | ~27 MB | ~38 MB |
| 10,000 | ~110 MB | ~270 MB | ~380 MB |
| 100,000 | ~1.1 GB | ~2.7 GB | ~3.8 GB |

**Total ~4.2 GB** (ASCII PLY, ~11 KB/mesh @ 200 v, ~27 KB @ 500 v).

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
| Very slow generation | Use `Release` build; large `--num-vertices-per-object` triggers many hull passes |
| `Nonrepresentable section on output` linking Qhull | Pull latest CMake: only Qhull **libraries** are built (not `user_eg3` / CLI tools) |
