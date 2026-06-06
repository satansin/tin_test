#!/usr/bin/env bash
set -euo pipefail

# Compute pairwise distance matrices from packed meshes and prebuilt R*-trees.
# Run compress_datasets.sh and build_rs_datasets.sh first.
#
# Usage:
#   ./scripts/compute_pd_datasets.sh [small|full]
#
# Output (per dataset):
#   ../tin_exp/datasets_norm_pd/<name>/pairwise_distances_vertex.txt

MODE="${1:-small}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"
PACK_ROOT="${ROOT}/../tin_exp/datasets_norm_pack"
RS_ROOT="${ROOT}/../tin_exp/datasets_norm_rs"
PD_ROOT="${ROOT}/../tin_exp/datasets_norm_pd"

mkdir -p "${PD_ROOT}"

echo ">>> compute_pd_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    input:  ${PACK_ROOT}"
echo "    rs:     ${RS_ROOT}"
echo "    output: ${PD_ROOT}"
echo ""

if [[ "${MODE}" == "full" ]]; then
  echo "--- synthetic_objects100_vertices200 ---"
  "${BIN}" pd -i "${PACK_ROOT}/synthetic_objects100_vertices200" \
    --rs-dir "${RS_ROOT}/synthetic_objects100_vertices200" --algorithm vertex \
    -o "${PD_ROOT}/synthetic_objects100_vertices200/pairwise_distances_vertex.txt"
  echo "--- synthetic_objects100_vertices500 ---"
  "${BIN}" pd -i "${PACK_ROOT}/synthetic_objects100_vertices500" \
    --rs-dir "${RS_ROOT}/synthetic_objects100_vertices500" --algorithm vertex \
    -o "${PD_ROOT}/synthetic_objects100_vertices500/pairwise_distances_vertex.txt"
  echo "--- synthetic_objects1000_vertices200 ---"
  "${BIN}" pd -i "${PACK_ROOT}/synthetic_objects1000_vertices200" \
    --rs-dir "${RS_ROOT}/synthetic_objects1000_vertices200" --algorithm vertex \
    -o "${PD_ROOT}/synthetic_objects1000_vertices200/pairwise_distances_vertex.txt"
  echo "--- synthetic_objects1000_vertices500 ---"
  "${BIN}" pd -i "${PACK_ROOT}/synthetic_objects1000_vertices500" \
    --rs-dir "${RS_ROOT}/synthetic_objects1000_vertices500" --algorithm vertex \
    -o "${PD_ROOT}/synthetic_objects1000_vertices500/pairwise_distances_vertex.txt"
  echo "--- synthetic_objects10000_vertices200 ---"
  "${BIN}" pd -i "${PACK_ROOT}/synthetic_objects10000_vertices200" \
    --rs-dir "${RS_ROOT}/synthetic_objects10000_vertices200" --algorithm vertex \
    -o "${PD_ROOT}/synthetic_objects10000_vertices200/pairwise_distances_vertex.txt"
  echo "--- synthetic_objects10000_vertices500 ---"
  "${BIN}" pd -i "${PACK_ROOT}/synthetic_objects10000_vertices500" \
    --rs-dir "${RS_ROOT}/synthetic_objects10000_vertices500" --algorithm vertex \
    -o "${PD_ROOT}/synthetic_objects10000_vertices500/pairwise_distances_vertex.txt"
  echo "--- ModelNet40 ---"
  "${BIN}" pd -i "${PACK_ROOT}/ModelNet40" --rs-dir "${RS_ROOT}/ModelNet40" --algorithm vertex \
    -o "${PD_ROOT}/ModelNet40/pairwise_distances_vertex.txt"
  echo "--- ModelNet40_auto_aligned ---"
  "${BIN}" pd -i "${PACK_ROOT}/ModelNet40_auto_aligned" \
    --rs-dir "${RS_ROOT}/ModelNet40_auto_aligned" --algorithm vertex \
    -o "${PD_ROOT}/ModelNet40_auto_aligned/pairwise_distances_vertex.txt"
  echo "--- ModelNet40_manually_aligned ---"
  "${BIN}" pd -i "${PACK_ROOT}/ModelNet40_manually_aligned" \
    --rs-dir "${RS_ROOT}/ModelNet40_manually_aligned" --algorithm vertex \
    -o "${PD_ROOT}/ModelNet40_manually_aligned/pairwise_distances_vertex.txt"
  echo "--- ShapeNetCore ---"
  "${BIN}" pd -i "${PACK_ROOT}/ShapeNetCore" --rs-dir "${RS_ROOT}/ShapeNetCore" --algorithm vertex \
    -o "${PD_ROOT}/ShapeNetCore/pairwise_distances_vertex.txt"
else
  echo "--- synthetic_objects100_vertices200 ---"
  "${BIN}" pd -i "${PACK_ROOT}/synthetic_objects100_vertices200" \
    --rs-dir "${RS_ROOT}/synthetic_objects100_vertices200" --algorithm vertex \
    -o "${PD_ROOT}/synthetic_objects100_vertices200/pairwise_distances_vertex.txt"
  echo "--- synthetic_objects100_vertices500 ---"
  "${BIN}" pd -i "${PACK_ROOT}/synthetic_objects100_vertices500" \
    --rs-dir "${RS_ROOT}/synthetic_objects100_vertices500" --algorithm vertex \
    -o "${PD_ROOT}/synthetic_objects100_vertices500/pairwise_distances_vertex.txt"
  echo "--- synthetic_objects1000_vertices200 ---"
  "${BIN}" pd -i "${PACK_ROOT}/synthetic_objects1000_vertices200" \
    --rs-dir "${RS_ROOT}/synthetic_objects1000_vertices200" --algorithm vertex \
    -o "${PD_ROOT}/synthetic_objects1000_vertices200/pairwise_distances_vertex.txt"
  echo "--- ModelNet40 ---"
  "${BIN}" pd -i "${PACK_ROOT}/ModelNet40" --rs-dir "${RS_ROOT}/ModelNet40" --algorithm vertex \
    --max-objects 100 -o "${PD_ROOT}/ModelNet40/pairwise_distances_vertex.txt"
  echo "--- ModelNet40_auto_aligned ---"
  "${BIN}" pd -i "${PACK_ROOT}/ModelNet40_auto_aligned" \
    --rs-dir "${RS_ROOT}/ModelNet40_auto_aligned" --algorithm vertex --max-objects 100 \
    -o "${PD_ROOT}/ModelNet40_auto_aligned/pairwise_distances_vertex.txt"
  echo "--- ModelNet40_manually_aligned ---"
  "${BIN}" pd -i "${PACK_ROOT}/ModelNet40_manually_aligned" \
    --rs-dir "${RS_ROOT}/ModelNet40_manually_aligned" --algorithm vertex --max-objects 100 \
    -o "${PD_ROOT}/ModelNet40_manually_aligned/pairwise_distances_vertex.txt"
  echo "--- ShapeNetCore ---"
  "${BIN}" pd -i "${PACK_ROOT}/ShapeNetCore" --rs-dir "${RS_ROOT}/ShapeNetCore" --algorithm vertex \
    --max-objects 100 -o "${PD_ROOT}/ShapeNetCore/pairwise_distances_vertex.txt"
fi

echo ""
echo "Done: ${PD_ROOT}"
