#!/usr/bin/env bash
# Generate synthetic TIN datasets under output_synthetic/.
#
# Usage:
#   ./scripts/generate_synthetic_datasets.sh [options] [MODE]
#
# Modes: (default) small | full
#
# Options:
#   --log PATH, -l PATH       Log to PATH (screen + file)
#   --log-only PATH           Log to PATH only (no screen output)
#
# Environment:
#   TIN_TEST_BIN  Path to tin_test (default: build/release/tin_test)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"
OUT_ROOT="${ROOT}/output_synthetic"

usage() {
  cat <<'EOF'
Usage: generate_synthetic_datasets.sh [options] [MODE]

Modes:
  (none)   Small preset: 3 datasets for local testing
  full     Large preset: 6 datasets (100–10000 objects × 200/500 vertices)

Options:
  --log PATH, -l PATH       Log to PATH (screen + file)
  --log-only PATH           Log to PATH only (no screen output)

Environment:
  TIN_TEST_BIN   tin_test executable

Examples:
  ./scripts/generate_synthetic_datasets.sh --log-only output_synthetic/generation.log
  ./scripts/generate_synthetic_datasets.sh --log-only output_synthetic/full.log full

See README.html for disk usage estimates.
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
      LOG_TEE=1
      shift 2
      ;;
    --log-only)
      if [[ $# -lt 2 ]]; then
        echo "error: missing path after $1" >&2
        exit 1
      fi
      LOG_FILE="$2"
      LOG_TEE=0
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
  if [[ "${LOG_TEE:-1}" == 1 ]]; then
    exec > >(tee "${LOG_FILE}") 2>&1
  else
    exec >"${LOG_FILE}" 2>&1
  fi
fi

if [[ ! -x "${BIN}" ]]; then
  echo "error: tin_test not found at ${BIN}" >&2
  echo "Build with:" >&2
  echo "  cmake --preset release && cmake --build --preset release" >&2
  exit 1
fi

echo ">>> generate_synthetic_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    output: ${OUT_ROOT}"
echo ""

if [[ "${MODE}" == "full" ]]; then
  echo "--- objects100_vertices200 ---"
  mkdir -p "${OUT_ROOT}/objects100_vertices200"
  "${BIN}" gen --num-objects 100 --num-vertices-per-object 200 \
    --output-dir "${OUT_ROOT}/objects100_vertices200" --seed 2001 --quiet
  echo "--- objects100_vertices500 ---"
  mkdir -p "${OUT_ROOT}/objects100_vertices500"
  "${BIN}" gen --num-objects 100 --num-vertices-per-object 500 \
    --output-dir "${OUT_ROOT}/objects100_vertices500" --seed 2002 --quiet
  echo "--- objects1000_vertices200 ---"
  mkdir -p "${OUT_ROOT}/objects1000_vertices200"
  "${BIN}" gen --num-objects 1000 --num-vertices-per-object 200 \
    --output-dir "${OUT_ROOT}/objects1000_vertices200" --seed 2003 --quiet
  echo "--- objects1000_vertices500 ---"
  mkdir -p "${OUT_ROOT}/objects1000_vertices500"
  "${BIN}" gen --num-objects 1000 --num-vertices-per-object 500 \
    --output-dir "${OUT_ROOT}/objects1000_vertices500" --seed 2004 --quiet
  echo "--- objects10000_vertices200 ---"
  mkdir -p "${OUT_ROOT}/objects10000_vertices200"
  "${BIN}" gen --num-objects 10000 --num-vertices-per-object 200 \
    --output-dir "${OUT_ROOT}/objects10000_vertices200" --seed 2005 --quiet
  echo "--- objects10000_vertices500 ---"
  mkdir -p "${OUT_ROOT}/objects10000_vertices500"
  "${BIN}" gen --num-objects 10000 --num-vertices-per-object 500 \
    --output-dir "${OUT_ROOT}/objects10000_vertices500" --seed 2006 --quiet
else
  echo "--- objects100_vertices200 ---"
  mkdir -p "${OUT_ROOT}/objects100_vertices200"
  "${BIN}" gen --num-objects 100 --num-vertices-per-object 200 \
    --output-dir "${OUT_ROOT}/objects100_vertices200" --seed 1001 --quiet
  echo "--- objects1000_vertices200 ---"
  mkdir -p "${OUT_ROOT}/objects1000_vertices200"
  "${BIN}" gen --num-objects 1000 --num-vertices-per-object 200 \
    --output-dir "${OUT_ROOT}/objects1000_vertices200" --seed 1002 --quiet
  echo "--- objects100_vertices500 ---"
  mkdir -p "${OUT_ROOT}/objects100_vertices500"
  "${BIN}" gen --num-objects 100 --num-vertices-per-object 500 \
    --output-dir "${OUT_ROOT}/objects100_vertices500" --seed 1003 --quiet
fi

echo ""
echo "All datasets written under ${OUT_ROOT}/"
