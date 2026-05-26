#!/usr/bin/env bash
# Generate synthetic TIN datasets under output_synthetic/.
#
# Usage:
#   ./scripts/generate_synthetic_datasets.sh [options] [MODE]
#
# Modes: (default) small | full
#
# Options:
#   --log PATH, -l PATH   Write stdout/stderr to PATH (and print to terminal)
#
# Environment (bash/sh only):
#   TIN_TEST_BIN  Path to tin_test (default: build/release, else build/debug)
#   LOG_FILE      Same as --log (bash/sh inline assignment only)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ -n "${TIN_TEST_BIN:-}" ]]; then
  TIN_TEST="${TIN_TEST_BIN}"
elif [[ -x "${ROOT}/build/release/tin_test" ]]; then
  TIN_TEST="${ROOT}/build/release/tin_test"
else
  TIN_TEST="${ROOT}/build/debug/tin_test"
fi
OUT_ROOT="${ROOT}/output_synthetic"

usage() {
  cat <<'EOF'
Usage: generate_synthetic_datasets.sh [options] [MODE]

Modes:
  (none)   Small preset: 3 datasets for local testing
  full     Large preset: 8 datasets (100–100000 objects × 200/500 vertices)

Options:
  --log PATH, -l PATH   Log to PATH (screen + file)

Environment (bash/sh):
  TIN_TEST_BIN   tin_test executable
  LOG_FILE       Same as --log

Examples:
  ./scripts/generate_synthetic_datasets.sh --log output_synthetic/generation.log
  ./scripts/generate_synthetic_datasets.sh --log output_synthetic/full.log full

On csh/tcsh, use --log (not LOG_FILE=... ./script).

See README.md for disk usage estimates.
EOF
}

MODE="small"
while [[ $# -gt 0 ]]; do
  case "$1" in
    -h | --help | help)
      usage
      exit 0
      ;;
    --log | -l)
      if [[ $# -lt 2 ]]; then
        echo "error: missing path after $1" >&2
        exit 1
      fi
      LOG_FILE="$2"
      shift 2
      ;;
    full | small)
      MODE="$1"
      shift
      ;;
    *)
      echo "error: unknown argument: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

mkdir -p "${OUT_ROOT}"

if [[ -n "${LOG_FILE:-}" ]]; then
  log_dir="$(dirname "${LOG_FILE}")"
  if [[ "${log_dir}" != "." ]]; then
    mkdir -p "${log_dir}"
  fi
  exec > >(tee "${LOG_FILE}") 2>&1
fi

if [[ ! -x "${TIN_TEST}" ]]; then
  echo "error: tin_test not found at ${TIN_TEST}" >&2
  echo "Build with:" >&2
  echo "  cmake --preset release && cmake --build --preset release" >&2
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

case "${MODE}" in
  small) run_small ;;
  full) run_full ;;
  *)
    echo "error: unknown mode: ${MODE}" >&2
    exit 1
    ;;
esac

echo ""
echo "All datasets written under ${OUT_ROOT}/"
