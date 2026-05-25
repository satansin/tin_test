# tin_test

Synthetic 3D TIN (triangular irregular network) generator. Both **CGAL** and **[TriMesh2](https://github.com/Forceflow/trimesh2)** are always compiled in; pick at runtime or in code. The original Python script uses [trimesh](https://github.com/mikedh/trimesh).

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

### Python (optional)

```bash
pip install numpy trimesh
```

## Build

```bash
cmake --preset debug
cmake --build --preset debug
```

Release: `cmake --preset release` / `cmake --build --preset release`.

## Generators (API)

| Function | Backend |
|----------|---------|
| `generate_random_tin()` | CGAL convex hull (default in CLI) |
| `generate_random_tin_trimesh()` | TriMesh2 + Qhull convex hull |

Both live in `include/tin_gen/generator.hpp` and are implemented in `src/backends/`.

## Configuration

Defaults in `AppConfig` (`include/tin_gen/config.hpp`):

| Field | Default |
|-------|---------|
| backend | `cgal` |
| format | `ply` |
| num_objects | `5` |
| num_vertices_per_object | `50` |
| scale | `10.0` |
| output_dir | `output` |
| random_seed | `0` (non-zero = fixed seed) |

## Run

```bash
./build/debug/tin_test
./build/debug/tin_test generate --backend trimesh2
./build/debug/tin_test --format obj --seed 42
./build/debug/tin_test help
```

Output: `output/object_1.ply` (or `.obj`). Prints backend and `generate_random_tin` CPU time.

### Python

```bash
python gen_syn_tin.py
```

## Tests

```bash
ctest --preset debug
```

## Project layout

```text
include/tin_gen/
src/backends/generator_cgal.cpp      # generate_random_tin()
src/backends/generator_trimesh2.cpp  # generate_random_tin_trimesh()
src/convex_hull_3d.cpp               # Qhull (used by TriMesh2 path)
cmake/trimesh2/
third_party/trimesh2/
```
