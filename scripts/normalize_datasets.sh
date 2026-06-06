#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./scripts/normalize_datasets.sh [small|full]
#
# Assumptions:
#   - synthetic datasets under output_synthetic/<dataset>/
#   - raw datasets under ../tin_exp/datasets_raw/<dataset>/
#
# Output:
#   - ../tin_exp/datasets_norm/synthetic_<dataset>/
#   - ../tin_exp/datasets_norm/<dataset>/

MODE="${1:-small}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"
SYNTH_ROOT="${ROOT}/output_synthetic"
RAW_ROOT="${ROOT}/../tin_exp/datasets_raw"
NORM_ROOT="${ROOT}/../tin_exp/datasets_norm"

mkdir -p "${NORM_ROOT}"

echo ">>> normalize_datasets.sh"
echo "    mode:        ${MODE}"
echo "    bin:         ${BIN}"
echo "    synthetic:   ${SYNTH_ROOT}"
echo "    raw:         ${RAW_ROOT}"
echo "    output:      ${NORM_ROOT}"
echo ""

echo "=== Synthetic datasets ==="
if [[ "${MODE}" == "full" ]]; then
  echo "--- objects100_vertices200 ---"
  "${BIN}" norm -i "${SYNTH_ROOT}/objects100_vertices200" \
    -o "${NORM_ROOT}/synthetic_objects100_vertices200"
  echo "--- objects100_vertices500 ---"
  "${BIN}" norm -i "${SYNTH_ROOT}/objects100_vertices500" \
    -o "${NORM_ROOT}/synthetic_objects100_vertices500"
  echo "--- objects1000_vertices200 ---"
  "${BIN}" norm -i "${SYNTH_ROOT}/objects1000_vertices200" \
    -o "${NORM_ROOT}/synthetic_objects1000_vertices200"
  echo "--- objects10000_vertices200 ---"
  "${BIN}" norm -i "${SYNTH_ROOT}/objects10000_vertices200" \
    -o "${NORM_ROOT}/synthetic_objects10000_vertices200"
  echo "--- objects10000_vertices500 ---"
  "${BIN}" norm -i "${SYNTH_ROOT}/objects10000_vertices500" \
    -o "${NORM_ROOT}/synthetic_objects10000_vertices500"
  echo "--- objects1000_vertices500 ---"
  "${BIN}" norm -i "${SYNTH_ROOT}/objects1000_vertices500" \
    -o "${NORM_ROOT}/synthetic_objects1000_vertices500"
else
  echo "--- objects100_vertices200 ---"
  "${BIN}" norm -i "${SYNTH_ROOT}/objects100_vertices200" \
    -o "${NORM_ROOT}/synthetic_objects100_vertices200"
  echo "--- objects100_vertices500 ---"
  "${BIN}" norm -i "${SYNTH_ROOT}/objects100_vertices500" \
    -o "${NORM_ROOT}/synthetic_objects100_vertices500"
  echo "--- objects1000_vertices200 ---"
  "${BIN}" norm -i "${SYNTH_ROOT}/objects1000_vertices200" \
    -o "${NORM_ROOT}/synthetic_objects1000_vertices200"
fi

echo ""
echo "=== Raw datasets ==="
if [[ "${MODE}" == "full" ]]; then
  echo "--- ModelNet40 ---"
  "${BIN}" norm -i "${RAW_ROOT}/ModelNet40" -o "${NORM_ROOT}/ModelNet40"
  echo "--- ModelNet40_auto_aligned ---"
  "${BIN}" norm -i "${RAW_ROOT}/ModelNet40_auto_aligned" \
    -o "${NORM_ROOT}/ModelNet40_auto_aligned"
  echo "--- ModelNet40_manually_aligned ---"
  "${BIN}" norm -i "${RAW_ROOT}/ModelNet40_manually_aligned" \
    -o "${NORM_ROOT}/ModelNet40_manually_aligned"
  echo "--- ShapeNetCore ---"
  "${BIN}" norm -i "${RAW_ROOT}/ShapeNetCore" -o "${NORM_ROOT}/ShapeNetCore"
else
  echo "--- ModelNet40 ---"
  "${BIN}" norm -i "${RAW_ROOT}/ModelNet40" -o "${NORM_ROOT}/ModelNet40" --max-objects 100
  echo "--- ModelNet40_auto_aligned ---"
  "${BIN}" norm -i "${RAW_ROOT}/ModelNet40_auto_aligned" \
    -o "${NORM_ROOT}/ModelNet40_auto_aligned" --max-objects 100
  echo "--- ModelNet40_manually_aligned ---"
  "${BIN}" norm -i "${RAW_ROOT}/ModelNet40_manually_aligned" \
    -o "${NORM_ROOT}/ModelNet40_manually_aligned" --max-objects 100
  echo "--- ShapeNetCore ---"
  "${BIN}" norm -i "${RAW_ROOT}/ShapeNetCore" -o "${NORM_ROOT}/ShapeNetCore" --max-objects 100
fi

echo ""
echo "Done: ${NORM_ROOT}"
