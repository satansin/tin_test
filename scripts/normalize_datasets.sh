#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./scripts/normalize_datasets.sh [small|full]
#
# Assumptions:
#   - ../tin_exp/ exists
#   - synthetic datasets are under output_synthetic/<dataset>/
#   - raw datasets are under ../tin_exp/datasets_raw/<dataset>/
#
# Output:
#   - ../tin_exp/datasets_normalized/synthetic_<dataset>/
#   - ../tin_exp/datasets_normalized/<dataset>/

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

SYNTH_ROOT="${ROOT}/output_synthetic"
RAW_ROOT="${ROOT}/../tin_exp/datasets_raw"
NORM_ROOT="${ROOT}/../tin_exp/datasets_normalized"

mkdir -p "${NORM_ROOT}"

normalize_dir() {
  local in_dir="$1"
  local out_dir="$2"
  local max_objects="$3"
  mkdir -p "${out_dir}"
  if [[ "${max_objects}" -gt 0 ]]; then
    "${TIN_TEST}" normalize --input-dir "${in_dir}" --output-dir "${out_dir}" --max-objects "${max_objects}"
  else
    "${TIN_TEST}" normalize --input-dir "${in_dir}" --output-dir "${out_dir}"
  fi
}

echo "=== Synthetic datasets ==="
if [[ "${MODE}" == "full" ]]; then
  for d in "${SYNTH_ROOT}"/*; do
    name="$(basename "${d}")"
    normalize_dir "${d}" "${NORM_ROOT}/synthetic_${name}" 0
  done
else
  for name in objects100_vertices200 objects100_vertices500 objects1000_vertices200; do
    if [[ -d "${SYNTH_ROOT}/${name}" ]]; then
      normalize_dir "${SYNTH_ROOT}/${name}" "${NORM_ROOT}/synthetic_${name}" 0
    fi
  done
fi

echo "=== Raw datasets ==="
for d in "${RAW_ROOT}"/*; do
  name="$(basename "${d}")"
  out="${NORM_ROOT}/${name}"
  echo "--- ${name} ---"
  if [[ "${MODE}" == "full" ]]; then
    normalize_dir "${d}" "${out}" 0
  else
    normalize_dir "${d}" "${out}" 100
  fi
done

echo "Done: ${NORM_ROOT}"

