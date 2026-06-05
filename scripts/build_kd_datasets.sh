#!/usr/bin/env bash
set -euo pipefail

# Build KD-tree indexes for datasets under ../tin_exp/datasets_normalized/.
#
# Usage:
#   ./scripts/build_kd_datasets.sh [small|full]
#
# Output (per dataset):
#   ../tin_exp/datasets_norm_kd/<name>/combined.kdtree

MODE="${1:-small}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"
NORM_ROOT="${ROOT}/../tin_exp/datasets_normalized"
KD_ROOT="${ROOT}/../tin_exp/datasets_norm_kd"

mkdir -p "${KD_ROOT}"

echo ">>> build_kd_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    input:  ${NORM_ROOT}"
echo "    output: ${KD_ROOT}"
echo ""

if [[ "${MODE}" == "full" ]]; then
  echo "--- synthetic_objects100_vertices200 ---"
  "${BIN}" kd -i "${NORM_ROOT}/synthetic_objects100_vertices200" \
    -o "${KD_ROOT}/synthetic_objects100_vertices200" --combined
  echo "--- synthetic_objects100_vertices500 ---"
  "${BIN}" kd -i "${NORM_ROOT}/synthetic_objects100_vertices500" \
    -o "${KD_ROOT}/synthetic_objects100_vertices500" --combined
  echo "--- synthetic_objects1000_vertices200 ---"
  "${BIN}" kd -i "${NORM_ROOT}/synthetic_objects1000_vertices200" \
    -o "${KD_ROOT}/synthetic_objects1000_vertices200" --combined
  echo "--- synthetic_objects1000_vertices500 ---"
  "${BIN}" kd -i "${NORM_ROOT}/synthetic_objects1000_vertices500" \
    -o "${KD_ROOT}/synthetic_objects1000_vertices500" --combined
  echo "--- synthetic_objects10000_vertices200 ---"
  "${BIN}" kd -i "${NORM_ROOT}/synthetic_objects10000_vertices200" \
    -o "${KD_ROOT}/synthetic_objects10000_vertices200" --combined
  echo "--- synthetic_objects10000_vertices500 ---"
  "${BIN}" kd -i "${NORM_ROOT}/synthetic_objects10000_vertices500" \
    -o "${KD_ROOT}/synthetic_objects10000_vertices500" --combined
  echo "--- ModelNet40 ---"
  "${BIN}" kd -i "${NORM_ROOT}/ModelNet40" -o "${KD_ROOT}/ModelNet40" --combined
  echo "--- ModelNet40_auto_aligned ---"
  "${BIN}" kd -i "${NORM_ROOT}/ModelNet40_auto_aligned" \
    -o "${KD_ROOT}/ModelNet40_auto_aligned" --combined
  echo "--- ModelNet40_manually_aligned ---"
  "${BIN}" kd -i "${NORM_ROOT}/ModelNet40_manually_aligned" \
    -o "${KD_ROOT}/ModelNet40_manually_aligned" --combined
  echo "--- ShapeNetCore ---"
  "${BIN}" kd -i "${NORM_ROOT}/ShapeNetCore" -o "${KD_ROOT}/ShapeNetCore" --combined
else
  echo "--- synthetic_objects100_vertices200 ---"
  "${BIN}" kd -i "${NORM_ROOT}/synthetic_objects100_vertices200" \
    -o "${KD_ROOT}/synthetic_objects100_vertices200" --combined
  echo "--- synthetic_objects100_vertices500 ---"
  "${BIN}" kd -i "${NORM_ROOT}/synthetic_objects100_vertices500" \
    -o "${KD_ROOT}/synthetic_objects100_vertices500" --combined
  echo "--- synthetic_objects1000_vertices200 ---"
  "${BIN}" kd -i "${NORM_ROOT}/synthetic_objects1000_vertices200" \
    -o "${KD_ROOT}/synthetic_objects1000_vertices200" --combined
  echo "--- ModelNet40 ---"
  "${BIN}" kd -i "${NORM_ROOT}/ModelNet40" -o "${KD_ROOT}/ModelNet40" --combined --max-objects 100
  echo "--- ModelNet40_auto_aligned ---"
  "${BIN}" kd -i "${NORM_ROOT}/ModelNet40_auto_aligned" \
    -o "${KD_ROOT}/ModelNet40_auto_aligned" --combined --max-objects 100
  echo "--- ModelNet40_manually_aligned ---"
  "${BIN}" kd -i "${NORM_ROOT}/ModelNet40_manually_aligned" \
    -o "${KD_ROOT}/ModelNet40_manually_aligned" --combined --max-objects 100
  echo "--- ShapeNetCore ---"
  "${BIN}" kd -i "${NORM_ROOT}/ShapeNetCore" -o "${KD_ROOT}/ShapeNetCore" --combined --max-objects 100
fi

echo ""
echo "Done: ${KD_ROOT}"
