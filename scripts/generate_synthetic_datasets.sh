#!/usr/bin/env bash
# Generate three synthetic TIN datasets under output_synthetic/.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIN_TEST="${TIN_TEST_BIN:-${ROOT}/build/debug/tin_test}"
OUT_ROOT="${ROOT}/output_synthetic"

mkdir -p "${OUT_ROOT}"

if [[ ! -x "${TIN_TEST}" ]]; then
  echo "error: tin_test not found at ${TIN_TEST}" >&2
  echo "Build with:" >&2
  echo "  cmake --preset debug && cmake --build --preset debug" >&2
  exit 1
fi

generate_dataset() {
  local name="$1"
  local num_objects="$2"
  local num_vertices="$3"
  local seed="$4"
  local out_dir="${OUT_ROOT}/${name}"

  echo ""
  echo "=== ${name} ==="
  echo "  objects: ${num_objects}"
  echo "  vertices per object: ${num_vertices}"
  echo "  output: ${out_dir}"

  mkdir -p "${out_dir}"
  "${TIN_TEST}" generate \
    --num-objects "${num_objects}" \
    --num-vertices-per-object "${num_vertices}" \
    --output-dir "${out_dir}" \
    --seed "${seed}"
}

echo "Using tin_test: ${TIN_TEST}"
echo "Output root: ${OUT_ROOT}"

# 1) 100 objects × 200 hull vertices
generate_dataset "objects100_vertices200" 100 200 1001

# 2) 1000 objects × 200 hull vertices
generate_dataset "objects1000_vertices200" 1000 200 1002

# 3) 100 objects × 500 hull vertices
generate_dataset "objects100_vertices500" 100 500 1003

echo ""
echo "All datasets written under ${OUT_ROOT}/"
