#!/usr/bin/env bash
set -euo pipefail

# Build R*-tree indexes from packed datasets under ../tin_exp/datasets_norm_pack/.
# Run compress_datasets.sh first.
#
# Usage:
#   ./scripts/build_rs_datasets.sh [small|full]
#
# Output (per dataset):
#   ../tin_exp/datasets_norm_rs/<name>/rs_merge_manifest.txt, merged_*.tinrs

MODE="${1:-small}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"
PACK_ROOT="${ROOT}/../tin_exp/datasets_norm_pack"
RS_ROOT="${ROOT}/../tin_exp/datasets_norm_rs"

mkdir -p "${RS_ROOT}"

echo ">>> build_rs_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    input:  ${PACK_ROOT}"
echo "    output: ${RS_ROOT}"
echo ""

if [[ "${MODE}" == "full" ]]; then
  echo "--- synthetic_objects100_vertices200 ---"
  "${BIN}" rs -i "${PACK_ROOT}/synthetic_objects100_vertices200" \
    -o "${RS_ROOT}/synthetic_objects100_vertices200" --combined
  echo "--- synthetic_objects100_vertices500 ---"
  "${BIN}" rs -i "${PACK_ROOT}/synthetic_objects100_vertices500" \
    -o "${RS_ROOT}/synthetic_objects100_vertices500" --combined
  echo "--- synthetic_objects1000_vertices200 ---"
  "${BIN}" rs -i "${PACK_ROOT}/synthetic_objects1000_vertices200" \
    -o "${RS_ROOT}/synthetic_objects1000_vertices200" --combined
  echo "--- synthetic_objects1000_vertices500 ---"
  "${BIN}" rs -i "${PACK_ROOT}/synthetic_objects1000_vertices500" \
    -o "${RS_ROOT}/synthetic_objects1000_vertices500" --combined
  echo "--- synthetic_objects10000_vertices200 ---"
  "${BIN}" rs -i "${PACK_ROOT}/synthetic_objects10000_vertices200" \
    -o "${RS_ROOT}/synthetic_objects10000_vertices200" --combined
  echo "--- synthetic_objects10000_vertices500 ---"
  "${BIN}" rs -i "${PACK_ROOT}/synthetic_objects10000_vertices500" \
    -o "${RS_ROOT}/synthetic_objects10000_vertices500" --combined
  echo "--- ModelNet40 ---"
  "${BIN}" rs -i "${PACK_ROOT}/ModelNet40" -o "${RS_ROOT}/ModelNet40" --combined
  echo "--- ModelNet40_auto_aligned ---"
  "${BIN}" rs -i "${PACK_ROOT}/ModelNet40_auto_aligned" \
    -o "${RS_ROOT}/ModelNet40_auto_aligned" --combined
  echo "--- ModelNet40_manually_aligned ---"
  "${BIN}" rs -i "${PACK_ROOT}/ModelNet40_manually_aligned" \
    -o "${RS_ROOT}/ModelNet40_manually_aligned" --combined
  echo "--- ShapeNetCore ---"
  "${BIN}" rs -i "${PACK_ROOT}/ShapeNetCore" -o "${RS_ROOT}/ShapeNetCore" --combined
else
  echo "--- synthetic_objects100_vertices200 ---"
  "${BIN}" rs -i "${PACK_ROOT}/synthetic_objects100_vertices200" \
    -o "${RS_ROOT}/synthetic_objects100_vertices200" --combined
  echo "--- synthetic_objects100_vertices500 ---"
  "${BIN}" rs -i "${PACK_ROOT}/synthetic_objects100_vertices500" \
    -o "${RS_ROOT}/synthetic_objects100_vertices500" --combined
  echo "--- synthetic_objects1000_vertices200 ---"
  "${BIN}" rs -i "${PACK_ROOT}/synthetic_objects1000_vertices200" \
    -o "${RS_ROOT}/synthetic_objects1000_vertices200" --combined
  echo "--- ModelNet40 ---"
  "${BIN}" rs -i "${PACK_ROOT}/ModelNet40" -o "${RS_ROOT}/ModelNet40" --combined --max-objects 100
  echo "--- ModelNet40_auto_aligned ---"
  "${BIN}" rs -i "${PACK_ROOT}/ModelNet40_auto_aligned" \
    -o "${RS_ROOT}/ModelNet40_auto_aligned" --combined --max-objects 100
  echo "--- ModelNet40_manually_aligned ---"
  "${BIN}" rs -i "${PACK_ROOT}/ModelNet40_manually_aligned" \
    -o "${RS_ROOT}/ModelNet40_manually_aligned" --combined --max-objects 100
  echo "--- ShapeNetCore ---"
  "${BIN}" rs -i "${PACK_ROOT}/ShapeNetCore" -o "${RS_ROOT}/ShapeNetCore" --combined --max-objects 100
fi

echo ""
echo "Done: ${RS_ROOT}"
