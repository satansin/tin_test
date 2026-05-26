# tin_test

Synthetic 3D TIN (triangular irregular network) generator. Each generated mesh is the convex hull of a random point set, with **exactly** the requested number of hull vertices. Both **CGAL** and **[TriMesh2](https://github.com/Forceflow/trimesh2)** backends are always compiled in; pick at runtime.

## Prerequisites

| Tool | macOS | Linux (Debian/Ubuntu) |
|------|-------|------------------------|
| CMake ≥ 3.27 | `brew install cmake` | `sudo apt install cmake` |
| Ninja | `brew install ninja` | `sudo apt install ninja-build` |
| C++ compiler | Xcode Command Line Tools | `sudo apt install g++` |
| CGAL | `brew install cgal` | `sudo apt install libcgal-dev` |
| Qhull | `brew install qhull` | `sudo apt install libqhull-dev` |

Clone TriMesh2 into the project:

```bash
git clone https://github.com/Forceflow/trimesh2.git third_party/trimesh2
```

## Build

```bash
cmake --preset debug
cmake --build --preset debug
```

Release: `cmake --preset release` / `cmake --build --preset release`.

## Run

The executable is `tin_test`. Running it with no arguments prints usage.

```bash
./build/debug/tin_test                                       # prints usage
./build/debug/tin_test generate                              # use defaults
./build/debug/tin_test generate --backend trimesh2 --seed 42
./build/debug/tin_test generate --format obj --num-objects 5
./build/debug/tin_test help
```

### Commands

| Command | Description |
|---------|-------------|
| `generate` | Build random TIN meshes and write files |
| `help` | Show usage |

### Generate options

| Flag | Default | Description |
|------|---------|-------------|
| `--backend NAME` | `cgal` | `cgal` or `trimesh2` |
| `--format FORMAT` | `ply` | `ply` or `obj` |
| `-o, --output-dir DIR` | `output` | Output directory |
| `--num-objects N` | `10` | Number of meshes to generate |
| `--num-vertices-per-object N` | `200` | **Exact** hull vertex count per mesh |
| `--scale VALUE` | `1.0` | Coordinate scale for random points |
| `--seed N` | `0` | RNG seed (`0` = random) |

Output files: `<output-dir>/object_1.ply`, `object_2.ply`, … (or `.obj`). The command prints the backend, CPU time, wall time, and mesh stats.

## Generators (C++ API)

| Function | Backend |
|----------|---------|
| `generate_random_tin()` | CGAL convex hull |
| `generate_random_tin_trimesh()` | TriMesh2 + Qhull convex hull |

Both live in `include/tin_gen/generator.hpp` and produce convex hulls with **exactly** `num_vertices_per_object` vertices. The shared point-generation logic lives in `src/convex_hull_vertices.cpp`: it grows the point set (adding exterior support points) and prunes it (removing random hull-input points) until the hull has the target vertex count.

> Note: Uniform random points in a 3D box have only about `N^(2/3)` points on the convex hull, so the iterative adjustment is needed for an exact count and scales worse than a single hull computation at large `N`.

## Synthetic datasets script

`scripts/generate_synthetic_datasets.sh` writes PLY meshes under `output_synthetic/`.

### Small preset (default)

```bash
./scripts/generate_synthetic_datasets.sh
```

| Folder | Objects | Vertices/object |
|--------|---------|-----------------|
| `objects100_vertices200` | 100 | 200 |
| `objects1000_vertices200` | 1000 | 200 |
| `objects100_vertices500` | 100 | 500 |

Rough disk use: **~15 MB** (measured on sample output).

### Full preset (server)

```bash
./scripts/generate_synthetic_datasets.sh full
```

Eight datasets: object counts **100, 1000, 10000, 100000** × hull vertices **200, 500** each. Folder names follow `objects{N}_vertices{V}` (e.g. `objects10000_vertices500`).

| Objects | 200 vertices | 500 vertices | Subtotal |
|---------|--------------|--------------|----------|
| 100 | ~1.1 MB | ~2.7 MB | ~4 MB |
| 1,000 | ~11 MB | ~27 MB | ~38 MB |
| 10,000 | ~110 MB | ~270 MB | ~380 MB |
| 100,000 | ~1.1 GB | ~2.7 GB | ~3.8 GB |

**Total (full preset): ~4.2 GB** — based on ~11 KB/PLY at 200 vertices and ~27 KB/PLY at 500 vertices (ASCII PLY). Actual size varies slightly with hull geometry.

Generation time scales with object count and vertex count; the full preset is intended for a server (expect hours at 100k objects).

### Build tip

Use a release build for speed:

```bash
cmake --preset release && cmake --build --preset release
TIN_TEST_BIN=build/release/tin_test ./scripts/generate_synthetic_datasets.sh
```

## Tests

```bash
ctest --preset debug
```

## Project layout

```text
include/tin_gen/                     # public headers (config, app, generator, …)
src/main.cpp                         # parses args, dispatches commands
src/app.cpp                          # command parsing + dispatch
src/commands/generate.cpp            # generate command (timing, save, stats)
src/backends/generator_cgal.cpp      # generate_random_tin()
src/backends/generator_trimesh2.cpp  # generate_random_tin_trimesh()
src/convex_hull_vertices.cpp         # exact-vertex-count point generator
src/convex_hull_3d.cpp               # Qhull wrapper (used by TriMesh2 path)
src/cpu_timer.cpp                    # CPU + wall-clock timers
scripts/generate_synthetic_datasets.sh
cmake/trimesh2/
third_party/trimesh2/                # cloned separately
```
