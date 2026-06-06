#!/usr/bin/env bash
set -euo pipefail

# Merge (compress) PLY files for datasets under ../tin_exp/datasets_norm/.
#
# Usage:
#   ./scripts/compress_datasets.sh [small|full]
#
# Assumptions:
#   - ../tin_exp/datasets_norm/<name>/ contains normalized .ply files
#
# Output (per dataset <name>):
#   - ../tin_exp/datasets_norm_pack/<name>/merged_*.tinply
#   - ../tin_exp/datasets_norm_pack/<name>/ply_merge_manifest.txt

MODE="${1:-small}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"
NORM_ROOT="${ROOT}/../tin_exp/datasets_norm"
PACK_ROOT="${ROOT}/../tin_exp/datasets_norm_pack"

mkdir -p "${PACK_ROOT}"

echo ">>> compress_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    input:  ${NORM_ROOT}"
echo "    output: ${PACK_ROOT}"
echo ""

if [[ "${MODE}" == "full" ]]; then
  echo "--- synthetic_objects100_vertices200 ---"
  "${BIN}" compress -i "${NORM_ROOT}/synthetic_objects100_vertices200" \
    -o "${PACK_ROOT}/synthetic_objects100_vertices200"
  echo "--- synthetic_objects100_vertices500 ---"
  "${BIN}" compress -i "${NORM_ROOT}/synthetic_objects100_vertices500" \
    -o "${PACK_ROOT}/synthetic_objects100_vertices500"
  echo "--- synthetic_objects1000_vertices200 ---"
  "${BIN}" compress -i "${NORM_ROOT}/synthetic_objects1000_vertices200" \
    -o "${PACK_ROOT}/synthetic_objects1000_vertices200"
  echo "--- synthetic_objects1000_vertices500 ---"
  "${BIN}" compress -i "${NORM_ROOT}/synthetic_objects1000_vertices500" \
    -o "${PACK_ROOT}/synthetic_objects1000_vertices500"
  echo "--- synthetic_objects10000_vertices200 ---"
  "${BIN}" compress -i "${NORM_ROOT}/synthetic_objects10000_vertices200" \
    -o "${PACK_ROOT}/synthetic_objects10000_vertices200"
  echo "--- synthetic_objects10000_vertices500 ---"
  "${BIN}" compress -i "${NORM_ROOT}/synthetic_objects10000_vertices500" \
    -o "${PACK_ROOT}/synthetic_objects10000_vertices500"
  echo "--- ModelNet40 ---"
  "${BIN}" compress -i "${NORM_ROOT}/ModelNet40" -o "${PACK_ROOT}/ModelNet40"
  echo "--- ModelNet40_auto_aligned ---"
  "${BIN}" compress -i "${NORM_ROOT}/ModelNet40_auto_aligned" \
    -o "${PACK_ROOT}/ModelNet40_auto_aligned"
  echo "--- ModelNet40_manually_aligned ---"
  "${BIN}" compress -i "${NORM_ROOT}/ModelNet40_manually_aligned" \
    -o "${PACK_ROOT}/ModelNet40_manually_aligned"
  echo "--- ShapeNetCore ---"
  "${BIN}" compress -i "${NORM_ROOT}/ShapeNetCore" -o "${PACK_ROOT}/ShapeNetCore"
else
  echo "--- synthetic_objects100_vertices200 ---"
  "${BIN}" compress -i "${NORM_ROOT}/synthetic_objects100_vertices200" \
    -o "${PACK_ROOT}/synthetic_objects100_vertices200"
  echo "--- synthetic_objects100_vertices500 ---"
  "${BIN}" compress -i "${NORM_ROOT}/synthetic_objects100_vertices500" \
    -o "${PACK_ROOT}/synthetic_objects100_vertices500"
  echo "--- synthetic_objects1000_vertices200 ---"
  "${BIN}" compress -i "${NORM_ROOT}/synthetic_objects1000_vertices200" \
    -o "${PACK_ROOT}/synthetic_objects1000_vertices200"
  echo "--- ModelNet40 ---"
  "${BIN}" compress -i "${NORM_ROOT}/ModelNet40" -o "${PACK_ROOT}/ModelNet40" --max-objects 100
  echo "--- ModelNet40_auto_aligned ---"
  "${BIN}" compress -i "${NORM_ROOT}/ModelNet40_auto_aligned" \
    -o "${PACK_ROOT}/ModelNet40_auto_aligned" --max-objects 100
  echo "--- ModelNet40_manually_aligned ---"
  "${BIN}" compress -i "${NORM_ROOT}/ModelNet40_manually_aligned" \
    -o "${PACK_ROOT}/ModelNet40_manually_aligned" --max-objects 100
  echo "--- ShapeNetCore ---"
  "${BIN}" compress -i "${NORM_ROOT}/ShapeNetCore" -o "${PACK_ROOT}/ShapeNetCore" --max-objects 100
fi

echo ""
echo "Done: ${PACK_ROOT}"
