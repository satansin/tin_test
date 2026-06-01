#!/usr/bin/env bash
set -euo pipefail

# Build KD-tree indexes for every dataset under ../tin_exp/datasets_normalized/.
#
# Usage:
#   ./scripts/build_kd_datasets.sh [small|full]
#
# Assumptions:
#   - ../tin_exp/datasets_normalized/<name>/ contains normalized .ply files
#
# Output (per dataset <name>):
#   - ../tin_exp/datasets_norm_kd/<name>/combined.kdtree

MODE="${1:-small}"
shopt -s nullglob

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIN_TEST="${TIN_TEST_BIN:-}"
if [[ -z "${TIN_TEST}" ]]; then
  if [[ -x "${ROOT}/build/release/tin_test" ]]; then
    TIN_TEST="${ROOT}/build/release/tin_test"
  else
    TIN_TEST="${ROOT}/build/debug/tin_test"
  fi
fi

NORM_ROOT="${ROOT}/../tin_exp/datasets_normalized"
KD_ROOT="${ROOT}/../tin_exp/datasets_norm_kd"

if [[ ! -d "${NORM_ROOT}" ]]; then
  echo "error: normalized datasets not found: ${NORM_ROOT}" >&2
  echo "Run ./scripts/normalize_datasets.sh first." >&2
  exit 1
fi

mkdir -p "${KD_ROOT}"

build_kd_dir() {
  local in_dir="$1"
  local out_dir="$2"
  local max_objects="$3"
  mkdir -p "${out_dir}"
  if [[ "${max_objects}" -gt 0 ]]; then
    "${TIN_TEST}" kd --input-dir "${in_dir}" --output-dir "${out_dir}" --combined \
      --max-objects "${max_objects}"
  else
    "${TIN_TEST}" kd --input-dir "${in_dir}" --output-dir "${out_dir}" --combined
  fi
}

process_dataset() {
  local name="$1"
  local in_dir="${NORM_ROOT}/${name}"
  local out_dir="${KD_ROOT}/${name}"
  if [[ ! -d "${in_dir}" ]]; then
    return 0
  fi
  echo "--- ${name} ---"
  build_kd_dir "${in_dir}" "${out_dir}" "${2}"
}

echo "=== KD indexes from datasets_normalized ==="
if [[ "${MODE}" == "full" ]]; then
  for d in "${NORM_ROOT}"/*; do
    [[ -d "${d}" ]] || continue
    process_dataset "$(basename "${d}")" 0
  done
else
  for name in synthetic_objects100_vertices200 synthetic_objects100_vertices500 \
    synthetic_objects1000_vertices200; do
    process_dataset "${name}" 0
  done
  for d in "${NORM_ROOT}"/*; do
    [[ -d "${d}" ]] || continue
    name="$(basename "${d}")"
    [[ "${name}" == synthetic_* ]] && continue
    process_dataset "${name}" 100
  done
fi

echo "Done: ${KD_ROOT}"
