#!/usr/bin/env bash
# Generate synthetic TIN datasets under output_synthetic/.
#
# Usage:
#   ./scripts/generate_synthetic_datasets.sh          # small preset (default)
#   ./scripts/generate_synthetic_datasets.sh full     # large preset (server)
#
# Environment:
#   TIN_TEST_BIN  Path to tin_test (default: build/debug/tin_test)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIN_TEST="${TIN_TEST_BIN:-${ROOT}/build/debug/tin_test}"
OUT_ROOT="${ROOT}/output_synthetic"

usage() {
  cat <<'EOF'
Usage: generate_synthetic_datasets.sh [MODE]

Modes:
  (none)   Small preset: 3 datasets for local testing
  full     Large preset: 8 datasets (100–100000 objects × 200/500 vertices)

Environment:
  TIN_TEST_BIN   tin_test executable (default: build/debug/tin_test)

See README.md for estimated disk usage.
EOF
}

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

run_small() {
  echo "Mode: small (default)"
  generate_dataset "objects100_vertices200" 100 200 1001
  generate_dataset "objects1000_vertices200" 1000 200 1002
  generate_dataset "objects100_vertices500" 100 500 1003
}

run_full() {
  echo "Mode: full"
  local seed=2001
  for num_objects in 100 1000 10000 100000; do
    for num_vertices in 200 500; do
      generate_dataset "objects${num_objects}_vertices${num_vertices}" \
        "${num_objects}" "${num_vertices}" "${seed}"
      seed=$((seed + 1))
    done
  done
}

MODE="${1:-small}"

case "${MODE}" in
  small|"") run_small ;;
  full) run_full ;;
  -h|--help|help) usage; exit 0 ;;
  *)
    echo "error: unknown mode: ${MODE}" >&2
    usage >&2
    exit 1
    ;;
esac

echo ""
echo "All datasets written under ${OUT_ROOT}/"
